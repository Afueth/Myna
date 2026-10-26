'use strict';

// Reading and summarising Myna review files (.myna/review.json). Nothing in
// here uses the VS Code API, so it can be tested with plain Node.

const path = require('path');

const VERDICTS = ['pass', 'fail', 'not_applicable'];

/**
 * Parse the text of a review file. Results with a missing item or an unknown
 * verdict are skipped; counts are recomputed from the results rather than
 * trusted. Returns { review } or { error }.
 */
function parseReview(text) {
    let data;
    try {
        data = JSON.parse(text);
    } catch {
        return { error: 'The review file is not valid JSON.' };
    }
    if (!data || typeof data !== 'object' || !Array.isArray(data.results)) {
        return { error: 'The review file has no results.' };
    }

    const results = [];
    for (const r of data.results) {
        if (!r || typeof r.item !== 'string' || !r.item || !VERDICTS.includes(r.verdict)) {
            continue;
        }
        results.push({
            number: positiveInt(r.number),
            item: r.item,
            verdict: r.verdict,
            file: typeof r.file === 'string' && r.file ? r.file : null,
            line: positiveInt(r.line),
            note: typeof r.note === 'string' && r.note ? r.note : null,
        });
    }

    const counts = { pass: 0, fail: 0, not_applicable: 0 };
    for (const r of results) {
        counts[r.verdict]++;
    }
    const created = typeof data.created === 'string' ? new Date(data.created) : null;

    return {
        review: {
            summary: typeof data.summary === 'string' ? data.summary : '',
            version: typeof data.version === 'string' ? data.version : '',
            created: created && !isNaN(created) ? created : null,
            concepts: Array.isArray(data.concepts) ? data.concepts.filter((c) => typeof c === 'string') : [],
            results,
            counts,
        },
    };
}

function positiveInt(value) {
    return Number.isInteger(value) && value > 0 ? value : null;
}

/** What Myna says about a review: a short headline and a mood for the bird. */
function summarize(review) {
    const { pass, fail } = review.counts;
    const checked = pass + fail;
    const what = review.summary || 'your code';

    if (checked === 0) {
        return { mood: 'empty', headline: `I reviewed ${what}, but no checks applied.` };
    }
    if (fail === 0) {
        return { mood: 'happy', headline: `${what}: all ${checked} checks passed!` };
    }
    return {
        mood: 'worried',
        headline: `${what}: ${pass} of ${checked} checks passed, ${fail} to fix.`,
    };
}

/** "5 minutes ago" and similar. */
function timeAgo(date, now = new Date()) {
    if (!date) {
        return '';
    }
    const seconds = Math.max(0, Math.round((now - date) / 1000));
    const units = [
        ['day', 86400],
        ['hour', 3600],
        ['minute', 60],
    ];
    for (const [name, size] of units) {
        const n = Math.floor(seconds / size);
        if (n >= 1) {
            return `${n} ${name}${n === 1 ? '' : 's'} ago`;
        }
    }
    return 'just now';
}

/** The absolute path of a result's file. Relative paths are relative to the project. */
function resolveFile(projectDir, file) {
    return path.isAbsolute(file) ? file : path.join(projectDir, file);
}

/** "app.js:14", "app.js" or "" for a result. */
function location(result) {
    if (!result.file) {
        return '';
    }
    return result.line ? `${result.file}:${result.line}` : result.file;
}

/** A message to paste into an AI chat, asking it to fix the failed items. */
function buildFixRequest(review) {
    const failed = review.results.filter((r) => r.verdict === 'fail');
    if (failed.length === 0) {
        return '';
    }
    const what = review.summary ? ` of ${review.summary}` : '';
    const lines = [`Myna's review${what} found ${failed.length} item${failed.length === 1 ? '' : 's'} to fix:`, ''];
    failed.forEach((r, i) => {
        const where = location(r);
        lines.push(`${i + 1}. ${r.item}${where ? ` (${where})` : ''}${r.note ? `: ${r.note}` : ''}`);
    });
    lines.push(
        '',
        "Please fix these, then check them again with Myna's review_checklist and report the new results with report_review."
    );
    return lines.join('\n');
}

function escapeHtml(text) {
    return String(text)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
}

module.exports = {
    VERDICTS,
    parseReview,
    summarize,
    timeAgo,
    resolveFile,
    location,
    buildFixRequest,
    escapeHtml,
};
