<p align="center">
  <img src="assets/myna-logo.png" alt="Myna logo" width="160">
</p>

# Myna — Developer Intent Engine

**Turn what you want into the technical language AI understands.**

You often know the result you want without knowing the terms that describe it.
Myna takes a plain-language description and shows the software concepts behind
it, with explanations, so you can ask AI coding assistants (and people)
precisely for what you mean.

> Core principle: help developers express intent precisely, rather than merely
> generate more code.

```console
$ myna
MYNA - Developer Intent Engine
==============================
107 concepts loaded. Type Q to quit.

Describe what you want to implement:
> Make this popup modern, blur the background, animate it, and close it when I click outside.

Possible concepts:

  1. Outside-click dismissal
     Closing a dialog, menu or popover when the user clicks or taps anywhere
     outside it.
     Matched: "click outside", "close it when i click"

  2. Backdrop blur / overlay
  3. Modal dialog
  4. Entry and exit transitions
     ...

Also consider: Escape-key handling, Focus management, Reduced motion

[S] Search again  [D] Details and related concepts  [P] Build prompt  [Q] Quit
```

Myna runs locally, needs no AI service, and is a small C program with a
curated, community-editable concept catalogue.

> **Status:** early prototype (0.1 — Vocabulary Search). See the
> [roadmap](docs/ROADMAP.md) for where it's heading.

## Building

You need a C11 compiler (GCC, Clang or MSVC) and either CMake or Make.

With Make (on Windows with MSYS2, use `mingw32-make`):

```sh
make          # builds build/myna
make test     # runs the tests
```

With CMake:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Usage

```
myna                          Start the interactive session
myna search <description>     Find concepts for a description and exit
myna show <id>                Show one concept in full
myna list                     List every concept
myna -d FILE ...              Use a different catalogue file
```

In a colour terminal, the interactive session opens with the Myna logo.

More examples are in [examples/requests.md](examples/requests.md).

### Windows shortcut

To open Myna like an app, with its own icon and window:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/install-shortcut.ps1
```

This adds a **Myna** shortcut to your Desktop and Start menu.

## The concept catalogue

Concepts live in [data/concepts.csv](data/concepts.csv). Each has a name, an
explanation, synonyms, everyday trigger phrases and related concepts. Adding
and improving concepts is the most valuable contribution; see
[docs/DATA_FORMAT.md](docs/DATA_FORMAT.md).

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Data format](docs/DATA_FORMAT.md)
- [Development](docs/DEVELOPMENT.md)
- [Roadmap](docs/ROADMAP.md)

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Please follow the
[Code of Conduct](CODE_OF_CONDUCT.md).

## License

MIT — see [LICENSE](LICENSE).
