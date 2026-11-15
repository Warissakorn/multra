#!/usr/bin/env python3
"""Check every slice's labelled schema cases against the composed project schema (D574).

Usage: check_schema_labels.py MULTRA_TESTS MULTRA_PROJECT_SCHEMA REPOSITORY [--jobs N]

Runs `MULTRA_TESTS schemalabels` with MULTRA_SCHEMA_LABELS set to dump the cases, validates each
document with Python's jsonschema (draft 2020-12) against `MULTRA_PROJECT_SCHEMA`'s output, and
requires the schema's verdict to equal the case's `schemaAccepts` label. Every `TrafficSim`
project under REPOSITORY's data/ and tests/ must also be accepted. The codec half (every refused
label is refused by parseDocument) runs in each slice's own C++ test.
"""
import glob
import json
import multiprocessing
import os
import subprocess
import sys
import tempfile

import jsonschema

_validator = None


def _init(schema):
    global _validator
    _validator = jsonschema.Draft202012Validator(schema)


def _verdict(document):
    return _validator.is_valid(document)


def main(argv):
    args = [a for a in argv[1:] if not a.startswith("--jobs")]
    jobs = next((int(a.split("=", 1)[1]) for a in argv[1:] if a.startswith("--jobs=")), os.cpu_count() or 1)
    if len(args) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    tests, schema_tool, root = args
    schema = json.loads(subprocess.run([schema_tool], check=True, capture_output=True).stdout)
    with tempfile.TemporaryDirectory() as directory:
        dump = os.path.join(directory, "labels.json")
        run = subprocess.run([tests, "schemalabels"], env={**os.environ, "MULTRA_SCHEMA_LABELS": dump},
                             capture_output=True, text=True)
        if run.returncode != 0 or not os.path.exists(dump):
            print(run.stdout, run.stderr, file=sys.stderr)
            return 1
        with open(dump, encoding="utf-8") as file:
            cases = json.load(file)
    projects = []
    for pattern in ("data/**/*.json", "tests/**/*.json"):
        for path in sorted(glob.glob(os.path.join(root, pattern), recursive=True)):
            try:
                with open(path, encoding="utf-8") as file:
                    document = json.load(file)
            except (OSError, ValueError, UnicodeDecodeError):
                continue
            if isinstance(document, dict) and document.get("format") == "TrafficSim":
                projects.append((os.path.relpath(path, root), document))
    documents = [c["document"] for c in cases] + [d for _, d in projects]
    with multiprocessing.Pool(jobs, _init, (schema,)) as pool:
        verdicts = pool.map(_verdict, documents, chunksize=64)
    failures = [f"{c['slice']}: {c['name']} (schema {'accepts' if v else 'refuses'}, labelled "
                f"{'accepted' if c['accepts'] else 'refused'})" for c, v in zip(cases, verdicts) if v != c["accepts"]]
    failures += [f"project refused: {p}" for (p, _), v in zip(projects, verdicts[len(cases):]) if not v]
    for line in failures[:50]:
        print(line, file=sys.stderr)
    print(f"{len(cases)} labelled cases, {len(projects)} projects, {len(failures)} disagreements")
    return 1 if failures or not cases or not projects else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
