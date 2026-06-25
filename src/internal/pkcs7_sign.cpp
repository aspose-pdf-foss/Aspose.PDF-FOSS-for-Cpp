#include "pkcs7_sign.hpp"

#include <stdexcept>
#include <string>

#include "bigint.hpp"
#include "digest.hpp"
#include "pkcs7_keymat.hpp"

namespace foundation::pkcs7 {

namespace {

using Bytes = std::vector<std::byte>;

void Append(Bytes& dst, const Bytes& src) {
    dst.insert(dst.end(), src.begin(), src.end());
}

Bytes FromHex(const char* hex) {
    Bytes out;
    for (const char* p = hex; p[0] && p[1]; p += 2) {
        auto nib = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            throw std::runtime_error("foundation::pkcs7: bad hex digit");
        };
        out.push_back(static_cast<std::byte>((nib(p[0]) << 4) | nib(p[1])));
    }
    return out;
}

// ---- DER primitives --------------------------------------------------------

Bytes DerLen(std::size_t n) {
    Bytes out;
    if (n < 0x80) {
        out.push_back(static_cast<std::byte>(n));
        return out;
    }
    Bytes tmp;
    while (n > 0) {
        tmp.push_back(static_cast<std::byte>(n & 0xFF));
        n >>= 8;
    }
    out.push_back(static_cast<std::byte>(0x80 | tmp.size()));
    for (auto it = tmp.rbegin(); it != tmp.rend(); ++it) out.push_back(*it);
    return out;
}

// Tag-length-value with an explicit single-byte tag.
Bytes Tlv(std::uint8_t tag, const Bytes& content) {
    Bytes out;
    out.push_back(static_cast<std::byte>(tag));
    Append(out, DerLen(content.size()));
    Append(out, content);
    return out;
}

Bytes DerInteger(const Bytes& magnitude) {
    // Strip leading zero bytes, then prepend 0x00 if the top bit is set
    // (the value is unsigned).
    std::size_t i = 0;
    while (i + 1 < magnitude.size() &&
           static_cast<std::uint8_t>(magnitude[i]) == 0) {
        ++i;
    }
    Bytes v(magnitude.begin() + static_cast<std::ptrdiff_t>(i),
            magnitude.end());
    if (v.empty()) v.push_back(std::byte{0});
    if (static_cast<std::uint8_t>(v.front()) & 0x80) {
        v.insert(v.begin(), std::byte{0});
    }
    return Tlv(0x02, v);
}

Bytes DerIntSmall(std::uint8_t v) { return Tlv(0x02, Bytes{std::byte{v}}); }

Bytes DerOid(const Bytes& body) { return Tlv(0x06, body); }
Bytes DerNull() { return Tlv(0x05, {}); }
Bytes DerSeq(const Bytes& c) { return Tlv(0x30, c); }
Bytes DerSet(const Bytes& c) { return Tlv(0x31, c); }
Bytes DerOctet(const Bytes& c) { return Tlv(0x04, c); }

Bytes Concat(std::initializer_list<Bytes> parts) {
    Bytes out;
    for (const auto& p : parts) Append(out, p);
    return out;
}

// OID value bytes (after tag/len).
const Bytes kOidSha256 =
    FromHex("608648016503040201");                    // 2.16.840.1.101.3.4.2.1
const Bytes kOidData = FromHex("2a864886f70d010701");  // 1.2.840.113549.1.7.1
const Bytes kOidSignedData =
    FromHex("2a864886f70d010702");                     // ...1.7.2
const Bytes kOidRsa = FromHex("2a864886f70d010101");   // ...1.1.1
const Bytes kOidCtype = FromHex("2a864886f70d010903");  // ...1.9.3
const Bytes kOidMdigest = FromHex("2a864886f70d010904");  // ...1.9.4
const Bytes kOidSigTime = FromHex("2a864886f70d010905");  // ...1.9.5

Bytes AlgSha256() { return DerSeq(Concat({DerOid(kOidSha256), DerNull()})); }
Bytes AlgRsa() { return DerSeq(Concat({DerOid(kOidRsa), DerNull()})); }

// ---- RSA PKCS#1 v1.5 signature over SHA-256(message) -----------------------

Bytes RsaSignSha256(const Bytes& message) {
    const Bytes n_raw = FromHex(keymat::kModulusHex);
    const Bytes d_raw = FromHex(keymat::kPrivExpHex);
    const auto n = bigint::BigUint::FromBytesBE(
        std::span<const std::byte>(n_raw.data(), n_raw.size()));
    const auto d = bigint::BigUint::FromBytesBE(
        std::span<const std::byte>(d_raw.data(), d_raw.size()));
    const std::size_t k = (n.BitLength() + 7) / 8;  // modulus byte length

    // DigestInfo ::= SEQUENCE { AlgorithmIdentifier(sha256,NULL), OCTET digest }
    const auto digest = digest::Sha256(
        std::span<const std::byte>(message.data(), message.size()));
    const Bytes digest_info =
        DerSeq(Concat({AlgSha256(), DerOctet(digest)}));

    // EMSA-PKCS1-v1.5: 0x00 0x01 [0xFF * ps] 0x00 DigestInfo, total length k.
    if (digest_info.size() + 11 > k) {
        throw std::runtime_error("foundation::pkcs7: modulus too small");
    }
    Bytes em;
    em.push_back(std::byte{0x00});
    em.push_back(std::byte{0x01});
    const std::size_t ps = k - digest_info.size() - 3;
    em.insert(em.end(), ps, std::byte{0xFF});
    em.push_back(std::byte{0x00});
    Append(em, digest_info);

    const auto m = bigint::BigUint::FromBytesBE(
        std::span<const std::byte>(em.data(), em.size()));
    const auto s = m.ModPow(d, n);
    return s.ToBytesBE(k);
}

}  // namespace

