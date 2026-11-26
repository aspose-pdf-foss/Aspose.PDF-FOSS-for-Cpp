// =============================================================================
// pkcs12_smoke_test — foundation::pkcs12 opens the OpenSSL-made fixtures in
// tests/fixtures/pkcs12 (see generate.sh): PBES2 / AES-256 and legacy 3DES +
// RC2-40, password "test". The extracted RSA key matches its certificate.
// =============================================================================

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "bigint.hpp"
#include "pkcs12.hpp"

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

void ExpectSignerContents(const foundation::pkcs12::Contents& c) {
    EXPECT_EQ(c.signer.subject_cn, "FOSS Test Signer");
    ASSERT_EQ(c.chain.size(), 1u);
    EXPECT_EQ(c.chain[0].subject_cn, "FOSS Test CA");
    EXPECT_EQ(c.key.modulus, c.signer.modulus);
    // The private exponent inverts the public one: (m^e)^d == m.
    using foundation::bigint::BigUint;
    const BigUint n = BigUint::FromBytesBE(c.key.modulus);
    const BigUint m(12345);
    const BigUint s = m.ModPow(BigUint::FromBytesBE(c.key.public_exponent), n);
    EXPECT_EQ(Compare(s.ModPow(BigUint::FromBytesBE(c.key.private_exponent), n), m), 0);
}

}  // namespace

TEST(Pkcs12, OpensPbes2Aes256) {
    ExpectSignerContents(foundation::pkcs12::Parse(Fixture("signer_aes.p12"), "test"));
}

TEST(Pkcs12, OpensLegacy3DesRc2) {
    ExpectSignerContents(foundation::pkcs12::Parse(Fixture("signer_legacy.p12"), "test"));
}

TEST(Pkcs12, WrongPasswordThrows) {
    try {
        foundation::pkcs12::Parse(Fixture("signer_aes.p12"), "nope");
        FAIL() << "expected an exception";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find("wrong password"), std::string::npos);
    }
    EXPECT_THROW(foundation::pkcs12::Parse(Fixture("signer_legacy.p12"), ""),
                 std::runtime_error);
}

TEST(Pkcs12, GarbageThrows) {
    const std::vector<std::byte> junk(32, std::byte{0x42});
    EXPECT_THROW(foundation::pkcs12::Parse(junk, "test"), std::runtime_error);
}
