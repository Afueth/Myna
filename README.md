<p align="center">
  <img src="assets/myna-logo.png" alt="Myna logo" width="160">
</p>

<h1 align="center">Myna</h1>

<p align="center">
  <strong>Describe what you want in plain words. Myna tells you what it's called.</strong>
</p>

<p align="center">
  <a href="https://github.com/Afueth/Myna/actions/workflows/ci.yml"><img src="https://github.com/Afueth/Myna/actions/workflows/ci.yml/badge.svg" alt="CI"></a>
  <a href="https://github.com/Afueth/Myna/releases/latest"><img src="https://img.shields.io/github/v/release/Afueth/Myna" alt="Latest release"></a>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/Afueth/Myna" alt="MIT license"></a>
  <a href="CONTRIBUTING.md"><img src="https://img.shields.io/badge/PRs-welcome-brightgreen" alt="PRs welcome"></a>
</p>

---

## What is Myna?

You know **what** you want your app to do, but not **what it's called**.

> "Close the popup when I click outside it."

Developers call that **outside-click dismissal**. When you know the right
name, you can search for it, read about it, and ask an AI assistant or a
colleague for exactly that, and get a much better answer.

**Myna finds those names for you.** Type what you want in everyday words, and
Myna shows the matching software concepts, explains each one in a sentence,
and reminds you of related things you might have forgotten.

It runs on your computer, needs no internet or AI service, and is free and
open source.

## See it in action

You type:

```text
Make this popup modern, blur the background, animate it, and close it when I click outside.
```

Myna answers:

```text
  1. Outside-click dismissal
     Closing a dialog, menu or popover when the user clicks or taps anywhere
     outside it.

  2. Backdrop blur / overlay
     A semi-transparent layer behind a dialog that dims or blurs the page
     underneath, often made with the CSS backdrop-filter property.

  3. Modal dialog
     A window that opens on top of the page and blocks interaction with the
     rest of the page until it is dismissed.

  4. Entry and exit transitions
     Animations that play when an element appears or disappears, such as
     fading or sliding in and out.

  5. Visual design system
     A consistent set of colours, typography, spacing, shadows and
     components that makes an interface look modern and coherent.

Also consider: Escape-key handling, Focus management, Reduced motion
```

### Why that helps

| Before Myna | After Myna |
|---|---|
| *"Make this popup modern, blur the background, animate it, and close it when I click outside."* | *"Build a **modal dialog** with a **backdrop blur overlay**, **entry and exit transitions** and **outside-click dismissal**. Close it with the **Escape key**, **trap focus** while it's open, and respect **reduced motion**."* |
| Vague: the AI has to guess what you mean. | Precise: the AI knows exactly what to build. |
| Easy to forget the details. | Myna reminded you of Escape, focus and reduced motion. |

## Who is it for?

- **New developers** who know what they want but not the terms yet
- **Anyone using AI coding assistants** who wants better answers from clearer requests
- **Designers, founders and product people** who want to talk with developers
  in the same language
- **Experienced developers** who want a checklist of related things not to forget

## Try it

**Windows:** download `Myna-Setup-<version>.exe` from the
[latest release](https://github.com/Afueth/Myna/releases/latest), run it, and
open **Myna** from the Start menu.

**macOS and Linux:** download the `.tar.gz` for your system from the
[latest release](https://github.com/Afueth/Myna/releases/latest), unpack it,
and run `./bin/myna` in a terminal.

Then just describe what you want:

```console
$ myna search "only admins can see this page"
  1. Authorization and roles
     Deciding what a signed-in user is allowed to do, usually through roles
     or permissions.
```

More details are in [Download and install](#download-and-install) below.

## What Myna knows today

Myna 0.1 knows **107 concepts** from everyday app development, including:

- **Interface:** popups, menus, tooltips, tabs, forms, dark mode, loading states, responsive layouts
- **Accounts and security:** sign-in, roles, two-factor login, sessions, password safety
- **Data and backend:** APIs, caching, search, file uploads, background jobs, databases
- **Releases:** feature flags, gradual rollouts, A/B tests, rollbacks, CI/CD

It's an early version: if your request uses words Myna hasn't learned yet, it
may find nothing. Every concept is written by people, and that's exactly where
you can help.

## Help make Myna smarter

**You don't need to know C to contribute.** Myna's knowledge lives in one
simple file, [data/concepts.csv](data/concepts.csv). Each line is a concept
with a short explanation and the everyday phrases people use for it.

- **Myna didn't understand your request?** [Open an issue](https://github.com/Afueth/Myna/issues/new/choose)
  with the sentence you typed. That's already a useful contribution.
- **Know a concept that's missing?** Add it to the CSV and open a pull
  request. See [CONTRIBUTING.md](CONTRIBUTING.md) and the
  [data format](docs/DATA_FORMAT.md).
- **Like C?** The [roadmap](docs/ROADMAP.md) lists what's coming next:
  better ranking, a prompt builder, framework awareness and editor
  integrations.

## Roadmap

| Version | What it adds |
|---|---|
| **0.1** (now) | Describe a change and see matching concepts with explanations |
| 0.2 | Richer links between concepts: synonyms, prerequisites, conflicts |
| 0.3 | Better ranking, so results are fewer and more relevant |
| 0.4 | Prompt builder: turn chosen concepts into a ready-to-paste AI prompt |
| 0.5 – 1.0 | Framework awareness, project awareness, a local API and editor integrations |

Full details are in [docs/ROADMAP.md](docs/ROADMAP.md).

---

## Download and install

All downloads are on the
[Releases page](https://github.com/Afueth/Myna/releases/latest).

### Windows installer

Download `Myna-Setup-<version>.exe` and run it. It adds Myna to the Start
menu, can create a desktop shortcut and add `myna` to your PATH, and can be
removed from **Settings → Apps**. No admin rights are needed.

Myna isn't code-signed yet, so Windows may show "Windows protected your PC".
Click **More info → Run anyway** to continue.

### Zip and tar archives

1. Download the archive for your system and unzip it.
2. Run the program in `bin/` from a terminal:

   ```console
   $ ./bin/myna                                    # Windows: .\bin\myna.exe
   $ ./bin/myna search "close the popup when I click outside"
   ```

On Windows you can also run `scripts\install-shortcut.ps1` from the unzipped
folder to add a Myna shortcut to your Desktop and Start menu.

On macOS, if the system blocks the program because it is from an unidentified
developer, run `xattr -d com.apple.quarantine bin/myna` once.

## Usage

```
myna                          Start the interactive session
myna search <description>     Find concepts for a description and exit
myna show <id>                Show one concept in full
myna list                     List every concept
myna -d FILE ...              Use a different catalogue file
```

In the interactive session, type a description, then `D` (or a result
number) for details and related concepts, `S` to search again, or `Q` to
quit. More examples are in [examples/requests.md](examples/requests.md).

## Building from source

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

To open a source build like an app on Windows, with its own icon and window:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/install-shortcut.ps1
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md): how Myna is built
- [Data format](docs/DATA_FORMAT.md): how concepts are written
- [Development](docs/DEVELOPMENT.md): building, testing and measuring coverage
- [Roadmap](docs/ROADMAP.md): where Myna is heading

## License

MIT. See [LICENSE](LICENSE). Please follow the [Code of Conduct](CODE_OF_CONDUCT.md).
