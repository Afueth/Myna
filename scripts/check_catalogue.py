#!/usr/bin/env python3
"""Validate a myna concept catalogue (see docs/DATA_FORMAT.md)."""

import csv
import re
import sys
from pathlib import Path

EXPECTED_HEADER = ["id", "name", "explanation", "synonyms", "triggers", "related",
                   "requirements", "done_when"]

# Byte limits match the buffers in include/myna/concept.h (size minus the NUL).
FIELD_LIMITS = {"id": 47, "name": 63, "explanation": 511}
TERM_LIMIT = 63
LIST_LIMITS = {"synonyms": 8, "triggers": 16, "related": 8}
# Checklist items are sentences, so they get more room than terms.
ITEM_LIMIT = 127
ITEM_LIST_LIMITS = {"requirements": 8, "done_when": 6}
ID_PATTERN = re.compile(r"^[a-z0-9]+(-[a-z0-9]+)*$")


def split_list(value: str) -> list[str]:
    return [item.strip() for item in value.split(";") if item.strip()]


def check(path: Path) -> list[str]:
    errors = []
    rows = []
    seen_ids = {}

    with path.open(encoding="utf-8-sig", newline="") as fh:
        reader = csv.reader(fh)
        header = next(reader, None)
        if header is None:
            return [f"{path}: file is empty"]
        if header != EXPECTED_HEADER:
            errors.append(f"{path}:1: header must be {','.join(EXPECTED_HEADER)}")

        for record in reader:
            lineno = reader.line_num
            if not record or not "".join(record).strip() or record[0].startswith("#"):
                continue
            if len(record) != len(EXPECTED_HEADER):
                errors.append(
                    f"{path}:{lineno}: expected {len(EXPECTED_HEADER)} fields, "
                    f"found {len(record)} (quote fields that contain commas)"
                )
                continue
            rows.append((lineno, dict(zip(EXPECTED_HEADER, record))))

    for lineno, row in rows:
        where = f"{path}:{lineno}"
        cid = row["id"].strip()

        for field, limit in FIELD_LIMITS.items():
            value = row[field].strip()
            if not value:
                errors.append(f"{where}: '{field}' is empty")
            elif len(value.encode("utf-8")) > limit:
                errors.append(f"{where}: '{field}' exceeds {limit} bytes")

        if cid and not ID_PATTERN.match(cid):
            errors.append(f"{where}: id '{cid}' should be lowercase words joined by '-'")
        if cid in seen_ids:
            errors.append(f"{where}: duplicate id '{cid}' (first seen on line {seen_ids[cid]})")
        else:
            seen_ids[cid] = lineno

        for field, limit in LIST_LIMITS.items():
            items = split_list(row[field])
            if len(items) > limit:
                errors.append(f"{where}: '{field}' has {len(items)} items (max {limit})")
            for item in items:
                if len(item.encode("utf-8")) > TERM_LIMIT:
                    errors.append(f"{where}: '{field}' item '{item}' exceeds {TERM_LIMIT} bytes")

        for field, limit in ITEM_LIST_LIMITS.items():
            items = split_list(row[field])
            if len(items) > limit:
                errors.append(f"{where}: '{field}' has {len(items)} items (max {limit})")
            for item in items:
                if len(item.encode("utf-8")) > ITEM_LIMIT:
                    errors.append(f"{where}: '{field}' item '{item}' exceeds {ITEM_LIMIT} bytes")

        if not split_list(row["synonyms"]) and not split_list(row["triggers"]):
            errors.append(f"{where}: needs at least one synonym or trigger to be findable")

    for lineno, row in rows:
        for rel in split_list(row["related"]):
            if rel not in seen_ids:
                errors.append(f"{path}:{lineno}: related id '{rel}' does not exist")
            elif rel == row["id"].strip():
                errors.append(f"{path}:{lineno}: concept lists itself as related")

    return errors


def catalogue_ids(path: Path) -> set[str]:
    with path.open(encoding="utf-8-sig", newline="") as fh:
        return {row[0].strip() for row in csv.reader(fh)
                if row and row[0].strip() and not row[0].startswith("#")} - {"id"}


def check_coverage(path: Path, ids: set[str]) -> list[str]:
    """Check source_coverage.csv: every concept it names must exist, and items
    without concepts must say why they are excluded."""
    errors = []
    seen = set()
    with path.open(encoding="utf-8-sig", newline="") as fh:
        reader = csv.reader(fh)
        if next(reader, None) != ["source", "item", "concepts", "note"]:
            return [f"{path}:1: header must be source,item,concepts,note"]
        for record in reader:
            where = f"{path}:{reader.line_num}"
            if not record or not "".join(record).strip() or record[0].startswith("#"):
                continue
            if len(record) != 4:
                errors.append(f"{where}: expected 4 fields, found {len(record)}")
                continue
            source, item, concepts, note = (field.strip() for field in record)
            if (source, item) in seen:
                errors.append(f"{where}: '{source} {item}' is listed twice")
            seen.add((source, item))
            listed = split_list(concepts)
            if not listed and not note.startswith("excluded:"):
                errors.append(f"{where}: '{item}' has no concepts and no 'excluded:' note")
            for cid in listed:
                if cid not in ids:
                    errors.append(f"{where}: concept '{cid}' does not exist")
    return errors


def main() -> int:
    path = Path(sys.argv[1] if len(sys.argv) > 1 else "data/concepts.csv")
    if not path.is_file():
        print(f"error: {path} not found", file=sys.stderr)
        return 2

    errors = check(path)
    coverage = path.with_name("source_coverage.csv")
    if coverage.is_file():
        errors += check_coverage(coverage, catalogue_ids(path))
    for err in errors:
        print(err, file=sys.stderr)
    if errors:
        print(f"{len(errors)} problem(s) found", file=sys.stderr)
        return 1

    print(f"{path}: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
