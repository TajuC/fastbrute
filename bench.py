from __future__ import annotations

import multiprocessing as mp
import os
import sys
from time import perf_counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import fastbrute
from fastbrute import _purepy as pure, hashcat

NO_MATCH = b"\xff" * 16


def main() -> None:
    challenge = b"benchmark-challenge"
    print(f"backend: {fastbrute.backend_name()} (native built: {fastbrute.HAVE_NATIVE})")
    if fastbrute.HAVE_NATIVE:
        n = fastbrute._native
        print(f"cpu isa: has_avx2={n.has_avx2()} has_avx512={n.has_avx512()} active={n.backend()}")
    print(f"keyspace: {fastbrute.KEYSPACE:,} candidates")

    if fastbrute.HAVE_NATIVE:
        started = perf_counter()
        fastbrute._native.find(challenge, NO_MATCH, 0)
        elapsed = perf_counter() - started
        rate = fastbrute.KEYSPACE / elapsed
        print(f"native full sweep: {rate / 1e6:,.0f} Mcand/s ({elapsed:.2f}s worst case)")

    span = 1_500_000
    started = perf_counter()
    pure.scan_range(0, span, challenge, NO_MATCH)
    single = span / (perf_counter() - started)
    print(f"pure single core : {single:,.0f} cand/s")

    workers = os.cpu_count() or 1
    chunks = pure._chunks(84_000_000, pure._CHUNK)
    context = mp.get_context("spawn")
    started = perf_counter()
    scanned = 0
    with context.Pool(
        workers, initializer=pure._init_worker, initargs=(challenge, NO_MATCH)
    ) as pool:
        for _, count in pool.imap_unordered(pure._scan_chunk, chunks):
            scanned += count
    aggregate = scanned / (perf_counter() - started)
    print(f"pure {workers} cores    : {aggregate:,.0f} cand/s ({aggregate / single:.1f}x)")
    print(f"pure full sweep  : {fastbrute.KEYSPACE / aggregate:,.0f} s")

    line, cmd = hashcat.recipe(b"\x00" * 16, b"example-challenge")
    print("\nhashcat (sub-second on a GPU):")
    print(f"  {line}")
    print(f"  {cmd}")


if __name__ == "__main__":
    main()
