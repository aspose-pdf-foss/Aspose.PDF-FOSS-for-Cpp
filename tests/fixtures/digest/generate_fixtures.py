#!/usr/bin/env python3
"""Generate raw-byte fixtures + oracle-agreed hash sidecars for
the foundation/digest primitive.

digest's freeze gate is two_oracle_agreement. Oracles:

  1. Python's hashlib (MD5/SHA-1/SHA-256/SHA-384/SHA-512).
     Uses CPython's built-in implementation which is a
     distinct codebase from OpenSSL's libcrypto.
  2. `openssl dgst -<alg>` CLI.

For each fixture, this script runs both oracles, asserts
byte-identical hex output for every digest, and writes a
.canonical sidecar in the format the C++ adapter's
ToCanonical() must match:

    md5: <32 lowercase hex>\\n
    sha1: <40 lowercase hex>\\n
    sha256: <64 lowercase hex>\\n
    sha384: <96 lowercase hex>\\n
    sha512: <128 lowercase hex>\\n

Any fixture on which the oracles disagree (should never
happen for these well-established digests) is surfaced as a
RuntimeError so the operator can investigate before the
sidecar is written.

Fixture coverage: empty, one-byte, MD5/SHA-1 block-boundary
edges (55, 56, 64 bytes — the 64-byte block size and the
55-vs-56 transition where padding overflows into a second
block), one SHA-384/SHA-512 block-boundary edge (111, 112,
128 bytes — these digests use 128-byte blocks), and one
mid-size mixed fixture (4 KiB).

Fixtures are public-domain, deterministic.

Run from this directory:
    python3 generate_fixtures.py
"""

from __future__ import annotations

import hashlib
import shutil
import subprocess
from pathlib import Path


_ALGORITHMS = ("md5", "sha1", "sha256", "sha384", "sha512")


def _python_digest(data: bytes, alg: str) -> str:
    """Hash `data` via Python's hashlib and return lowercase
    hex digest. hashlib uses CPython's internal C
    implementation for the common digests, distinct from the
    openssl CLI's backend in Homebrew builds."""
    h = hashlib.new(alg)
    h.update(data)
    return h.hexdigest()


def _openssl_digest(data: bytes, alg: str) -> str:
    """Hash `data` via `openssl dgst -<alg>` CLI and return
    the lowercase hex digest. `openssl dgst` writes
    "<(stdin)= <hex>" or "(stdin)= <hex>" depending on
    version — we strip everything up to and including the
    last '= '."""
    r = subprocess.run(
        ["openssl", "dgst", f"-{alg}"],
        input=data, capture_output=True, check=True)
    line = r.stdout.decode("ascii").strip()
    # Line form: "MD5(stdin)= abcdef..." or similar
    marker = "= "
    idx = line.rfind(marker)
    if idx < 0:
        raise RuntimeError(
            f"openssl dgst output format not recognised: {line!r}")
    return line[idx + len(marker):].strip()


def _produce_canonical(data: bytes) -> str:
    """Compute canonical text for one fixture. Asserts the
    two oracles agree on every digest; raises if they don't."""
    lines = []
    for alg in _ALGORITHMS:
        hex_py = _python_digest(data, alg)
        hex_ossl = _openssl_digest(data, alg)
        if hex_py != hex_ossl:
            raise RuntimeError(
                f"oracle disagreement on {alg}: "
                f"python={hex_py!r} openssl={hex_ossl!r}")
        lines.append(f"{alg}: {hex_py}")
    return "\n".join(lines) + "\n"


# -------------------------------------------------------------
# Fixture generators
# -------------------------------------------------------------


def _empty() -> bytes:
    """Zero-byte input. Every digest has a well-known
    empty-input value; an adapter that short-circuits on
    empty produces the wrong hash."""
    return b""


def _one_byte() -> bytes:
    """One byte — smallest non-empty case."""
    return b"A"


def _md5_block_55() -> bytes:
    """55 bytes — one byte short of the MD5/SHA-1 block
    size (64 bytes). Padding (0x80 + zeros + 8-byte length)
    fits in the same 64-byte block. Classic edge for
    stream-cipher implementations that allocate padding
    scratch space wrong."""
    return b"A" * 55


def _md5_block_56() -> bytes:
    """56 bytes — the transition point where padding cannot
    fit in the current 64-byte block and MD5/SHA-1 must
    allocate a SECOND padding block. An adapter that
    mis-counts block usage fails on this boundary."""
    return b"A" * 56


def _md5_block_64() -> bytes:
    """Exactly 64 bytes — one full MD5/SHA-1 block. Padding
    MUST go into a SECOND block (0x80 + zeros + length).
    Common pattern-matching bug: encoders that special-case
    "block-aligned input" skip the second block."""
    return b"A" * 64


def _sha512_block_111() -> bytes:
    """111 bytes — one byte short of the SHA-384/SHA-512
    block size (128 bytes). Padding fits in the same block.
    Mirror of the 55-byte case for the 128-byte-block
    algorithms."""
    return b"A" * 111


def _sha512_block_112() -> bytes:
    """112 bytes — padding overflows into a second 128-byte
    block for SHA-384/SHA-512 (the length field is 16 bytes
    for those, not 8, so the threshold is 112, not 120)."""
    return b"A" * 112


def _mixed_4k() -> bytes:
    """~4 KiB mixed content. Representative of real PDF
    password-plus-metadata inputs flowing through the
    /Encrypt key-derivation algorithms."""
    prose = (
        b"Digest test corpus. "
        b"The quick brown fox jumps over the lazy dog. "
    ) * 24
    binary = bytes((i * 53 + 11) % 256 for i in range(2000))
    return prose + binary


_FIXTURES: dict[str, callable] = {
    "empty.bin":           _empty,
    "one_byte.bin":        _one_byte,
    "md5_block_55.bin":    _md5_block_55,
    "md5_block_56.bin":    _md5_block_56,
    "md5_block_64.bin":    _md5_block_64,
    "sha512_block_111.bin": _sha512_block_111,
    "sha512_block_112.bin": _sha512_block_112,
    "mixed_4k.bin":        _mixed_4k,
}


def main() -> int:
    here = Path(__file__).parent
    if shutil.which("openssl") is None:
        print("ERROR: openssl not on PATH")
        return 1
    print("oracle 1: python hashlib built-in (CPython)")
    print(f"oracle 2: {shutil.which('openssl')}")

    for name, gen in _FIXTURES.items():
        data = gen()
        try:
            canonical = _produce_canonical(data)
        except RuntimeError as e:
            print(f"ERROR on {name}: {e}")
            return 1
        bin_path = here / name
        canon_path = bin_path.with_suffix(".canonical")
        bin_path.write_bytes(data)
        canon_path.write_text(canonical)
        print(f"wrote {name:24s} {len(data):5d} B, "
              f"canonical {len(canonical):3d} B")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
