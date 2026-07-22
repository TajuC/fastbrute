from __future__ import annotations

import argparse
import binascii

from . import backend_name, find_secret


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="fastbrute",
        description=(
            "Recover the 5-character secret in md5(challenge + hex(md5(challenge + secret)))."
        ),
    )
    parser.add_argument("expected", help="target digest as 32 hex characters")
    parser.add_argument("challenge", help="challenge string, or 0x-prefixed raw hex")
    parser.add_argument("-j", "--processes", type=int, default=None,
                        help="worker count for the pure-Python backend")
    args = parser.parse_args(argv)

    expected = binascii.unhexlify(args.expected)
    if args.challenge.startswith("0x"):
        challenge = binascii.unhexlify(args.challenge[2:])
    else:
        challenge = args.challenge.encode()

    print(f"backend: {backend_name()}")
    result = find_secret(challenge, expected, processes=args.processes)
    if result is None:
        print("no match in the keyspace")
        return 1
    secret, index = result
    print(f"secret: {secret.decode('ascii')}  (candidate #{index:,})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
