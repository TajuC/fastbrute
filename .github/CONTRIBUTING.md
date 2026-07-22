# Contributing

Thanks for wanting to help with fastbrute. This page covers how to build and test the project, the bar every change needs to clear, and how to send changes in.

## How the project is laid out

fastbrute has three backends that all have to produce the same results:

- Pure Python in fastbrute/_purepy.py, with no build step.
- The C and C++ extension in fastbrute/_native_src/, built by setup.py.
- The Odin command line tool in odin/, which is standalone and built with build.sh or build.bat.

docs/ARCHITECTURE.md explains how they fit together.

## Build and test

For Python and the native extension you need a C and C++ compiler. On Windows that is the MSVC Build Tools, and elsewhere it is gcc or clang.

```
pip install .
python tests/test_correctness.py
```

To run the tests without dropping bytecode into the tree:

```
python -B tests/test_correctness.py
```

For Odin you need the Odin toolchain on your PATH:

```
cd odin
./build.sh        # build.bat on Windows
./sf test
```

To benchmark:

```
python bench.py
```

## The bar every change has to clear

Before review, a change needs to pass the same checks CI runs.

- The correctness suite passes with python tests/test_correctness.py, and the native backend stays byte for byte identical to the hashlib reference. The Odin self check, ./sf test, passes too.
- ruff check . is clean.
- The source stays free of comments unless a comment documents something that is genuinely not obvious, such as an unsafe contract, a tricky invariant, or an FFI detail.
- New behavior comes with a test. Any change to a SIMD kernel keeps the hashlib reference tests green across every padding boundary and every ISA tier.

## Commits and pull requests

- Write commit subjects in the imperative mood, describing the change itself.
- Keep each pull request focused on one thing.
- Fill in the pull request template and confirm the checks pass.
- If you claim a speedup, say which machine and ISA tier you measured on and how, using bench.py, and mention any thermal caveats.

## Bugs and feature requests

Open an issue with one of the templates in .github/ISSUE_TEMPLATE. Anything with a security angle should go through SECURITY.md instead of a public issue.
