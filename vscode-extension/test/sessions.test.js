'use strict';

const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { projectKey, isSessionId, findSessions, titleFromLines } = require('../sessions');
const { claudeCodeQuery } = require('../review');

const A = '1c81e703-1b00-49b5-9ceb-d8b384b41f0c';
const B = '9a1f213e-7316-4202-8c77-5c834e496ee4';

function line(entry) {
    return JSON.stringify(entry);
}

/** A fake Claude Code folder with two chats for `projectDir`. */
function fakeClaude(projectDir) {
    const config = fs.mkdtempSync(path.join(os.tmpdir(), 'myna-sessions-'));
    // Claude Code writes the drive letter in either case; use the other one.
    const key = projectKey(projectDir);
    const folder = path.join(config, 'projects', key[0] === key[0].toLowerCase() ? key.toUpperCase() : key.toLowerCase());
    fs.mkdirSync(folder, { recursive: true });

    fs.writeFileSync(
        path.join(folder, `${A}.jsonl`),
        [
            line({ type: 'user', message: { role: 'user', content: 'Fix the five items Myna found' } }),
            line({ type: 'ai-title', aiTitle: 'Fixing review items' }),
            line({ type: 'ai-title', aiTitle: 'Myna review of Settings page' }),
            '{"type":"assistant", cut off',
        ].join('\n')
    );
    fs.writeFileSync(
        path.join(folder, `${B}.jsonl`),
        line({ type: 'user', message: { content: [{ type: 'text', text: '<ide>x</ide> Build a settings page' }] } })
    );
    fs.writeFileSync(path.join(folder, 'notes.jsonl'), 'not a session');
    fs.mkdirSync(path.join(folder, 'memory'));

    const now = Date.now() / 1000;
    fs.utimesSync(path.join(folder, `${A}.jsonl`), now, now);
    fs.utimesSync(path.join(folder, `${B}.jsonl`), now - 3600, now - 3600);
    return config;
}

test('turns a project path into Claude Code folder name', () => {
    // On Windows the drive letter comes first, so only check the end.
    assert.match(projectKey('/home/me/My App (copy)'), /-home-me-My-App--copy-$/);
    assert.match(projectKey('/home/me/My App (copy)'), /^[a-zA-Z0-9-]+$/);
});

test('finds the project chats, newest first, with their titles', () => {
    const project = path.join(os.tmpdir(), 'Compare-B');
    const config = fakeClaude(project);
    try {
        const sessions = findSessions(project, { configDir: config });
        assert.deepEqual(
            sessions.map((s) => [s.id, s.title]),
            [
                [A, 'Myna review of Settings page'],
                [B, 'Build a settings page'],
            ]
        );
        assert.ok(sessions[0].modified > sessions[1].modified);
        assert.equal(findSessions(project, { configDir: config, limit: 1 }).length, 1);
    } finally {
        fs.rmSync(config, { recursive: true, force: true });
    }
});

test('finds nothing, without failing, for unknown projects or folders', () => {
    assert.deepEqual(findSessions('/no/such/project', { configDir: '/no/such/claude' }), []);
    const config = fs.mkdtempSync(path.join(os.tmpdir(), 'myna-empty-'));
    try {
        assert.deepEqual(findSessions('/some/project', { configDir: config }), []);
    } finally {
        fs.rmSync(config, { recursive: true, force: true });
    }
});

test('titles fall back to the first message, shortened to one line', () => {
    assert.equal(titleFromLines([line({ type: 'user', message: { content: 'Hello\n\n  world' } })]), 'Hello world');
    const long = titleFromLines([line({ type: 'user', message: { content: 'x'.repeat(200) } })]);
    assert.equal(long.length, 80);
    assert.ok(long.endsWith('…'));
    assert.equal(titleFromLines(['garbage']), '');
    assert.equal(
        titleFromLines([line({ type: 'user', message: { content: '<ide_opened_file>a.js</ide_opened_file>Fix the menu' } })]),
        'Fix the menu'
    );
});

test('only accepts real session ids', () => {
    assert.ok(isSessionId(A));
    assert.ok(!isSessionId('../../etc/passwd'));
    assert.ok(!isSessionId(''));
});

test('adds the session to the Claude Code link only when given', () => {
    assert.equal(claudeCodeQuery('hi'), 'prompt=hi');
    const query = claudeCodeQuery('fix this & that', A);
    const params = new URLSearchParams(query);
    assert.equal(params.get('prompt'), 'fix this & that');
    assert.equal(params.get('session'), A);
});