std::string BundledSignerName() { return "Aspose.PDF FOSS Test Signer"; }

Bytes SignDetached(std::span<const std::byte> content,
                   const std::string& signing_time_utc) {
    const Bytes content_vec(content.begin(), content.end());
    const auto content_digest = digest::Sha256(content);

    // Authenticated (signed) attributes, in ascending DER order
    // (contentType < messageDigest < signingTime by OID value).
    const Bytes attr_ctype =
        DerSeq(Concat({DerOid(kOidCtype), DerSet(DerOid(kOidData))}));
    const Bytes attr_mdigest = DerSeq(
        Concat({DerOid(kOidMdigest), DerSet(DerOctet(content_digest))}));
    Bytes utc;
    for (char c : signing_time_utc) utc.push_back(static_cast<std::byte>(c));
    const Bytes attr_sigtime =
        DerSeq(Concat({DerOid(kOidSigTime), DerSet(Tlv(0x17, utc))}));

    const Bytes attrs_concat =
        Concat({attr_ctype, attr_mdigest, attr_sigtime});

    // Sign the DER of the attributes as an explicit SET OF (tag 0x31).
    const Bytes signed_attrs_der = DerSet(attrs_concat);
    const Bytes signature = RsaSignSha256(signed_attrs_der);

    // SignerInfo.
    const Bytes sid = DerSeq(Concat({FromHex(keymat::kIssuerDerHex),
                                     FromHex(keymat::kSerialTlvHex)}));
    const Bytes signer_info = DerSeq(Concat({
        DerIntSmall(1),                     // version
        sid,                                // issuerAndSerialNumber
        AlgSha256(),                        // digestAlgorithm
        Tlv(0xA0, attrs_concat),            // signedAttrs [0] IMPLICIT
        AlgRsa(),                           // signatureAlgorithm
        DerOctet(signature),                // signature
    }));

    // EncapContentInfo (detached: eContentType only, no eContent).
    const Bytes encap = DerSeq(DerOid(kOidData));

    // SignedData.
    const Bytes signed_data = DerSeq(Concat({
        DerIntSmall(1),                     // version
        DerSet(AlgSha256()),                // digestAlgorithms
        encap,                              // encapContentInfo
        Tlv(0xA0, FromHex(keymat::kCertDerHex)),  // certificates [0] IMPLICIT
        DerSet(signer_info),                // signerInfos
    }));

    // ContentInfo.
    return DerSeq(Concat({DerOid(kOidSignedData), Tlv(0xA0, signed_data)}));
}

}  // namespace foundation::pkcs7
