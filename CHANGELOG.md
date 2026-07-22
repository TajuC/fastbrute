# Changelog

All notable changes to this project are recorded here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-07-22

First public release.

### Added
- A pure Python backend in fastbrute/_purepy.py. It uses multiprocessing with spawn, a branch light base 62 odometer scanner, and a hashlib based reference.
- A C and C++ native extension that dispatches at run time by CPUID to scalar, AVX2 with 8 lanes, and AVX-512 with 16 lanes. The fixed challenge prefix is folded into an MD5 midstate, the message schedule is folded into constants worked out ahead of time, the inner digest is expanded to hex entirely inside the SIMD registers, the first 32 digest bits are checked before the full 128 bit compare, and the keyspace is split across cores with work stealing.
- Two-way ILP multi-buffering in the AVX2 and AVX-512 kernels, meaning two independent MD5 dependency chains interleaved to keep the vector execution ports busy.
- A standalone Odin command line tool, sf, with an 8 wide core:simd kernel and self tests.
- A hashcat recipe printer for mode 4010, the `md5($salt.md5($salt.$pass))` construction.
- A correctness suite pinned against hashlib across MD5 padding boundaries and every ISA tier.

### Security
- Range, length, and alphabet validation on the public helpers candidate_at, index_of, group8, and group16. Out of range group8 and group16 inputs now raise instead of reading out of bounds.
- Exception safe worker creation in the native search. A thread creation failure now raises a Python error you can catch instead of terminating the interpreter.
- Bounds checked and validated hex decoding in the Odin command line tool.
- A genuine atomic cancellation flag shared across the C worker threads on every supported compiler.
