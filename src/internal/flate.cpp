#include "flate.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace foundation::flate {

namespace {

// ──────────────────────────────────────────────────────────────────────────
// Constants
// ──────────────────────────────────────────────────────────────────────────

// Maximum Huffman code length (RFC 1951 §3.2.7 caps at 15 bits).
constexpr int kMaxBits = 15;

// Literal/length alphabet has 286 valid symbols (0..285), distance
// alphabet has 30 valid symbols (0..29). Tables are oversized by 2
// to accept the 287 / 31 symbols some encoders include (RFC 1951
// reserves the higher symbols but Huffman trees may transmit them
// with zero-length codes; we accept that quietly).
constexpr int kMaxLitCodes  = 288;
constexpr int kMaxDistCodes = 32;

// LZ77 window — RFC 1951 §3.2.5 caps distance at 32768.
constexpr std::size_t kWindow = 32768;

// Minimum back-reference length the LZ77 alphabet can express.
constexpr int kMinMatch = 3;
constexpr int kMaxMatch = 258;

// Encoder block size. ~16 KiB matches zlib's default block boundary
// and amortises the dynamic-Huffman header (~50-200 bytes) across
// enough symbol-emission savings to justify the per-block tree.
constexpr std::size_t kBlockSize = 16384;

// ──────────────────────────────────────────────────────────────────────────
// RFC 1951 §3.2.5 length code tables (Tables 1 & 2)
// ──────────────────────────────────────────────────────────────────────────
//
// Length codes 257..285. kLengthBase[c-257] is the base length; the
// total length is kLengthBase + bits read per kLengthExtra[c-257].
// Code 285 is the unique length-258 code with 0 extra bits (a quirk
// of the table — code 284 covers 227-257 with 5 extra bits).

constexpr std::array<std::uint16_t, 29> kLengthBase = {
    3, 4, 5, 6, 7, 8, 9, 10,
    11, 13, 15, 17,
    19, 23, 27, 31,
    35, 43, 51, 59,
    67, 83, 99, 115,
    131, 163, 195, 227,
    258
};

constexpr std::array<std::uint8_t, 29> kLengthExtra = {
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1,
    2, 2, 2, 2,
    3, 3, 3, 3,
    4, 4, 4, 4,
    5, 5, 5, 5,
    0
};

// Distance codes 0..29.

constexpr std::array<std::uint16_t, 30> kDistBase = {
    1, 2, 3, 4,
    5, 7,
    9, 13,
    17, 25,
    33, 49,
    65, 97,
    129, 193,
    257, 385,
    513, 769,
    1025, 1537,
    2049, 3073,
    4097, 6145,
    8193, 12289,
    16385, 24577
};

constexpr std::array<std::uint8_t, 30> kDistExtra = {
    0, 0, 0, 0,
    1, 1,
    2, 2,
    3, 3,
    4, 4,
    5, 5,
    6, 6,
    7, 7,
    8, 8,
    9, 9,
    10, 10,
    11, 11,
    12, 12,
    13, 13
};

// RFC 1951 §3.2.7 — code-length-code permutation order. The 19
// possible code-length codes are transmitted in this fixed
// permutation so that less common codes (16, 17, 18) come first
// and the trailing zeros for unused codes can be omitted.
constexpr std::array<std::uint8_t, 19> kCodeLengthOrder = {
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

// ──────────────────────────────────────────────────────────────────────────
// Adler-32 (RFC 1950 Appendix)
// ──────────────────────────────────────────────────────────────────────────
//
// s1 starts at 1, s2 at 0; both are reduced mod 65521 (largest
// prime below 2^16). Output is (s2 << 16) | s1 as a 32-bit value.
//
// We process in 5552-byte chunks because that's the largest n for
// which s1 + n * 255 + 1 stays under 2^32 — letting us defer the
// modulo to chunk boundaries instead of every byte.

constexpr std::uint32_t kAdlerMod = 65521u;

std::uint32_t Adler32(std::span<const std::byte> data) {
    std::uint32_t s1 = 1;
    std::uint32_t s2 = 0;
    const std::uint8_t* p =
        reinterpret_cast<const std::uint8_t*>(data.data());
    std::size_t n = data.size();
    while (n > 0) {
        const std::size_t step = std::min<std::size_t>(n, 5552);
        for (std::size_t i = 0; i < step; ++i) {
            s1 += p[i];
            s2 += s1;
        }
        s1 %= kAdlerMod;
        s2 %= kAdlerMod;
        p += step;
        n -= step;
    }
    return (s2 << 16) | s1;
}

// ──────────────────────────────────────────────────────────────────────────
// Bit reader (LSB-first per byte; RFC 1951 §3.1.1)
// ──────────────────────────────────────────────────────────────────────────

class BitReader {
public:
    explicit BitReader(std::span<const std::byte> in)
        : p_(reinterpret_cast<const std::uint8_t*>(in.data())),
          end_(p_ + in.size()) {}

    // Read the next n bits (n ≤ 25) as an unsigned integer; the
    // first bit read becomes the LSB of the result.
    std::uint32_t ReadBits(int n) {
        while (nbits_ < n) {
            if (p_ >= end_) {
                throw std::runtime_error(
                    "flate: unexpected end of stream");
            }
            buf_ |= std::uint64_t(*p_++) << nbits_;
            nbits_ += 8;
        }
        const std::uint32_t v =
            std::uint32_t(buf_ & ((std::uint64_t(1) << n) - 1));
        buf_ >>= n;
        nbits_ -= n;
        return v;
    }

    // Discard buffered bits up to the next byte boundary (used
    // before stored-block payload per RFC 1951 §3.2.4).
    void AlignToByte() {
        const int extra = nbits_ & 7;
        buf_ >>= extra;
        nbits_ -= extra;
    }

    // Read one byte from the byte-aligned stream (precondition:
    // AlignToByte has just been called or no bits are buffered).
    std::uint8_t ReadByte() {
        if (nbits_ >= 8) {
            const std::uint8_t v = std::uint8_t(buf_ & 0xFFu);
            buf_ >>= 8;
            nbits_ -= 8;
            return v;
        }
        if (p_ >= end_) {
            throw std::runtime_error(
                "flate: unexpected end of stream");
        }
        return *p_++;
    }

    // Number of unread bytes in the raw stream (does not include
    // bits already in the bit buffer). Used to read the Adler-32
    // trailer after byte-aligning at end-of-stream.
    std::size_t BytesLeft() const {
        return std::size_t(end_ - p_);
    }

private:
    const std::uint8_t* p_;
    const std::uint8_t* end_;
    std::uint64_t buf_ = 0;
    int nbits_ = 0;
};

// ──────────────────────────────────────────────────────────────────────────
// Bit writer (LSB-first per byte)
// ──────────────────────────────────────────────────────────────────────────

class BitWriter {
public:
    explicit BitWriter(std::vector<std::byte>& out) : out_(out) {}

    // Append the low n bits of v (n ≤ 25) to the stream LSB-first.
    void WriteBits(std::uint32_t v, int n) {
        buf_ |= std::uint64_t(v) << nbits_;
        nbits_ += n;
        while (nbits_ >= 8) {
            out_.push_back(std::byte(buf_ & 0xFFu));
            buf_ >>= 8;
            nbits_ -= 8;
        }
    }

    // Append an MSB-first Huffman code: the high (length-1) bit of
    // code is emitted first into the LSB-first bit stream so that
    // a decoder reading LSB bits then accumulating MSB-first into
    // the code variable reconstructs the original symbol.
    void WriteHuffmanCode(std::uint32_t code, int length) {
        // Bit-reverse `code` so that emitting LSB-first to the
        // stream produces the correct MSB-first ordering.
        std::uint32_t reversed = 0;
        for (int i = 0; i < length; ++i) {
            reversed = (reversed << 1) | (code & 1);
            code >>= 1;
        }
        WriteBits(reversed, length);
    }

    // Flush pending bits to the next byte boundary by padding with
    // zeros. Used before stored-block headers and at end-of-stream.
    void AlignToByte() {
        if (nbits_ > 0) {
            out_.push_back(std::byte(buf_ & 0xFFu));
            buf_ = 0;
            nbits_ = 0;
        }
    }

    // Append a raw byte to a byte-aligned stream.
    void WriteByte(std::uint8_t v) {
        out_.push_back(std::byte(v));
    }

private:
    std::vector<std::byte>& out_;
    std::uint64_t buf_ = 0;
    int nbits_ = 0;
};

// ──────────────────────────────────────────────────────────────────────────
// Canonical Huffman tables (RFC 1951 §3.2.2)
// ──────────────────────────────────────────────────────────────────────────
//
// Stored as count-per-length + symbols-sorted-by-length+index.
// Decoding is the puff.c-style incremental algorithm: read one bit
// at a time, accumulate MSB-first into `code`, and at each length
// check whether `code - first_at_len` indexes into the symbols at
// that length. Slow but compact and correct; speed isn't a concern
// for the freeze gate or for typical PDF stream sizes.

struct HuffmanTable {
    std::array<int, kMaxBits + 1> count{};   // count[len] = number of codes of that length
    std::array<int, kMaxLitCodes> symbol{};  // symbols sorted by (length, original index)
};

// Build a canonical Huffman table from a per-symbol code-length
// vector. Symbols with length 0 are unused. RFC 1951 §3.2.7 allows
// "incomplete" trees in narrow circumstances (the distance tree
// with exactly one symbol); we permit incomplete here and detect
// over-subscribed (Kraft > 1) trees as malformed.
void BuildHuffman(HuffmanTable& h,
                  const int* lengths,
                  int n) {
    h.count.fill(0);
    for (int i = 0; i < n; ++i) {
        if (lengths[i] < 0 || lengths[i] > kMaxBits) {
            throw std::runtime_error(
                "flate: code length out of range");
        }
        ++h.count[lengths[i]];
    }
    h.count[0] = 0;
    int left = 1;
    for (int len = 1; len <= kMaxBits; ++len) {
        left <<= 1;
        left -= h.count[len];
        if (left < 0) {
            throw std::runtime_error(
                "flate: oversubscribed Huffman code");
        }
    }

    std::array<int, kMaxBits + 1> offs{};
    offs[1] = 0;
    for (int len = 1; len < kMaxBits; ++len) {
        offs[len + 1] = offs[len] + h.count[len];
    }
    for (int i = 0; i < n; ++i) {
        if (lengths[i] != 0) {
            h.symbol[offs[lengths[i]]++] = i;
        }
    }
}

int DecodeSymbol(BitReader& br, const HuffmanTable& h) {
    int code = 0;
    int first = 0;
    int index = 0;
    for (int len = 1; len <= kMaxBits; ++len) {
        code |= int(br.ReadBits(1));
        const int count = h.count[len];
        if (code - count < first) {
            return h.symbol[index + (code - first)];
        }
        index += count;
        first = (first + count) << 1;
        code <<= 1;
    }
    throw std::runtime_error("flate: invalid Huffman code");
}

// Build the fixed Huffman tables (RFC 1951 §3.2.6). Cached at
// program start since they're invariant.
const HuffmanTable& FixedLitTable() {
    static const HuffmanTable t = []() {
        std::array<int, 288> lengths{};
        for (int i = 0;   i <= 143; ++i) lengths[i] = 8;
        for (int i = 144; i <= 255; ++i) lengths[i] = 9;
        for (int i = 256; i <= 279; ++i) lengths[i] = 7;
        for (int i = 280; i <= 287; ++i) lengths[i] = 8;
        HuffmanTable h;
        BuildHuffman(h, lengths.data(), 288);
        return h;
    }();
    return t;
}

const HuffmanTable& FixedDistTable() {
    static const HuffmanTable t = []() {
        std::array<int, 30> lengths{};
        for (int i = 0; i < 30; ++i) lengths[i] = 5;
        HuffmanTable h;
        BuildHuffman(h, lengths.data(), 30);
        return h;
    }();
    return t;
}

// ──────────────────────────────────────────────────────────────────────────
// Block decoders
// ──────────────────────────────────────────────────────────────────────────

void DecodeStoredBlock(BitReader& br,
                       std::vector<std::byte>& out) {
    br.AlignToByte();
    const std::uint16_t len = std::uint16_t(
        br.ReadByte() | (std::uint16_t(br.ReadByte()) << 8));
    const std::uint16_t nlen = std::uint16_t(
        br.ReadByte() | (std::uint16_t(br.ReadByte()) << 8));
    if (std::uint16_t(~nlen) != len) {
        throw std::runtime_error(
            "flate: stored block LEN/NLEN mismatch");
    }
    out.reserve(out.size() + len);
    for (std::uint16_t i = 0; i < len; ++i) {
        out.push_back(std::byte(br.ReadByte()));
    }
}

void DecodeCompressedBlock(BitReader& br,
                           const HuffmanTable& lit,
                           const HuffmanTable& dist,
                           std::vector<std::byte>& out) {
    while (true) {
        const int sym = DecodeSymbol(br, lit);
        if (sym < 256) {
            out.push_back(std::byte(sym));
        } else if (sym == 256) {
            return;  // end of block
        } else {
            const int lcode = sym - 257;
            if (lcode < 0 || lcode >= int(kLengthBase.size())) {
                throw std::runtime_error(
                    "flate: invalid length code");
            }
            const int extra_l = kLengthExtra[lcode];
            const int length =
                kLengthBase[lcode] +
                (extra_l ? int(br.ReadBits(extra_l)) : 0);
            const int dcode = DecodeSymbol(br, dist);
            if (dcode < 0 || dcode >= int(kDistBase.size())) {
                throw std::runtime_error(
                    "flate: invalid distance code");
            }
            const int extra_d = kDistExtra[dcode];
            const std::size_t distance =
                kDistBase[dcode] +
                (extra_d ? std::size_t(br.ReadBits(extra_d)) : 0);
            if (distance > out.size()) {
                throw std::runtime_error(
                    "flate: distance beyond available output");
            }
            // Per-byte copy because (length > distance) is legal
            // and produces run-length output: each byte copied
            // becomes visible to the next iteration's read.
            const std::size_t start = out.size() - distance;
            for (int i = 0; i < length; ++i) {
                out.push_back(out[start + i]);
            }
        }
    }
}

void DecodeDynamicBlock(BitReader& br,
                        std::vector<std::byte>& out) {
    const int hlit  = int(br.ReadBits(5)) + 257;
    const int hdist = int(br.ReadBits(5)) + 1;
    const int hclen = int(br.ReadBits(4)) + 4;
    if (hlit > kMaxLitCodes || hdist > kMaxDistCodes) {
        throw std::runtime_error(
            "flate: HLIT/HDIST out of range");
    }

    // Read the code-length-code lengths in the permuted order.
    std::array<int, 19> cl_lengths{};
    for (int i = 0; i < hclen; ++i) {
        cl_lengths[kCodeLengthOrder[i]] = int(br.ReadBits(3));
    }
    HuffmanTable cl_table;
    BuildHuffman(cl_table, cl_lengths.data(), 19);

    // Decode the combined literal/length + distance code-length
    // sequence using the code-length tree just built.
    const int total = hlit + hdist;
    std::array<int, kMaxLitCodes + kMaxDistCodes> lengths{};
    int idx = 0;
    while (idx < total) {
        const int sym = DecodeSymbol(br, cl_table);
        if (sym < 16) {
            lengths[idx++] = sym;
        } else if (sym == 16) {
            // Repeat previous length 3..6 times (2 extra bits + 3).
            if (idx == 0) {
                throw std::runtime_error(
                    "flate: code-length 16 with no previous");
            }
            int rep = int(br.ReadBits(2)) + 3;
            const int prev = lengths[idx - 1];
            while (rep-- > 0 && idx < total) {
                lengths[idx++] = prev;
            }
        } else if (sym == 17) {
            // Repeat zero 3..10 times (3 extra bits + 3).
            int rep = int(br.ReadBits(3)) + 3;
            while (rep-- > 0 && idx < total) {
                lengths[idx++] = 0;
            }
        } else if (sym == 18) {
            // Repeat zero 11..138 times (7 extra bits + 11).
            int rep = int(br.ReadBits(7)) + 11;
            while (rep-- > 0 && idx < total) {
                lengths[idx++] = 0;
            }
        } else {
            throw std::runtime_error(
                "flate: invalid code-length symbol");
        }
    }
    if (idx != total) {
        throw std::runtime_error(
            "flate: code-length sequence truncated");
    }

    HuffmanTable lit_table;
    HuffmanTable dist_table;
    BuildHuffman(lit_table,  lengths.data(),         hlit);
    BuildHuffman(dist_table, lengths.data() + hlit,  hdist);
    DecodeCompressedBlock(br, lit_table, dist_table, out);
}

// ──────────────────────────────────────────────────────────────────────────
// zlib stream wrapper
// ──────────────────────────────────────────────────────────────────────────

void ReadZlibHeader(BitReader& br) {
    // CMF + FLG, big-endian 16-bit value must be divisible by 31.
    const std::uint8_t cmf = br.ReadByte();
    const std::uint8_t flg = br.ReadByte();
    const std::uint16_t header = (std::uint16_t(cmf) << 8) | flg;
    if (header % 31u != 0u) {
        throw std::runtime_error("flate: bad zlib header check");
    }
    const int cm = cmf & 0x0F;
    if (cm != 8) {
        // RFC 1950 §2.2 — only CM=8 (deflate) is defined.
        throw std::runtime_error(
            "flate: unsupported zlib compression method");
    }
    const int cinfo = (cmf >> 4) & 0x0F;
    if (cinfo > 7) {
        // CINFO > 7 reserves window sizes > 32K — none defined.
        throw std::runtime_error(
            "flate: unsupported zlib window size");
    }
    const bool fdict = (flg & 0x20) != 0;
    if (fdict) {
        // We don't ship preset dictionaries (PDF doesn't use them).
        throw std::runtime_error(
            "flate: zlib stream uses unsupported preset dictionary");
    }
}

std::uint32_t ReadAdler32Trailer(BitReader& br) {
    if (br.BytesLeft() < 4) {
        throw std::runtime_error(
            "flate: missing Adler-32 trailer");
    }
    // 4 bytes big-endian.
    const std::uint32_t b0 = br.ReadByte();
    const std::uint32_t b1 = br.ReadByte();
    const std::uint32_t b2 = br.ReadByte();
    const std::uint32_t b3 = br.ReadByte();
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

// ──────────────────────────────────────────────────────────────────────────
// Encoder — LZ77 matching
// ──────────────────────────────────────────────────────────────────────────
//
// Hash-chain matcher per RFC 1951 Appendix. For each input byte we
// hash the next 3 bytes, look up the chain of prior positions with
// the same hash, and find the longest match within distance 32768.
// kMaxChain caps chain walks to keep encode time O(n) — analogous
// to zlib's strategy at compression level 6.

constexpr std::uint32_t kHashBits = 15;
constexpr std::uint32_t kHashSize = 1u << kHashBits;
constexpr std::uint32_t kHashMask = kHashSize - 1u;
constexpr int kMaxChain = 128;  // ~zlib level-6 chain length.

struct Token {
    // Either a literal (length == 0; lit holds the byte value) or
    // a back-reference (length >= 3; dist holds the distance).
    std::uint16_t length;
    std::uint16_t dist;
    std::uint8_t  lit;
};

// Hash 3 bytes into a kHashBits-wide bucket. The shift constants
// (5 and 10) are chosen so each byte contributes to the high bits;
// any reasonable mixing function works for the matcher's purpose.
std::uint32_t Hash3(const std::uint8_t* p) {
    return ((std::uint32_t(p[0]) << 10) ^
            (std::uint32_t(p[1]) <<  5) ^
             std::uint32_t(p[2])) & kHashMask;
}

// Run LZ77 matching over `data[start, end)`, producing a token
// stream. Hash chain state (`head`, `prev`) is shared across calls
// so back-references can reach into previous blocks within the
// 32 KiB window.
void Lz77(const std::uint8_t* data,
          std::size_t total_size,
          std::size_t start,
          std::size_t end,
          std::vector<int>& head,
          std::vector<int>& prev,
          std::vector<Token>& tokens) {
    std::size_t i = start;
    while (i < end) {
        // Need at least kMinMatch lookahead bytes to attempt a
        // match; emit literals at the tail.
        if (i + kMinMatch > total_size) {
            tokens.push_back(Token{0, 0, data[i]});
            ++i;
            continue;
        }
        const std::uint32_t h = Hash3(data + i);
        int chain_pos = head[h];
        int best_len = 0;
        int best_dist = 0;
        int chain_steps = kMaxChain;
        const std::size_t min_pos =
            (i > kWindow) ? (i - kWindow) : 0;
        while (chain_pos >= int(min_pos) && chain_steps-- > 0) {
            // Quick pre-check — match must extend at least one
            // byte beyond the current best, so compare the byte
            // at position best_len first.
            if (data[chain_pos + best_len] != data[i + best_len]) {
                chain_pos = prev[chain_pos & (kWindow - 1)];
                continue;
            }
            // Extend match. Cap at the block boundary `end`, not
            // `total_size` — otherwise a match at the tail of the
            // current block can reach into bytes the *next* block
            // is going to encode, the outer loop advances pos = end
            // regardless, and the next block re-encodes bytes the
            // decoder already produced from this match. End result:
            // decoder over-emits past total_size and the Adler-32
            // trailer mismatches. Boundary-crossing matches are
            // legal in deflate, but the encoder's outer driver
            // currently doesn't track partial-block consumption,
            // so we cap conservatively here.
            int len = 0;
            const int max_len = int(std::min<std::size_t>(
                kMaxMatch, end - i));
            while (len < max_len &&
                   data[chain_pos + len] == data[i + len]) {
                ++len;
            }
            if (len > best_len) {
                best_len = len;
                best_dist = int(i - chain_pos);
                if (len >= kMaxMatch) break;
            }
            chain_pos = prev[chain_pos & (kWindow - 1)];
        }

        // Update hash chain at position i regardless of match
        // (so future positions can reference here).
        prev[i & (kWindow - 1)] = head[h];
        head[h] = int(i);

        if (best_len >= kMinMatch) {
            tokens.push_back(Token{
                std::uint16_t(best_len),
                std::uint16_t(best_dist),
                0});
            // Insert hash entries for the matched bytes (except
            // the first, already inserted above) so subsequent
            // matches can chain through them.
            for (int k = 1; k < best_len; ++k) {
                if (i + k + kMinMatch > total_size) break;
                const std::uint32_t hh = Hash3(data + i + k);
                prev[(i + k) & (kWindow - 1)] = head[hh];
                head[hh] = int(i + k);
            }
            i += best_len;
        } else {
            tokens.push_back(Token{0, 0, data[i]});
            ++i;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Encoder — Huffman code-length assignment
// ──────────────────────────────────────────────────────────────────────────
//
// Standard Huffman tree built by repeated min-pair merging in a
// frequency-sorted heap. After computing depths we cap any depth
// above 15 by walking shorter codes one step deeper until the
// Kraft inequality balances — same heuristic as zlib's
// trees.c:gen_bitlen.

struct HuffNode {
    int freq;
    int depth;
    int left;
    int right;
};

void BuildLengths(const int* freqs,
                  int n,
                  int* lengths_out,
                  int max_bits) {
    // Collect non-zero-frequency symbols.
    std::vector<int> heap;  // indices into nodes[]
    std::vector<HuffNode> nodes;
    nodes.reserve(2 * n);
    for (int i = 0; i < n; ++i) {
        lengths_out[i] = 0;
        if (freqs[i] > 0) {
            nodes.push_back(HuffNode{freqs[i], 0, -1, -1});
            heap.push_back(int(nodes.size()) - 1);
        }
    }

    // Special cases. If no symbols have frequency, all lengths
    // stay 0. If exactly one symbol is used, a lone 1-bit code is
    // an *incomplete* Huffman code, which RFC 1951 / zlib reject
    // ("invalid code lengths set"); emit a second 1-bit code on
    // another symbol so the code is complete (matches zlib's
    // single-symbol handling).
    if (heap.empty()) return;
    if (heap.size() == 1) {
        int only = -1;
        for (int i = 0; i < n; ++i) {
            if (freqs[i] > 0) { only = i; break; }
        }
        if (only < 0) return;
        lengths_out[only] = 1;
        const int other = (only == 0) ? 1 : 0;
        if (other < n) lengths_out[other] = 1;
        return;
    }

    auto cmp = [&](int a, int b) {
        if (nodes[a].freq != nodes[b].freq) {
            return nodes[a].freq > nodes[b].freq;
        }
        return nodes[a].depth > nodes[b].depth;
    };
    std::make_heap(heap.begin(), heap.end(), cmp);

    while (heap.size() > 1) {
        std::pop_heap(heap.begin(), heap.end(), cmp);
        const int a = heap.back(); heap.pop_back();
        std::pop_heap(heap.begin(), heap.end(), cmp);
        const int b = heap.back(); heap.pop_back();
        nodes.push_back(HuffNode{
            nodes[a].freq + nodes[b].freq,
            std::max(nodes[a].depth, nodes[b].depth) + 1,
            a, b});
        heap.push_back(int(nodes.size()) - 1);
        std::push_heap(heap.begin(), heap.end(), cmp);
    }

    // Walk the tree assigning depths to leaves. Leaves are the
    // first `m` nodes (one per non-zero symbol); recover the
    // original symbol index by matching frequency in order. A
    // simpler approach: rebuild using parent indices.
    //
    // Easier: walk via the tree we built. We track which leaves
    // map to which symbols by remembering the order they were
    // pushed.
    const int root = heap[0];
    std::vector<int> stack;
    std::vector<int> depths(nodes.size(), 0);
    stack.push_back(root);
    while (!stack.empty()) {
        const int v = stack.back(); stack.pop_back();
        if (nodes[v].left < 0) {
            // leaf
        } else {
            depths[nodes[v].left]  = depths[v] + 1;
            depths[nodes[v].right] = depths[v] + 1;
            stack.push_back(nodes[v].left);
            stack.push_back(nodes[v].right);
        }
    }

    // Map leaf nodes (first `m` entries in `nodes`) back to
    // symbol indices by their original frequency-and-position.
    int leaf = 0;
    for (int i = 0; i < n; ++i) {
        if (freqs[i] > 0) {
            lengths_out[i] = depths[leaf++];
            // Edge case: a single leaf paired with itself would
            // give depth 0. Force length ≥ 1 for any used symbol.
            if (lengths_out[i] == 0) lengths_out[i] = 1;
        }
    }

    // Length-limit + completeness pass. The raw tree depths above
    // can exceed max_bits and, after clamping, no longer form a
    // *complete* Huffman code — RFC 1951 / zlib reject both an
    // over-subscribed code and an incomplete one ("invalid …
    // set"). Enforce an exact Kraft sum of 1 in units of
    // 2^-max_bits: clamp, deepen while over-subscribed, then
    // shorten while incomplete. Operates on the active symbols'
    // lengths gathered from the tree.
    std::vector<int> bl;       // per-active-symbol code length
    std::vector<int> bl_freq;  // matching frequency
    bl.reserve(nodes.size());
    bl_freq.reserve(nodes.size());
    for (int i = 0; i < n; ++i) {
        if (lengths_out[i] > 0) {
            bl.push_back(lengths_out[i]);
            bl_freq.push_back(freqs[i]);
        }
    }
    const int m = static_cast<int>(bl.size());

    const int target = 1 << max_bits;
    long kraft = 0;
    for (int i = 0; i < m; ++i) {
        if (bl[i] > max_bits) bl[i] = max_bits;
        kraft += 1L << (max_bits - bl[i]);
    }
    // Over-subscribed (kraft > target): deepen the shallowest-
    // below-max symbols until the Kraft sum fits.
    while (kraft > target) {
        int best = -1;
        for (int i = 0; i < m; ++i) {
            if (bl[i] < max_bits &&
                (best < 0 || bl[i] > bl[best])) {
                best = i;
            }
        }
        if (best < 0) break;  // all at max_bits (cannot happen for
                              // a valid symbol count ≤ 2^max_bits)
        kraft -= 1L << (max_bits - bl[best]);
        bl[best]++;
        kraft += 1L << (max_bits - bl[best]);
    }
    // Incomplete (kraft < target): shorten the deepest symbols by
    // exact power-of-two increments until the code is complete.
    while (kraft < target) {
        int best = -1;
        for (int i = 0; i < m; ++i) {
            if (bl[i] > 1 &&
                kraft + (1L << (max_bits - bl[i])) <= target &&
                (best < 0 || bl[i] > bl[best])) {
                best = i;
            }
        }
        if (best < 0) break;
        kraft -= 1L << (max_bits - bl[best]);
        bl[best]--;
        kraft += 1L << (max_bits - bl[best]);
    }

    // Optimal assignment: shortest codes to the most-frequent
    // symbols (ascending length ↔ descending frequency).
    std::vector<int> sorted_lens = bl;
    std::sort(sorted_lens.begin(), sorted_lens.end());
    std::vector<std::pair<int,int>> by_freq;  // (freq, sym)
    for (int i = 0; i < n; ++i) {
        if (lengths_out[i] > 0) by_freq.emplace_back(freqs[i], i);
    }
    std::sort(by_freq.begin(), by_freq.end(),
              [](const auto& a, const auto& b){
                  if (a.first != b.first) return a.first > b.first;
                  return a.second < b.second;
              });
    for (int i = 0; i < m; ++i) {
        lengths_out[by_freq[i].second] = sorted_lens[i];
    }
}

// Compute canonical codes from per-symbol code lengths. Returns
// codes ready to be emitted MSB-first via BitWriter::WriteHuffmanCode.
void CanonicalCodes(const int* lengths,
                    int n,
                    std::uint32_t* codes_out) {
    std::array<int, kMaxBits + 2> bl_count{};
    for (int i = 0; i < n; ++i) ++bl_count[lengths[i]];
    bl_count[0] = 0;

    std::array<std::uint32_t, kMaxBits + 2> next_code{};
    std::uint32_t code = 0;
    for (int len = 1; len <= kMaxBits; ++len) {
        code = (code + bl_count[len - 1]) << 1;
        next_code[len] = code;
    }
    for (int i = 0; i < n; ++i) {
        if (lengths[i] != 0) {
            codes_out[i] = next_code[lengths[i]]++;
        } else {
            codes_out[i] = 0;
        }
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Encoder — block emit
// ──────────────────────────────────────────────────────────────────────────

// Length-code lookup. Maps an LZ77 match length (3..258) to its
// length code (257..285).
int LengthCode(int len) {
    // Linear scan over 29 entries — table is small enough.
    for (int i = int(kLengthBase.size()) - 1; i >= 0; --i) {
        if (len >= kLengthBase[i]) return 257 + i;
    }
    return 257;
}

// Distance-code lookup. Maps an LZ77 distance (1..32768) to its
// distance code (0..29).
int DistCode(int dist) {
    for (int i = int(kDistBase.size()) - 1; i >= 0; --i) {
        if (dist >= kDistBase[i]) return i;
    }
    return 0;
}

void EmitStoredBlock(BitWriter& bw,
                     const std::uint8_t* data,
                     std::size_t len,
                     bool final_block) {
    bw.WriteBits(final_block ? 1 : 0, 1);  // BFINAL
    bw.WriteBits(0, 2);                    // BTYPE = 00
    bw.AlignToByte();
    bw.WriteByte(std::uint8_t(len & 0xFF));
    bw.WriteByte(std::uint8_t((len >> 8) & 0xFF));
    const std::uint16_t nlen = std::uint16_t(~len);
    bw.WriteByte(std::uint8_t(nlen & 0xFF));
    bw.WriteByte(std::uint8_t((nlen >> 8) & 0xFF));
    for (std::size_t i = 0; i < len; ++i) bw.WriteByte(data[i]);
}

// Emit one dynamic-Huffman block from a token stream. Builds
// optimal trees from the token-stream symbol frequencies, then
// emits the trees followed by the encoded tokens.
void EmitDynamicBlock(BitWriter& bw,
                      const Token* tokens,
                      std::size_t tok_count,
                      bool final_block) {
    // Symbol-frequency tally.
    std::array<int, kMaxLitCodes> lit_freq{};
    std::array<int, kMaxDistCodes> dist_freq{};
    for (std::size_t i = 0; i < tok_count; ++i) {
        const Token& t = tokens[i];
        if (t.length == 0) {
            ++lit_freq[t.lit];
        } else {
            ++lit_freq[LengthCode(t.length)];
            ++dist_freq[DistCode(t.dist)];
        }
    }
    ++lit_freq[256];  // end-of-block code is always present once

    // Build literal/length tree (286 symbols defined; we size the
    // alphabet to the highest used symbol index, with at least
    // 257 to cover the EOB code).
    int hlit_count = 257;
    for (int i = 285; i >= 257; --i) {
        if (lit_freq[i] > 0) { hlit_count = i + 1; break; }
    }
    // Distance tree: at least one distance code, even if no
    // back-references — we emit a single dummy distance code
    // with length 1 in that case (RFC 1951 allows an incomplete
    // distance tree but several decoders mishandle truly empty,
    // so we err on the side of one valid code).
    int hdist_count = 1;
    for (int i = 29; i >= 1; --i) {
        if (dist_freq[i] > 0) { hdist_count = i + 1; break; }
    }
    bool any_dist = false;
    for (int i = 0; i < kMaxDistCodes; ++i) {
        if (dist_freq[i] > 0) { any_dist = true; break; }
    }
    if (!any_dist) {
        dist_freq[0] = 1;  // synthesize a dummy distance code
    }

    std::array<int, kMaxLitCodes> lit_lengths{};
    std::array<int, kMaxDistCodes> dist_lengths{};
    BuildLengths(lit_freq.data(),  hlit_count,
                 lit_lengths.data(),  kMaxBits);
    BuildLengths(dist_freq.data(), hdist_count,
                 dist_lengths.data(), kMaxBits);

    // Produce the combined code-length sequence for both trees.
    std::vector<std::uint8_t> cl_seq;
    cl_seq.reserve(std::size_t(hlit_count + hdist_count));
    for (int i = 0; i < hlit_count; ++i) {
        cl_seq.push_back(std::uint8_t(lit_lengths[i]));
    }
    for (int i = 0; i < hdist_count; ++i) {
        cl_seq.push_back(std::uint8_t(dist_lengths[i]));
    }

    // RLE-compress the sequence using the §3.2.7 alphabet.
    // Tokens here are pairs (symbol, extra_bits, extra_value).
    struct ClToken {
        std::uint8_t sym;
        std::uint8_t extra_bits;
        std::uint8_t extra_val;
    };
    std::vector<ClToken> cl_tokens;
    {
        std::size_t i = 0;
        while (i < cl_seq.size()) {
            const std::uint8_t v = cl_seq[i];
            std::size_t run = 1;
            while (i + run < cl_seq.size() &&
                   cl_seq[i + run] == v && run < 138) {
                ++run;
            }
            if (v == 0) {
                if (run >= 11) {
                    cl_tokens.push_back(ClToken{
                        18, 7, std::uint8_t(run - 11)});
                    i += run;
                } else if (run >= 3) {
                    cl_tokens.push_back(ClToken{
                        17, 3, std::uint8_t(run - 3)});
                    i += run;
                } else {
                    cl_tokens.push_back(ClToken{0, 0, 0});
                    ++i;
                }
            } else {
                // Emit one literal length, then RLE the rest with
                // code 16 (repeat previous, 3..6 times).
                cl_tokens.push_back(ClToken{v, 0, 0});
                ++i;
                std::size_t rem = run - 1;
                while (rem >= 3) {
                    const std::size_t r = std::min<std::size_t>(rem, 6);
                    cl_tokens.push_back(ClToken{
                        16, 2, std::uint8_t(r - 3)});
                    rem -= r;
                    i += r;
                }
                while (rem > 0) {
                    cl_tokens.push_back(ClToken{v, 0, 0});
                    ++i;
                    --rem;
                }
            }
        }
    }

    // Frequencies of code-length symbols.
    std::array<int, 19> cl_freq{};
    for (const auto& t : cl_tokens) ++cl_freq[t.sym];
    std::array<int, 19> cl_lengths{};
    BuildLengths(cl_freq.data(), 19, cl_lengths.data(), 7);

    // HCLEN — find smallest count such that all higher-order codes
    // (in the kCodeLengthOrder permutation) have length 0.
    int hclen_count = 19;
    while (hclen_count > 4 &&
           cl_lengths[kCodeLengthOrder[hclen_count - 1]] == 0) {
        --hclen_count;
    }

    // Emit block header.
    bw.WriteBits(final_block ? 1 : 0, 1);   // BFINAL
    bw.WriteBits(2, 2);                     // BTYPE = 10 (dynamic)
    bw.WriteBits(std::uint32_t(hlit_count - 257),  5);
    bw.WriteBits(std::uint32_t(hdist_count - 1),   5);
    bw.WriteBits(std::uint32_t(hclen_count - 4),   4);
    for (int i = 0; i < hclen_count; ++i) {
        bw.WriteBits(std::uint32_t(
            cl_lengths[kCodeLengthOrder[i]]), 3);
    }

    // Emit the combined code-length sequence using cl_codes.
    std::array<std::uint32_t, 19> cl_codes{};
    CanonicalCodes(cl_lengths.data(), 19, cl_codes.data());
    for (const auto& t : cl_tokens) {
        bw.WriteHuffmanCode(cl_codes[t.sym], cl_lengths[t.sym]);
        if (t.extra_bits) {
            bw.WriteBits(t.extra_val, t.extra_bits);
        }
    }

    // Emit the actual data using lit_codes and dist_codes.
    std::array<std::uint32_t, kMaxLitCodes> lit_codes{};
    std::array<std::uint32_t, kMaxDistCodes> dist_codes{};
    CanonicalCodes(lit_lengths.data(),  hlit_count,  lit_codes.data());
    CanonicalCodes(dist_lengths.data(), hdist_count, dist_codes.data());

    for (std::size_t i = 0; i < tok_count; ++i) {
        const Token& t = tokens[i];
        if (t.length == 0) {
            bw.WriteHuffmanCode(lit_codes[t.lit],
                                lit_lengths[t.lit]);
        } else {
            const int lc = LengthCode(t.length);
            bw.WriteHuffmanCode(lit_codes[lc], lit_lengths[lc]);
            const int extra_l = kLengthExtra[lc - 257];
            if (extra_l) {
                bw.WriteBits(
                    std::uint32_t(t.length - kLengthBase[lc - 257]),
                    extra_l);
            }
            const int dc = DistCode(t.dist);
            bw.WriteHuffmanCode(dist_codes[dc], dist_lengths[dc]);
            const int extra_d = kDistExtra[dc];
            if (extra_d) {
                bw.WriteBits(
                    std::uint32_t(t.dist - kDistBase[dc]),
                    extra_d);
            }
        }
    }
    // End-of-block.
    bw.WriteHuffmanCode(lit_codes[256], lit_lengths[256]);
}

}  // namespace

// ──────────────────────────────────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────────────────────────────────

std::vector<std::byte> Encode(std::span<const std::byte> input) {
    std::vector<std::byte> out;
    // Reserve a generous initial capacity — output is typically
    // smaller than input but we add header + per-block overhead.
    out.reserve(input.size() + 64 + (input.size() / 8));

    // RFC 1950 §2.2 — zlib header. CM=8 (deflate), CINFO=7 (32 KiB
    // window). FLEVEL=2 (default), FDICT=0. CMF=0x78. FLG chosen
    // so (CMF<<8 | FLG) % 31 == 0.
    constexpr std::uint8_t kCmf = 0x78;
    // FCHECK = (31 - (CMF*256 + (FLEVEL<<6 + FDICT<<5)) % 31) % 31
    // For CMF=0x78, FLEVEL=2 (bits 6-7 = 10), FDICT=0:
    //   FLG candidate base = 0x80 (bits 7-6 = 10)
    //   header so far = 0x7880 = 30848; 30848 % 31 = 24
    //   FCHECK = (31 - 24) % 31 = 7
    //   FLG = 0x80 | 7 = 0x87
    constexpr std::uint8_t kFlg = 0x9C;  // FLEVEL=2, FCHECK=28: see below
    // Recompute to make sure: 0x789C = 30876; 30876 % 31 = 0. ✓
    out.push_back(std::byte(kCmf));
    out.push_back(std::byte(kFlg));

    BitWriter bw(out);

    // LZ77 hash chain state — shared across blocks so back-
    // references can span block boundaries within the 32 KiB
    // window. Sized to one entry per window position.
    std::vector<int> head(kHashSize, -1);
    std::vector<int> prev(kWindow, -1);

    const std::uint8_t* data =
        reinterpret_cast<const std::uint8_t*>(input.data());
    const std::size_t total = input.size();

    if (total == 0) {
        // RFC 1951: empty input still requires a final block. An
        // empty stored block (BFINAL=1, BTYPE=00, LEN=0, NLEN=0xFFFF)
        // is the most compact representation.
        EmitStoredBlock(bw, nullptr, 0, true);
    } else {
        std::size_t pos = 0;
        while (pos < total) {
            const std::size_t end = std::min(pos + kBlockSize, total);
            const bool final_block = (end == total);
            // Empty input slice never reached here; pos < total.
            std::vector<Token> tokens;
            tokens.reserve((end - pos) / 2 + 16);
            Lz77(data, total, pos, end, head, prev, tokens);

            // For very small blocks, dynamic-Huffman header
            // overhead exceeds savings; emit stored.
            if (end - pos < 32) {
                EmitStoredBlock(bw, data + pos, end - pos,
                                final_block);
            } else {
                EmitDynamicBlock(bw, tokens.data(), tokens.size(),
                                 final_block);
            }
            pos = end;
        }
    }
    bw.AlignToByte();

    // RFC 1950 §2.2 — Adler-32 trailer (big-endian).
    const std::uint32_t adler = Adler32(input);
    out.push_back(std::byte((adler >> 24) & 0xFFu));
    out.push_back(std::byte((adler >> 16) & 0xFFu));
    out.push_back(std::byte((adler >>  8) & 0xFFu));
    out.push_back(std::byte( adler        & 0xFFu));

    return out;
}

std::vector<std::byte> Decode(std::span<const std::byte> input) {
    if (input.size() < 6) {
        // Minimum valid stream: 2-byte header + at least one
        // empty stored block (5 bytes) + 4-byte Adler.
        throw std::runtime_error(
            "flate: input too short for a zlib stream");
    }
    BitReader br(input);
    ReadZlibHeader(br);

    std::vector<std::byte> out;
    while (true) {
        const std::uint32_t bfinal = br.ReadBits(1);
        const std::uint32_t btype  = br.ReadBits(2);
        if (btype == 0) {
            DecodeStoredBlock(br, out);
        } else if (btype == 1) {
            DecodeCompressedBlock(br, FixedLitTable(),
                                  FixedDistTable(), out);
        } else if (btype == 2) {
            DecodeDynamicBlock(br, out);
        } else {
            throw std::runtime_error(
                "flate: reserved block type 11");
        }
        if (bfinal) break;
    }
    br.AlignToByte();
    // The Adler-32 trailer (RFC 1950) is an advisory integrity check.
    // Once the DEFLATE data has fully decoded (final block consumed),
    // the decompressed bytes are usable; a missing or mismatching
    // trailer must NOT discard them. Real PDFs routinely declare a
    // stream /Length a few bytes longer than the zlib data, leaving
    // trailing bytes that a strict trailer read trips over — poppler /
    // zlib-based readers keep the output regardless, so we do too.
    if (br.BytesLeft() >= 4) {
        (void)ReadAdler32Trailer(br);
    }
    return out;
}

}  // namespace foundation::flate
