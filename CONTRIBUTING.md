# Contributing to myna

Thanks for your interest in improving myna!

## Ways to contribute

- **Add concepts** to the right category file in `data/concepts/` (see [docs/DATA_FORMAT.md](docs/DATA_FORMAT.md)).
- **Improve checklists**: every concept has `requirements` and `done_when`
  items, which the prompt builder and AI tools use. Add what's missing, fix
  what's wrong, and check against the references in
  [data/SOURCES.md](data/SOURCES.md) (see "Writing good checklists" in the
  data format).
- **Fix bugs** or **add features** — check [docs/ROADMAP.md](docs/ROADMAP.md) for ideas.
- **Improve the docs.**

## Workflow

1. Fork the repository and create a branch from `main`.
2. Make your change. Keep commits focused.
3. Run the checks:
   ```sh
   cmake -S . -B build && cmake --build build && ctest --test-dir build
   python scripts/check_catalogue.py data/concepts
   ```
4. Open a pull request describing what changed and why.

For larger changes, open an issue first so we can agree on the approach.

## Guidelines

- Follow the code style in [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).
- Add or update tests for behaviour changes.
- Keep catalogue ids unique and stable — never renumber existing entries.

## Licensing your contribution

Code is licensed under [MIT](LICENSE) and the data in `data/` under
[CC BY 4.0](data/LICENSE). By submitting a contribution you agree to the
[Contributor License Agreement](CLA.md), which lets Myna keep using and
relicensing your work.

By participating, you agree to follow our [Code of Conduct](CODE_OF_CONDUCT.md).
