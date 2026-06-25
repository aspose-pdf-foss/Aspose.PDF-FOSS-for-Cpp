#include "asciihex.hpp"
#include <algorithm>
#include <cctype>

namespace {

// PDF 32000-1:2008 §7.2.3: whitespace characters for ASCIIHexDecode
bool is_whitespace(std::byte b) noexcept {
    const unsigned char c = static_cast<unsigned char>(b);
    return c == 0x00 || c == 0x09 || c == 0x0A || c == 0x0D || c == 0x0C || c == 0x20;
}

// Convert a hex character to its numeric value (0..15)
// Returns -1 if not a valid hex digit
int hex_digit_value(unsigned char c) noexcept {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// Uppercase hex digit string per PDF spec
constexpr char HEX_DIGITS[] = "0123456789ABCDEF";

} // anonymous namespace

namespace foundation::asciihex {

std::vector<std::byte> Encode(std::span<const std::byte> input) {
    // PDF 32000-1:2008 §7.4.2: each byte becomes two uppercase hex digits
    std::vector<std::byte> output;
    output.reserve(2 * input.size());

    for (const auto b : input) {
        const unsigned char byte_val = static_cast<unsigned char>(b);
        output.emplace_back(static_cast<std::byte>(HEX_DIGITS[(byte_val >> 4) & 0x0F]));
        output.emplace_back(static_cast<std::byte>(HEX_DIGITS[byte_val & 0x0F]));
    }

    return output;
}

std::vector<std::byte> Decode(std::span<const std::byte> input) {
    // PDF 32000-1:2008 §7.4.2: decode ASCII hex, skip whitespace, handle odd-length
    std::vector<std::byte> output;
    output.reserve(input.size() / 2); // rough estimate

    bool have_high_nibble = false;
    unsigned char high_nibble = 0;

    for (const auto b : input) {
        const unsigned char c = static_cast<unsigned char>(b);

        // PDF 32000-1:2008 §7.4.2: stop at '>' EOD marker
        if (c == '>') {
            // If we have a pending high nibble, pad with '0' and emit.
            // Clear the flag so the post-loop end-of-input guard does
            // not re-flush the same nibble.
            if (have_high_nibble) {
                output.emplace_back(static_cast<std::byte>((high_nibble << 4) | 0x00));
                have_high_nibble = false;
            }
            break;
        }

        // PDF 32000-1:2008 §7.4.2: skip whitespace
        if (is_whitespace(b)) {
            continue;
        }

        // PDF 32000-1:2008 §7.4.2: accept hex digits (case-insensitive)
        const int digit_val = hex_digit_value(c);
        if (digit_val < 0) {
            // PDF 32000-1:2008 §7.4.2: non-hex, non-whitespace, non-'>' is malformed
            throw std::runtime_error(
                "ASCIIHexDecode: invalid character '" + std::string(1, static_cast<char>(c)) +
                "' (0x" + std::string(2, HEX_DIGITS[(c >> 4) & 0x0F]) +
                HEX_DIGITS[c & 0x0F] + ")");
        }

        // PDF 32000-1:2008 §7.4.2: odd-length input pads with '0'
        if (!have_high_nibble) {
            high_nibble = static_cast<unsigned char>(digit_val);
            have_high_nibble = true;
        } else {
            output.emplace_back(static_cast<std::byte>((high_nibble << 4) | digit_val));
            have_high_nibble = false;
        }
    }

    // If we have a pending high nibble at end-of-input (no '>'), pad with '0'
    if (have_high_nibble) {
        output.emplace_back(static_cast<std::byte>((high_nibble << 4) | 0x00));
    }

    return output;
}

} // namespace foundation::asciihex
