'use strict';

// The Myna extension: shows the latest review saved by Myna's report_review
// tool (.myna/review.json) in the Explorer, as warnings on the reviewed
// lines, and as a score in the status bar.

const vscode = require('vscode');
const path = require('path');
const crypto = require('crypto');
const {
    parseReview,
    summarize,
    timeAgo,
    resolveFile,
    location,
    buildFixRequest,
    buildReviewRequest,
    claudeCodeQuery,
    escapeHtml,
} = require('./review');
const { findSessions } = require('./sessions');

const REVIEW_GLOB = '**/.myna/review.json';
const EXCLUDE_GLOB = '**/node_modules/**';
const CLAUDE_CODE = 'anthropic.claude-code';

/** The latest review, as shown everywhere: { file, projectDir, review } or { error } or null. */
let current = null;

function activate(context) {
    const diagnostics = vscode.languages.createDiagnosticCollection('myna');
    const status = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Left, 50);
    status.command = 'myna.showReview';
    const view = new ReviewView(context.extensionUri);

    const refresh = async () => {
        current = await loadLatestReview();
        updateDiagnostics(diagnostics);
        updateStatus(status);
        view.render();
    };

    const watcher = vscode.workspace.createFileSystemWatcher(REVIEW_GLOB);
    watcher.onDidCreate(refresh);
    watcher.onDidChange(refresh);
    watcher.onDidDelete(refresh);

    context.subscriptions.push(
        diagnostics,
        status,
        watcher,
        vscode.window.registerWebviewViewProvider('myna.review', view),
        vscode.workspace.onDidChangeWorkspaceFolders(refresh),
        vscode.commands.registerCommand('myna.refresh', refresh),
        vscode.commands.registerCommand('myna.showReview', () =>
            vscode.commands.executeCommand('myna.review.focus')
        ),
        vscode.commands.registerCommand('myna.fixWithAI', () => askAI(fixText(), 'fix request')),
        vscode.commands.registerCommand('myna.reviewWithAI', () => askAI(reviewText(), 'review request')),
        vscode.commands.registerCommand('myna.copyFixRequest', () => copyText(fixText(), 'fix request')),
        vscode.commands.registerCommand('myna.copyReviewRequest', () =>
            copyText(reviewText(), 'review request')
        ),
        vscode.extensions.onDidChange(() => view.render()),
        vscode.commands.registerCommand('myna.openResult', openResult)
    );

    // Keep "5 minutes ago" current without reloading anything.
    const timer = setInterval(() => view.render(), 60 * 1000);
    context.subscriptions.push({ dispose: () => clearInterval(timer) });

    refresh();
}

function deactivate() {}

/** Find the most recently written review file in the workspace. */
async function loadLatestReview() {
    const files = await vscode.workspace.findFiles(REVIEW_GLOB, EXCLUDE_GLOB, 20);
    let newest = null;
    for (const uri of files) {
        try {
            const stat = await vscode.workspace.fs.stat(uri);
            if (!newest || stat.mtime > newest.mtime) {
                newest = { uri, mtime: stat.mtime };
            }
        } catch {
            // Deleted between the search and the stat.
        }
    }
    if (!newest) {
        return null;
    }

    const projectDir = path.dirname(path.dirname(newest.uri.fsPath));
    try {
        const bytes = await vscode.workspace.fs.readFile(newest.uri);
        const parsed = parseReview(Buffer.from(bytes).toString('utf8'));
        if (parsed.error) {
            return { file: newest.uri, projectDir, error: parsed.error };
        }
        return { file: newest.uri, projectDir, review: parsed.review };
    } catch {
        return { file: newest.uri, projectDir, error: 'The review file could not be read.' };
    }
}

function updateDiagnostics(collection) {
    collection.clear();
    if (!current || !current.review) {
        return;
    }
    const byFile = new Map();
    for (const r of current.review.results) {
        if (r.verdict !== 'fail' || !r.file) {
            continue;
        }
        const file = resolveFile(current.projectDir, r.file);
        const line = (r.line || 1) - 1;
        const diagnostic = new vscode.Diagnostic(
            new vscode.Range(line, 0, line, Number.MAX_SAFE_INTEGER),
            r.note ? `${r.item}: ${r.note}` : r.item,
            vscode.DiagnosticSeverity.Warning
        );
        diagnostic.source = 'Myna';
        if (r.number) {
            diagnostic.code = `item ${r.number}`;
        }
        if (!byFile.has(file)) {
            byFile.set(file, []);
        }
        byFile.get(file).push(diagnostic);
    }
    for (const [file, list] of byFile) {
        collection.set(vscode.Uri.file(file), list);
    }
}

