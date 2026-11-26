// =============================================================================
// ecdsa_smoke_test — foundation::ec (P-256/384/521): curve self-checks, the
// RFC 6979 A.2.5 deterministic signature, sign/verify round trips; EC keys in
// PKCS#12 (tests/fixtures/pkcs12/ec*.p12, password "test"); OpenSSL ECDSA CMS
// signatures verify, and ours verify in turn.
// =============================================================================

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "der.hpp"
#include "ec.hpp"
#include "pkcs12.hpp"
#include "pkcs7_sign.hpp"
#include "pkcs7_verify.hpp"

#include <gtest/gtest.h>

namespace {

namespace ec = foundation::ec;
namespace crypto = foundation::crypto;

std::vector<std::byte> Hex(const std::string& h) {
    std::vector<std::byte> out;
    for (std::size_t i = 0; i + 1 < h.size(); i += 2)
        out.push_back(static_cast<std::byte>(std::stoi(h.substr(i, 2), nullptr, 16)));
    return out;
}

std::string ToHex(std::span<const std::byte> b) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (auto x : b) {
        s += d[std::to_integer<int>(x) >> 4];
        s += d[std::to_integer<int>(x) & 15];
    }
    return s;
}

std::vector<std::byte> Bytes(const std::string& s) {
    return {reinterpret_cast<const std::byte*>(s.data()),
            reinterpret_cast<const std::byte*>(s.data()) + s.size()};
}

std::vector<std::byte> Fixture(const std::string& name) {
    const auto path = std::filesystem::path(__FILE__).parent_path() / "fixtures" /
                      "pkcs12" / name;
    std::ifstream in(path, std::ios::binary);
    std::vector<char> raw((std::istreambuf_iterator<char>(in)), {});
    return {reinterpret_cast<const std::byte*>(raw.data()),
            reinterpret_cast<const std::byte*>(raw.data()) + raw.size()};
}

}  // namespace

TEST(Ecdsa, CurveParametersAreConsistent) {
    for (const char* name : {"P-256", "P-384", "P-521"}) {
        const ec::Curve* c = ec::CurveByName(name);
        ASSERT_NE(c, nullptr) << name;
        std::vector<std::byte> g{std::byte{0x04}};
        const auto gx = c->gx.ToBytesBE(c->bytes), gy = c->gy.ToBytesBE(c->bytes);
        g.insert(g.end(), gx.begin(), gx.end());
        g.insert(g.end(), gy.begin(), gy.end());
        EXPECT_TRUE(ec::OnCurve(*c, g)) << name;
        EXPECT_TRUE(ec::OrderAnnihilatesGenerator(*c)) << name;
    }
}

// RFC 6979 A.2.5: P-256, SHA-256, message "sample".
TEST(Ecdsa, Rfc6979P256Sha256) {
    const ec::Curve* c = ec::CurveByName("P-256");
    const auto d = Hex("c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b120f6721");
    EXPECT_EQ(ToHex(ec::PublicKey(*c, d)),
              "04"
              "60fed4ba255a9d31c961eb74c6356d68c049b8923b61fa6ce669622e60f29fb6"
              "7903fe1008b8bc99a41ae9e95628bc64f2f1b20c2d7e9f5177a3c294d4462299");
    const auto sig = ec::SignDigest(*c, d, crypto::HashOf(crypto::Hash::Sha256, Bytes("sample")),
                                    crypto::Hash::Sha256);
    const auto parts = foundation::der::Children(foundation::der::ReadOne(sig));
    ASSERT_EQ(parts.size(), 2u);
    EXPECT_EQ(ToHex(foundation::der::UnsignedInteger(parts[0])),
              "efd48b2aacb6a8fd1140dd9cd45e81d69d2c877b56aaf991c34d0ea84eaf3716");
    EXPECT_EQ(ToHex(foundation::der::UnsignedInteger(parts[1])),
              "f7cb1c942d657c41d436c7a1b6e29f65f3e900dbb9aff4064dc4ab2f843acda8");
}

TEST(Ecdsa, SignVerifyRoundTripAllCurves) {
    for (const char* name : {"P-256", "P-384", "P-521"}) {
        const ec::Curve* c = ec::CurveByName(name);
        std::vector<std::byte> d(c->bytes, std::byte{0x11});
        d[0] = std::byte{0x01};
        const auto q = ec::PublicKey(*c, d);
        const auto digest = crypto::HashOf(c->hash, Bytes("hello"));
        const auto sig = ec::SignDigest(*c, d, digest, c->hash);
        EXPECT_TRUE(ec::VerifyDigest(*c, q, digest, sig)) << name;
        const auto other = crypto::HashOf(c->hash, Bytes("hellO"));
        EXPECT_FALSE(ec::VerifyDigest(*c, q, other, sig)) << name;
    }
}

TEST(Ecdsa, Pkcs12EcKeys) {
    for (const char* name : {"ec256.p12", "ec384.p12", "ec521.p12"}) {
        const auto c = foundation::pkcs12::Parse(Fixture(name), "test");
        EXPECT_FALSE(c.key.ec_curve_oid.empty()) << name;
        EXPECT_EQ(c.signer.ec_point, c.key.ec_public) << name;
        ASSERT_EQ(c.chain.size(), 1u) << name;
        EXPECT_EQ(c.chain[0].subject_cn, "FOSS Test EC CA") << name;
    }
    EXPECT_EQ(foundation::pkcs12::Parse(Fixture("ec384.p12"), "test").signer.subject_cn,
              "FOSS EC P-384 Signer");
}

TEST(Ecdsa, OpenSslEcdsaCmsVerifies) {
    for (const char* name : {"openssl_cms_ec256.der", "openssl_cms_ec384.der"}) {
        const auto v = foundation::pkcs7::VerifyDetached(Fixture(name), Fixture("content.bin"));
        EXPECT_TRUE(v.valid) << name << ": " << v.error;
        auto modified = Fixture("content.bin");
        modified[1] = std::byte{'!'};
        EXPECT_FALSE(foundation::pkcs7::VerifyDetached(Fixture(name), modified).valid) << name;
    }
}

TEST(Ecdsa, EcPfxCmsRoundTrip) {
    const auto content = Fixture("content.bin");
    for (const char* name : {"ec256.p12", "ec384.p12", "ec521.p12"}) {
        const auto signer = foundation::pkcs7::SignerFromPkcs12(Fixture(name), "test");
        auto cms = foundation::pkcs7::SignDetached(content, "261126120000Z", signer);
        cms.resize(cms.size() + 64, std::byte{0});
        const auto v = foundation::pkcs7::VerifyDetached(cms, content);
        EXPECT_TRUE(v.valid) << name << ": " << v.error;
        EXPECT_EQ(v.signer.subject_cn, signer.name) << name;
    }
}
