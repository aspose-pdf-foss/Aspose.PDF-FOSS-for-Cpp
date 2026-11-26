#include "pkcs7_verify.hpp"

#include <stdexcept>
#include <vector>

#include "bigint.hpp"
#include "crypto.hpp"

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
        const std::string sig_oid = Oid(Children(sig_alg).at(0));
        if (sig_oid == "1.2.840.113549.1.1.10") {
            v.error = "RSA-PSS signatures are not supported";
            return v;
        }
        if (sig_oid.rfind("1.2.840.113549.1.1.", 0) != 0) {
            v.error = "only RSA signatures are supported";
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
        for (const der::Certificate* c : candidates) {
            bool ok;
            if (raw_digest) {
                // Signature directly over the content digest: RsaVerify
                // hashes its message, so compare against `data` instead.
                ok = RsaVerify(*c, data, signature.content);
            } else {
                ok = RsaVerify(*c, signed_bytes, signature.content);
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
