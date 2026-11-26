#include "pkcs12.hpp"

#include <stdexcept>

#include "aes_cbc.hpp"
#include "crypto.hpp"

namespace foundation::pkcs12 {

namespace {

using Bytes = std::vector<std::byte>;
using der::Children;
using der::Node;
using der::Oid;

[[noreturn]] void Fail(const std::string& what) {
    throw std::runtime_error("pkcs12: " + what);
}

const char* kData = "1.2.840.113549.1.7.1";
const char* kEncryptedData = "1.2.840.113549.1.7.6";
const char* kKeyBag = "1.2.840.113549.1.12.10.1.1";
const char* kShroudedKeyBag = "1.2.840.113549.1.12.10.1.2";
const char* kCertBag = "1.2.840.113549.1.12.10.1.3";
const char* kX509Cert = "1.2.840.113549.1.9.22.1";
const char* kRsa = "1.2.840.113549.1.1.1";

Bytes Copy(std::span<const std::byte> s) { return {s.begin(), s.end()}; }

crypto::Hash HashForOid(const std::string& oid) {
    if (oid == "1.3.14.3.2.26" || oid == "1.2.840.113549.2.7")
        return crypto::Hash::Sha1;                     // sha1 / hmacWithSHA1
    if (oid == "2.16.840.1.101.3.4.2.1" || oid == "1.2.840.113549.2.9")
        return crypto::Hash::Sha256;                   // sha256 / hmacWithSHA256
    if (oid == "2.16.840.1.101.3.4.2.2" || oid == "1.2.840.113549.2.10")
        return crypto::Hash::Sha384;
    if (oid == "2.16.840.1.101.3.4.2.3" || oid == "1.2.840.113549.2.11")
        return crypto::Hash::Sha512;
    Fail("unsupported hash " + oid);
}

// Decrypt `data` per the AlgorithmIdentifier `alg` with `password`.
Bytes Decrypt(const Node& alg, std::span<const std::byte> data,
              const std::string& password) {
    const auto parts = Children(alg);
    if (parts.empty()) Fail("bad AlgorithmIdentifier");
    const std::string oid = Oid(parts[0]);
    if (oid == "1.2.840.113549.1.5.13") {  // PBES2
        const auto p = Children(parts.at(1));
        const auto kdf = Children(p.at(0));
        if (Oid(kdf.at(0)) != "1.2.840.113549.1.5.12") Fail("PBES2 KDF is not PBKDF2");
        const auto kp = Children(kdf.at(1));
        const auto salt = kp.at(0).content;
        const auto iterations = static_cast<std::uint32_t>(der::SmallInteger(kp.at(1)));
        crypto::Hash prf = crypto::Hash::Sha1;
        for (std::size_t i = 2; i < kp.size(); ++i)
            if (kp[i].tag == 0x30) prf = HashForOid(Oid(Children(kp[i]).at(0)));
        const auto enc = Children(p.at(1));
        const std::string cipher = Oid(enc.at(0));
        std::size_t key_len = 0;
        if (cipher == "2.16.840.1.101.3.4.1.2") key_len = 16;       // aes128-CBC
        else if (cipher == "2.16.840.1.101.3.4.1.42") key_len = 32; // aes256-CBC
        else Fail("unsupported PBES2 cipher " + cipher);
        const std::span<const std::byte> pw(
            reinterpret_cast<const std::byte*>(password.data()), password.size());
        const Bytes key = crypto::Pbkdf2(prf, pw, salt, iterations, key_len);
        return aes_cbc::Decrypt(key, enc.at(1).content, data);
    }
    // PKCS#12 PBE (RFC 7292 appendix C): SHA-1 KDF, params {salt, iterations}.
    const auto p = Children(parts.at(1));
    const auto salt = p.at(0).content;
    const auto iterations = static_cast<std::uint32_t>(der::SmallInteger(p.at(1)));
    auto derive = [&](std::uint8_t id, std::size_t n) {
        return crypto::Pkcs12Kdf(crypto::Hash::Sha1, password, salt, id, iterations, n);
    };
    if (oid == "1.2.840.113549.1.12.1.3")  // pbeWithSHAAnd3-KeyTripleDES-CBC
        return crypto::Des3CbcDecrypt(derive(1, 24), derive(2, 8), data);
    if (oid == "1.2.840.113549.1.12.1.6")  // pbeWithSHAAnd40BitRC2-CBC
        return crypto::Rc2CbcDecrypt(derive(1, 5), 40, derive(2, 8), data);
    if (oid == "1.2.840.113549.1.12.1.5")  // pbeWithSHAAnd128BitRC2-CBC
        return crypto::Rc2CbcDecrypt(derive(1, 16), 128, derive(2, 8), data);
    Fail("unsupported encryption " + oid);
}

// PrivateKeyInfo (PKCS#8) -> the RSA key.
RsaPrivateKey ParsePrivateKeyInfo(std::span<const std::byte> der_bytes) {
    const auto pki = Children(der::ReadOne(der_bytes));
    if (pki.size() < 3 || Oid(Children(pki[1]).at(0)) != kRsa)
        Fail("the private key is not RSA (only RSA keys are supported)");
    const auto rsa = Children(der::ReadOne(pki[2].content));
    if (rsa.size() < 4) Fail("bad RSAPrivateKey");
    return {der::UnsignedInteger(rsa[1]), der::UnsignedInteger(rsa[2]),
            der::UnsignedInteger(rsa[3])};
}

void ReadSafeContents(std::span<const std::byte> safe, const std::string& password,
                      std::vector<RsaPrivateKey>& keys, std::vector<der::Certificate>& certs) {
    for (const Node& bag : Children(der::ReadOne(safe))) {
        const auto parts = Children(bag);
        if (parts.size() < 2) continue;
        const std::string type = Oid(parts[0]);
        const Node value = der::ReadOne(parts[1].content);  // [0] EXPLICIT
        if (type == kKeyBag) {
            keys.push_back(ParsePrivateKeyInfo(value.whole));
        } else if (type == kShroudedKeyBag) {
            const auto epki = Children(value);
            const Bytes plain = Decrypt(epki.at(0), epki.at(1).content, password);
            keys.push_back(ParsePrivateKeyInfo(plain));
        } else if (type == kCertBag) {
            const auto cb = Children(value);
            if (cb.size() >= 2 && Oid(cb[0]) == kX509Cert) {
                const Node octets = der::ReadOne(cb[1].content);  // [0] OCTET STRING
                certs.push_back(der::ParseCertificate(octets.content));
            }
        }
    }
}

}  // namespace

Contents Parse(std::span<const std::byte> pfx, const std::string& password) {
    const auto top = Children(der::ReadOne(pfx));
    if (top.size() < 2) Fail("not a PFX");
    const auto auth_safe = Children(top[1]);
    if (auth_safe.size() < 2 || Oid(auth_safe[0]) != kData)
        Fail("only password-integrity PFX files are supported");
    const Node octets = der::ReadOne(auth_safe[1].content);  // [0] OCTET STRING
    const std::span<const std::byte> safe_bytes = octets.content;

    // MAC (RFC 7292 §5): HMAC over the AuthenticatedSafe with a PKCS#12-KDF key.
    if (top.size() >= 3) {
        const auto mac = Children(top[2]);
        const auto digest_info = Children(mac.at(0));
        const crypto::Hash h = HashForOid(Oid(Children(digest_info.at(0)).at(0)));
        const auto salt = mac.at(1).content;
        const std::uint32_t iterations =
            mac.size() > 2 ? static_cast<std::uint32_t>(der::SmallInteger(mac[2])) : 1;
        const Bytes key = crypto::Pkcs12Kdf(h, password, salt, 3, iterations,
                                            crypto::HashSize(h));
        const Bytes expected = crypto::Hmac(h, key, safe_bytes);
        if (expected != Copy(digest_info.at(1).content)) Fail("wrong password");
    }

    std::vector<RsaPrivateKey> keys;
    std::vector<der::Certificate> certs;
    for (const Node& ci : Children(der::ReadOne(safe_bytes))) {
        const auto parts = Children(ci);
        if (parts.size() < 2) continue;
        const std::string type = Oid(parts[0]);
        if (type == kData) {
            ReadSafeContents(der::ReadOne(parts[1].content).content, password, keys, certs);
        } else if (type == kEncryptedData) {
            const auto ed = Children(der::ReadOne(parts[1].content));
            const auto eci = Children(ed.at(1));  // EncryptedContentInfo
            const Bytes plain = Decrypt(eci.at(1), eci.at(2).content, password);
            ReadSafeContents(plain, password, keys, certs);
        }
    }
    if (keys.empty()) Fail("no private key");
    Contents out;
    out.key = keys.front();
    bool found = false;
    for (auto& c : certs) {
        if (!found && c.modulus == out.key.modulus) {
            out.signer = std::move(c);
            found = true;
        } else {
            out.chain.push_back(std::move(c));
        }
    }
    if (!found) Fail("no certificate for the private key");
    return out;
}

}  // namespace foundation::pkcs12
