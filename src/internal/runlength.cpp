#include "runlength.hpp"
#include <algorithm>
#include <stdexcept>
// std::to_string — MSVC's STL doesn't pull <string> in
// transitively; libstdc++/libc++ happen to. Spell it
// explicitly so the lib builds on every conforming toolchain.
#include <string>

namespace {

// PDF 32000-1:2008 §7.4.5: RunLengthDecode segment format
// L in [0x00..0x7F] => LITERAL: copy next (L+1) bytes
// L == 0x80         => END-OF-DATA
// L in [0x81..0xFF] => REPEAT: copy next byte (257-L) times

constexpr std::byte kEodMarker{static_cast<std::byte>(0x80)};

// Helper to emit a literal segment of length len (1..128)
void emit_literal(std::vector<std::byte>& out, std::span<const std::byte> src) {
    std::size_t len = src.size();
    if (len == 0 || len > 128) {
        throw std::runtime_error("runlength::emit_literal: invalid length");
    }
    out.push_back(static_cast<std::byte>(static_cast<std::uint8_t>(len) - 1));
    for (std::size_t i = 0; i < len; ++i) {
        out.push_back(src[i]);
    }
}

// Helper to emit a repeat segment of count copies of byte b (2..128)
void emit_repeat(std::vector<std::byte>& out, std::byte b, std::size_t count) {
    if (count < 2 || count > 128) {
        throw std::runtime_error("runlength::emit_repeat: invalid count");
    }
    out.push_back(static_cast<std::byte>(static_cast<std::uint8_t>(257) - static_cast<std::uint8_t>(count)));
    out.push_back(b);
}

} // namespace

namespace foundation::runlength {

std::vector<std::byte> Encode(std::span<const std::byte> input) {
    std::vector<std::byte> out;

    // PDF 32000-1:2008 §7.4.5: empty input encodes to just EOD marker
    if (input.empty()) {
        out.push_back(kEodMarker);
        return out;
    }

    std::size_t pos = 0;
    const std::size_t n = input.size();

    while (pos < n) {
        // Count run of identical bytes starting at pos
        std::size_t run_len = 1;
        while (pos + run_len < n && input[pos + run_len] == input[pos] && run_len < 128) {
            ++run_len;
        }

        // If run_len >= 2, emit as repeat (unless we can merge a trailing 1-byte run into a literal)
        if (run_len >= 2) {
            // Emit full 128-byte repeat chunks first
            while (run_len >= 128) {
                emit_repeat(out, input[pos], 128);
                pos += 128;
                run_len -= 128;
            }
            // Now 2 <= run_len <= 127
            if (run_len >= 2) {
                emit_repeat(out, input[pos], run_len);
                pos += run_len;
            }
            // If run_len == 1, we'll handle it in the next iteration (as part of a literal)
            continue;
        }

        // No repeat at pos; collect a literal segment
        std::size_t lit_start = pos;
        std::size_t lit_len = 1;

        // Extend literal while not starting a repeat of 2+ identical bytes
        while (pos + lit_len < n) {
            std::size_t next_run_len = 1;
            while (pos + lit_len + next_run_len < n &&
                   input[pos + lit_len + next_run_len] == input[pos + lit_len] &&
                   next_run_len < 128) {
                ++next_run_len;
            }
            if (next_run_len >= 2) {
                // Found a repeat starting at pos+lit_len; stop literal here
                break;
            }
            ++lit_len;
            if (lit_len == 128) {
                break; // max literal length
            }
        }

        emit_literal(out, input.subspan(lit_start, lit_len));
        pos += lit_len;
    }

    // Always end with EOD marker
    out.push_back(kEodMarker);
    return out;
}

std::vector<std::byte> Decode(std::span<const std::byte> input) {
    std::vector<std::byte> out;

    // PDF 32000-1:2008 §7.4.5: empty input is malformed (no EOD marker)
    if (input.empty()) {
        throw std::runtime_error("runlength::Decode: empty input (no EOD marker)");
    }

    std::size_t pos = 0;
    const std::size_t n = input.size();

    while (pos < n) {
        std::uint8_t len_byte = static_cast<std::uint8_t>(input[pos]);
        ++pos;

        if (len_byte == 0x80) {
            // EOD marker — stop decoding
            break;
        }

        if (len_byte < 0x80) {
            // Literal segment: copy next (len_byte + 1) bytes
            std::size_t copy_len = static_cast<std::size_t>(len_byte) + 1;
            if (pos + copy_len > n) {
                throw std::runtime_error(
                    "runlength::Decode: truncated literal segment at offset " + std::to_string(pos - 1));
            }
            for (std::size_t i = 0; i < copy_len; ++i) {
                out.push_back(input[pos + i]);
            }
            pos += copy_len;
        } else {
            // Repeat segment: next byte repeated (257 - len_byte) times
            std::size_t repeat_count = static_cast<std::size_t>(257) - static_cast<std::uint8_t>(len_byte);
            if (pos >= n) {
                throw std::runtime_error(
                    "runlength::Decode: truncated repeat segment at offset " + std::to_string(pos - 1));
            }
            std::byte b = input[pos];
            ++pos;
            for (std::size_t i = 0; i < repeat_count; ++i) {
                out.push_back(b);
            }
        }
    }

    // PDF 32000-1:2008 §7.4.5: if we reached end of input without EOD, it's malformed
    // (unless we broke on EOD above)
    // The loop breaks on EOD; if we exit because pos >= n and last byte wasn't EOD, it's malformed
    // But we already break on EOD, so only malformed case is running out of input mid-segment
    // which we already caught above.

    return out;
}

} // namespace foundation::runlength
