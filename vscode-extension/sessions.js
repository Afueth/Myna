'use strict';

// Finding recent Claude Code chats for a project, so Myna can offer to put
// its message into one of them instead of a new chat.
//
// Claude Code keeps each chat as <session id>.jsonl in
// ~/.claude/projects/<project key>/, where the key is the project's path with
// every character other than a letter or digit replaced by "-". That layout
// is Claude Code's own and not a documented interface, so everything here is
// defensive: anything unexpected means "no chats found", and Myna then opens
// a new chat. Nothing here uses the VS Code API, so it can be tested with Node.

const fs = require('fs');
const os = require('os');
const path = require('path');

const SESSION_ID = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;
const HEAD_BYTES = 256 * 1024; // the title and first message are near the start
const TITLE_MAX = 80;

/** Claude Code's settings folder: CLAUDE_CONFIG_DIR, or ~/.claude. */
function claudeDir(env = process.env) {
    return env.CLAUDE_CONFIG_DIR || path.join(os.homedir(), '.claude');
}

/** The name of the folder Claude Code keeps a project's chats in. */
function projectKey(projectDir) {
    return path.resolve(projectDir).replace(/[^a-zA-Z0-9]/g, '-');
}

/** Whether `id` looks like a Claude Code session id. */
function isSessionId(id) {
    return typeof id === 'string' && SESSION_ID.test(id);
}

/**
 * The project's recent chats, newest first: [{ id, title, modified }].
 * Returns [] when there are none or anything looks unfamiliar.
 */
function findSessions(projectDir, { configDir = claudeDir(), limit = 5 } = {}) {
    try {
        const projects = path.join(configDir, 'projects');
        const key = projectKey(projectDir).toLowerCase();
        // The drive letter's case differs between tools, so compare without case.
        const folder = fs.readdirSync(projects).find((name) => name.toLowerCase() === key);
        if (!folder) {
            return [];
        }
        const dir = path.join(projects, folder);
        return fs
            .readdirSync(dir)
            .filter((name) => name.endsWith('.jsonl') && isSessionId(name.slice(0, -6)))
            .map((name) => {
                const file = path.join(dir, name);
                return { id: name.slice(0, -6), file, modified: fs.statSync(file).mtime };
            })
            .sort((a, b) => b.modified - a.modified)
            .slice(0, limit)
            .map(({ id, file, modified }) => ({ id, title: readTitle(file), modified }));
    } catch {
        return [];
    }
}

/** The chat's AI-written title, else its first message, else "". */
function readTitle(file) {
    let head;
    try {
        const fd = fs.openSync(file, 'r');
        try {
            const buffer = Buffer.alloc(HEAD_BYTES);
            head = buffer.toString('utf8', 0, fs.readSync(fd, buffer, 0, HEAD_BYTES, 0));
        } finally {
            fs.closeSync(fd);
        }
    } catch {
        return '';
    }
    return titleFromLines(head.split('\n'));
}

/** Pick a title from the lines of a chat file: the last AI title, else the first user message. */
function titleFromLines(lines) {
    let aiTitle = '';
    let firstMessage = '';
    for (const line of lines) {
        let entry;
        try {
            entry = JSON.parse(line);
        } catch {
            continue; // the last line may be cut off
        }
        if (!entry || typeof entry !== 'object') {
            continue;
        }
        if (entry.type === 'ai-title' && typeof entry.aiTitle === 'string' && entry.aiTitle.trim()) {
            aiTitle = entry.aiTitle;
        } else if (entry.type === 'user' && !firstMessage) {
            firstMessage = messageText(entry.message);
        }
    }
    return shorten(aiTitle || firstMessage);
}

function messageText(message) {
    const content = message && message.content;
    if (typeof content === 'string') {
        return content;
    }
    if (Array.isArray(content)) {
        const part = content.find((p) => p && p.type === 'text' && typeof p.text === 'string');
        return part ? part.text : '';
    }
    return '';
}

/** One line, without tagged blocks such as editor notes, at most TITLE_MAX characters. */
function shorten(text) {
    const plain = String(text)
        .replace(/<([a-z][\w-]*)[^>]*>[\s\S]*?<\/\1>/gi, ' ')
        .replace(/<[^>]*>/g, ' ')
        .replace(/\s+/g, ' ')
        .trim();
    return plain.length > TITLE_MAX ? `${plain.slice(0, TITLE_MAX - 1)}…` : plain;
}

module.exports = { claudeDir, projectKey, isSessionId, findSessions, titleFromLines };
