from __future__ import annotations

from time import perf_counter

from ._purepy import (
    ALPHABET,
    KEYSPACE,
    SECRET_LENGTH,
    Match,
    candidate_at,
    find_secret as _search_python,
    index_of,
    report,
)

try:
    from . import _native
    HAVE_NATIVE = hasattr(_native, "find")
except ImportError:
    _native = None
    HAVE_NATIVE = False

__all__ = [
    "find_secret",
    "candidate_at",
    "index_of",
    "backend_name",
    "ALPHABET",
    "SECRET_LENGTH",
    "KEYSPACE",
    "HAVE_NATIVE",
]


def backend_name() -> str:
    if HAVE_NATIVE:
        try:
            return f"native-{_native.backend()}"
        except Exception:
            return "native"
    return "python-multiprocessing"


def find_secret(
    challenge: bytes,
    expected: bytes,
    *,
    backend: str = "auto",
    processes: int | None = None,
    verbose: bool = True,
) -> Match | None:
    if backend not in ("auto", "native", "python"):
        raise ValueError("backend must be 'auto', 'native', or 'python'")
    if len(expected) != 16:
        raise ValueError("expected must be exactly 16 bytes")
    challenge = bytes(challenge)
    expected = bytes(expected)

    if HAVE_NATIVE and backend in ("auto", "native"):
        try:
            started = perf_counter()
            result = _native.find(challenge, expected, processes or 0)
            if verbose:
                elapsed = perf_counter() - started
                if result is None:
                    report(KEYSPACE, elapsed, None)
                else:
                    print(f"[found] secret at candidate #{result[1]:,} in {elapsed:.2f}s")
            return result
        except Exception as exc:
            if backend == "native":
                raise
            if verbose:
                print(f"[warn] native backend failed ({exc!r}); using pure-Python fallback")

    if backend == "native":
        raise RuntimeError("native backend is not built; run 'pip install .' (see README)")
    return _search_python(challenge, expected, processes=processes, verbose=verbose)
