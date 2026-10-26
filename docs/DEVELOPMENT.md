# Development

## Requirements

- A C11 compiler (GCC, Clang or MSVC)
- CMake 3.16+ or GNU Make
- Python 3.9+ for `scripts/check_catalogue.py`

## Build and test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Using Make instead (on Windows with MSYS2 the command is `mingw32-make`):

```sh
make          # builds build/myna
make test     # builds and runs build/test_search
make check    # validates data/concepts.csv
make coverage # reports which example requests Myna answers badly
make clean
```

## Running locally

```sh
./build/myna                                  # interactive session
./build/myna search blur the background       # one-shot search
./build/myna show modal-dialog
./build/myna -d path/to/other.csv list
```

To try the interactive session non-interactively, pipe input into it:

```sh
printf 'close the popup when I click outside\nD\n1\nQ\n' | ./build/myna
```

## Windows setup

1. Install [MSYS2](https://www.msys2.org), then in the *MSYS2 UCRT64* terminal run
   `pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-make`.
2. Add `C:\msys64\ucrt64\bin` to your user `Path` and restart VS Code.
3. Check with `gcc --version`, then build with `mingw32-make` and run
   `.\build\myna.exe`.

## Logo and icon

`assets/myna-logo.png` is the source logo. Two files are generated from it and
committed, so building needs no extra tools:

- `src/cli/logo.c`: the colour terminal logo (half-block characters with
  24-bit colour escapes)
- `assets/myna.ico`: the Windows icon, embedded in `myna.exe` via `assets/myna.rc`

After changing the logo, regenerate both on Windows:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/make_logo.ps1
```

The logo is only drawn when stdout is an interactive terminal. It is skipped
when output is piped or `NO_COLOR` is set.

## Measuring coverage

`tests/requests.csv` holds plain-language requests, each with the concept ids
a good answer should include. `scripts/coverage.py` runs every request through
`myna search` and sorts them into:

- **ok**: the expected concepts were found, and one of them ranked first
- **weak**: they were found, but none ranked first
- **missed**: an expected concept exists but was not returned, so its
  triggers or synonyms need work
- **no match**: Myna returned nothing
- **gap**: the expected concept is not in the catalogue yet

It ends with a list of the missing concepts most often asked for.

```sh
python scripts/coverage.py        # or: make coverage
python scripts/coverage.py -v     # also list the requests that are ok
```

When you see a real request Myna handles badly, add it to `tests/requests.csv`
in your own words, then fix the catalogue until it reports **ok**. This is a
report, not a test, so it is not run in CI.

## Adding a test

Tests live in `tests/` and use a small `CHECK` macro rather than a framework.
`test_search` takes the catalogue path as its first argument and also checks
the bundled dataset, including the example requests from the project overview.
Add a `test_*` function to `tests/test_search.c` and call it from `main`, or
create a new `tests/test_<module>.c` and register it in `CMakeLists.txt` and
the `Makefile`.

## Code style

- C11, 4-space indent, braces on the same line except for function bodies.
- Public symbols are prefixed with `myna_`; file-local helpers are `static`.
- The core library must not print to stdout or stderr.