function updateStatus(status) {
    if (!current || !current.review) {
        status.hide();
        return;
    }
    const { pass, fail } = current.review.counts;
    status.text = fail
        ? `$(warning) Myna ${pass}/${pass + fail}`
        : `$(pass) Myna ${pass}/${pass + fail}`;
    status.tooltip = summarize(current.review).headline;
    status.show();
}

/** The fix request for the latest review, or '' when nothing failed. */
function fixText() {
    return current && current.review ? buildFixRequest(current.review, current.projectDir) : '';
}

/** The review-again request for the latest review, or '' when there is none. */
function reviewText() {
    return current && current.review ? buildReviewRequest(current.review, current.projectDir) : '';
}

function hasClaudeCode() {
    return vscode.extensions.getExtension(CLAUDE_CODE) !== undefined;
}

async function copyText(text, label) {
    if (!text) {
        vscode.window.showInformationMessage(`Myna: there is no ${label} to copy for the latest review.`);
        return;
    }
    await vscode.env.clipboard.writeText(text);
    vscode.window.showInformationMessage(`Myna: ${label} copied. Paste it into your AI chat.`);
}

/**
 * Put `text` in a Claude Code chat, through Claude Code's documented
 * vscode://anthropic.claude-code/open?prompt=&session= link: an existing chat
 * of this project or a new one, as the user chooses (setting
 * myna.claudeCodeChat). The user still presses Enter to send it. Without
 * Claude Code, or if the link fails, copy instead.
 */
async function askAI(text, label) {
    if (!text) {
        vscode.window.showInformationMessage(`Myna: there is no ${label} for the latest review.`);
        return;
    }
    if (hasClaudeCode()) {
        const chat = await chooseClaudeChat(label);
        if (!chat) {
            return; // the user closed the menu
        }
        const link = vscode.Uri.from({
            scheme: vscode.env.uriScheme,
            authority: CLAUDE_CODE,
            path: '/open',
            query: claudeCodeQuery(text, chat.session),
        });
        if (await vscode.env.openExternal(link)) {
            const where = chat.session ? `your Claude Code chat "${chat.title}"` : 'a new Claude Code chat';
            vscode.window.showInformationMessage(`Myna: the ${label} is in ${where}. Check it and press Enter to send.`);
            return;
        }
    }
    await copyText(text, label);
}

/**
 * Which Claude Code chat to use: { session, title } for an existing chat of
 * the reviewed project, { session: null } for a new chat, or undefined when
 * the user closes the menu.
 */
async function chooseClaudeChat(label) {
    const mode = vscode.workspace.getConfiguration('myna').get('claudeCodeChat', 'ask');
    const sessions = mode === 'new' || !current ? [] : findSessions(current.projectDir);
    const named = sessions.map((s) => ({ session: s.id, title: s.title || 'Untitled chat', modified: s.modified }));

    if (named.length === 0 || mode === 'new') {
        return { session: null };
    }
    if (mode === 'latest') {
        return named[0];
    }

    const items = [
        ...named.map((s) => ({
            label: `$(comment-discussion) ${s.title}`,
            description: timeAgo(s.modified),
            chat: s,
        })),
        { label: '', kind: vscode.QuickPickItemKind.Separator },
        { label: '$(add) New chat', description: 'Start a new Claude Code chat', chat: { session: null } },
    ];
    const picked = await vscode.window.showQuickPick(items, {
        title: `Myna: where should the ${label} go?`,
        placeHolder: 'Continue a Claude Code chat of this project, or start a new one',
    });
    return picked ? picked.chat : undefined;
}

async function openResult(index) {
    const r = current && current.review ? current.review.results[index] : null;
    if (!r || !r.file) {
        return;
    }
    const uri = vscode.Uri.file(resolveFile(current.projectDir, r.file));
    try {
        const doc = await vscode.workspace.openTextDocument(uri);
        const line = Math.min((r.line || 1) - 1, doc.lineCount - 1);
        const range = doc.lineAt(line).range;
        await vscode.window.showTextDocument(doc, { selection: range });
    } catch {
        vscode.window.showWarningMessage(`Myna: could not open ${r.file}.`);
    }
}

class ReviewView {
    constructor(extensionUri) {
        this.extensionUri = extensionUri;
        this.view = null;
    }

