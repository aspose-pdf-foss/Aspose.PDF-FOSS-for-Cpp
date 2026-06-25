#!/usr/bin/env python3
"""python-xmp-toolkit (libxmp) oracle adapter for the
foundation/xmp freeze gate.

python-xmp-toolkit wraps Adobe's XMP Toolkit C library
(licence MIT/BSD-equivalent — used at fixture-authoring
time only, not linked into our foundation library).

Canonical form: one line per property, sorted by key,
``<prefix:name>\\t<value>\\n``. rdf:Alt values flatten
to the x-default li (or first li if no x-default).
rdf:Bag/Seq values join with TAB.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from collections import defaultdict

from libxmp import XMPMeta, XMPIterator


# Map URI → canonical prefix used in our keys. Mirrors PDF's
# common XMP namespaces.
_PREFIX_MAP = {
    "http://purl.org/dc/elements/1.1/": "dc",
    "http://ns.adobe.com/pdf/1.3/": "pdf",
    "http://ns.adobe.com/xap/1.0/": "xmp",
    "http://ns.adobe.com/xap/1.0/mm/": "xmpMM",
    "http://ns.adobe.com/xap/1.0/rights/": "xmpRights",
    "http://ns.adobe.com/pdfx/1.3/": "pdfx",
}

# array_path_re: matches "dc:title[1]" → ("dc:title", 1).
_ARRAY_PATH = re.compile(r"^(.+?)\[(\d+)\]$")


def adapt(xmp_path: Path) -> str:
    raw = xmp_path.read_bytes()
    xmp = XMPMeta()
    xmp.parse_from_str(raw.decode("utf-8"))

    # Walk every entry from the iterator. Group array members
    # by parent path so rdf:Alt/Bag/Seq can flatten in a second
    # pass. Track xml:lang qualifiers per array index for Alt
    # x-default selection.
    simple: dict[str, str] = {}
    arrays: dict[str, list[tuple[int, str, str]]] = defaultdict(list)
    # arrays[parent_key] = [(index, lang_or_empty, value), ...]
    array_kinds: dict[str, str] = {}
    # array_kinds[parent_key] = "alt" | "ordered" | "unordered"

    # Buffer entries first so we can resolve qualifiers.
    entries = list(XMPIterator(xmp))

    # First pass: identify array parents + kinds.
    for ns, path, val, opts in entries:
        if not path or opts["IS_SCHEMA"] or opts["IS_QUALIFIER"]:
            continue
        if opts["VALUE_IS_ARRAY"]:
            key = _to_key(ns, path)
            if opts["ARRAY_IS_ALT"]:
                array_kinds[key] = "alt"
            elif opts["ARRAY_IS_ORDERED"]:
                array_kinds[key] = "ordered"
            else:
                array_kinds[key] = "unordered"

    # Second pass: collect simple values + array members. We
    # need a small state machine to track "the last array
    # member I saw" so a following xml:lang qualifier can
    # attach to it.
    last_member: tuple[str, int] | None = None
    pending_member_value: str = ""
    member_lang: dict[tuple[str, int], str] = {}

    for ns, path, val, opts in entries:
        if not path or opts["IS_SCHEMA"]:
            continue
        if opts["VALUE_IS_ARRAY"]:
            continue  # handled by array_kinds above
        m = _ARRAY_PATH.match(path)
        if opts["IS_QUALIFIER"]:
            # e.g. "dc:title[1]/?xml:lang" with val="x-default"
            if path.endswith("/?xml:lang") and last_member:
                member_lang[last_member] = val
            continue
        if m:
            # Array member: "dc:title[1]"
            parent_path = m.group(1)
            idx = int(m.group(2))
            parent_key = _to_key(ns, parent_path)
            arrays[parent_key].append((idx, "", val))
            last_member = (parent_key, idx)
        else:
            # Simple property
            key = _to_key(ns, path)
            simple[key] = val
            last_member = None

    # Apply lang qualifiers to array members.
    for parent_key, members in list(arrays.items()):
        members_with_lang = []
        for idx, _empty, val in sorted(members):
            lang = member_lang.get((parent_key, idx), "")
            members_with_lang.append((idx, lang, val))
        arrays[parent_key] = members_with_lang

    # Flatten arrays per anchor rules.
    flat: dict[str, str] = dict(simple)
    for key, members in arrays.items():
        kind = array_kinds.get(key, "ordered")
        if kind == "alt":
            # Pick x-default if present, else first.
            chosen = None
            for idx, lang, val in members:
                if lang == "x-default":
                    chosen = val
                    break
            if chosen is None and members:
                chosen = sorted(members)[0][2]
            flat[key] = chosen or ""
        elif kind == "ordered":
            ordered = sorted(members, key=lambda m: m[0])
            flat[key] = "\t".join(v for _, _, v in ordered)
        else:  # unordered (Bag) — sort lexicographically
            sorted_vals = sorted(v for _, _, v in members)
            flat[key] = "\t".join(sorted_vals)

    if not flat:
        return ""
    lines = [f"{k}\t{v}" for k, v in sorted(flat.items())]
    return "\n".join(lines) + "\n"


def _to_key(ns_uri: str, path: str) -> str:
    """Convert (namespace_uri, path_with_prefix) to canonical
    'prefix:name' key. The path comes in with the xmpns prefix
    already attached (e.g. 'dc:title'); we use _PREFIX_MAP to
    rewrite to our canonical prefix if the URI maps."""
    canonical = _PREFIX_MAP.get(ns_uri)
    if not canonical:
        return path  # unknown ns: trust libxmp's prefix
    if ":" in path:
        _, name = path.split(":", 1)
        return f"{canonical}:{name}"
    return f"{canonical}:{path}"


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: oracle_libxmp.py <fixture.xmp>", file=sys.stderr)
        return 2
    sys.stdout.write(adapt(Path(sys.argv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
