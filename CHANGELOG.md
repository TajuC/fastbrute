# Changelog

All notable changes to this project are documented here. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-07-22

Initial public release.

### Added
- Pure-Python backend (`fastbrute._purepy`): multiprocessing (`spawn`) with a branch-light base-62
  odometer scanner and a `hashlib`-based reference.
- C/C++ native extension with runtime CPUID dispatch to scalar, AVX2 (8-wide), and AVX-512 (16-wide)
  kernels: the fixed challenge prefix is folded into an MD5 midstate, the message schedule is folded
  into precomputed constants, the inner digest is hex-expanded entirely in SIMD registers, the first
  32 digest bits are pre-filtered before the full 128-bit compare, and the keyspace is split across
  cores with work-stealing.
- Two-way ILP multi-buffering in the AVX2 and AVX-512 kernels (two independent MD5 dependency chains
  interleaved to fill vector execution ports).
- Standalone Odin CLI (`sf`) with an 8-wide `core:simd` kernel and self-tests.
- `hashcat` recipe emitter for mode 4010 (`md5($salt.md5($salt.$pass))`).
- Correctness suite pinned against `hashlib` across MD5 padding boundaries and per-ISA tiers.

### Security
- Range/length/alphabet validation on the public helpers (`candidate_at`, `index_of`, `group8`,
  `group16`); out-of-range `group8`/`group16` inputs raise instead of reading out of bounds.
- Exception-safe worker creation in the native search (thread-creation failure raises a catchable
  Python error instead of terminating the interpreter).
- Bounds-checked, validated hex decoding in the Odin CLI.
- Genuine atomic cancellation flag across the C worker threads on all supported compilers.
