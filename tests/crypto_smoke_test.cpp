// =============================================================================
// crypto_smoke_test — foundation::crypto against published test vectors:
// HMAC (RFC 4231 / RFC 2202), PBKDF2 (RFC 7914 / RFC 6070), RC2 (RFC 2268),
// and Triple-DES / RC2 CBC against OpenSSL `enc` output.
// =============================================================================

#include <string>
#include <vector>

#include "crypto.hpp"

#include <gtest/gtest.h>

namespace c = foundation::crypto;

namespace {

std::vector<std::byte> Hex(const std::string& h) {
    std::vector<std::byte> out;
    for (std::size_t i = 0; i + 1 < h.size(); i += 2)
        out.push_back(static_cast<std::byte>(std::stoi(h.substr(i, 2), nullptr, 16)));
    return out;
}

std::vector<std::byte> Bytes(const std::string& s) {
    return {reinterpret_cast<const std::byte*>(s.data()),
            reinterpret_cast<const std::byte*>(s.data()) + s.size()};
}

std::string ToHex(const std::vector<std::byte>& b) {
    static const char* d = "0123456789abcdef";
    std::string s;
    for (auto x : b) {
        s += d[std::to_integer<int>(x) >> 4];
        s += d[std::to_integer<int>(x) & 15];
    }
    return s;
}

std::string Text(const std::vector<std::byte>& b) {
    return {reinterpret_cast<const char*>(b.data()), b.size()};
}

}  // namespace

TEST(Crypto, HmacRfcVectors) {
    EXPECT_EQ(ToHex(c::Hmac(c::Hash::Sha256, Bytes("Jefe"),
                            Bytes("what do ya want for nothing?"))),
              "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    EXPECT_EQ(ToHex(c::Hmac(c::Hash::Sha1, Bytes("Jefe"),
                            Bytes("what do ya want for nothing?"))),
              "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79");
}

TEST(Crypto, Pbkdf2RfcVectors) {
    EXPECT_EQ(ToHex(c::Pbkdf2(c::Hash::Sha256, Bytes("passwd"), Bytes("salt"), 1, 64)),
              "55ac046e56e3089fec1691c22544b605f94185216dde0465e68b9d57c20dacbc"
              "49ca9cccf179b645991664b39d77ef317c71b845b1e30bd509112041d3a19783");
    EXPECT_EQ(ToHex(c::Pbkdf2(c::Hash::Sha1, Bytes("password"), Bytes("salt"), 2, 20)),
              "ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957");
}

TEST(Crypto, Rc2Rfc2268Vectors) {
    EXPECT_EQ(ToHex(c::Rc2DecryptBlock(Hex("0000000000000000"), 63,
                                       Hex("ebb773f993278eff"))),
              "0000000000000000");
    EXPECT_EQ(ToHex(c::Rc2DecryptBlock(Hex("ffffffffffffffff"), 64,
                                       Hex("278b27e42e2f0d49"))),
              "ffffffffffffffff");
    EXPECT_EQ(ToHex(c::Rc2DecryptBlock(Hex("3000000000000000"), 64,
                                       Hex("30649edf9be7d2c2"))),
              "1000000000000001");
    EXPECT_EQ(ToHex(c::Rc2DecryptBlock(Hex("88"), 64, Hex("61a8a244adacccf0"))),
              "0000000000000000");
    EXPECT_EQ(ToHex(c::Rc2DecryptBlock(Hex("88bca90e90875a7f0f79c384627bafb2"),
                                       128, Hex("2269552ab0f85ca6"))),
              "0000000000000000");
}

TEST(Crypto, CbcDecryptAgainstOpenSsl) {
    const std::vector<std::byte> iv = Hex("0011223344556677");
    EXPECT_EQ(Text(c::Des3CbcDecrypt(
                  Hex("0123456789abcdeff1e0d3c2b5a49786fedcba9876543210"), iv,
                  Hex("d3bd2ef706d22a4b468c4264954b6866a87dc2b37b3836fdf4fe5771171923bf"))),
              "The quick brown fox jumps!!");
    EXPECT_EQ(Text(c::Rc2CbcDecrypt(
                  Hex("0102030405"), 40, iv,
                  Hex("5234d29a1fafb9b73419ec733562695d64c1e28c69d1f86b33da6b12dc65f3c4"))),
              "The quick brown fox jumps!!");
    EXPECT_EQ(Text(c::Rc2CbcDecrypt(
                  Hex("000102030405060708090a0b0c0d0e0f"), 128, iv,
                  Hex("cffccf50fde40fdabf109b7c7090cb8ee2db2cad8dfa1c61e749c15b8c5e38eb"))),
              "The quick brown fox jumps!!");
}

TEST(Crypto, BadPaddingThrows) {
    EXPECT_THROW(c::Des3CbcDecrypt(Hex("0123456789abcdeff1e0d3c2b5a49786fedcba9876543210"),
                                   Hex("0011223344556677"), Hex("0011223344556677")),
                 std::runtime_error);
}
