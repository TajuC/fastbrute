from __future__ import annotations

import argparse
import binascii

MODE = 4010


def _is_simple_salt(challenge: bytes) -> bool:
    return all(0x20 <= b < 0x7f and b != ord(":") for b in challenge)


def hash_line(expected: bytes, challenge: bytes, *, hex_salt: bool = False) -> str:
    if len(expected) != 16:
        raise ValueError("expected must be exactly 16 bytes")
    digest = binascii.hexlify(expected).decode("ascii")
    salt = binascii.hexlify(challenge).decode("ascii") if hex_salt else challenge.decode("ascii")
    return f"{digest}:{salt}"


def command(*, hashfile: str = "hash.txt", hex_salt: bool = False) -> str:
    parts = ["hashcat", "-m", str(MODE), "-a", "3", "-1", "?l?u?d", hashfile, "?1?1?1?1?1"]
    if hex_salt:
        parts.insert(1, "--hex-salt")
    return " ".join(parts)


def recipe(expected: bytes, challenge: bytes) -> tuple[str, str]:
    hex_salt = not _is_simple_salt(challenge)
    return hash_line(expected, challenge, hex_salt=hex_salt), command(hex_salt=hex_salt)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="fastbrute.hashcat",
        description="Emit a hashcat -m 4010 recipe for this challenge.",
    )
    parser.add_argument("expected", help="target digest as 32 hex characters")
    parser.add_argument("challenge", help="challenge string")
    args = parser.parse_args(argv)
    line, cmd = recipe(binascii.unhexlify(args.expected), args.challenge.encode())
    print("hash.txt:")
    print(f"  {line}")
    print("run:")
    print(f"  {cmd}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