    resolveWebviewView(webviewView) {
        this.view = webviewView;
        webviewView.webview.options = {
            enableScripts: true,
            localResourceRoots: [vscode.Uri.joinPath(this.extensionUri, 'media')],
        };
        webviewView.webview.onDidReceiveMessage((message) => {
            if (message && message.type === 'open' && Number.isInteger(message.index)) {
                openResult(message.index);
            } else if (message && message.type === 'copy') {
                askAI(fixText(), 'fix request');
            } else if (message && message.type === 'review') {
                askAI(reviewText(), 'review request');
            } else if (message && message.type === 'refresh') {
                vscode.commands.executeCommand('myna.refresh');
            }
        });
        this.render();
    }

    render() {
        if (!this.view) {
            return;
        }
        const webview = this.view.webview;
        const media = (name) => webview.asWebviewUri(vscode.Uri.joinPath(this.extensionUri, 'media', name));
        const nonce = crypto.randomBytes(16).toString('base64');

        webview.html = `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta http-equiv="Content-Security-Policy" content="default-src 'none'; img-src ${webview.cspSource}; style-src ${webview.cspSource}; script-src 'nonce-${nonce}';">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<link rel="stylesheet" href="${media('view.css')}">
<title>Myna</title>
</head>
<body>
<main>
  <img class="bird" src="${media('myna.png')}" alt="Myna, a black bird with a yellow beak, sitting on a branch">
  ${renderBody()}
</main>
<script nonce="${nonce}" src="${media('view.js')}"></script>
</body>
</html>`;
    }
}

function renderBody() {
    if (!current) {
        return bubble(
            'idle',
            "Hi, I'm Myna!",
            'When your AI reviews its code with my checklists, I will show the results here.'
        );
    }
    if (current.error) {
        return bubble('worried', "I couldn't read the latest review.", current.error);
    }

    const review = current.review;
    const { mood, headline } = summarize(review);
    const when = timeAgo(review.created);
    const details = when ? `Reviewed ${when}.` : '';

    const failed = [];
    const passed = [];
    const skipped = [];
    review.results.forEach((r, index) => {
        (r.verdict === 'fail' ? failed : r.verdict === 'pass' ? passed : skipped).push({ r, index });
    });

    let html = bubble(mood, headline, details);
    if (failed.length) {
        html += `<h2>To fix</h2><ul class="results">${failed.map(resultItem).join('')}</ul>`;
    }
    // With Claude Code, the buttons put the message in its chat box; without
    // it, they copy the message to paste into any AI chat.
    const claude = hasClaudeCode();
    html += '<div class="actions">';
    if (failed.length) {
        html += claude
            ? '<button type="button" class="primary" data-action="copy" title="Put the fix request in a Claude Code chat">Fix with Claude Code</button>'
            : '<button type="button" class="primary" data-action="copy">Copy fix request for your AI</button>';
    }
    html += claude
        ? '<button type="button" class="secondary" data-action="review" title="Ask Claude Code, in a chat you choose, to check the code again with Myna">Review again with Claude Code</button>'
        : '<button type="button" class="secondary" data-action="review" title="Copy a message that asks your AI to check the code again with Myna">Review again</button>';
    html += '</div>';
    if (claude) {
        html +=
            '<p class="hint">Choose a Claude Code chat of this project, or a new one. ' +
            'The message is filled in; press Enter there to send it.</p>';
    }
    if (passed.length) {
        html += `<details><summary>Passed (${passed.length})</summary><ul class="results">${passed
            .map(resultItem)
            .join('')}</ul></details>`;
    }
    if (skipped.length) {
        html += `<details><summary>Not applicable (${skipped.length})</summary><ul class="results">${skipped
            .map(resultItem)
            .join('')}</ul></details>`;
    }
    return html;
}

function bubble(mood, headline, details) {
    return `<section class="bubble ${mood}" aria-live="polite">
  <p class="headline">${escapeHtml(headline)}</p>
  ${details ? `<p class="details">${escapeHtml(details)}</p>` : ''}
</section>`;
}

function resultItem({ r, index }) {
    const where = location(r);
    const text = `<span class="item">${r.number ? `<span class="number">${r.number}.</span> ` : ''}${escapeHtml(
        r.item
    )}</span>${where ? `<span class="where">${escapeHtml(where)}</span>` : ''}${
        r.note ? `<span class="note">${escapeHtml(r.note)}</span>` : ''
    }`;
    if (r.file) {
        return `<li class="${r.verdict}"><button type="button" class="result" data-action="open" data-index="${index}" title="Open ${escapeHtml(
            where
        )}">${text}</button></li>`;
    }
    return `<li class="${r.verdict}"><div class="result">${text}</div></li>`;
}

module.exports = { activate, deactivate };
