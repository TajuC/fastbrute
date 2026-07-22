# fastbrute

**Throughput-optimized recovery of the 5-character secret in `md5(challenge + hex(md5(challenge + secret)))`** — the hashcat mode-4010 construction — with pure-Python, C/C++ (AVX2 / AVX-512, two-way ILP), and Odin backends on one shared algorithm.

[![CI](https://github.com/TajuC/fastbrute/actions/workflows/ci.yml/badge.svg)](https://github.com/TajuC/fastbrute/actions/workflows/ci.yml)
[![CodeQL](https://github.com/TajuC/fastbrute/actions/workflows/codeql.yml/badge.svg)](https://github.com/TajuC/fastbrute/actions/workflows/codeql.yml)
[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/TajuC/fastbrute/badge)](https://securityscorecards.dev/viewer/?uri=github.com/TajuC/fastbrute)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![Python](https://img.shields.io/badge/python-3.9%2B-blue)
![Platform](https://img.shields.io/badge/native-x86--64%20(AVX2%20%7C%20AVX--512)-informational)

## Contents

- [What it does](#what-it-does)
- [Authorized use](#authorized-use)
- [Performance](#performance)
- [Install](#install)
- [Usage](#usage)
- [How it works](#how-it-works)
- [Development](#development)
- [Security](#security)
- [License](#license)

## What it does

Given the `challenge` bytes and the 16-byte target digest, fastbrute searches for the 5-character
`secret` (drawn from `0-9 a-z A-Z`, 62 symbols, so 62^5 = 916,132,832 candidates) that satisfies

```
md5(challenge + hex(md5(challenge + secret)))  ==  target
```

This is exactly **hashcat mode 4010** (`md5($salt.md5($salt.$pass))`). MD5 has no usable shortcut, so
the whole game is throughput. Every backend hashes the fixed `challenge` prefix once and reuses that
state, folds the message schedule into precomputed constants, builds only the varying words in SIMD
lane form, expands the inner digest to hex inside the vector registers, checks the first 32 digest
bits before the full 128-bit compare, and splits the keyspace across every core with work-stealing.

## Authorized use

fastbrute is a security-research and password-auditing tool. Use it **only** against hashes, systems,
and data you own or are explicitly authorized to test — CTF challenges, your own credentials,
sanctioned penetration tests, and academic or defensive research. Using it against targets you do not
own or lack permission to test may be illegal. You are responsible for how you use it.

## Performance

The whole keyspace is 916,132,832 candidates. A **GPU wins outright** — `hashcat -m 4010` finishes in
under a second and needs none of the code here. fastbrute is the fast *CPU-only, dependency-free*
option and a study in how far a folded, vectorized, multi-buffered MD5 can be pushed.

| Backend / tier | Rate | Notes |
|---|---|---|
| `hashcat -m 4010` (GPU) | sub-second | The right tool if you have a GPU. |
| Native, AVX-512 (16-wide, 2-way ILP) | ~192 Mcand/s | Measured on a 5-core Xeon. |
| Native, AVX2 (8-wide, 2-way ILP) | ~410 Mcand/s peak | Measured on an i7-14700HX (Raptor Lake). |
| Native, scalar | baseline | Portable fallback. |
| Pure Python (all cores) | minutes | At the CPython ceiling; try PyPy or the native build. |

Numbers are single cold sweeps; sustained back-to-back sweeps thermally throttle on a laptop and read
lower. The two-way ILP multi-buffering (two independent MD5 dependency chains interleaved) is worth
about **+22%** over a single chain on AVX2 in the measured environment. Reproduce with `bench.py` and
pin a tier with `FASTBRUTE_ISA=scalar|avx2|avx512`.

## Install

The native extension is portable: it compiles a scalar baseline plus per-file AVX2 and AVX-512 kernels
and selects one at runtime by CPUID, so a single build runs on any x86-64 CPU and steps up when the
hardware allows. Building needs a C/C++ compiler (MSVC Build Tools on Windows; gcc or clang elsewhere):

```
pip install .
```

Pure Python needs no build and uses every core; it is the automatic fallback when the native module is
absent.

## Usage

### Python

```python
from fastbrute import find_secret

result = find_secret(challenge, expected)   # (b'ax9Q2', 500123) or None
```

Command line (the challenge may be raw hex with an `0x` prefix):

```
python -m fastbrute <target_digest_hex> <challenge>
```

On Windows, call `find_secret` from inside `if __name__ == "__main__":` — the pure-Python fallback uses
`spawn`.

### hashcat recipe

Print the `hash.txt` line and command for a challenge:

```
python -m fastbrute.hashcat <target_digest_hex> <challenge>
```

Then, with a GPU:

```
hashcat -m 4010 -a 3 -1 ?l?u?d hash.txt ?1?1?1?1?1
```

### Odin

A standalone binary with no runtime dependencies:

```
cd odin
./build.sh                 # build.bat on Windows
./sf <target_digest_hex> <challenge>
./sf test                  # known-answer self-check
```

On Windows, Odin links with MSVC — build from an "x64 Native Tools Command Prompt for VS" or have both
Odin and the MSVC tools on PATH. The build targets the `x86-64-v3` baseline (AVX2); swap in
`-microarch:native` to tune for the build machine.

## How it works

See [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) for the shared algorithm (prefix folding, schedule
folding, SoA lanes, in-register hex expansion, the 32-bit pre-filter, work-stealing), the three
backends, and the two-way ILP multi-buffering in the vector kernels.

## Development

```
pip install .                       # build the native extension
python tests/test_correctness.py    # correctness suite (pinned against hashlib)
ruff check .                        # lint
python bench.py                     # benchmark
cd odin && ./build.sh && ./sf test  # Odin build + self-test
```

Contributions are welcome — see [`.github/CONTRIBUTING.md`](.github/CONTRIBUTING.md) and the
[changelog](CHANGELOG.md).

## Security

Report vulnerabilities privately via GitHub Security Advisories — see
[`.github/SECURITY.md`](.github/SECURITY.md). Do not open a public issue for a security report.

## License

[MIT](LICENSE) © 2026 TajuC.

The construction targeted here is hashcat mode 4010; the `hashcat` recipe emitter exists so you can
hand the same problem to a GPU when one is available.
