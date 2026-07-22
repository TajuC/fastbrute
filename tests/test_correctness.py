from __future__ import annotations

import hashlib
import itertools
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import fastbrute
from fastbrute import _purepy


def reference(challenge: bytes, candidate: bytes) -> bytes:
    inner = hashlib.md5(challenge)
    inner.update(candidate)
    outer = hashlib.md5(challenge)
    outer.update(inner.hexdigest().encode("ascii"))
    return outer.digest()


def test_index_roundtrip():
    rng = random.Random(99)
    samples = [rng.randrange(fastbrute.KEYSPACE) for _ in range(2000)]
    samples += [0, 1, 61, 62, 63, fastbrute.KEYSPACE - 1]
    for i in samples:
        assert fastbrute.index_of(fastbrute.candidate_at(i)) == i
    print("ok  test_index_roundtrip")


def test_ordering_matches_product():
    product = itertools.product(fastbrute.ALPHABET, repeat=fastbrute.SECRET_LENGTH)
    for i, tup in zip(range(5000), product):
        assert fastbrute.candidate_at(i) == bytes(tup), i
    print("ok  test_ordering_matches_product")


def test_scan_every_length():
    rng = random.Random(1234)
    for clen in [0, 1, 45, 50, 51, 52, 55, 56, 59, 60, 61, 62, 63, 64, 65, 120, 127, 128, 200]:
        challenge = bytes(rng.randrange(256) for _ in range(clen))
        index = rng.randrange(fastbrute.KEYSPACE)
        candidate = fastbrute.candidate_at(index)
        expected = reference(challenge, candidate)
        hit, count = _purepy.scan_range(index, index + 1, challenge, expected)
        assert hit == (candidate, index + 1), (clen, index, hit)
        assert count == 1
        miss, _ = _purepy.scan_range(index + 1, index + 2, challenge, expected)
        assert miss is None, (clen, index)
    print("ok  test_scan_every_length")


def test_not_found():
    result, _ = _purepy.scan_range(0, 4096, b"abc", b"\x00" * 16)
    assert result is None
    print("ok  test_not_found")


def test_full_search():
    challenge = b"challenge-string-42"
    secret = fastbrute.candidate_at(20117)
    expected = reference(challenge, secret)
    result = fastbrute.find_secret(
        challenge, expected, backend="python", processes=4, verbose=False
    )
    assert result == (secret, 20118), result
    print("ok  test_full_search")


def test_native_matches_reference():
    if not fastbrute.HAVE_NATIVE:
        print("skip test_native_matches_reference (native not built)")
        return
    rng = random.Random(7)
    for clen in range(0, 64):
        for blocks in (0, 1, 2):
            challenge = bytes(rng.randrange(256) for _ in range(clen + 64 * blocks))
            candidate = fastbrute.candidate_at(rng.randrange(fastbrute.KEYSPACE))
            got = fastbrute._native.hash_one(challenge, candidate)
            assert got == reference(challenge, candidate)
    print("ok  test_native_matches_reference")


def test_native_full_search():
    if not fastbrute.HAVE_NATIVE:
        print("skip test_native_full_search (native not built)")
        return
    for clen in (0, 51, 60, 63, 64):
        challenge = bytes((clen * 7 + i) & 255 for i in range(clen))
        secret = fastbrute.candidate_at(123457)
        expected = reference(challenge, secret)
        result = fastbrute.find_secret(challenge, expected, backend="native", verbose=False)
        assert result == (secret, 123458), (clen, result)
    print("ok  test_native_full_search")


def test_native_group8_matches_reference():
    if not fastbrute.HAVE_NATIVE or not hasattr(fastbrute._native, "group8"):
        print("skip test_native_group8_matches_reference (native not built)")
        return
    if not fastbrute._native.has_avx2():
        print("skip test_native_group8_matches_reference (no avx2 on this cpu)")
        return
    rng = random.Random(2024)
    bases = [0, 1, 3, 7, 8, 55, 61, 62, 63, 64, 62 * 62 - 4, 62 ** 3 - 5, fastbrute.KEYSPACE - 8]
    bases += [rng.randrange(fastbrute.KEYSPACE - 8) for _ in range(4)]
    for clen in range(0, 131):
        challenge = bytes((clen * 31 + i * 7 + 3) & 255 for i in range(clen))
        for base in bases:
            digs = fastbrute._native.group8(challenge, base)
            for j in range(8):
                want = reference(challenge, fastbrute.candidate_at(base + j))
                assert digs[j] == want, (clen, base, j)
    print("ok  test_native_group8_matches_reference")


def test_native_group16_matches_reference():
    if not fastbrute.HAVE_NATIVE or not hasattr(fastbrute._native, "group16"):
        print("skip test_native_group16_matches_reference (native not built)")
        return
    if not fastbrute._native.has_avx512():
        print("skip test_native_group16_matches_reference (no avx512 on this cpu)")
        return
    rng = random.Random(4048)
    bases = [0, 1, 7, 15, 16, 61, 62, 63, 64, 62 * 62 - 8, fastbrute.KEYSPACE - 16]
    bases += [rng.randrange(fastbrute.KEYSPACE - 16) for _ in range(4)]
    for clen in range(0, 131):
        challenge = bytes((clen * 17 + i * 5 + 1) & 255 for i in range(clen))
        for base in bases:
            digs = fastbrute._native.group16(challenge, base)
            for j in range(16):
                want = reference(challenge, fastbrute.candidate_at(base + j))
                assert digs[j] == want, (clen, base, j)
    print("ok  test_native_group16_matches_reference")


def test_native_isa_tiers():
    if not fastbrute.HAVE_NATIVE or not hasattr(fastbrute._native, "backend"):
        print("skip test_native_isa_tiers (native not built)")
        return
    tiers = ["scalar", "avx2"]
    if fastbrute._native.has_avx512():
        tiers.append("avx512")
    for isa in tiers:
        os.environ["FASTBRUTE_ISA"] = isa
        for clen in (0, 19, 23, 24, 51, 60, 63, 64, 90):
            challenge = bytes((clen * 7 + i) & 255 for i in range(clen))
            secret = fastbrute.candidate_at(200003)
            expected = reference(challenge, secret)
            result = fastbrute.find_secret(challenge, expected, backend="native", verbose=False)
            assert result == (secret, 200004), (isa, clen, result)
    os.environ.pop("FASTBRUTE_ISA", None)
    print("ok  test_native_isa_tiers")


def main():
    test_index_roundtrip()
    test_ordering_matches_product()
    test_scan_every_length()
    test_not_found()
    test_full_search()
    test_native_matches_reference()
    test_native_full_search()
    test_native_group8_matches_reference()
    test_native_group16_matches_reference()
    test_native_isa_tiers()
    print("ALL TESTS PASSED")


if __name__ == "__main__":
    main()
