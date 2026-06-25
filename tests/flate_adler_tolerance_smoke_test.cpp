// =============================================================================
// flate_adler_tolerance_smoke_test — a fully-decoded DEFLATE stream whose
// RFC-1950 Adler-32 trailer is wrong (or whose declared length leaves trailing
// bytes) must still yield its decompressed bytes, not be rejected (task_06 B3).
// Real PDFs declare a stream /Length a few bytes longer than the zlib data
// (e.g. 30066.pdf, /Length 1924 over a 1922-byte stream), which made the strict
// trailer check throw "flate: Adler-32 mismatch" and the page render blank.
// poppler / zlib-based readers keep the output; so do we.
// =============================================================================

#include "flate.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace {

std::vector<std::byte> Bytes(const std::string& s) {
    std::vector<std::byte> v(s.size());
    for (std::size_t i = 0; i < s.size(); ++i)
        v[i] = static_cast<std::byte>(s[i]);
    return v;
}

// A valid stream whose Adler-32 trailer is corrupted still decodes.
TEST(FlateAdlerToleranceSmoke, CorruptTrailerStillDecodes) {
    const auto original = Bytes("The quick brown fox jumps over the lazy dog. ");
    // Build a few KiB so the DEFLATE stream is non-trivial.
    std::vector<std::byte> payload;
    for (int i = 0; i < 200; ++i)
        payload.insert(payload.end(), original.begin(), original.end());

    auto encoded = foundation::flate::Encode(
        std::span<const std::byte>(payload.data(), payload.size()));
    ASSERT_GE(encoded.size(), 8u);
    // Corrupt the last Adler-32 byte.
    encoded.back() = static_cast<std::byte>(
        static_cast<std::uint8_t>(encoded.back()) ^ 0xFF);

    const auto decoded = foundation::flate::Decode(
        std::span<const std::byte>(encoded.data(), encoded.size()));
    EXPECT_EQ(decoded, payload);
}

// Trailing bytes after a complete stream do not discard the output.
TEST(FlateAdlerToleranceSmoke, TrailingBytesIgnored) {
    std::vector<std::byte> payload;
    const auto chunk = Bytes("payload-data-payload-data-payload-data\n");
    for (int i = 0; i < 100; ++i)
        payload.insert(payload.end(), chunk.begin(), chunk.end());

    auto encoded = foundation::flate::Encode(
        std::span<const std::byte>(payload.data(), payload.size()));
    encoded.push_back(std::byte{0x0A});  // a stray trailing byte
    encoded.push_back(std::byte{0x00});

    const auto decoded = foundation::flate::Decode(
        std::span<const std::byte>(encoded.data(), encoded.size()));
    EXPECT_EQ(decoded, payload);
}

}  // namespace
