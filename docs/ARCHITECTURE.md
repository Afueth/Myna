# Architecture

Myna is split into a reusable core library and a thin terminal interface, with
the concept data kept in a separate file. This keeps the core easy to test and
lets future integrations (prompt builder, local API, editor plugins) reuse it.

```
include/myna/   Public headers
src/core/       Core library (myna_core): catalogue loading and search
src/cli/        Interactive session, commands and output formatting
                (terminal.c: colour and title; logo.c: generated logo)
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
| cli         | `myna/cli.h`        | Interactive session (`myna_repl`) and one-shot commands         |

## How a search works

1. **Tokenize.** The description is lowercased and split into words. Letters
   and digits form words, `%` is its own word, everything else separates.
   Simple suffixes are stripped (`users` → `user`, `clicking` → `click`).
2. **Match terms.** Each concept's name, synonyms and triggers are tokenized
   the same way and looked for as consecutive words in the description. Close
   variants match (`animate` ↔ `animation`).
3. **Score.** Each matched term adds `4 + 4 × content words`, plus a bonus for
   the concept's name. Filler words don't count, and a term repeated in the
   data scores once.
4. **Rank.** The top 5 concepts by score are returned; ties keep catalogue
   order.
5. **Suggest.** Related concepts of the results that aren't results
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
