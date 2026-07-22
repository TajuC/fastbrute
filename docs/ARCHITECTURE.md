# Architecture

fastbrute recovers the 5-character `secret` in
`md5(challenge + hex(md5(challenge + secret)))` (hashcat mode 4010). MD5 has no
usable shortcut, so the problem is pure throughput over the 62^5 = 916,132,832
candidate keyspace. Three independent backends implement the same search and
must produce identical results.

## The shared algorithm

Every backend applies the same optimizations:

1. **Fold the fixed prefix.** The `challenge` is constant across candidates, so
   its whole MD5 blocks are absorbed once into a midstate (`mid[4]`), and both
   the inner and outer hash resume from it.
2. **Fold the message schedule.** For words that never change, `K[i] +
   message_word` is precomputed, so each round adds a single constant instead of
   recomputing the schedule per candidate. Only the 1-2 words that carry the
   secret (inner) and the 8-9 words that carry the hex digest (outer) vary.
3. **Structure-of-arrays lanes.** The varying words are built directly in SIMD
   lane form, so N candidates are hashed per group.
4. **In-register hex expansion.** The inner digest is expanded to its 32-byte
   lowercase hex form entirely in vector registers.
5. **Pre-filter.** The first 32 bits of the outer digest are compared before the
   full 128-bit check, so serialize-and-compare runs only on a candidate match.
6. **Work-stealing.** The keyspace is split into chunks handed out atomically to
   worker threads; the first full match stops the rest.

## Backends

### Pure Python (`fastbrute/_purepy.py`)

A portable fallback with no build step. It hashes with `hashlib`, hoists the
`md5(challenge)` state with `.copy()`, drives a branch-light base-62 odometer,
and fans out across cores with `multiprocessing` (`spawn`). It is the reference
the other backends are tested against.

### Native C/C++ extension (`fastbrute/_native_src/`)

A CPython extension that dispatches at runtime by CPUID to the widest available
kernel:

- `md5_scalar.c` - portable baseline.
- `md5_avx2.c` - 8 lanes.
- `md5_avx512.c` - 16 lanes; each round's boolean function is one `vpternlogd`
  and each rotate is one `vprold`.

Both vector kernels run **two independent MD5 dependency chains interleaved**
(two-way ILP multi-buffering) so the vector execution ports stay busy while a
single chain would stall on its own latency. CPU detection lives in a dedicated
baseline translation unit (`cpu_detect.c`, compiled with no ISA flags) so the
dispatcher can never itself execute an instruction the CPU lacks. `bruteforce.cpp`
owns the CPython boundary, the thread pool, and the work-stealing cursor.

### Odin (`odin/`)

A standalone CLI (`sf`) with no runtime dependencies, an 8-wide `core:simd`
kernel, and its own known-answer self-tests (`./sf test`). It targets the
`x86-64-v3` baseline (AVX2).

## The boundary

There is no FFI between Python and Odin: the Python native backend is C/C++, and
Odin is a separate binary. The only coupling is semantic - all three must
reproduce the same digest and candidate ordering, which the `hashlib`-referenced
correctness suite enforces for the Python and native backends and the Odin
self-test enforces for Odin.

## Testing

`tests/test_correctness.py` pins the Python and native backends against a
`hashlib` reference across MD5 padding boundaries (challenge tail lengths around
51, 55, 56, 60, 63, 64) and across every ISA tier
(`FASTBRUTE_ISA=scalar|avx2|avx512`). The group-of-8 and group-of-16 helpers are
validated digest-for-digest.
