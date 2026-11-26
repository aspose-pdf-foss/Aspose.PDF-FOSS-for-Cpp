// =============================================================================
// pkcs7_verify_smoke_test — foundation::pkcs7::VerifyDetached accepts the
// OpenSSL-made detached CMS fixture and rejects modified content; signatures
// made by SignDetached with a PKCS#12 key (or the bundled key) verify.
// =============================================================================

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "pkcs12.hpp"
#include "pkcs7_sign.hpp"
#include "pkcs7_verify.hpp"

#include <gtest/gtest.h>

namespace {

std::vector<std::byte> Fixture(const std::string& name) {
    const auto path = std::filesystem::path(__FILE__).parent_path() / "fixtures" /
                      "pkcs12" / name;
    std::ifstream in(path, std::ios::binary);
    std::vector<char> raw((std::istreambuf_iterator<char>(in)), {});
    return {reinterpret_cast<const std::byte*>(raw.data()),
            reinterpret_cast<const std::byte*>(raw.data()) + raw.size()};
}

foundation::pkcs7::Signer PfxSigner() {
    const auto c = foundation::pkcs12::Parse(Fixture("signer_aes.p12"), "test");
    foundation::pkcs7::Signer s;
    s.modulus = c.key.modulus;
    s.private_exponent = c.key.private_exponent;
    s.certificate = c.signer.der;
    s.issuer = c.signer.issuer;
    s.serial = c.signer.serial;
    for (const auto& cert : c.chain) s.chain.push_back(cert.der);
    s.name = c.signer.subject_cn;
    return s;
}

}  // namespace

TEST(Pkcs7Verify, OpenSslSignatureVerifies) {
    const auto v = foundation::pkcs7::VerifyDetached(Fixture("openssl_cms.der"),
                                                     Fixture("content.bin"));
    EXPECT_TRUE(v.valid) << v.error;
    EXPECT_EQ(v.signer.subject_cn, "FOSS Test Signer");
}

TEST(Pkcs7Verify, ModifiedContentFails) {
    auto content = Fixture("content.bin");
    content[0] = std::byte{'X'};
    const auto v = foundation::pkcs7::VerifyDetached(Fixture("openssl_cms.der"), content);
    EXPECT_FALSE(v.valid);
    EXPECT_NE(v.error.find("modified"), std::string::npos) << v.error;
}

TEST(Pkcs7Verify, PfxSignedRoundTripWithPadding) {
    const auto content = Fixture("content.bin");
    auto cms = foundation::pkcs7::SignDetached(content, "261126120000Z", PfxSigner());
    cms.resize(cms.size() + 300, std::byte{0});  // /Contents placeholder padding
    const auto v = foundation::pkcs7::VerifyDetached(cms, content);
    EXPECT_TRUE(v.valid) << v.error;
    EXPECT_EQ(v.signer.subject_cn, "FOSS Test Signer");
}

TEST(Pkcs7Verify, BundledSignerRoundTrip) {
    const auto content = Fixture("content.bin");
    const auto v = foundation::pkcs7::VerifyDetached(
        foundation::pkcs7::SignDetached(content, "261126120000Z"), content);
    EXPECT_TRUE(v.valid) << v.error;
    EXPECT_EQ(v.signer.subject_cn, foundation::pkcs7::BundledSignerName());
}

TEST(Pkcs7Verify, TamperedSignatureFails) {
    const auto content = Fixture("content.bin");
    auto cms = foundation::pkcs7::SignDetached(content, "261126120000Z", PfxSigner());
    cms[cms.size() - 5] ^= std::byte{0x01};  // inside the RSA signature
    const auto v = foundation::pkcs7::VerifyDetached(cms, content);
    EXPECT_FALSE(v.valid);
}

TEST(Pkcs7Verify, GarbageIsInvalid) {
    const std::vector<std::byte> junk(16, std::byte{0x30});
    const auto v = foundation::pkcs7::VerifyDetached(junk, junk);
    EXPECT_FALSE(v.valid);
    EXPECT_FALSE(v.error.empty());
}

// RSASSA-PSS signatures made by OpenSSL: SHA-256, SHA-384 with maximum salt,
// and SHA-1 with every parameter defaulted (empty RSASSA-PSS-params).
TEST(Pkcs7Verify, OpenSslRsaPssVerifies) {
    for (const char* name :
         {"openssl_cms_pss.der", "openssl_cms_pss_max.der", "openssl_cms_pss_sha1.der"}) {
        const auto v =
            foundation::pkcs7::VerifyDetached(Fixture(name), Fixture("content.bin"));
        EXPECT_TRUE(v.valid) << name << ": " << v.error;
        EXPECT_EQ(v.signer.subject_cn, "FOSS Test Signer") << name;
        auto modified = Fixture("content.bin");
        modified[2] = std::byte{'?'};
        EXPECT_FALSE(foundation::pkcs7::VerifyDetached(Fixture(name), modified).valid)
            << name;
        auto tampered = Fixture(name);
        tampered[tampered.size() - 3] ^= std::byte{0x40};  // inside the signature
        EXPECT_FALSE(
            foundation::pkcs7::VerifyDetached(tampered, Fixture("content.bin")).valid)
            << name;
    }
}
