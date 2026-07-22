# fastbrute

fastbrute finds the 5 character secret hidden inside the hashcat mode 4010 hash. You give it a challenge and a target digest, and it recovers the secret that produced them. It comes in three versions that share one core: plain Python, a C and C++ extension that uses AVX2 and AVX-512, and a small standalone program written in Odin.

[![CI](https://github.com/TajuC/fastbrute/actions/workflows/ci.yml/badge.svg)](https://github.com/TajuC/fastbrute/actions/workflows/ci.yml)
[![CodeQL](https://github.com/TajuC/fastbrute/actions/workflows/codeql.yml/badge.svg)](https://github.com/TajuC/fastbrute/actions/workflows/codeql.yml)
[![OpenSSF Scorecard](https://api.securityscorecards.dev/projects/github.com/TajuC/fastbrute/badge)](https://securityscorecards.dev/viewer/?uri=github.com/TajuC/fastbrute)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
![Python](https://img.shields.io/badge/python-3.9%2B-blue)
![Native](https://img.shields.io/badge/native-x86--64%20AVX2%20and%20AVX--512-informational)

## Contents

- [What it solves](#what-it-solves)
- [Please use it responsibly](#please-use-it-responsibly)
- [Speed](#speed)
- [Install](#install)
- [Using it](#using-it)
- [How it works](#how-it-works)
- [Working on fastbrute](#working-on-fastbrute)
- [Reporting security issues](#reporting-security-issues)
- [License](#license)

## What it solves

The hash looks like this:

```
md5(challenge + hex(md5(challenge + secret)))  ==  target
```

You hand fastbrute the challenge bytes and the 16 byte target digest, and it looks for the 5 character secret that makes the two sides equal. The secret is drawn from 62 symbols, the digits 0 through 9 and the letters a through z and A through Z, so there are 916,132,832 possible secrets to try.

This is exactly hashcat mode 4010, which hashcat writes as `md5($salt.md5($salt.$pass))`. MD5 has no shortcut anyone can use, so the only thing that matters is how many candidates you can hash per second. fastbrute leans on that in a few ways. It hashes the fixed challenge prefix a single time and reuses that state for every candidate. It works out the parts of the MD5 message schedule that never change ahead of time and keeps them as constants. It builds only the handful of words that do change straight into SIMD lanes, turns the inner digest into hex without ever leaving the vector registers, checks the first 32 bits of the result before it bothers with the full 128 bit compare, and hands slices of the keyspace to every core with a work stealing loop.

## Please use it responsibly

fastbrute is a tool for security research and password auditing. Only run it against hashes, systems, and data that you own or have clear permission to test. Good examples are CTF challenges, your own credentials, a penetration test you were hired to do, and academic or defensive research. Running it against anything you do not own or are not allowed to test can be illegal, and that is on you.

## Speed

The full search covers 916,132,832 candidates. If you have a GPU, use it. hashcat with mode 4010 finishes in under a second and needs none of the code here. fastbrute is the fast option when you want to stay on the CPU with no dependencies, and it is a good look at how far a carefully folded and vectorized MD5 can go.

Here are the numbers I measured.

| Backend and tier | Rate | Where |
|---|---|---|
| hashcat mode 4010 on a GPU | under a second | The right tool when a GPU is available. |
| Native AVX-512, 16 wide, two chains interleaved | about 192 million candidates per second | A 5 core Xeon. |
| Native AVX2, 8 wide, two chains interleaved | about 410 million candidates per second at peak | An i7-14700HX, Raptor Lake. |
| Native scalar | the portable baseline | Any x86-64 CPU. |
| Pure Python across every core | minutes | Already at the CPython ceiling. Try PyPy or the native build. |

Each figure is a single cold sweep. If you run sweeps back to back, a laptop will thermally throttle and read lower. Interleaving two independent MD5 chains, which the code calls two-way ILP, is worth roughly 22 percent over a single chain on AVX2 in the setup I tested. You can reproduce all of this with bench.py, and you can pin a tier by setting the environment variable FASTBRUTE_ISA to scalar, avx2, or avx512.

## Install

The native extension is portable. It compiles a scalar baseline plus separate AVX2 and AVX-512 kernels, then picks one at run time based on what your CPU reports through CPUID. One build runs on any x86-64 machine and steps up when the hardware allows. You need a C and C++ compiler, which means the MSVC Build Tools on Windows, or gcc or clang everywhere else.

```
pip install .
```

If the native module is missing, fastbrute falls back to pure Python on its own. That path needs no build and still uses every core.

## Using it

From Python:

```python
from fastbrute import find_secret

result = find_secret(challenge, expected)
```

`find_secret` returns the secret and the index where it was found as a two item tuple, for example `b'ax9Q2'` and `500123`. It returns `None` if no candidate matched.

From the command line, where the challenge can be raw hex if you prefix it with `0x`:

```
python -m fastbrute <target_digest_hex> <challenge>
```

On Windows, call `find_secret` from inside an `if __name__ == "__main__":` block, because the pure Python fallback starts its workers with spawn.

### The hashcat recipe

If you would rather hand the problem to a GPU, fastbrute can print the hash.txt line and the command for you:

```
python -m fastbrute.hashcat <target_digest_hex> <challenge>
```

Then run hashcat:

```
hashcat -m 4010 -a 3 -1 ?l?u?d hash.txt ?1?1?1?1?1
```

### Odin

The Odin build is a single self contained program with no runtime dependencies:

```
cd odin
./build.sh                 # build.bat on Windows
./sf <target_digest_hex> <challenge>
./sf test                  # runs the known answer self check
```

On Windows, Odin links through MSVC, so build it from an x64 Native Tools Command Prompt for VS, or make sure both Odin and the MSVC tools are on your PATH. The build targets the x86-64-v3 baseline, which is AVX2. Swap in -microarch:native if you want to tune it for one specific machine.

## How it works

The full write up is in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). It walks through the shared algorithm, the prefix folding, the schedule folding, the SIMD lanes, the in register hex expansion, the 32 bit pre-filter, and the work stealing loop. It also covers the three backends and the two chain interleaving inside the vector kernels.

## Working on fastbrute

```
pip install .                       # build the native extension
python tests/test_correctness.py    # the correctness suite, checked against hashlib
ruff check .                        # lint
python bench.py                     # benchmark
cd odin && ./build.sh && ./sf test  # build and self check the Odin program
```

Contributions are welcome. Have a look at [.github/CONTRIBUTING.md](.github/CONTRIBUTING.md) and the [changelog](CHANGELOG.md) before you start.

## Reporting security issues

If you find a vulnerability, please report it privately through GitHub Security Advisories instead of opening a public issue. The full policy is in [.github/SECURITY.md](.github/SECURITY.md).

## License

fastbrute is released under the MIT license. Copyright 2026 TajuC.
