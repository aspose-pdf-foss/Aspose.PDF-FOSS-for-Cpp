#pragma once

// =============================================================================
// foundation::bigint — minimal arbitrary-precision unsigned integer.
//
// Hand-authored foundation support for RSA modular exponentiation (the
// digital-signature parity beat). Not an auto-generated primitive: it has
// no external standard to pin and exists solely to back foundation::pkcs7's
// PKCS#1 v1.5 signer. Base 2^32 limbs, little-endian, normalised (no
// trailing zero limbs; zero == empty limb vector). Division is Knuth
// Algorithm D (TAOCP Vol. 2 §4.3.1).
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace foundation::bigint {

class BigUint {
public:
    BigUint() = default;
    explicit BigUint(std::uint32_t v);

    // Big-endian byte interpretation (most-significant byte first).
    static BigUint FromBytesBE(std::span<const std::byte> bytes);
    // Fixed-width big-endian serialisation, left zero-padded to `len`.
    // Throws std::runtime_error when the value does not fit in `len`.
    std::vector<std::byte> ToBytesBE(std::size_t len) const;

    bool IsZero() const noexcept { return limbs_.empty(); }
    std::size_t BitLength() const noexcept;

    // Modular exponentiation: (*this) ^ exp mod m. Requires m != 0.
    BigUint ModPow(const BigUint& exp, const BigUint& m) const;

    friend int Compare(const BigUint& a, const BigUint& b);
    friend BigUint Add(const BigUint& a, const BigUint& b);
    friend BigUint Sub(const BigUint& a, const BigUint& b);  // a >= b
    friend BigUint Mul(const BigUint& a, const BigUint& b);
    friend void DivMod(const BigUint& a, const BigUint& b,
                       BigUint& q, BigUint& r);  // b != 0

private:
    void Normalize();
    std::vector<std::uint32_t> limbs_;  // little-endian, base 2^32
};

}  // namespace foundation::bigint
