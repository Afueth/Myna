# Catalogue data format

The catalogue is a UTF-8 CSV file, by default `data/concepts.csv`. It is the
heart of Myna: search quality depends far more on good data than on code.

## Layout

The first line is a header and must be exactly:

```
id,name,explanation,synonyms,triggers,related
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

Each list item may be up to 63 bytes. Fields that contain commas must be
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

## Writing good triggers

- Think about how someone who doesn't know the term would describe it.
- Don't repeat a phrase that differs only by plural or `-ing`; it's already
  covered by variant matching.
- Avoid very generic single words that would match unrelated requests.
- After editing, run the checker and tests and try your example sentences in
  the interactive session.

## Validation

```sh
python scripts/check_catalogue.py data/concepts.csv
```

The checker reports wrong field counts, empty or over-long values, invalid or
duplicate ids, lists over their limit, and related ids that don't exist. CI
runs it on every push.
