#!/usr/bin/env python3
"""Run plain-language requests through `myna search` and report coverage.

Each request in the requests CSV (default tests/requests.csv) is searched with
the built myna program. A request is:

  ok        every expected concept is in the results, one of them first
  weak      the expected concepts are in the results, but none ranks first
  missed    an expected concept that exists is not in the results
  no match  myna returned nothing
  gap       every expected concept is missing from the catalogue

The report ends with the missing concepts most often asked for, so it shows
what to add to the catalogue next.
"""

import argparse
import csv
import os
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RESULT_LINE = re.compile(r"^  (\d+)\. (.+)$")
STATUSES = ["ok", "weak", "missed", "no match", "gap"]


def find_myna() -> Path | None:
    for rel in ("build/myna.exe", "build/myna", "build/Release/myna.exe",
                "build/Debug/myna.exe"):
        path = ROOT / rel
        if path.is_file():
            return path
    return None


def run_myna(myna: Path, data: Path, *args: str) -> subprocess.CompletedProcess:
    env = dict(os.environ, NO_COLOR="1")
    return subprocess.run([str(myna), "-d", str(data), *args], capture_output=True,
                          text=True, encoding="utf-8", env=env, cwd=ROOT)


def load_ids(myna: Path, data: Path) -> dict[str, str]:
    """Map each concept name to its id, from `myna list`."""
    proc = run_myna(myna, data, "list")
    if proc.returncode != 0:
        sys.exit(f"error: myna list failed: {proc.stderr.strip()}")
    ids = {}
    for line in proc.stdout.splitlines():
        parts = line.split(None, 1)
        if len(parts) == 2:
            ids[parts[1].strip()] = parts[0]
    return ids


def search(myna: Path, data: Path, request: str, ids: dict[str, str]) -> list[str]:
    """Return the ids of the results for `request`, best first."""
    proc = run_myna(myna, data, "search", request)
    results = []
    for line in proc.stdout.splitlines():
        m = RESULT_LINE.match(line)
        if m:
            results.append(ids.get(m.group(2).strip(), m.group(2).strip()))
    return results


def load_requests(path: Path) -> list[tuple[str, list[str]]]:
    requests = []
    with path.open(encoding="utf-8-sig", newline="") as fh:
        reader = csv.reader(fh)
        next(reader, None)
        for record in reader:
            if not record or not record[0].strip() or record[0].startswith("#"):
                continue
            expected = record[1] if len(record) > 1 else ""
            requests.append((record[0].strip(),
                             [e.strip() for e in expected.split(";") if e.strip()]))
    return requests


def classify(results: list[str], expected: list[str], known: set[str]) -> str:
    present = [e for e in expected if e in known]
    if expected and not present:
        return "gap"
    if not results:
        return "no match"
    if any(e not in results for e in present):
        return "missed"
    if present and results[0] not in present:
        return "weak"
    return "ok"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("requests", nargs="?", type=Path,
                        default=ROOT / "tests" / "requests.csv")
    parser.add_argument("-d", "--data", type=Path, default=ROOT / "data" / "concepts.csv",
                        help="catalogue CSV (default: data/concepts.csv)")
    parser.add_argument("--myna", type=Path, help="path to the myna program")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="also list the requests that are ok")
    args = parser.parse_args()

    myna = args.myna or find_myna()
    if not myna or not myna.is_file():
        print("error: myna program not found; build it first or pass --myna",
              file=sys.stderr)
        return 2

    ids = load_ids(myna, args.data)
    known = set(ids.values())
    by_status = {s: [] for s in STATUSES}
    wanted = Counter()

    for request, expected in load_requests(args.requests):
        results = search(myna, args.data, request, ids)
        status = classify(results, expected, known)
        by_status[status].append((request, expected, results))
        wanted.update(e for e in expected if e not in known)

    total = sum(len(v) for v in by_status.values())
    print(f"Myna coverage: {total} requests, {len(known)} concepts\n")
    for status in STATUSES:
        n = len(by_status[status])
        print(f"  {status:<9} {n:>4}  ({100 * n / total if total else 0:.0f}%)")

    for status in STATUSES:
        if status == "ok" and not args.verbose:
            continue
        if not by_status[status]:
            continue
        print(f"\n{status.upper()}")
        for request, expected, results in by_status[status]:
            print(f"  \"{request}\"")
            if expected:
                print(f"      expected: {', '.join(expected)}")
            if results:
                print(f"      got:      {', '.join(results)}")

    if wanted:
        print("\nCONCEPTS TO ADD (most requested first)")
        for concept_id, n in wanted.most_common():
            print(f"  {concept_id:<26} {n} request{'s' if n != 1 else ''}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
