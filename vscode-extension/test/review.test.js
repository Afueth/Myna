'use strict';

const test = require('node:test');
const assert = require('node:assert/strict');
const path = require('path');
const {
    parseReview,
    summarize,
    timeAgo,
    resolveFile,
    location,
    buildFixRequest,
    buildReviewRequest,
    escapeHtml,
} = require('../review');

const SAMPLE = JSON.stringify({
    tool: 'myna',
    version: '0.2.1',
    created: '2026-10-26T13:08:03Z',
    summary: 'Settings page',
    concepts: ['toggle-switch', 'dropdown-menu'],
    counts: { pass: 99, fail: 99, not_applicable: 99 },
    results: [
        { number: 1, item: 'Use role=switch', verdict: 'pass', file: 'index.html', line: 51 },
        { number: 14, item: 'Keep the menu on screen', verdict: 'fail', file: 'app.js', line: 156, note: 'No positioning' },
        { number: 15, item: 'Typeahead', verdict: 'fail' },
        { number: 17, item: 'Menubar', verdict: 'not_applicable' },
        { item: 'Broken entry', verdict: 'maybe' },
        { verdict: 'pass' },
    ],
});

test('parses a review and recomputes the counts from the results', () => {
    const { review, error } = parseReview(SAMPLE);
    assert.equal(error, undefined);
    assert.equal(review.summary, 'Settings page');
    assert.equal(review.results.length, 4); // the two broken entries are skipped
    assert.deepEqual(review.counts, { pass: 1, fail: 2, not_applicable: 1 });
    assert.equal(review.results[1].line, 156);
    assert.equal(review.results[2].file, null);
    assert.equal(review.created.toISOString(), '2026-10-26T13:08:03.000Z');
});

test('reports unreadable files instead of throwing', () => {
    assert.ok(parseReview('not json').error);
    assert.ok(parseReview('{"tool":"myna"}').error);
    assert.ok(parseReview('null').error);
});

test('ignores bad line numbers and dates', () => {
    const { review } = parseReview(
        JSON.stringify({ created: 'yesterday', results: [{ item: 'x', verdict: 'fail', line: 0, number: -2 }] })
    );
    assert.equal(review.created, null);
    assert.equal(review.results[0].line, null);
    assert.equal(review.results[0].number, null);
});

test('summarizes with a mood for the bird', () => {
    const { review } = parseReview(SAMPLE);
    assert.deepEqual(summarize(review), {
        mood: 'worried',
        headline: 'Settings page: 1 of 3 checks passed, 2 to fix.',
    });

    const allGood = parseReview(JSON.stringify({ results: [{ item: 'a', verdict: 'pass' }] })).review;
    assert.equal(summarize(allGood).mood, 'happy');
    assert.equal(summarize(allGood).headline, 'your code: all 1 checks passed!');

    const none = parseReview(JSON.stringify({ results: [{ item: 'a', verdict: 'not_applicable' }] })).review;
    assert.equal(summarize(none).mood, 'empty');
});

test('says how long ago a review was made', () => {
    const now = new Date('2026-10-26T12:00:00Z');
    assert.equal(timeAgo(new Date('2026-10-26T11:59:30Z'), now), 'just now');
    assert.equal(timeAgo(new Date('2026-10-26T11:59:00Z'), now), '1 minute ago');
    assert.equal(timeAgo(new Date('2026-10-26T09:00:00Z'), now), '3 hours ago');
    assert.equal(timeAgo(new Date('2026-10-24T12:00:00Z'), now), '2 days ago');
    assert.equal(timeAgo(null, now), '');
});

test('resolves files relative to the project', () => {
    assert.equal(resolveFile('/work/app', 'src/a.js'), path.join('/work/app', 'src/a.js'));
    const absolute = path.resolve('/elsewhere/b.js');
    assert.equal(resolveFile('/work/app', absolute), absolute);
    assert.equal(location({ file: 'a.js', line: 3 }), 'a.js:3');
    assert.equal(location({ file: 'a.js', line: null }), 'a.js');
    assert.equal(location({ file: null, line: null }), '');
});

test('builds a fix request listing only the failed items', () => {
    const { review } = parseReview(SAMPLE);
    const text = buildFixRequest(review, 'C:\\work\\app');
    assert.match(text, /^Myna's review of Settings page found 2 items to fix:/);
    assert.match(text, /1\. Keep the menu on screen \(app\.js:156\): No positioning/);
    assert.match(text, /2\. Typeahead\n/);
    assert.doesNotMatch(text, /role=switch/);

    const allGood = parseReview(JSON.stringify({ results: [{ item: 'a', verdict: 'pass' }] })).review;
    assert.equal(buildFixRequest(allGood, '/work'), '');
});

test('makes reviewing again a required step of the fix request', () => {
    const { review } = parseReview(SAMPLE);
    const text = buildFixRequest(review, 'C:\\work\\app');
    assert.match(text, /A\. Fix every item above\./);
    assert.match(text, /B\. Then review again: call Myna's review_checklist with concept_ids \["toggle-switch", "dropdown-menu"\]/);
    assert.match(text, /report_review with project_dir "C:\\work\\app", summary "Settings page"/);
    assert.match(text, /Step B is required/);
});

test('builds a review-again request with the same concepts and folder', () => {
    const { review } = parseReview(SAMPLE);
    assert.equal(
        buildReviewRequest(review, '/work/app'),
        "Review the code again with Myna (Settings page): call Myna's review_checklist with concept_ids " +
            '["toggle-switch", "dropdown-menu"], check every item against the current code, then call ' +
            `Myna's report_review with project_dir "/work/app", summary "Settings page" and one result per item.`
    );

    const bare = parseReview(JSON.stringify({ results: [{ item: 'a', verdict: 'pass' }] })).review;
    assert.match(buildReviewRequest(bare, '/w'), /^Review the code again with Myna: call Myna's review_checklist with the same concepts as the last review/);
});

test('escapes text before it goes into the view', () => {
    assert.equal(escapeHtml('<img src=x onerror="a()">'), '&lt;img src=x onerror=&quot;a()&quot;&gt;');
    assert.equal(escapeHtml("Tom & Jerry's"), 'Tom &amp; Jerry&#39;s');
});
