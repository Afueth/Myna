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

Then it **builds a precise prompt** for your AI assistant: what to build,
what a good version must do, and how to check it works. It's the expert
checklist most people don't know to ask for.

And it can work **inside your AI coding tool** (Claude Code, Cursor, GitHub
Copilot and others): the assistant asks Myna for the checklist before it
writes code, and checks its own work against it afterwards.
See [Use Myna inside your AI tool](#use-myna-inside-your-ai-tool).

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

### Then press P to build a prompt

Myna turns your rough request into a complete one, ready to paste into
ChatGPT, Copilot, Claude or any other assistant, and copies it to your
clipboard:

```text
My request: the dialog should close with escape and blur the background

Build it using these techniques:
- Modal dialog: A window that opens on top of the page and blocks interaction
  with the rest of the page until it is dismissed.

Requirements:
- Close it with the Escape key and a visible close button
- Move focus into the dialog when it opens and keep it there while open
- Return focus to the element that opened it when it closes
- Stop the page behind from scrolling while it is open
- Give it role=dialog, aria-modal=true and a label taken from its title

Done when:
- Escape, the close button and, if wanted, a click outside all close it
- Keyboard users cannot tab to the page behind it
- Screen readers announce its title when it opens

Also consider whether these apply: Backdrop blur / overlay,
Outside-click dismissal, Focus management
```

### Why that helps

| Before Myna | After Myna |
|---|---|
| *"The dialog should close with escape and blur the background."* | A request that names the technique, lists what a good dialog must do, and says how to check it. |
| Vague: the AI has to guess what you mean. | Precise: the AI knows exactly what to build. |
| Easy to forget the details. | Myna adds what's easy to miss: focus, scrolling, screen readers. |

## Use Myna inside your AI tool

Myna runs as an [MCP](https://modelcontextprotocol.io) server, the standard
way AI coding assistants connect to tools. Once connected, the assistant can:

| Step | The assistant calls | Myna gives it |
|---|---|---|
| Understand the request | `find_concepts` | The concepts behind your plain-language request |
| Before writing code | `get_checklist` | What a good version must do, and how to check it works |
| After writing code | `review_checklist` | A numbered review rubric to mark PASS or FAIL against the code |
| After checking | `report_review` | Saves the result in your project (`.myna/review.json`), so you can see what passed and what to fix |

Setup is one line in Claude Code:

```console
$ claude mcp add myna -- myna mcp
```

Setup for Cursor, VS Code and others is in [docs/MCP.md](docs/MCP.md).

Not using MCP? `myna review modal-dialog` prints the same review rubric to
paste into any AI chat next to your code.

**See the review in VS Code:** the [Myna extension](vscode-extension/) shows
the latest review next to your files, with warnings on the exact lines.

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

$ myna prompt "close the popup when I click outside"
My request: close the popup when I click outside
...
```

More details are in [Download and install](#download-and-install) below.

## What Myna knows today

Myna knows **187 concepts** from everyday app development, including:

- **Interface:** popups, menus, drawers, bottom sheets, cards, tags, tabs, sliders, tree views, rich text editors, dark mode, loading states
- **Forms and pages:** names, addresses, phone numbers, passwords, dates of birth, check-your-answers, confirmation and error pages
- **Accessibility:** page structure, keyboard use, contrast, captions, touch gestures
- **Accounts and security:** sign-in, passkeys, roles, sessions, security headers, injection, SSRF
- **Data and backend:** APIs (errors, versioning, retries, ETags, batches), GraphQL, caching, search, jobs, databases, multi-tenancy
- **Mobile:** secure storage, WebViews, biometrics, hardening against tampering
- **Performance:** Core Web Vitals, images, lazy loading
- **AI features:** chatbots, document Q&A and AI agents, built safely
- **Releases:** feature flags, gradual rollouts, A/B tests, rollbacks, CI/CD, dependencies

**Every concept comes with a checklist**: about 1,480 requirements and "done
when" checks in total, built on trusted references:

| Reference | Covered |
|---|---|
| [W3C ARIA Authoring Practices](https://www.w3.org/WAI/ARIA/apg/patterns/) | All 30 UI patterns |
| [WCAG 2.2](https://www.w3.org/TR/WCAG22/) | All 55 level A and AA success criteria |
| [OWASP Cheat Sheets](https://cheatsheetseries.owasp.org/) | All 88 sheets about building web, mobile, API and AI apps |
| [OWASP MASVS](https://mas.owasp.org/MASVS/) | All 24 mobile app security controls |
| [Core Web Vitals](https://web.dev/articles/vitals) (web.dev) | All 3 metrics and their supporting metrics |
| [NIST SP 800-63B](https://pages.nist.gov/800-63-4/sp800-63b.html) | Password, session and authenticator rules |
| API guidelines: [RFC 9457](https://www.rfc-editor.org/rfc/rfc9457), [Microsoft](https://github.com/microsoft/api-guidelines), [Zalando](https://opensource.zalando.com/restful-api-guidelines/), [Google AIPs](https://google.aip.dev/general) | Every topic for building APIs |
| Design systems: [component.gallery](https://component.gallery/components/), [GOV.UK](https://design-system.service.gov.uk/patterns/), [USWDS](https://designsystem.digital.gov/components/overview/) | 127 of 136 components and patterns |

[data/source_coverage.csv](data/source_coverage.csv) shows which concept
covers each item, and [data/SOURCES.md](data/SOURCES.md) explains what was
left out and why.

It's an early version: if your request uses words Myna hasn't learned yet, it
may find nothing. Every concept and checklist is written by people, and that's
exactly where you can help.

## Help make Myna smarter

**You don't need to know C to contribute.** Myna's knowledge lives in one
simple file, [data/concepts.csv](data/concepts.csv). Each line is a concept
with a short explanation and the everyday phrases people use for it.

- **Myna didn't understand your request?** [Open an issue](https://github.com/Afueth/Myna/issues/new/choose)
  with the sentence you typed. That's already a useful contribution.
- **Know what a good implementation needs?** Improve a checklist: add a
  missing "requirement" or "done when" line, or make one clearer.
- **Know a concept that's missing?** Add it to the CSV and open a pull
  request. See [CONTRIBUTING.md](CONTRIBUTING.md) and the
  [data format](docs/DATA_FORMAT.md).
- **Like C?** The [roadmap](docs/ROADMAP.md) lists what's coming next:
  better ranking, framework awareness and editor integrations.

## Roadmap

| Version | What it adds |
|---|---|
| 0.1 | Describe a change and see matching concepts with explanations |
| **0.2** (now) | Prompt builder, plus MCP so AI tools use Myna's checklists before and after writing code |
| 0.3 | Better ranking, so results are fewer and more relevant |
| 0.4 | Richer links between concepts: synonyms, prerequisites, conflicts |
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
myna prompt <description>     Build an AI prompt from the best matches
myna review <id>...           Print a checklist for reviewing finished code
myna mcp                      Run as an MCP server for AI coding tools
myna show <id>                Show one concept in full
myna list                     List every concept
myna -d FILE ...              Use a different catalogue file
```

In the interactive session, type a description, then `P` to build a prompt
from some or all of the results (it's copied to your clipboard), `D` (or a
result number) for details and related concepts, `S` to search again, or `Q`
to quit.

`myna prompt` prints the prompt instead, so you can pipe it: for example
`myna prompt "add dark mode" | clip` on Windows or `| pbcopy` on macOS. More examples are in [examples/requests.md](examples/requests.md).

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
- [Using Myna in AI tools (MCP)](docs/MCP.md): setup for Claude Code, Cursor, VS Code and more
- [Does Myna make AI code better?](docs/COMPARISON.md): blind tests with and without Myna
- [Development](docs/DEVELOPMENT.md): building, testing and measuring coverage
- [Roadmap](docs/ROADMAP.md): where Myna is heading

## License

© 2026 Afueth.

- **Code:** MIT. See [LICENSE](LICENSE).
- **Concept data** (everything in [`data/`](data/)): CC BY 4.0. You can use
  it anywhere, including in commercial products, as long as you credit Myna.
  See [data/LICENSE](data/LICENSE).

Contributions are accepted under the [Contributor License Agreement](CLA.md).
Please follow the [Code of Conduct](CODE_OF_CONDUCT.md).
