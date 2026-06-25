#!/usr/bin/env python3
"""Generate the fixture corpus for the foundation/crypt_filter
primitive.

crypt_filter's freeze gate is binding_roundtrip with a wire-
format-wrapped fixture protocol (see crypt_filter.yaml). Each
fixture file is a flat byte buffer carrying the (method,
fileKey, objNum, genNum, iv, plaintext) tuple — the gate's
Encode wrapper parses the header, runs Encrypt on the plaintext
payload, and re-emits header || cipher; Decode reverses. The
gate asserts byte-equality of Decode(Encode(input)) == input.

Wire format (per crypt_filter.yaml):

    byte 0       : method_id (1 = V2, 2 = AESV2, 3 = AESV3)
    byte 1       : key_len  (5..16 / 16 / 32)
    bytes 2..2+kl: fileKey
    next 4 bytes : objNum  (uint32 LE)
    next 4 bytes : genNum  (uint32 LE)
    next 16 bytes: iv      (passed to Encrypt for AES; ignored
                            for V2 — slot is always present so
                            the parser shape is fixed)
    remainder    : plaintext

The corpus axes are:

  * Method coverage          — V2, AESV2, AESV3 each get
                               multiple fixtures so the gate
                               regresses every dispatch path.
  * fileKey length variants  — V2 has 5, 10, 16-byte fileKeys
                               (the 5-byte and 10-byte cases
                               regress the min(n+5, 16) cap on
                               per-object key length).
  * obj/gen variants         — at least one V2 and one AESV2
                               fixture have gen != 0 (regresses
                               Algorithm 1's gen-byte handling).
                               At least one AESV3 fixture has
                               obj != 1 / gen != 0 to PROVE the
                               implementation ignores obj/gen
                               for V=5 (PDF 32000-2 §7.6.4.4).
  * Plaintext shapes         — empty, one byte, block-aligned,
                               mid-size, large mixed-entropy.
                               AES variants need empty +
                               block-aligned to regress PKCS#7.

Fixtures are public-domain — generated deterministically from
this script.

Run from this directory:
    python3 generate_fixtures.py
"""

from __future__ import annotations

import struct
from pathlib import Path

# Method IDs — must match the wire format described in
# crypt_filter.yaml. Any change here is also a TS body change
# and a spec yaml change.
METHOD_V2 = 1
METHOD_AESV2 = 2
METHOD_AESV3 = 3

# Deterministic IV for AES fixtures. The actual IV bytes don't
# matter for round-trip correctness — Encrypt prepends iv to
# its output and Decrypt reads the same bytes back. Mid-entropy
# pattern is visually distinct from zero-fill so a misalignment
# bug in the wire-format parser shows up clearly.
_IV_MID_ENTROPY = bytes((i * 13 + 7) % 256 for i in range(16))

# Zero IV for V2 fixtures — V2 ignores the iv slot entirely,
# but the slot must be present in the wire format.
_IV_ZERO = bytes(16)


def _encode(
    method_id: int,
    file_key: bytes,
    obj_num: int,
    gen_num: int,
    iv: bytes,
    plaintext: bytes,
) -> bytes:
    if len(iv) != 16:
        raise ValueError(f"iv must be 16 bytes, got {len(iv)}")
    if not (0 < len(file_key) <= 32):
        raise ValueError(f"file_key length out of range: {len(file_key)}")
    return (
        bytes([method_id, len(file_key)])
        + file_key
        + struct.pack("<II", obj_num, gen_num)
        + iv
        + plaintext
    )


# --- V2 (RC4) fixtures ----------------------------------------

def _v2_n5_short() -> bytes:
    """40-bit RC4: smallest /Length-supported V=2 case. n=5 →
    perObjKey is min(5+5, 16) = 10 bytes, NOT 16. A buggy
    implementation that always takes 16 bytes of MD5 produces
    the wrong RC4 key here."""
    return _encode(
        METHOD_V2, bytes(range(5)), obj_num=1, gen_num=0,
        iv=_IV_ZERO, plaintext=b"40-bit-RC4-fixture",
    )


def _v2_n10_mid() -> bytes:
    """80-bit RC4 keyed file: perObjKey = min(10+5, 16) = 15
    bytes. Mid-entropy 256-byte plaintext exercises a real
    keystream length."""
    return _encode(
        METHOD_V2, bytes(range(10)), obj_num=42, gen_num=0,
        iv=_IV_ZERO,
        plaintext=bytes((i * 53 + 7) % 256 for i in range(256)),
    )


def _v2_n16_long() -> bytes:
    """128-bit RC4: perObjKey = 16 (cap). Larger objNum
    exercises 24-bit-LE encoding past the low byte."""
    return _encode(
        METHOD_V2, bytes(range(16)), obj_num=12345, gen_num=0,
        iv=_IV_ZERO,
        plaintext=bytes((i * 79 + 31) % 256 for i in range(2048)),
    )


def _v2_n16_gen_nonzero() -> bytes:
    """Non-zero generation regresses the 16-bit-LE gen byte
    handling in Algorithm 1. A first-draft that uses gen=0 in
    the MD5 input regardless of input would silently pass the
    other V2 fixtures and fail this one."""
    return _encode(
        METHOD_V2, bytes(range(16)), obj_num=7, gen_num=5,
        iv=_IV_ZERO, plaintext=b"non-zero-generation-V2",
    )


def _v2_empty() -> bytes:
    """Empty plaintext under V2. RC4 is a stream cipher so
    Encrypt(empty) returns empty (no padding). A buggy
    implementation that treats the V2 path like AES (prepending
    16 IV bytes) breaks round-trip here — Decrypt would return
    a non-empty result."""
    return _encode(
        METHOD_V2, bytes(range(16)), obj_num=99, gen_num=0,
        iv=_IV_ZERO, plaintext=b"",
    )


