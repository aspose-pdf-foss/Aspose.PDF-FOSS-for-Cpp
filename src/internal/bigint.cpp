#include "bigint.hpp"

#include <algorithm>
#include <stdexcept>

namespace foundation::bigint {

namespace {
constexpr std::uint64_t kBase = 1ULL << 32;
}  // namespace

BigUint::BigUint(std::uint32_t v) {
    if (v != 0) limbs_.push_back(v);
}

void BigUint::Normalize() {
    while (!limbs_.empty() && limbs_.back() == 0) limbs_.pop_back();
}

BigUint BigUint::FromBytesBE(std::span<const std::byte> bytes) {
    // Skip leading zero bytes, then pack 4 bytes per limb from the LSB end.
    std::size_t start = 0;
    while (start < bytes.size() &&
           static_cast<std::uint8_t>(bytes[start]) == 0) {
        ++start;
    }
    BigUint out;
    const std::size_t n = bytes.size() - start;
    out.limbs_.resize((n + 3) / 4, 0);
    for (std::size_t i = 0; i < n; ++i) {
        const std::uint8_t byte =
            static_cast<std::uint8_t>(bytes[bytes.size() - 1 - i]);
        out.limbs_[i / 4] |= static_cast<std::uint32_t>(byte)
                             << (8 * (i % 4));
    }
    out.Normalize();
    return out;
}

std::vector<std::byte> BigUint::ToBytesBE(std::size_t len) const {
    std::vector<std::byte> out(len, std::byte{0});
    for (std::size_t i = 0; i < limbs_.size(); ++i) {
        std::uint32_t limb = limbs_[i];
        for (int b = 0; b < 4; ++b) {
            const std::size_t byte_index = i * 4 + static_cast<std::size_t>(b);
            const std::uint8_t v = static_cast<std::uint8_t>(limb & 0xFF);
            limb >>= 8;
            if (v == 0 && byte_index >= len) continue;
            if (byte_index >= len) {
                throw std::runtime_error(
                    "foundation::bigint: value does not fit in requested width");
            }
            out[len - 1 - byte_index] = static_cast<std::byte>(v);
        }
    }
    return out;
}

std::size_t BigUint::BitLength() const noexcept {
    if (limbs_.empty()) return 0;
    std::size_t bits = (limbs_.size() - 1) * 32;
    std::uint32_t top = limbs_.back();
    while (top != 0) {
        ++bits;
        top >>= 1;
    }
    return bits;
}

int Compare(const BigUint& a, const BigUint& b) {
    if (a.limbs_.size() != b.limbs_.size()) {
        return a.limbs_.size() < b.limbs_.size() ? -1 : 1;
    }
    for (std::size_t i = a.limbs_.size(); i-- > 0;) {
        if (a.limbs_[i] != b.limbs_[i]) {
            return a.limbs_[i] < b.limbs_[i] ? -1 : 1;
        }
    }
    return 0;
}

BigUint Add(const BigUint& a, const BigUint& b) {
    BigUint out;
    const std::size_t n = std::max(a.limbs_.size(), b.limbs_.size());
    out.limbs_.resize(n, 0);
    std::uint64_t carry = 0;
    for (std::size_t i = 0; i < n; ++i) {
        std::uint64_t sum = carry;
        if (i < a.limbs_.size()) sum += a.limbs_[i];
        if (i < b.limbs_.size()) sum += b.limbs_[i];
        out.limbs_[i] = static_cast<std::uint32_t>(sum & 0xFFFFFFFF);
        carry = sum >> 32;
    }
    if (carry) out.limbs_.push_back(static_cast<std::uint32_t>(carry));
    out.Normalize();
    return out;
}

BigUint Sub(const BigUint& a, const BigUint& b) {
    // Precondition: a >= b.
    BigUint out;
    out.limbs_.resize(a.limbs_.size(), 0);
    std::int64_t borrow = 0;
    for (std::size_t i = 0; i < a.limbs_.size(); ++i) {
        std::int64_t diff = static_cast<std::int64_t>(a.limbs_[i]) - borrow -
            (i < b.limbs_.size()
                 ? static_cast<std::int64_t>(b.limbs_[i])
                 : 0);
        if (diff < 0) {
            diff += static_cast<std::int64_t>(kBase);
            borrow = 1;
        } else {
            borrow = 0;
        }
        out.limbs_[i] = static_cast<std::uint32_t>(diff);
    }
    out.Normalize();
    return out;
}

BigUint Mul(const BigUint& a, const BigUint& b) {
    BigUint out;
    if (a.limbs_.empty() || b.limbs_.empty()) return out;
    out.limbs_.assign(a.limbs_.size() + b.limbs_.size(), 0);
    for (std::size_t i = 0; i < a.limbs_.size(); ++i) {
        std::uint64_t carry = 0;
        const std::uint64_t ai = a.limbs_[i];
        for (std::size_t j = 0; j < b.limbs_.size(); ++j) {
            std::uint64_t cur = out.limbs_[i + j] + ai * b.limbs_[j] + carry;
            out.limbs_[i + j] = static_cast<std::uint32_t>(cur & 0xFFFFFFFF);
            carry = cur >> 32;
        }
        out.limbs_[i + b.limbs_.size()] += static_cast<std::uint32_t>(carry);
    }
    out.Normalize();
    return out;
}

// Knuth Algorithm D (TAOCP Vol. 2 §4.3.1) — long division base 2^32.
void DivMod(const BigUint& a, const BigUint& b, BigUint& q, BigUint& r) {
    if (b.limbs_.empty()) {
        throw std::runtime_error("foundation::bigint: division by zero");
    }
    if (Compare(a, b) < 0) {
        q = BigUint();
        r = a;
        return;
    }
    if (b.limbs_.size() == 1) {
        // Fast path: single-limb divisor.
        const std::uint64_t d = b.limbs_[0];
        BigUint quo;
        quo.limbs_.resize(a.limbs_.size(), 0);
        std::uint64_t rem = 0;
        for (std::size_t i = a.limbs_.size(); i-- > 0;) {
            const std::uint64_t cur = (rem << 32) | a.limbs_[i];
            quo.limbs_[i] = static_cast<std::uint32_t>(cur / d);
            rem = cur % d;
        }
        quo.Normalize();
        q = std::move(quo);
        r = BigUint(static_cast<std::uint32_t>(rem));
        return;
    }

    // Normalise so the divisor's top limb has its high bit set.
    int shift = 0;
    std::uint32_t top = b.limbs_.back();
    while ((top & 0x80000000u) == 0) {
        top <<= 1;
        ++shift;
    }

    const auto shl = [](const BigUint& x, int s) {
        if (s == 0) return x;
        BigUint out;
        out.limbs_.resize(x.limbs_.size() + 1, 0);
        std::uint32_t carry = 0;
        for (std::size_t i = 0; i < x.limbs_.size(); ++i) {
            const std::uint64_t cur =
                (static_cast<std::uint64_t>(x.limbs_[i]) << s) | carry;
            out.limbs_[i] = static_cast<std::uint32_t>(cur & 0xFFFFFFFF);
            carry = static_cast<std::uint32_t>(cur >> 32);
        }
        out.limbs_.back() = carry;
        while (!out.limbs_.empty() && out.limbs_.back() == 0)
            out.limbs_.pop_back();
        return out;
    };

    BigUint u = shl(a, shift);
    BigUint v = shl(b, shift);
    const std::size_t n = v.limbs_.size();
    const std::size_t m = u.limbs_.size() >= n ? u.limbs_.size() - n : 0;

    // Ensure u has exactly m + n + 1 limbs (with a leading zero limb).
    u.limbs_.resize(u.limbs_.size() + 1 < m + n + 1 ? m + n + 1
                                                    : u.limbs_.size() + 1,
                    0);

    BigUint quo;
    quo.limbs_.assign(m + 1, 0);

    const std::uint64_t vtop = v.limbs_[n - 1];
    const std::uint64_t vsec = v.limbs_[n - 2];

    for (std::size_t j = m + 1; j-- > 0;) {
        // Estimate q̂.
        const std::uint64_t numhi =
            (static_cast<std::uint64_t>(u.limbs_[j + n]) << 32) |
            u.limbs_[j + n - 1];
        std::uint64_t qhat = numhi / vtop;
        std::uint64_t rhat = numhi % vtop;
        while (qhat >= kBase ||
               qhat * vsec > ((rhat << 32) | u.limbs_[j + n - 2])) {
            --qhat;
            rhat += vtop;
            if (rhat >= kBase) break;
        }

        // Multiply and subtract q̂ * v from u[j .. j+n].
        std::int64_t borrow = 0;
        std::uint64_t carry = 0;
        for (std::size_t i = 0; i < n; ++i) {
            const std::uint64_t p = qhat * v.limbs_[i] + carry;
            carry = p >> 32;
            const std::int64_t sub =
                static_cast<std::int64_t>(u.limbs_[j + i]) - borrow -
                static_cast<std::int64_t>(p & 0xFFFFFFFF);
            if (sub < 0) {
                u.limbs_[j + i] =
                    static_cast<std::uint32_t>(sub + static_cast<std::int64_t>(kBase));
                borrow = 1;
            } else {
                u.limbs_[j + i] = static_cast<std::uint32_t>(sub);
                borrow = 0;
            }
        }
        const std::int64_t sub_top =
            static_cast<std::int64_t>(u.limbs_[j + n]) - borrow -
            static_cast<std::int64_t>(carry);
        if (sub_top < 0) {
            // q̂ was one too large: add back v.
            u.limbs_[j + n] =
                static_cast<std::uint32_t>(sub_top + static_cast<std::int64_t>(kBase));
            --qhat;
            std::uint64_t addc = 0;
            for (std::size_t i = 0; i < n; ++i) {
                const std::uint64_t s =
                    static_cast<std::uint64_t>(u.limbs_[j + i]) +
                    v.limbs_[i] + addc;
                u.limbs_[j + i] = static_cast<std::uint32_t>(s & 0xFFFFFFFF);
                addc = s >> 32;
            }
            u.limbs_[j + n] += static_cast<std::uint32_t>(addc);
        } else {
            u.limbs_[j + n] = static_cast<std::uint32_t>(sub_top);
        }
        quo.limbs_[j] = static_cast<std::uint32_t>(qhat);
    }

    quo.Normalize();
    q = std::move(quo);

    // Remainder = u[0 .. n) >> shift.
    u.limbs_.resize(n);
    if (shift != 0) {
        BigUint rem;
        rem.limbs_.resize(n, 0);
        std::uint32_t carry = 0;
        for (std::size_t i = n; i-- > 0;) {
            const std::uint32_t cur = u.limbs_[i];
            rem.limbs_[i] = (cur >> shift) | carry;
            carry = static_cast<std::uint32_t>(
                static_cast<std::uint64_t>(cur) << (32 - shift));
        }
        rem.Normalize();
        r = std::move(rem);
    } else {
        u.Normalize();
        r = std::move(u);
    }
}

BigUint BigUint::ModPow(const BigUint& exp, const BigUint& m) const {
    if (m.limbs_.empty()) {
        throw std::runtime_error("foundation::bigint: modulus is zero");
    }
    BigUint result(1);
    BigUint q, base;
    DivMod(*this, m, q, base);  // base = (*this) mod m
    const std::size_t bits = exp.BitLength();
    for (std::size_t i = 0; i < bits; ++i) {
        const std::uint32_t limb = exp.limbs_[i / 32];
        if ((limb >> (i % 32)) & 1u) {
            BigUint prod = Mul(result, base);
            DivMod(prod, m, q, result);
        }
        BigUint sq = Mul(base, base);
        DivMod(sq, m, q, base);
    }
    return result;
}

}  // namespace foundation::bigint
