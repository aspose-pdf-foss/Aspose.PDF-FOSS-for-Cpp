#include "pkcs7_verify.hpp"

#include <stdexcept>
#include <vector>

#include "bigint.hpp"
#include "crypto.hpp"
#include "ec.hpp"

namespace foundation::pkcs7 {

namespace {

using Bytes = std::vector<std::byte>;
using der::Children;
using der::Node;
using der::Oid;

bool HashForDigestOid(const std::string& oid, crypto::Hash& h) {
    if (oid == "1.3.14.3.2.26") { h = crypto::Hash::Sha1; return true; }
    if (oid == "2.16.840.1.101.3.4.2.1") { h = crypto::Hash::Sha256; return true; }
    if (oid == "2.16.840.1.101.3.4.2.2") { h = crypto::Hash::Sha384; return true; }
    if (oid == "2.16.840.1.101.3.4.2.3") { h = crypto::Hash::Sha512; return true; }
    return false;
}

Bytes Copy(std::span<const std::byte> s) { return {s.begin(), s.end()}; }

// RSASSA-PKCS1-v1_5 verification of `signature` over `message` with the
// certificate's key: the hash comes from the recovered DigestInfo.
bool RsaVerify(const der::Certificate& cert, std::span<const std::byte> message,
               std::span<const std::byte> signature) {
    if (cert.modulus.empty()) return false;
    using bigint::BigUint;
    const BigUint n = BigUint::FromBytesBE(cert.modulus);
    const std::size_t k = (n.BitLength() + 7) / 8;
    if (signature.size() != k) return false;
    const BigUint s = BigUint::FromBytesBE(signature);
    if (Compare(s, n) >= 0) return false;
    const Bytes em = s.ModPow(BigUint::FromBytesBE(cert.exponent), n).ToBytesBE(k);
    if (em.size() < 11 || em[0] != std::byte{0} || em[1] != std::byte{1}) return false;
    std::size_t i = 2;
    while (i < em.size() && em[i] == std::byte{0xFF}) ++i;
    if (i < 10 || i >= em.size() || em[i] != std::byte{0}) return false;
    try {
        const Node digest_info = der::ReadOne(std::span<const std::byte>(em).subspan(i + 1));
        const auto parts = Children(digest_info);
        crypto::Hash h;
        if (parts.size() != 2 || !HashForDigestOid(Oid(Children(parts[0]).at(0)), h))
            return false;
        return Copy(parts[1].content) == crypto::HashOf(h, message);
    } catch (const std::exception&) {
        return false;
    }
}

// MGF1 (RFC 8017 B.2.1).
Bytes Mgf1(crypto::Hash h, std::span<const std::byte> seed, std::size_t len) {
    Bytes out;
    for (std::uint32_t counter = 0; out.size() < len; ++counter) {
        Bytes block(seed.begin(), seed.end());
        for (int shift = 24; shift >= 0; shift -= 8)
            block.push_back(static_cast<std::byte>((counter >> shift) & 0xFF));
        const Bytes d = crypto::HashOf(h, block);
        out.insert(out.end(), d.begin(), d.end());
    }
    out.resize(len);
    return out;
}

struct PssParams {
    crypto::Hash hash = crypto::Hash::Sha1;
    crypto::Hash mgf_hash = crypto::Hash::Sha1;
    std::size_t salt_len = 20;
};

// RSASSA-PSS-params (RFC 8017 A.2.3); every field has a default.
bool ParsePssParams(const std::vector<Node>& sig_alg, PssParams& p) {
    if (sig_alg.size() < 2 || sig_alg[1].tag != 0x30) return true;  // all defaults
    for (const Node& f : Children(sig_alg[1])) {
        const Node inner = der::ReadOne(f.content);  // [n] EXPLICIT
        if (f.tag == 0xA0) {
            if (!HashForDigestOid(Oid(Children(inner).at(0)), p.hash)) return false;
        } else if (f.tag == 0xA1) {
            const auto mgf = Children(inner);
            if (Oid(mgf.at(0)) != "1.2.840.113549.1.1.8") return false;  // MGF1 only
            if (!HashForDigestOid(Oid(Children(mgf.at(1)).at(0)), p.mgf_hash)) return false;
        } else if (f.tag == 0xA2) {
            p.salt_len = static_cast<std::size_t>(der::SmallInteger(inner));
        } else if (f.tag == 0xA3) {
            if (der::SmallInteger(inner) != 1) return false;  // trailer 0xBC only
        }
    }
    return true;
}

// RSASSA-PSS verification (RFC 8017 §8.1.2 / EMSA-PSS-VERIFY §9.1.2).
bool RsaPssVerify(const der::Certificate& cert, std::span<const std::byte> message,
                  std::span<const std::byte> signature, const PssParams& p) {
    if (cert.modulus.empty()) return false;
    using bigint::BigUint;
    const BigUint n = BigUint::FromBytesBE(cert.modulus);
    const std::size_t mod_bits = n.BitLength();
    const std::size_t k = (mod_bits + 7) / 8;
    if (signature.size() != k) return false;
    const BigUint s = BigUint::FromBytesBE(signature);
    if (Compare(s, n) >= 0) return false;
    const std::size_t em_bits = mod_bits - 1;
    const std::size_t em_len = (em_bits + 7) / 8;
    Bytes em = s.ModPow(BigUint::FromBytesBE(cert.exponent), n).ToBytesBE(k);
    if (em_len < k) {  // the top octet must be zero
        if (em[0] != std::byte{0}) return false;
        em.erase(em.begin());
    }
    const Bytes m_hash = crypto::HashOf(p.hash, message);
    const std::size_t h_len = m_hash.size();
    if (em_len < h_len + p.salt_len + 2 || em.back() != std::byte{0xBC}) return false;
    const std::size_t db_len = em_len - h_len - 1;
    const std::span<const std::byte> masked_db(em.data(), db_len);
    const std::span<const std::byte> hh(em.data() + db_len, h_len);
    const unsigned top_mask = 0xFFu >> (8 * em_len - em_bits);
    if ((std::to_integer<unsigned>(masked_db[0]) & ~top_mask & 0xFFu) != 0) return false;
    Bytes db = Mgf1(p.mgf_hash, hh, db_len);
    for (std::size_t i = 0; i < db_len; ++i) db[i] ^= masked_db[i];
    db[0] = static_cast<std::byte>(std::to_integer<unsigned>(db[0]) & top_mask);
    const std::size_t ps_len = em_len - h_len - p.salt_len - 2;
    for (std::size_t i = 0; i < ps_len; ++i)
        if (db[i] != std::byte{0}) return false;
    if (db[ps_len] != std::byte{0x01}) return false;
    Bytes m_prime(8, std::byte{0});
    m_prime.insert(m_prime.end(), m_hash.begin(), m_hash.end());
    m_prime.insert(m_prime.end(), db.end() - static_cast<std::ptrdiff_t>(p.salt_len), db.end());
    return crypto::HashOf(p.hash, m_prime) == Bytes(hh.begin(), hh.end());
}

}  // namespace

Verification VerifyDetached(std::span<const std::byte> cms,
                            std::span<const std::byte> content) {
    Verification v;
    try {
        std::size_t pos = 0;
        const Node ci = der::Read(cms, pos);  // trailing padding ignored
        const auto ci_parts = Children(ci);
        if (ci_parts.size() < 2 || Oid(ci_parts[0]) != "1.2.840.113549.1.7.2") {
            v.error = "not a CMS SignedData";
            return v;
        }
        const auto sd = Children(der::ReadOne(ci_parts[1].content));
        std::vector<der::Certificate> certs;
        const Node* signer_infos = nullptr;
        const Node* encap = sd.size() > 2 ? &sd[2] : nullptr;
        for (std::size_t i = 3; i < sd.size(); ++i) {
            if (sd[i].tag == 0xA0)
                for (const Node& c : Children(sd[i]))
                    if (c.tag == 0x30) certs.push_back(der::ParseCertificate(c.whole));
            if (sd[i].tag == 0x31) signer_infos = &sd[i];
        }
        if (signer_infos == nullptr || Children(*signer_infos).empty()) {
            v.error = "no signer";
            return v;
        }
        const auto si = Children(Children(*signer_infos).front());
        if (si.size() < 5) {
            v.error = "malformed SignerInfo";
            return v;
        }
        crypto::Hash h;
        if (!HashForDigestOid(Oid(Children(si[2]).at(0)), h)) {
            v.error = "unsupported digest algorithm";
            return v;
        }
        const Node* attrs = si[3].tag == 0xA0 ? &si[3] : nullptr;
        const Node& sig_alg = si[attrs ? 4 : 3];
        const Node& signature = si[attrs ? 5 : 4];
        const auto sig_alg_parts = Children(sig_alg);
        const std::string sig_oid = Oid(sig_alg_parts.at(0));
        const bool is_pss = sig_oid == "1.2.840.113549.1.1.10";
        PssParams pss;
        if (is_pss && !ParsePssParams(sig_alg_parts, pss)) {
            v.error = "unsupported RSASSA-PSS parameters";
            return v;
        }
        const bool is_rsa = !is_pss && sig_oid.rfind("1.2.840.113549.1.1.", 0) == 0;
        // ECDSA: ecdsa-with-SHA* names its hash; bare id-ecPublicKey uses
        // the digest algorithm.
        crypto::Hash ecdsa_hash = h;
        bool is_ecdsa = true;
        if (sig_oid == "1.2.840.10045.4.1") ecdsa_hash = crypto::Hash::Sha1;
        else if (sig_oid == "1.2.840.10045.4.3.2") ecdsa_hash = crypto::Hash::Sha256;
        else if (sig_oid == "1.2.840.10045.4.3.3") ecdsa_hash = crypto::Hash::Sha384;
        else if (sig_oid == "1.2.840.10045.4.3.4") ecdsa_hash = crypto::Hash::Sha512;
        else if (sig_oid != "1.2.840.10045.2.1") is_ecdsa = false;
        if (!is_rsa && !is_pss && !is_ecdsa) {
            v.error = "unsupported signature algorithm " + sig_oid;
            return v;
        }

        // What was digested: the content, or (adbe.pkcs7.sha1) the
        // encapsulated SHA-1 of it.
        Bytes data = Copy(content);
        if (encap != nullptr) {
            const auto ec = Children(*encap);
            if (ec.size() >= 2) {
                const Node octets = der::ReadOne(ec[1].content);
                const Bytes sha1 = crypto::HashOf(crypto::Hash::Sha1, content);
                if (Copy(octets.content) != sha1) {
                    v.error = "the document was modified (digest mismatch)";
                    return v;
                }
                data = Copy(octets.content);
            }
        }
        const Bytes digest = crypto::HashOf(h, data);

        Bytes signed_bytes = digest;  // no signed attributes: sign the digest
        bool raw_digest = true;
        if (attrs != nullptr) {
            bool found = false;
            for (const Node& attr : Children(*attrs)) {
                const auto a = Children(attr);
                if (a.size() == 2 && Oid(a[0]) == "1.2.840.113549.1.9.4") {
                    const auto vals = Children(a[1]);
                    found = !vals.empty() && Copy(vals[0].content) == digest;
                }
            }
            if (!found) {
                v.error = "the document was modified (digest mismatch)";
                return v;
            }
            signed_bytes = Copy(attrs->whole);
            signed_bytes[0] = std::byte{0x31};  // signed as SET OF
            raw_digest = false;
        }

        // The signer's certificate: by issuer + serial, else each in turn.
        std::vector<const der::Certificate*> candidates;
        if (si[1].tag == 0x30) {
            const auto ias = Children(si[1]);
            for (const auto& c : certs)
                if (ias.size() == 2 && c.issuer == Copy(ias[0].whole) &&
                    c.serial == Copy(ias[1].whole))
                    candidates.push_back(&c);
        }
        if (candidates.empty())
            for (const auto& c : certs) candidates.push_back(&c);
        // Without signed attributes the signature covers the content
        // itself (RsaVerify hashes its message), else the attributes.
        const Bytes& message = raw_digest ? data : signed_bytes;
        for (const der::Certificate* c : candidates) {
            bool ok = false;
            if (is_rsa) {
                ok = RsaVerify(*c, message, signature.content);
            } else if (is_pss) {
                ok = RsaPssVerify(*c, message, signature.content, pss);
            } else if (const ec::Curve* curve = ec::CurveByOid(c->ec_curve_oid)) {
                ok = ec::VerifyDigest(*curve, c->ec_point,
                                      crypto::HashOf(ecdsa_hash, message),
                                      signature.content);
            }
            if (ok) {
                v.valid = true;
                v.signer = *c;
                return v;
            }
        }
        v.error = candidates.empty() ? "no signer certificate"
                                     : "the signature does not match the signer's key";
        if (!candidates.empty()) v.signer = *candidates.front();
    } catch (const std::exception& e) {
        v.error = std::string("malformed signature: ") + e.what();
    }
    return v;
}

}  // namespace foundation::pkcs7
