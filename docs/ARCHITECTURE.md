# Architecture

Myna is split into a reusable core library and a thin terminal interface, with
the concept data kept in a separate file. This keeps the core easy to test and
lets future integrations (prompt builder, local API, editor plugins) reuse it.

```
include/myna/   Public headers
src/core/       Core library (myna_core): catalogue loading, search, prompts
src/cli/        Interactive session, commands and output formatting
                (terminal.c: colour, title, clipboard; logo.c: generated logo;
                mcp.c and json.c: the MCP server)
src/main.c      Entry point; delegates to myna_cli_run()
data/           The concept catalogue (CSV)
assets/         Logo, Windows icon and resource script
```

## Components

| Module      | Header              | Responsibility                                                  |
|-------------|---------------------|-----------------------------------------------------------------|
| concept     | `myna/concept.h`    | The `myna_concept` record: name, explanation, synonyms, triggers, related ids |
| catalogue   | `myna/catalogue.h`  | Growable array of concepts; CSV parsing and loading             |
| search      | `myna/search.h`     | Scored matching of a description, related suggestions, lookup by id |
| prompt      | `myna/prompt.h`     | Builds an AI prompt, or a numbered review rubric, from chosen concepts: merged checklists without duplicates |
| mcp         | `src/cli/mcp.h`     | MCP server over stdio (`myna mcp`): JSON-RPC dispatch and the five tools |
| json        | `src/cli/json.h`    | Small strict JSON reader and writer used by the MCP server |
| cli         | `myna/cli.h`        | Interactive session (`myna_repl`) and one-shot commands         |

## How a search works

1. **Tokenize.** The description is lowercased and split into words. Letters
   and digits form words, `%` is its own word, everything else separates.
   Simple suffixes are stripped (`users` → `user`, `clicking` → `click`).
2. **Weigh words.** Each word of the description gets a weight from how many
   concepts use it: a word used by one or two concepts (`passkey`) weighs 4,
   one used by many (`page`) weighs 1.
3. **Match terms.** Each concept's name, synonyms and triggers are tokenized
   the same way and looked for as consecutive words in the description. Close
   variants that differ only by a common ending match (`animate` ↔
   `animation`, but not `contact` ↔ `container`), and filler words such as
   `the` may sit between the words (`change the language` matches `change
   language`).
4. **Score.** A term found in order adds `4 +` the weights of its words, plus
   a bonus for the concept's name. A term whose words are all present but
   apart adds only the weights. Words left over that appear somewhere in the
   concept's terms add a little: rare words and name words only. Filler words
   never count, a term repeated in the data scores once, and totals below 2
   are dropped as noise.
5. **Rank.** The top 5 concepts by score are returned; ties keep catalogue
   order.
6. **Suggest.** Related concepts of the results that aren't results
   themselves are offered as "Also consider".

## Design notes

- The core library does no I/O beyond reading the catalogue file, and never
  prints, so it is testable and embeddable.
- Concepts use fixed-size fields, so a catalogue is one contiguous allocation.
  Over-long values are truncated on load; `scripts/check_catalogue.py` flags
  them first.
- `myna_repl` takes `FILE *` streams rather than using stdin/stdout directly,
  so the session can be driven by scripts and tests.
- Search is a linear scan, which is fast for thousands of concepts. Indexing
  can come with 0.3 if needed.
- The MCP server answers each message through `myna_mcp_handle()`, which takes
  one line and returns one line, so tests drive it without pipes or processes.
  The checking in `review_checklist` is done by the AI assistant, which can
  read the code; Myna supplies the expert checklist.
- The catalogue is looked up beside the executable before `./data`, because
  AI tools start `myna mcp` inside the user's own project.
