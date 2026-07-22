from __future__ import annotations

import binascii
import hashlib
import multiprocessing as mp
import os
import string
from time import perf_counter
from typing import Final

ALPHABET: Final[bytes] = (string.digits + string.ascii_letters).encode("ascii")
SECRET_LENGTH: Final[int] = 5
BASE: Final[int] = len(ALPHABET)
KEYSPACE: Final[int] = BASE ** SECRET_LENGTH

Match = tuple[bytes, int]

_CHUNK: Final[int] = 4_000_000


_POSITIONS: Final[dict[int, int]] = {value: i for i, value in enumerate(ALPHABET)}


def candidate_at(index: int) -> bytes:
    if not 0 <= index < KEYSPACE:
        raise ValueError("index out of range")
    digits = [0] * SECRET_LENGTH
    for pos in reversed(range(SECRET_LENGTH)):
        index, digits[pos] = divmod(index, BASE)
    return bytes(ALPHABET[d] for d in digits)


def index_of(candidate: bytes) -> int:
    if len(candidate) != SECRET_LENGTH:
        raise ValueError("candidate must be exactly 5 bytes")
    index = 0
    for byte in candidate:
        if byte not in _POSITIONS:
            raise ValueError("candidate contains a byte outside the alphabet")
        index = index * BASE + _POSITIONS[byte]
    return index


def report(tested: int, elapsed: float, result: Match | None) -> None:
    rate = tested / elapsed if elapsed else 0.0
    status = "found" if result else "exhausted"
    print(f"[{status}] tested {tested:,} candidates ({rate:,.0f}/s, {elapsed:.2f}s)")


def scan_range(lo: int, hi: int, challenge: bytes, expected: bytes) -> tuple[Match | None, int]:
    if lo >= hi:
        return None, 0
    seed = hashlib.md5(challenge, usedforsecurity=False).copy
    hexlify = binascii.hexlify
    alphabet = ALPHABET
    first = expected[0]
    digits = [0] * SECRET_LENGTH
    remaining = lo
    for pos in reversed(range(SECRET_LENGTH)):
        remaining, digits[pos] = divmod(remaining, BASE)
    d0, d1, d2, d3, d4 = digits
    cand = bytearray((alphabet[d0], alphabet[d1], alphabet[d2], alphabet[d3], alphabet[d4]))

    count = 0
    n = lo
    while n < hi:
        inner = seed()
        inner.update(cand)
        outer = seed()
        outer.update(hexlify(inner.digest()))
        digest = outer.digest()
        count += 1
        if digest[0] == first and digest == expected:
            return (bytes(cand), n + 1), count
        n += 1
        if n >= hi:
            break
        d4 += 1
        if d4 < BASE:
            cand[4] = alphabet[d4]
        else:
            d4 = 0
            cand[4] = alphabet[0]
            d3 += 1
            if d3 < BASE:
                cand[3] = alphabet[d3]
            else:
                d3 = 0
                cand[3] = alphabet[0]
                d2 += 1
                if d2 < BASE:
                    cand[2] = alphabet[d2]
                else:
                    d2 = 0
                    cand[2] = alphabet[0]
                    d1 += 1
                    if d1 < BASE:
                        cand[1] = alphabet[d1]
                    else:
                        d1 = 0
                        cand[1] = alphabet[0]
                        d0 += 1
                        cand[0] = alphabet[d0]
    return None, count


_challenge: bytes = b""
_expected: bytes = b""


def _init_worker(challenge: bytes, expected: bytes) -> None:
    global _challenge, _expected
    _challenge, _expected = challenge, expected


def _scan_chunk(bounds: tuple[int, int]) -> tuple[Match | None, int]:
    return scan_range(bounds[0], bounds[1], _challenge, _expected)


def _chunks(total: int, size: int) -> list[tuple[int, int]]:
    return [(lo, min(lo + size, total)) for lo in range(0, total, size)]


def find_secret(
    challenge: bytes,
    expected: bytes,
    *,
    processes: int | None = None,
    verbose: bool = True,
) -> Match | None:
    if len(expected) != 16:
        raise ValueError("expected must be exactly 16 bytes")
    challenge = bytes(challenge)
    expected = bytes(expected)
    workers = processes or os.cpu_count() or 1
    workers = max(1, min(workers, KEYSPACE))
    started = perf_counter()

    if workers == 1:
        result, tested = scan_range(0, KEYSPACE, challenge, expected)
        if verbose:
            report(tested, perf_counter() - started, result)
        return result

    best: Match | None = None
    tested = 0
    context = mp.get_context("spawn")
    with context.Pool(workers, initializer=_init_worker, initargs=(challenge, expected)) as pool:
        for result, count in pool.imap_unordered(_scan_chunk, _chunks(KEYSPACE, _CHUNK)):
            tested += count
            if result is not None:
                best = result
                break
    if verbose:
        report(tested, perf_counter() - started, best)
    return best
