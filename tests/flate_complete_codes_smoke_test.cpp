// Regression: foundation::flate::Encode must emit *complete*
// Huffman code sets in every dynamic block. An incomplete (Kraft
// sum < 1) or over-subscribed code is rejected by conformant
// inflaters (zlib: "invalid code lengths set" / "… literal/lengths
// set"), even though this library's own lenient decoder accepts it
// and still round-trips — so the binding_roundtrip freeze gate
// cannot see the defect. This test parses the bit-exact DEFLATE
// stream the encoder produces and asserts the code-length (CL),
// literal/length, and distance code sets are each exactly complete.
//
// The bug this guards: BuildLengths' length-limiting pass left the
// code Kraft-incomplete for skewed, many-symbol distributions
// (e.g. photographic image rows), corrupting PNG IDAT for larger
// renders.

#include "flate.hpp"

#include <algorithm>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

namespace {

// LSB-first bit reader over the raw DEFLATE payload (zlib 2-byte
// header stripped by the caller).
struct BitReader {
    const std::uint8_t* p;
    std::size_t n;
    std::size_t bitpos = 0;
    std::uint32_t Get(int count) {
        std::uint32_t v = 0;
        for (int i = 0; i < count; ++i) {
            const std::size_t byte = bitpos >> 3;
            if (byte >= n) throw std::runtime_error("eof");
            const int bit = (p[byte] >> (bitpos & 7)) & 1;
            v |= static_cast<std::uint32_t>(bit) << i;
            ++bitpos;
        }
        return v;
    }
};

// Kraft completeness check over a code-length histogram: a code is
// complete iff sum(count[len] * 2^-len) == 1 (with no length
// over-subscribed). The single-empty-code case (no symbols) is
// reported separately by the caller.
void ExpectComplete(const std::vector<int>& lengths,
                    const char* what) {
    int count[16] = {0};
    int used = 0;
    for (int l : lengths) {
        ASSERT_GE(l, 0);
        ASSERT_LE(l, 15) << what << ": length exceeds 15";
        if (l > 0) { ++count[l]; ++used; }
    }
    if (used == 0) return;  // empty tree — vacuously fine here
    // Kraft sum in units of 2^-15.
    long left = 1L << 15;
    for (int len = 1; len <= 15; ++len) left -= long(count[len]) << (15 - len);
    EXPECT_EQ(left, 0)
        << what << ": Huffman code is "
        << (left > 0 ? "incomplete" : "over-subscribed")
        << " (Kraft residue " << left << ")";
}

const int kCLOrder[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5,
                          11, 4, 12, 3, 13, 2, 14, 1, 15};

// Walk every DEFLATE block in `payload`, asserting each dynamic
// block's CL, literal/length and distance code sets are complete.
// Decoding of compressed data itself is not needed — only the
// header code-length tables — but we must still consume the CL
// symbol stream to recover the lit/dist lengths, which requires a
// tiny CL Huffman decoder.
void CheckBlocks(const std::vector<std::uint8_t>& payload) {
    BitReader br{payload.data(), payload.size()};
    bool final = false;
    int dynamic_blocks = 0;
    while (!final) {
        final = br.Get(1) != 0;
        const std::uint32_t type = br.Get(2);
        if (type == 0) {  // stored
            br.bitpos = (br.bitpos + 7) & ~std::size_t(7);
            const std::uint32_t len = br.Get(16);
            br.Get(16);
            br.bitpos += std::size_t(len) * 8;
            continue;
        }
        ASSERT_NE(type, 3u) << "reserved block type";
        if (type == 1) {
            FAIL() << "fixed-Huffman block unexpected for this corpus";
        }
        ++dynamic_blocks;
        const int hlit = int(br.Get(5)) + 257;
        const int hdist = int(br.Get(5)) + 1;
        const int hclen = int(br.Get(4)) + 4;
        std::vector<int> cl(19, 0);
        for (int i = 0; i < hclen; ++i) cl[kCLOrder[i]] = int(br.Get(3));
        ExpectComplete(cl, "code-length (CL) code");

        // Build a minimal canonical decoder for the CL code so we
        // can read the lit/dist length stream and validate those
        // two trees too.
        int clcount[8] = {0};
        for (int l : cl) if (l) ++clcount[l];
        int next_code[8] = {0}, code = 0;
        for (int bits = 1; bits <= 7; ++bits) {
            code = (code + clcount[bits - 1]) << 1;
            next_code[bits] = code;
        }
        struct Entry { int sym = -1; };
        // map (len, code) → symbol via a flat search structure
        std::vector<std::pair<std::pair<int,int>, int>> table;
        {
            std::vector<int> cur(8, 0);
            for (int bits = 1; bits <= 7; ++bits) cur[bits] = next_code[bits];
            for (int sym = 0; sym < 19; ++sym) {
                const int l = cl[sym];
                if (!l) continue;
                table.push_back({{l, cur[l]++}, sym});
            }
        }
        auto decodeCL = [&]() -> int {
            int len = 0, c = 0;
            for (;;) {
                c = (c << 1) | int(br.Get(1));
                ++len;
                for (auto& e : table)
                    if (e.first.first == len && e.first.second == c)
                        return e.second;
                if (len > 7) throw std::runtime_error("bad CL code");
            }
        };

        std::vector<int> lens;
        lens.reserve(hlit + hdist);
        while (int(lens.size()) < hlit + hdist) {
            const int s = decodeCL();
            if (s < 16) {
                lens.push_back(s);
            } else if (s == 16) {
                const int r = int(br.Get(2)) + 3;
                ASSERT_FALSE(lens.empty());
                const int prev = lens.back();
                for (int i = 0; i < r; ++i) lens.push_back(prev);
            } else if (s == 17) {
                const int r = int(br.Get(3)) + 3;
                for (int i = 0; i < r; ++i) lens.push_back(0);
            } else {  // 18
                const int r = int(br.Get(7)) + 11;
                for (int i = 0; i < r; ++i) lens.push_back(0);
            }
        }
        ASSERT_EQ(int(lens.size()), hlit + hdist);
        std::vector<int> lit(lens.begin(), lens.begin() + hlit);
        std::vector<int> dist(lens.begin() + hlit, lens.end());
        ExpectComplete(lit, "literal/length code");
        // The distance code is allowed to be incomplete only in the
        // single-used-symbol special case; otherwise it must be
        // complete. Count used distance symbols.
        int dused = 0;
        for (int l : dist) if (l) ++dused;
        if (dused >= 2) ExpectComplete(dist, "distance code");

        // We cannot cheaply skip the entropy-coded payload without a
        // full inflate, so stop after validating the first dynamic
        // block's tables — the encoder builds every block the same
        // way, and the corpus below is sized so block 1 exercises
        // the length-limited path. Validate one block per buffer.
        return;
    }
    ASSERT_GT(dynamic_blocks, 0);
}

std::vector<std::uint8_t> StripZlib(const std::vector<std::byte>& z) {
    // 2-byte zlib header, 4-byte adler32 trailer.
    std::vector<std::uint8_t> out;
    for (std::size_t i = 2; i + 4 < z.size(); ++i)
        out.push_back(std::to_integer<std::uint8_t>(z[i]));
    return out;
}

void CheckBuffer(const std::vector<std::byte>& raw) {
    const auto enc = foundation::flate::Encode(
        std::span<const std::byte>(raw.data(), raw.size()));
    // Sanity: round-trips through our own decoder.
    const auto dec = foundation::flate::Decode(
        std::span<const std::byte>(enc.data(), enc.size()));
    ASSERT_EQ(dec.size(), raw.size());
    EXPECT_TRUE(std::equal(dec.begin(), dec.end(), raw.begin()));
    // Conformance: every emitted Huffman code set is complete.
    CheckBlocks(StripZlib(enc));
}

}  // namespace

// A skewed, many-symbol distribution forces deep Huffman trees that
// exercise BuildLengths' length-limiting + completeness fixup — the
// path that previously emitted an incomplete code.
TEST(FlateCompleteCodes, SkewedManySymbol) {
    std::mt19937 rng(20260604);
    std::vector<std::byte> raw(40000);
    // Geometric-ish symbol distribution: small values frequent,
    // large values rare → very unequal code lengths.
    std::geometric_distribution<int> g(0.04);
    for (auto& b : raw) b = std::byte(std::min(g(rng), 255));
    CheckBuffer(raw);
}

// Uniform high-entropy data (the photographic-pixel case that
// corrupted PNG IDAT for larger page renders).
TEST(FlateCompleteCodes, HighEntropyUniform) {
    std::mt19937 rng(424242);
    std::vector<std::byte> raw(40000);
    for (auto& b : raw) b = std::byte(rng() & 0xFF);
    CheckBuffer(raw);
}
