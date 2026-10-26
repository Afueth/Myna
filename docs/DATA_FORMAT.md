# Catalogue data format

The catalogue is a UTF-8 CSV file, by default `data/concepts.csv`. It is the
heart of Myna: search quality depends far more on good data than on code.

## Layout

The first line is a header and must be exactly:

```
id,name,explanation,synonyms,triggers,related,requirements,done_when
```

Each following line is one concept:

| Field         | Limit            | Notes                                                        |
|---------------|------------------|--------------------------------------------------------------|
| `id`          | 47 bytes         | Unique, lowercase words joined by `-`, e.g. `modal-dialog`.  |
| `name`        | 63 bytes         | Display name, e.g. `Modal dialog`.                           |
| `explanation` | 511 bytes        | Plain-language explanation. Quote it if it contains commas.  |
| `synonyms`    | 8 items          | Other technical names, `;`-separated, e.g. `popup;dialog`.   |
| `triggers`    | 16 items         | Everyday phrases people type, e.g. `click outside;tap outside`. |
| `related`     | 8 items          | Ids of related concepts, `;`-separated.                      |
| `requirements`| 8 items          | What a good implementation must do, e.g. `Close it with the Escape key`. Optional. |
| `done_when`   | 6 items          | How to check it works, e.g. `Keyboard users cannot tab out of it`. Optional. |

Items in `synonyms`, `triggers` and `related` may be up to 63 bytes; items in
`requirements` and `done_when` are sentences and may be up to 127 bytes. Fields that contain commas must be
wrapped in double quotes; write a literal quote as `""`. Records must fit on
one line. Blank lines and lines starting with `#` are ignored.

## Example

```
modal-dialog,Modal dialog,"A window that opens on top of the page, blocking it.",popup;dialog,pop up;dialog box,backdrop-overlay;escape-key
```

## How matching uses the data

Myna matches the concept's **name, synonyms and triggers** against the user's
description:

- Matching ignores case and punctuation, and treats simple variants as equal
  (`users`/`user`, `clicking`/`click`, `animate`/`animation`).
- `%` counts as a word, so the trigger `% of users` matches "10% of users".
- Longer phrases score higher than single words, so prefer specific triggers
  such as `blur the background` over `background`.
- Common filler words (`the`, `it`, `make`, `only`…) never match on their own.

## Writing good checklists

`requirements` and `done_when` feed the prompt builder (`[P]` in the
interactive session, or `myna prompt`). They are what turns a vague request
into a precise one, so they are the most valuable part of a concept.

- Write each item as a short instruction or a checkable outcome, in plain
  words: `Close it with the Escape key`, not `Escape support`.
- Cover what people forget: keyboard use, screen readers, errors, edge cases,
  mobile, performance.
- Keep items independent of any framework, and don't use `;` inside an item.
- Repeating an item that another concept also lists is fine: the prompt
  builder lists it once.

## Writing good triggers

- Think about how someone who doesn't know the term would describe it.
- Don't repeat a phrase that differs only by plural or `-ing`; it's already
  covered by variant matching.
- Avoid very generic single words that would match unrelated requests.
- After editing, run the checker and tests and try your example sentences in
  the interactive session.

## Source coverage

`data/source_coverage.csv` records which concepts cover each W3C ARIA
pattern, WCAG 2.2 A/AA success criterion and OWASP cheat sheet:

```
source,item,concepts,note
APG,Checkbox,checkbox,
WCAG,2.4.2 Page Titled,page-structure,
OWASP,Drone Security,,excluded: specialised domain
```

An item with no concepts must have a note starting with `excluded:`. When a
change adds or removes what a concept covers, update this file too. See
`data/SOURCES.md` for the sources themselves.

## Validation

```sh
python scripts/check_catalogue.py data/concepts.csv
```

The checker reports wrong field counts, empty or over-long values, invalid or
duplicate ids, lists over their limit, and related ids that don't exist. It
also checks `source_coverage.csv`: every concept it names must exist, and
every item without concepts must be marked as excluded. CI
runs it on every push.
