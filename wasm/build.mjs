// Builds Myna and its tests as WebAssembly (WASI) with a Clang that itself
// runs on Node.js, so no native compiler is needed. Output: build/wasm/.
import { runClang } from '@yowasp/clang';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '..');
const out = path.join(root, 'build', 'wasm');

// Clang runs in a virtual filesystem: give it the C sources and headers.
function load(dir) {
    const node = {};
    for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
        const full = path.join(dir, entry.name);
        if (entry.isDirectory())
            node[entry.name] = load(full);
        else if (/\.[ch]$/.test(entry.name))
            node[entry.name] = fs.readFileSync(full, 'utf8');
    }
    return node;
}

const tree = {
    src: load(path.join(root, 'src')),
    include: load(path.join(root, 'include')),
    tests: load(path.join(root, 'tests')),
    'wasi_compat.h': fs.readFileSync(path.join(here, 'wasi_compat.h'), 'utf8'),
};

const core = ['src/core/catalogue.c', 'src/core/search.c', 'src/core/prompt.c'];
const cli = ['src/main.c', 'src/cli/cli.c', 'src/cli/terminal.c', 'src/cli/logo.c',
             'src/cli/json.c', 'src/cli/mcp.c', 'src/cli/report.c'];
const mcp = ['src/cli/json.c', 'src/cli/mcp.c', 'src/cli/report.c'];

const flags = ['--target=wasm32-wasip1', '-std=c11', '-O2', '-Wall', '-Wextra', '-Wpedantic',
               '-Iinclude', '-Isrc/cli', '-include', 'wasi_compat.h',
               '-D_WASI_EMULATED_SIGNAL', '-lwasi-emulated-signal'];

const targets = {
    'myna.wasm': [...core, ...cli],
    'test_search.wasm': ['tests/test_search.c', ...core],
    'test_prompt.wasm': ['tests/test_prompt.c', ...core],
    'test_mcp.wasm': ['tests/test_mcp.c', ...mcp, ...core],
};

fs.mkdirSync(out, { recursive: true });
for (const [name, sources] of Object.entries(targets)) {
    const result = await runClang(['clang', ...flags, ...sources, '-o', name], tree);
    fs.writeFileSync(path.join(out, name), result[name]);
    console.log(`built build/wasm/${name}`);
}
