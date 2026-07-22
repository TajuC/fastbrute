# Architecture

fastbrute recovers the 5 character secret in the hash `md5(challenge + hex(md5(challenge + secret)))`, which is hashcat mode 4010. MD5 has no useful shortcut, so the whole problem is throughput over the 62^5 = 916,132,832 candidate keyspace. Three backends run the same search and all have to produce the same results.

## The shared algorithm

Every backend does the same six things.

1. Fold the fixed prefix. The challenge is the same for every candidate, so its whole MD5 blocks are absorbed once into a midstate called mid, and both the inner and outer hash resume from there.
2. Fold the message schedule. For the words that never change, the sum of the round constant and the message word is worked out ahead of time, so each round adds a single constant instead of rebuilding the schedule per candidate. Only the one or two words that carry the secret in the inner hash, and the eight or nine words that carry the hex digest in the outer hash, actually vary.
3. Structure of arrays lanes. The varying words are built straight into SIMD lane form, so several candidates are hashed per group.
4. In register hex expansion. The inner digest is turned into its 32 byte lowercase hex form entirely inside the vector registers.
5. Pre-filter. The first 32 bits of the outer digest are compared before the full 128 bit check, so the serialize and compare path runs only when a candidate looks like a match.
6. Work stealing. The keyspace is split into chunks handed out atomically to worker threads, and the first full match stops the rest.

## Backends

### Pure Python

fastbrute/_purepy.py is a portable fallback with no build step. It hashes with hashlib, keeps the md5 of the challenge state with copy, drives a branch light base 62 odometer, and spreads across cores with multiprocessing using spawn. It is the reference the other backends are tested against.

### Native C and C++ extension

fastbrute/_native_src/ is a CPython extension that dispatches at run time by CPUID to the widest kernel the CPU supports.

- md5_scalar.c is the portable baseline.
- md5_avx2.c runs 8 lanes.
- md5_avx512.c runs 16 lanes, where each round's boolean function is one vpternlogd and each rotate is one vprold.

Both vector kernels run two independent MD5 dependency chains interleaved, which is called two-way ILP multi-buffering, so the vector execution ports stay busy instead of stalling on the latency of a single chain. CPU detection lives in its own baseline translation unit, cpu_detect.c, compiled with no ISA flags, so the dispatcher can never itself run an instruction the CPU lacks. bruteforce.cpp owns the CPython boundary, the thread pool, and the work stealing cursor.

### Odin

odin/ is a standalone command line tool called sf, with no runtime dependencies, an 8 wide core:simd kernel, and its own known answer self tests through ./sf test. It targets the x86-64-v3 baseline, which is AVX2.

## The boundary

There is no FFI between Python and Odin. The Python native backend is C and C++, and Odin is a separate program. The only thing that ties them together is meaning: all three have to reproduce the same digest and the same candidate ordering. The hashlib referenced correctness suite enforces that for the Python and native backends, and the Odin self test enforces it for Odin.

## Testing

tests/test_correctness.py pins the Python and native backends against a hashlib reference across MD5 padding boundaries, meaning challenge tail lengths around 51, 55, 56, 60, 63, and 64, and across every ISA tier through FASTBRUTE_ISA set to scalar, avx2, or avx512. The group of 8 and group of 16 helpers are checked digest for digest.
