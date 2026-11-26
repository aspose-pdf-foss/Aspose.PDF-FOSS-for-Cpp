#include "der.hpp"

#include <stdexcept>

namespace foundation::der {

namespace {

[[noreturn]] void Fail(const char* what) {
    throw std::runtime_error(std::string("der: ") + what);
}

std::vector<std::byte> Copy(std::span<const std::byte> s) {
    return {s.begin(), s.end()};
}

}  // namespace

Node Read(std::span<const std::byte> in, std::size_t& pos) {
    const std::size_t start = pos;
    if (pos + 2 > in.size()) Fail("truncated");
    Node n;
    n.tag = std::to_integer<std::uint8_t>(in[pos++]);
    if ((n.tag & 0x1F) == 0x1F) Fail("multi-byte tags unsupported");
    std::size_t len = std::to_integer<std::size_t>(in[pos++]);
    if (len & 0x80) {
        const std::size_t count = len & 0x7F;
        if (count == 0) Fail("indefinite length unsupported");
        if (count > sizeof(std::size_t) || pos + count > in.size())
            Fail("bad length");
        len = 0;
        for (std::size_t i = 0; i < count; ++i)
            len = (len << 8) | std::to_integer<std::size_t>(in[pos++]);
    }
    if (len > in.size() - pos) Fail("truncated");
    n.content = in.subspan(pos, len);
    pos += len;
    n.whole = in.subspan(start, pos - start);
    return n;
}

Node ReadOne(std::span<const std::byte> in) {
    std::size_t pos = 0;
    Node n = Read(in, pos);
    if (pos != in.size()) Fail("trailing bytes");
    return n;
}

std::vector<Node> Children(const Node& n) {
    std::vector<Node> out;
    std::size_t pos = 0;
    while (pos < n.content.size()) out.push_back(Read(n.content, pos));
    return out;
}

std::string Oid(const Node& n) {
    if (n.tag != 0x06 || n.content.empty()) Fail("not an OID");
    std::string s;
    std::uint64_t v = 0;
    bool first = true;
    for (std::byte b : n.content) {
        v = (v << 7) | (std::to_integer<std::uint64_t>(b) & 0x7F);
        if ((std::to_integer<unsigned>(b) & 0x80) != 0) continue;
        if (first) {
            const std::uint64_t a = v < 80 ? v / 40 : 2;
            s = std::to_string(a) + "." + std::to_string(v - a * 40);
            first = false;
        } else {
            s += "." + std::to_string(v);
        }
        v = 0;
    }
    return s;
}

std::vector<std::byte> UnsignedInteger(const Node& n) {
    if (n.tag != 0x02) Fail("not an INTEGER");
    std::size_t i = 0;
    while (i + 1 < n.content.size() && n.content[i] == std::byte{0}) ++i;
    return Copy(n.content.subspan(i));
}

std::uint64_t SmallInteger(const Node& n) {
    const auto mag = UnsignedInteger(n);
    if (mag.size() > 8) Fail("INTEGER too large");
    std::uint64_t v = 0;
    for (std::byte b : mag) v = (v << 8) | std::to_integer<std::uint64_t>(b);
    return v;
}

std::string CommonName(std::span<const std::byte> name_der) {
    const Node name = ReadOne(name_der);
    for (const Node& rdn : Children(name))           // SET
        for (const Node& atv : Children(rdn)) {       // SEQUENCE {oid, value}
            const auto parts = Children(atv);
            if (parts.size() == 2 && parts[0].tag == 0x06 &&
                Oid(parts[0]) == "2.5.4.3")
                return {reinterpret_cast<const char*>(parts[1].content.data()),
                        parts[1].content.size()};
        }
    return {};
}

Certificate ParseCertificate(std::span<const std::byte> der) {
    Certificate c;
    c.der = Copy(der);
    const Node cert = ReadOne(der);
    const auto top = Children(cert);
    if (top.size() < 3 || top[0].tag != 0x30) Fail("not a certificate");
    auto tbs = Children(top[0]);
    std::size_t i = 0;
    if (i < tbs.size() && tbs[i].tag == 0xA0) ++i;  // [0] version
    if (i + 6 > tbs.size()) Fail("short tbsCertificate");
    c.serial = Copy(tbs[i].whole);
    c.issuer = Copy(tbs[i + 2].whole);
    c.subject = Copy(tbs[i + 4].whole);
    c.subject_cn = CommonName(tbs[i + 4].whole);
    const auto spki = Children(tbs[i + 5]);
    if (spki.size() == 2 && spki[1].tag == 0x03 && !spki[1].content.empty()) {
        const auto alg = Children(spki[0]);
        if (!alg.empty() && Oid(alg[0]) == "1.2.840.113549.1.1.1") {
            const Node key = ReadOne(spki[1].content.subspan(1));
            const auto parts = Children(key);
            if (parts.size() >= 2) {
                c.modulus = UnsignedInteger(parts[0]);
                c.exponent = UnsignedInteger(parts[1]);
            }
        } else if (alg.size() >= 2 && Oid(alg[0]) == "1.2.840.10045.2.1" &&
                   alg[1].tag == 0x06) {  // id-ecPublicKey, namedCurve
            c.ec_curve_oid = Oid(alg[1]);
            c.ec_point = Copy(spki[1].content.subspan(1));
        }
    }
    return c;
}

}  // namespace foundation::der
