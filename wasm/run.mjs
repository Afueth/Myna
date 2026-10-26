// Runs a WebAssembly build of Myna on Node.js.
//   node wasm/run.mjs [program.wasm] [args...]
// With no .wasm file given it runs build/wasm/myna.wasm.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { WASI } from 'node:wasi';

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, '..');

let args = process.argv.slice(2);
let program = path.join(root, 'build', 'wasm', 'myna.wasm');
if (args[0]?.endsWith('.wasm'))
    program = path.resolve(args.shift());

if (!fs.existsSync(program)) {
    console.error(`${path.relative(process.cwd(), program)} not found: run "npm run build" in wasm/ first.`);
    process.exit(1);
}

// Node's WASI cannot open folders on Windows, so the catalogue folder is
// merged into one CSV the way Myna loads it: files in name order, one header.
function mergedCatalogue() {
    const dir = path.join(root, 'data', 'concepts');
    const merged = path.join(root, 'build', 'wasm', 'concepts.csv');
    const files = fs.readdirSync(dir).filter(f => f.endsWith('.csv')).sort();
    const built = fs.existsSync(merged) ? fs.statSync(merged).mtimeMs : 0;
    if (files.some(f => fs.statSync(path.join(dir, f)).mtimeMs > built)) {
        let text = '';
        files.forEach((f, i) => {
            const lines = fs.readFileSync(path.join(dir, f), 'utf8').split(/\r?\n/);
            text += lines.slice(i === 0 ? 0 : 1).filter(Boolean).join('\n') + '\n';
        });
        fs.writeFileSync(merged, text);
    }
    return '/myna/build/wasm/concepts.csv';
}

// The current folder is "/" so relative paths (like .myna/review.json) work;
// the Myna folder is "/myna" so the catalogue is found from anywhere.
const env = {
    MYNA_DATA: process.env.MYNA_DATA || mergedCatalogue(),
};
for (const name of ['TERM', 'NO_COLOR', 'COLORTERM'])
    if (process.env[name] !== undefined)
        env[name] = process.env[name];

const wasi = new WASI({
    version: 'preview1',
    args: [path.basename(program, '.wasm'), ...args],
    env,
    preopens: { '/': process.cwd(), '/myna': root },
    returnOnExit: true,
});

const module = await WebAssembly.compile(fs.readFileSync(program));
const instance = await WebAssembly.instantiate(module, wasi.getImportObject());
process.exitCode = wasi.start(instance);