# --- AESV2 (AES-128-CBC, /V=4) fixtures -----------------------

def _aesv2_empty() -> bytes:
    """Empty plaintext under AESV2. aes_cbc emits exactly one
    16-byte PKCS#7 padding block; crypt_filter prepends iv;
    output length is 32 bytes. Regresses both the padding rule
    AND the iv-prepend behaviour."""
    return _encode(
        METHOD_AESV2, bytes(range(16)), obj_num=1, gen_num=0,
        iv=_IV_MID_ENTROPY, plaintext=b"",
    )


def _aesv2_one_byte() -> bytes:
    """Smallest non-empty input under AESV2. PKCS#7 pads with
    15 copies of 0x0F; output is iv (16) || cipher (16) = 32 bytes."""
    return _encode(
        METHOD_AESV2, bytes(range(16)), obj_num=2, gen_num=0,
        iv=_IV_MID_ENTROPY, plaintext=b"\x42",
    )


def _aesv2_block() -> bytes:
    """16-byte block-aligned plaintext under AESV2. PKCS#7
    rule: a full pad block is added unconditionally, so output
    is iv (16) || cipher (32) = 48 bytes. Regresses encoders
    that 'optimise' away the pad block when input is
    block-aligned."""
    return _encode(
        METHOD_AESV2, bytes(range(16)), obj_num=3, gen_num=0,
        iv=_IV_MID_ENTROPY, plaintext=b"sixteen bytes!!!",
    )


def _aesv2_mid_512() -> bytes:
    """Mid-entropy 512-byte plaintext (32 blocks). Exercises
    aes_cbc's chained block emission across a representative
    PDF content-stream size."""
    return _encode(
        METHOD_AESV2, bytes(range(16)), obj_num=99, gen_num=0,
        iv=_IV_MID_ENTROPY,
        plaintext=bytes((i * 79 + 31) % 256 for i in range(512)),
    )


def _aesv2_gen_nonzero() -> bytes:
    """AESV2 with gen != 0. Regresses both Algorithm 1's gen
    bytes AND the sAlT salt for AESV2."""
    return _encode(
        METHOD_AESV2, bytes(range(16)), obj_num=7, gen_num=3,
        iv=_IV_MID_ENTROPY, plaintext=b"non-zero-generation-AESV2",
    )


# --- AESV3 (AES-256-CBC, /V=5) fixtures -----------------------

def _aesv3_empty() -> bytes:
    """Empty plaintext under AESV3. fileKey is 32 bytes; per-
    object key derivation is SKIPPED for V=5 — the file key IS
    used directly as the AES-256 key. Output: iv (16) || pad
    block (16) = 32 bytes."""
    return _encode(
        METHOD_AESV3, bytes(range(32)), obj_num=1, gen_num=0,
        iv=_IV_MID_ENTROPY, plaintext=b"",
    )


def _aesv3_short() -> bytes:
    """Short ASCII plaintext under AESV3."""
    return _encode(
        METHOD_AESV3, bytes(range(32)), obj_num=2, gen_num=0,
        iv=_IV_MID_ENTROPY, plaintext=b"AES-256 short fixture",
    )


def _aesv3_mid_512() -> bytes:
    """Mid-entropy 512-byte plaintext under AESV3."""
    return _encode(
        METHOD_AESV3, bytes(range(32)), obj_num=42, gen_num=0,
        iv=_IV_MID_ENTROPY,
        plaintext=bytes((i * 53 + 7) % 256 for i in range(512)),
    )


def _aesv3_gen_nonzero() -> bytes:
    """AESV3 with non-trivial obj/gen. PDF 32000-2 §7.6.4.4
    says V=5 SKIPS Algorithm 1; the fileKey is used directly.
    A buggy implementation that copies the AESV2 dispatch
    (running Algorithm 1 + sAlT to derive a 16-byte AES-128
    key) would compute the wrong key here AND try AES-128
    instead of AES-256, which fails on key-length validation
    inside foundation::aes_cbc. This fixture's job is to FAIL
    that bug — every other AESV3 fixture has obj=1 or obj=2
    which by accident matches some buggy derivations."""
    return _encode(
        METHOD_AESV3, bytes(range(32)), obj_num=99, gen_num=7,
        iv=_IV_MID_ENTROPY,
        plaintext=b"AES-256 ignores obj/gen for V=5",
    )


_FIXTURES = {
    "v2_n5_short.bin":         _v2_n5_short,
    "v2_n10_mid.bin":          _v2_n10_mid,
    "v2_n16_long.bin":         _v2_n16_long,
    "v2_n16_gen_nonzero.bin":  _v2_n16_gen_nonzero,
    "v2_empty.bin":            _v2_empty,
    "aesv2_empty.bin":         _aesv2_empty,
    "aesv2_one_byte.bin":      _aesv2_one_byte,
    "aesv2_block.bin":         _aesv2_block,
    "aesv2_mid_512.bin":       _aesv2_mid_512,
    "aesv2_gen_nonzero.bin":   _aesv2_gen_nonzero,
    "aesv3_empty.bin":         _aesv3_empty,
    "aesv3_short.bin":         _aesv3_short,
    "aesv3_mid_512.bin":       _aesv3_mid_512,
    "aesv3_gen_nonzero.bin":   _aesv3_gen_nonzero,
}


def main() -> int:
    here = Path(__file__).parent
    for name, gen in _FIXTURES.items():
        data = gen()
        out = here / name
        out.write_bytes(data)
        print(f"wrote {name:28s} {len(data):6d} B")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
