#!/usr/bin/env python3
"""lxml oracle adapter for the foundation/xmp freeze gate.

Uses lxml (Python C extension over libxml2) to parse the
XMP packet as plain XML and extract simple-text, rdf:Alt,
and rdf:Bag/Seq properties per the same canonical form
the libxmp adapter produces.

INDEPENDENCE: lxml's underlying parser is libxml2
(maintained by the GNOME project, no shared lineage with
Adobe's XMP Toolkit). The libxmp adapter wraps Adobe's C
library; this adapter walks the XML directly. Two truly
independent code paths even though both are in Python.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from collections import defaultdict

from lxml import etree


# Map URI → canonical prefix used in our keys.
_PREFIX_MAP = {
    "http://purl.org/dc/elements/1.1/": "dc",
    "http://ns.adobe.com/pdf/1.3/": "pdf",
    "http://ns.adobe.com/xap/1.0/": "xmp",
    "http://ns.adobe.com/xap/1.0/mm/": "xmpMM",
    "http://ns.adobe.com/xap/1.0/rights/": "xmpRights",
    "http://ns.adobe.com/pdfx/1.3/": "pdfx",
}

_RDF_NS = "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
_XML_NS = "http://www.w3.org/XML/1998/namespace"


def _strip_xpacket(raw: bytes) -> bytes:
    """Strip xpacket processing instructions if present so
    lxml can parse what's inside as standalone XML."""
    text = raw.decode("utf-8")
    # Remove leading xpacket begin PI if present.
    text = re.sub(r"^\s*<\?xpacket\s+begin=[^?]*\?>\s*", "", text)
    # Remove trailing xpacket end PI if present.
    text = re.sub(r"\s*<\?xpacket\s+end=[^?]*\?>\s*$", "", text)
    return text.encode("utf-8")


def _qname_to_key(qname: str) -> str | None:
    """Convert a Clark-notation qname '{uri}local' to our
    canonical 'prefix:local' form. Returns None for unknown
    namespaces (treated as out-of-scope properties)."""
    if not qname.startswith("{"):
        return None
    end = qname.find("}")
    uri = qname[1:end]
    local = qname[end + 1:]
    prefix = _PREFIX_MAP.get(uri)
    if prefix is None:
        return None
    return f"{prefix}:{local}"


def _extract_array_value(prop_elem) -> tuple[str, str]:
    """Walk a property's children. Returns (kind, value) where
    kind is one of 'simple', 'alt', 'ordered', 'unordered'.
    """
    # Check for a single rdf:Alt / rdf:Bag / rdf:Seq child.
    for child in prop_elem:
        tag = child.tag
        if tag == f"{{{_RDF_NS}}}Alt":
            return ("alt", _flatten_alt(child))
        if tag == f"{{{_RDF_NS}}}Seq":
            return ("ordered", _flatten_seq(child))
        if tag == f"{{{_RDF_NS}}}Bag":
            return ("unordered", _flatten_bag(child))

    # Otherwise it's a simple text property — but it may have
    # whitespace + child text mixed. Take the joined text.
    text = (prop_elem.text or "").strip()
    return ("simple", text)


def _flatten_alt(alt_elem) -> str:
    """rdf:Alt — pick x-default li, fallback first li."""
    items: list[tuple[str, str]] = []  # (lang, value)
    for li in alt_elem:
        if li.tag != f"{{{_RDF_NS}}}li":
            continue
        lang = li.get(f"{{{_XML_NS}}}lang", "")
        items.append((lang, (li.text or "").strip()))
    for lang, val in items:
        if lang == "x-default":
            return val
    return items[0][1] if items else ""


def _flatten_seq(seq_elem) -> str:
    """rdf:Seq — TAB-join values in document order."""
    return "\t".join(
        (li.text or "").strip()
        for li in seq_elem
        if li.tag == f"{{{_RDF_NS}}}li")


def _flatten_bag(bag_elem) -> str:
    """rdf:Bag — TAB-join values in lexicographical order."""
    return "\t".join(sorted(
        (li.text or "").strip()
        for li in bag_elem
        if li.tag == f"{{{_RDF_NS}}}li"))


def adapt(xmp_path: Path) -> str:
    raw = _strip_xpacket(xmp_path.read_bytes())
    root = etree.fromstring(raw)

    # Find <rdf:RDF> — may be the root or a child of
    # <x:xmpmeta>.
    rdf = None
    if root.tag == f"{{{_RDF_NS}}}RDF":
        rdf = root
    else:
        for elem in root.iter(f"{{{_RDF_NS}}}RDF"):
            rdf = elem
            break
    if rdf is None:
        return ""

    # Walk every <rdf:Description> child.
    flat: dict[str, str] = {}
    for desc in rdf.iter(f"{{{_RDF_NS}}}Description"):
        # Attribute-form properties: any attribute on
        # rdf:Description whose qname is a known prefix.
        for attr_name, attr_val in desc.attrib.items():
            key = _qname_to_key(attr_name)
            if key is None:
                continue
            flat[key] = attr_val

        # Element-form properties: each direct child whose
        # qname is a known prefix.
        for prop in desc:
            key = _qname_to_key(prop.tag)
            if key is None:
                continue
            kind, value = _extract_array_value(prop)
            flat[key] = value

    if not flat:
        return ""
    lines = [f"{k}\t{v}" for k, v in sorted(flat.items())]
    return "\n".join(lines) + "\n"


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: oracle_lxml.py <fixture.xmp>", file=sys.stderr)
        return 2
    sys.stdout.write(adapt(Path(sys.argv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
