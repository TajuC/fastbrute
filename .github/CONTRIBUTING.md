# Contributing

Thanks for your interest in fastbrute. This document covers how to build and test
the project, the quality bar every change must clear, and how to propose changes.

## Project layout

fastbrute has three independent backends that must produce identical results:

- **Pure Python** — `fastbrute/_purepy.py` (no build step).
- **C/C++ native extension** — `fastbrute/_native_src/` (built by `setup.py`).
- **Odin CLI** — `odin/` (standalone, `build.sh` / `build.bat`).

See `docs/ARCHITECTURE.md` for how they fit together.

## Build and test

Python and the native extension (needs a C/C++ compiler — MSVC Build Tools on
Windows, gcc or clang elsewhere):

```
pip install .
python tests/test_correctness.py
```

Run the tests without writing bytecode into the tree:

```
python -B tests/test_correctness.py
```

Odin (needs the Odin toolchain on PATH):

```
cd odin
./build.sh        # build.bat on Windows
./sf test
```

Benchmark:

```
python bench.py
```

## Quality bar

Every change must pass the same gates CI enforces, before review:

- **Correctness**: `python tests/test_correctness.py` passes; the native backend
  stays byte-for-byte identical to the `hashlib` reference. The Odin self-test
  (`./sf test`) passes.
- **Lint**: `ruff check .` is clean.
- **No comments in the hot-path source** unless they document a non-obvious
  invariant, an unsafe contract, or an FFI/ABI detail.
- New behavior needs a test. Any change to a SIMD kernel must keep the
  `hashlib`-reference tests green across all padding boundaries and ISA tiers.

## Commits and pull requests

- Write commit subjects in the imperative mood, describing the change itself.
- Keep each pull request focused on a single concern.
- Fill in the pull request template and confirm the quality bar passes.
- Performance claims must include the machine, the ISA tier, and a repeatable
  measurement (see `bench.py`); note thermal caveats.

## Reporting bugs and requesting features

Open an issue using the templates under `.github/ISSUE_TEMPLATE`. For anything
with a security impact, follow [`SECURITY.md`](SECURITY.md) instead of opening a
public issue.
