#include "ec.hpp"

#include <algorithm>
#include <stdexcept>

#include "der.hpp"

namespace foundation::ec {

namespace {

using bigint::BigUint;
using Bytes = std::vector<std::byte>;

BigUint Hex(const char* hex) {
    Bytes b;
    std::string s(hex);
    if (s.size() % 2) s = "0" + s;
    for (std::size_t i = 0; i < s.size(); i += 2)
        b.push_back(static_cast<std::byte>(std::stoi(s.substr(i, 2), nullptr, 16)));
    return BigUint::FromBytesBE(b);
}

Curve Make(const char* oid, const char* name, std::size_t bytes, const char* p,
           const char* b, const char* gx, const char* gy, const char* n, crypto::Hash h) {
    Curve c;
    c.oid = oid;
    c.name = name;
    c.bytes = bytes;
    c.p = Hex(p);
    c.b = Hex(b);
    c.gx = Hex(gx);
    c.gy = Hex(gy);
    c.n = Hex(n);
    c.hash = h;
    return c;
}

const std::vector<Curve>& Curves() {
    static const std::vector<Curve> curves = {
        Make("1.2.840.10045.3.1.7", "P-256", 32,
             "ffffffff00000001000000000000000000000000ffffffffffffffffffffffff",
             "5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b",
             "6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296",
             "4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5",
             "ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551",
             crypto::Hash::Sha256),
        Make("1.3.132.0.34", "P-384", 48,
             "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe"
             "ffffffff0000000000000000ffffffff",
             "b3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875a"
             "c656398d8a2ed19d2a85c8edd3ec2aef",
             "aa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a38"
             "5502f25dbf55296c3a545e3872760ab7",
             "3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c0"
             "0a60b1ce1d7e819d7a431d7c90ea0e5f",
             "ffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf"
             "581a0db248b0a77aecec196accc52973",
             crypto::Hash::Sha384),
        Make("1.3.132.0.35", "P-521", 66,
             "01ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
             "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff",
             "0051953eb9618e1c9a1f929a21a0b68540eea2da725b99b315f3b8b489918ef1"
             "09e156193951ec7e937b1652c0bd3bb1bf073573df883d2c34f1ef451fd46b503f00",
             "00c6858e06b70404e9cd9e3ecb662395b4429c648139053fb521f828af606b4d"
             "3dbaa14b5e77efe75928fe1dc127a2ffa8de3348b3c1856a429bf97e7e31c2e5bd66",
             "011839296a789a3bc0045c8a5fb42c7d1bd998f54449579b446817afbd17273e"
             "662c97ee72995ef42640c550b9013fad0761353c7086a272c24088be94769fd16650",
             "01ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
             "fa51868783bf2f966b7fcc0148f709a5d03bb5c9b8899c47aebb6fb71e91386409",
             crypto::Hash::Sha512),
    };
    return curves;
}

// ---- modular arithmetic -------------------------------------------------------

BigUint Mod(const BigUint& a, const BigUint& m) {
    BigUint q, r;
    DivMod(a, m, q, r);
    return r;
}
BigUint MulM(const BigUint& a, const BigUint& b, const BigUint& m) { return Mod(Mul(a, b), m); }
BigUint AddM(const BigUint& a, const BigUint& b, const BigUint& m) {
    BigUint s = Add(a, b);
    return Compare(s, m) >= 0 ? Sub(s, m) : s;
}
BigUint SubM(const BigUint& a, const BigUint& b, const BigUint& m) {
    return Compare(a, b) >= 0 ? Sub(a, b) : Sub(Add(a, m), b);
}
BigUint InvM(const BigUint& a, const BigUint& m) {  // m prime
    return a.ModPow(Sub(m, BigUint(2)), m);
}
bool Eq(const BigUint& a, const BigUint& b) { return Compare(a, b) == 0; }

// ---- points (Jacobian; z == 0 is infinity) ------------------------------------

struct Point {
    BigUint x, y, z;
};

Point Infinity() { return {BigUint(1), BigUint(1), BigUint()}; }

Point Double(const Curve& c, const Point& pt) {
    if (pt.z.IsZero() || pt.y.IsZero()) return Infinity();
    const BigUint& p = c.p;
    const BigUint delta = MulM(pt.z, pt.z, p);
    const BigUint gamma = MulM(pt.y, pt.y, p);
    const BigUint beta = MulM(pt.x, gamma, p);
    const BigUint t = MulM(SubM(pt.x, delta, p), AddM(pt.x, delta, p), p);
    const BigUint alpha = AddM(AddM(t, t, p), t, p);
    const BigUint beta4 = AddM(AddM(beta, beta, p), AddM(beta, beta, p), p);
    const BigUint beta8 = AddM(beta4, beta4, p);
    Point r;
    r.x = SubM(MulM(alpha, alpha, p), beta8, p);
    const BigUint yz = AddM(pt.y, pt.z, p);
    r.z = SubM(SubM(MulM(yz, yz, p), gamma, p), delta, p);
    const BigUint g2 = MulM(gamma, gamma, p);
    const BigUint g8 = [&] {
        BigUint v = AddM(g2, g2, p);
        v = AddM(v, v, p);
        return AddM(v, v, p);
    }();
    r.y = SubM(MulM(alpha, SubM(beta4, r.x, p), p), g8, p);
    return r;
}

Point AddPts(const Curve& c, const Point& a, const Point& b) {
    if (a.z.IsZero()) return b;
    if (b.z.IsZero()) return a;
    const BigUint& p = c.p;
    const BigUint z1z1 = MulM(a.z, a.z, p);
    const BigUint z2z2 = MulM(b.z, b.z, p);
    const BigUint u1 = MulM(a.x, z2z2, p);
    const BigUint u2 = MulM(b.x, z1z1, p);
    const BigUint s1 = MulM(a.y, MulM(b.z, z2z2, p), p);
    const BigUint s2 = MulM(b.y, MulM(a.z, z1z1, p), p);
    if (Eq(u1, u2)) return Eq(s1, s2) ? Double(c, a) : Infinity();
    const BigUint h = SubM(u2, u1, p);
    const BigUint r = SubM(s2, s1, p);
    const BigUint hh = MulM(h, h, p);
    const BigUint hhh = MulM(hh, h, p);
    const BigUint v = MulM(u1, hh, p);
    Point out;
    out.x = SubM(SubM(MulM(r, r, p), hhh, p), AddM(v, v, p), p);
    out.y = SubM(MulM(r, SubM(v, out.x, p), p), MulM(s1, hhh, p), p);
    out.z = MulM(MulM(a.z, b.z, p), h, p);
    return out;
}

Point ScalarMul(const Curve& c, const BigUint& k, const Point& pt) {
    Point r = Infinity();
    const std::size_t bits = k.BitLength();
    if (bits == 0) return r;
    const Bytes kb = k.ToBytesBE((bits + 7) / 8);
    for (std::size_t i = 0; i < kb.size() * 8; ++i) {
        r = Double(c, r);
        if ((std::to_integer<unsigned>(kb[i / 8]) >> (7 - i % 8)) & 1) r = AddPts(c, r, pt);
    }
    return r;
}

// Affine x and y (pt not infinity).
void ToAffine(const Curve& c, const Point& pt, BigUint& x, BigUint& y) {
    const BigUint zi = InvM(pt.z, c.p);
    const BigUint zi2 = MulM(zi, zi, c.p);
    x = MulM(pt.x, zi2, c.p);
    y = MulM(pt.y, MulM(zi2, zi, c.p), c.p);
}

Point Generator(const Curve& c) { return {c.gx, c.gy, BigUint(1)}; }

std::size_t OrderBits(const Curve& c) { return c.n.BitLength(); }

// bits2int (RFC 6979 §2.3.2): the leftmost qlen bits of `in`.
BigUint Bits2Int(const Curve& c, std::span<const std::byte> in) {
    const std::size_t qlen = OrderBits(c);
    const std::size_t qbytes = (qlen + 7) / 8;
    if (in.size() * 8 <= qlen) return BigUint::FromBytesBE(in);
    // Keep the leftmost qbytes bytes, then drop the extra low bits (< 8).
    BigUint v = BigUint::FromBytesBE(in.subspan(0, std::min(in.size(), qbytes)));
    const std::size_t extra = std::min(in.size(), qbytes) * 8 - qlen;
    if (extra > 0) {
        BigUint q, r;
        DivMod(v, BigUint(1u << extra), q, r);
        v = q;
    }
    return v;
}

Bytes Int2Octets(const Curve& c, const BigUint& v) {
    return v.ToBytesBE((OrderBits(c) + 7) / 8);
}

Bytes DerInteger(const Bytes& mag) {
    std::size_t i = 0;
    while (i + 1 < mag.size() && mag[i] == std::byte{0}) ++i;
    Bytes v(mag.begin() + static_cast<std::ptrdiff_t>(i), mag.end());
    if (std::to_integer<unsigned>(v[0]) & 0x80) v.insert(v.begin(), std::byte{0});
    Bytes out{std::byte{0x02}};
    out.push_back(static_cast<std::byte>(v.size()));  // < 128 for these curves
    out.insert(out.end(), v.begin(), v.end());
    return out;
}

bool ParsePoint(const Curve& c, std::span<const std::byte> q, BigUint& x, BigUint& y) {
    if (q.size() != 1 + 2 * c.bytes || q[0] != std::byte{0x04}) return false;
    x = BigUint::FromBytesBE(q.subspan(1, c.bytes));
    y = BigUint::FromBytesBE(q.subspan(1 + c.bytes, c.bytes));
    if (Compare(x, c.p) >= 0 || Compare(y, c.p) >= 0) return false;
    // y^2 == x^3 - 3x + b
    const BigUint lhs = MulM(y, y, c.p);
    const BigUint x3 = MulM(MulM(x, x, c.p), x, c.p);
    const BigUint three_x = AddM(AddM(x, x, c.p), x, c.p);
    const BigUint rhs = AddM(SubM(x3, three_x, c.p), c.b, c.p);
    return Eq(lhs, rhs);
}

}  // namespace

const Curve* CurveByOid(const std::string& oid) {
    for (const auto& c : Curves())
        if (c.oid == oid) return &c;
    return nullptr;
}

const Curve* CurveByName(const std::string& name) {
    for (const auto& c : Curves())
        if (c.name == name) return &c;
    return nullptr;
}

std::vector<std::byte> PublicKey(const Curve& c, std::span<const std::byte> d) {
    const Point q = ScalarMul(c, BigUint::FromBytesBE(d), Generator(c));
    if (q.z.IsZero()) throw std::runtime_error("ec: bad private key");
    BigUint x, y;
    ToAffine(c, q, x, y);
    Bytes out{std::byte{0x04}};
    const Bytes xb = x.ToBytesBE(c.bytes), yb = y.ToBytesBE(c.bytes);
    out.insert(out.end(), xb.begin(), xb.end());
    out.insert(out.end(), yb.begin(), yb.end());
    return out;
}

std::vector<std::byte> SignDigest(const Curve& c, std::span<const std::byte> d_bytes,
                                  std::span<const std::byte> digest, crypto::Hash h) {
    const BigUint d = BigUint::FromBytesBE(d_bytes);
    if (d.IsZero() || Compare(d, c.n) >= 0) throw std::runtime_error("ec: bad private key");
    const BigUint e = Bits2Int(c, digest);
    // RFC 6979 §3.2 deterministic k.
    const Bytes x = Int2Octets(c, d);
    const Bytes h1 = Int2Octets(c, Mod(e, c.n));
    const std::size_t hlen = crypto::HashSize(h);
    Bytes v(hlen, std::byte{0x01}), k(hlen, std::byte{0x00});
    auto mac = [&](std::initializer_list<std::span<const std::byte>> parts) {
        Bytes m;
        for (auto p : parts) m.insert(m.end(), p.begin(), p.end());
        return crypto::Hmac(h, k, m);
    };
    const Bytes zero{std::byte{0}}, one{std::byte{1}};
    k = mac({v, zero, x, h1});
    v = crypto::Hmac(h, k, v);
    k = mac({v, one, x, h1});
    v = crypto::Hmac(h, k, v);
    const std::size_t qlen = OrderBits(c);
    for (int attempt = 0; attempt < 64; ++attempt) {
        Bytes t;
        while (t.size() * 8 < qlen) {
            v = crypto::Hmac(h, k, v);
            t.insert(t.end(), v.begin(), v.end());
        }
        const BigUint kk = Bits2Int(c, t);
        if (!kk.IsZero() && Compare(kk, c.n) < 0) {
            const Point rp = ScalarMul(c, kk, Generator(c));
            BigUint rx, ry;
            ToAffine(c, rp, rx, ry);
            const BigUint r = Mod(rx, c.n);
            if (!r.IsZero()) {
                const BigUint s =
                    MulM(InvM(kk, c.n), AddM(Mod(e, c.n), MulM(r, d, c.n), c.n), c.n);
                if (!s.IsZero()) {
                    const Bytes body = [&] {
                        Bytes b = DerInteger(r.ToBytesBE(c.bytes));
                        const Bytes sb = DerInteger(s.ToBytesBE(c.bytes));
                        b.insert(b.end(), sb.begin(), sb.end());
                        return b;
                    }();
                    Bytes out{std::byte{0x30}};
                    if (body.size() >= 128) out.push_back(std::byte{0x81});
                    out.push_back(static_cast<std::byte>(body.size()));
                    out.insert(out.end(), body.begin(), body.end());
                    return out;
                }
            }
        }
        k = mac({v, zero});
        v = crypto::Hmac(h, k, v);
    }
    throw std::runtime_error("ec: could not derive a nonce");
}

bool VerifyDigest(const Curve& c, std::span<const std::byte> q,
                  std::span<const std::byte> digest, std::span<const std::byte> sig) {
    try {
        BigUint qx, qy;
        if (!ParsePoint(c, q, qx, qy)) return false;
        const auto parts = der::Children(der::ReadOne(sig));
        if (parts.size() != 2) return false;
        const BigUint r = BigUint::FromBytesBE(der::UnsignedInteger(parts[0]));
        const BigUint s = BigUint::FromBytesBE(der::UnsignedInteger(parts[1]));
        if (r.IsZero() || s.IsZero() || Compare(r, c.n) >= 0 || Compare(s, c.n) >= 0)
            return false;
        const BigUint e = Mod(Bits2Int(c, digest), c.n);
        const BigUint w = InvM(s, c.n);
        const Point x = AddPts(c, ScalarMul(c, MulM(e, w, c.n), Generator(c)),
                               ScalarMul(c, MulM(r, w, c.n), Point{qx, qy, BigUint(1)}));
        if (x.z.IsZero()) return false;
        BigUint ax, ay;
        ToAffine(c, x, ax, ay);
        return Eq(Mod(ax, c.n), r);
    } catch (const std::exception&) {
        return false;
    }
}

bool OnCurve(const Curve& c, std::span<const std::byte> q) {
    BigUint x, y;
    return ParsePoint(c, q, x, y);
}

bool OrderAnnihilatesGenerator(const Curve& c) {
    return ScalarMul(c, c.n, Generator(c)).z.IsZero();
}

}  // namespace foundation::ec
