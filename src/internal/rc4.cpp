#include "rc4.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>

namespace foundation::rc4 {

namespace {

// RC4 Key Scheduling Algorithm (KSA). Initialise the 256-byte
// permutation S to the identity, then mix in the key by walking
// i=0..255 with a running j and swapping S[i] with S[j].
std::array<std::uint8_t, 256> Ksa(std::span<const std::byte> key) {
    std::array<std::uint8_t, 256> S{};
    for (int i = 0; i < 256; ++i) S[i] = std::uint8_t(i);
    std::uint8_t j = 0;
    const std::size_t klen = key.size();
    for (int i = 0; i < 256; ++i) {
        const std::uint8_t k = std::uint8_t(key[i % klen]);
        j = std::uint8_t(j + S[i] + k);
        std::uint8_t tmp = S[i];
        S[i] = S[j];
        S[j] = tmp;
    }
    return S;
}

}  // namespace

// Production API — RC4 is symmetric, the same keystream-XOR
// covers both directions. Validates the PDF key-size window
// (5..16 bytes per §7.6.4) and applies the keystream byte-by-
// byte over the input span.
std::vector<std::byte> Apply(
    std::span<const std::byte> key,
    std::span<const std::byte> data) {
    if (key.size() < 5 || key.size() > 16) {
        throw std::runtime_error(
            "rc4: key size must be between 5 and 16 bytes");
    }
    if (data.empty()) {
        return std::vector<std::byte>{};
    }

    auto S = Ksa(key);
    std::uint8_t i = 0, j = 0;
    std::vector<std::byte> out(data.size());
    for (std::size_t k = 0; k < data.size(); ++k) {
        i = std::uint8_t(i + 1);
        j = std::uint8_t(j + S[i]);
        std::uint8_t tmp = S[i];
        S[i] = S[j];
        S[j] = tmp;
        const std::uint8_t ks = S[std::uint8_t(S[i] + S[j])];
        out[k] = std::byte(std::uint8_t(std::uint8_t(data[k]) ^ ks));
    }
    return out;
}

// Gate-only wrappers — both call Apply with the same fixed key.
// The binding_roundtrip driver's Decode(Encode(X)) == X check
// holds because RC4 is symmetric (the same keystream XORed
// twice cancels out — Apply is involutive at the data level).
static constexpr std::byte kGateTestKey[16] = {
    std::byte{0x00}, std::byte{0x01}, std::byte{0x02}, std::byte{0x03},
    std::byte{0x04}, std::byte{0x05}, std::byte{0x06}, std::byte{0x07},
    std::byte{0x08}, std::byte{0x09}, std::byte{0x0A}, std::byte{0x0B},
    std::byte{0x0C}, std::byte{0x0D}, std::byte{0x0E}, std::byte{0x0F}
};

std::vector<std::byte> Encode(std::span<const std::byte> data) {
    return Apply(std::span<const std::byte>{kGateTestKey, 16}, data);
}

std::vector<std::byte> Decode(std::span<const std::byte> data) {
    return Apply(std::span<const std::byte>{kGateTestKey, 16}, data);
}

}  // namespace foundation::rc4
