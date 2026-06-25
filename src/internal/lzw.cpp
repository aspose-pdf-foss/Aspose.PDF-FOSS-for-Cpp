#include "lzw.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

// PDF LZWDecode per PDF 32000-1:2008 §7.4.4.2.
// EarlyChange = 1 (the PDF default): the bit-width transition
// fires when a NEW code assignment would reach (1 << width) - 1
// — one code earlier than classical UNIX compress(1).

namespace foundation::lzw {

namespace {

constexpr int kClearCode   = 256;
constexpr int kEodCode     = 257;
constexpr int kInitWidth   = 9;
constexpr int kMaxWidth    = 12;
constexpr int kMaxCodes    = 1 << kMaxWidth;   // 4096
constexpr int kInitNext    = 258;              // first free code

// -------------------------------------------------------------
// MSB-first bit packer / unpacker
// -------------------------------------------------------------

// Packer: holds up to 31 bits in a 32-bit buffer, MSB-first.
// Flush() pads the last byte with zero bits, per spec — the
// decoder stops at EOD and doesn't read past, so the pad is
// opaque. Written out as one helper so every encode-side
// call site uses the same bit-alignment rule.
class BitPacker {
 public:
    void Push(std::uint32_t value, int width) {
        // Left-justify the value into the running buffer's
        // remaining high bits: (32 - bits_ - width) is the
        // shift that places value's MSB at the highest
        // currently-unused bit.
        buffer_ |= (value & ((1u << width) - 1))
                   << (32 - bits_ - width);
        bits_ += width;
        while (bits_ >= 8) {
            out_.push_back(static_cast<std::byte>(
                (buffer_ >> 24) & 0xff));
            buffer_ <<= 8;
            bits_ -= 8;
        }
    }
    // Emit any tail bits as the high bits of one more byte,
    // with zero padding.
    void Flush() {
        if (bits_ > 0) {
            out_.push_back(static_cast<std::byte>(
                (buffer_ >> 24) & 0xff));
            buffer_ = 0;
            bits_ = 0;
        }
    }
    std::vector<std::byte> Take() { return std::move(out_); }
 private:
    std::vector<std::byte> out_;
    std::uint32_t buffer_ = 0;  // pending high bits left-justified
    int bits_ = 0;
};

// Unpacker: pulls bits MSB-first out of an input byte span.
// The buffer_/bits_ invariant mirrors BitPacker's — the next
// `width` bits always live at the top of the 32-bit buffer.
class BitUnpacker {
 public:
    explicit BitUnpacker(std::span<const std::byte> input)
        : input_(input) {}

    // Read `width` bits. Returns -1 when there aren't enough
    // input bits left for one whole code (the caller decides
    // whether that's an error or a clean stream end).
    int Pull(int width) {
        while (bits_ < width) {
            if (pos_ >= input_.size()) {
                if (bits_ == 0) return -1;  // clean end
                // Trailing partial bits the caller needed —
                // return -1 so the caller can decide whether
                // to treat as trunction or as pad-after-EOD.
                return -1;
            }
            buffer_ |= (static_cast<std::uint32_t>(input_[pos_])
                         & 0xff) << (24 - bits_);
            ++pos_;
            bits_ += 8;
        }
        int code = static_cast<int>((buffer_ >> (32 - width))
                                    & ((1u << width) - 1));
        buffer_ <<= width;
        bits_ -= width;
        return code;
    }
 private:
    std::span<const std::byte> input_;
    std::uint32_t buffer_ = 0;  // pending high bits
    int bits_ = 0;
    std::size_t pos_ = 0;
};

// -------------------------------------------------------------
// String table — parent-pointer layout
// -------------------------------------------------------------

// Each entry (code ≥ 258) records its parent code + the final
// byte it appends. To reconstruct the decoded string for a
// code, follow parent pointers until < 256 (a literal byte),
// collecting the final bytes; reverse to get the string.
// Compact: 5 bytes per entry * 4096 = 20 KiB — cheap on stack.
struct Entry {
    int parent;
    std::uint8_t last;
};

// For encode-side: look up whether (prefix_code, next_byte) is
// already in the table. Classical hash-per-slot would work;
// we use an open-addressed linear-probe hash keyed on the
// packed (prefix << 8 | byte) value.
struct Lookup {
    static constexpr int kSize = 5207;  // prime > 4096
    std::array<std::int32_t, kSize> key;  // -1 == empty
    std::array<std::int32_t, kSize> val;  // code value
    void Clear() {
        for (int i = 0; i < kSize; ++i) {
            key[i] = -1;
            val[i] = 0;
        }
    }
    // packed_key = (prefix << 8) | byte.  Returns assigned code
    // or -1 when not present.
    int Get(int packed_key) const {
        int h = static_cast<unsigned>(packed_key) % kSize;
        while (key[h] != -1) {
            if (key[h] == packed_key) return val[h];
            h = (h + 1) % kSize;
        }
        return -1;
    }
    void Put(int packed_key, int code) {
        int h = static_cast<unsigned>(packed_key) % kSize;
        while (key[h] != -1) h = (h + 1) % kSize;
        key[h] = packed_key;
        val[h] = code;
    }
};

// -------------------------------------------------------------
// Encoder body
// -------------------------------------------------------------

void EmitClearAndReset(BitPacker& packer, int& width,
                       int& next_code, Lookup& lookup) {
    packer.Push(kClearCode, width);
    width = kInitWidth;
    next_code = kInitNext;
    lookup.Clear();
}

// Decide whether `next_code` is about to need a wider code
// field. PDF EarlyChange=1: transition when next_code reaches
// (1 << width) - 1. TIFF EarlyChange=0: transition when
// next_code reaches (1 << width). Called after inserting a
// new entry.
bool ShouldWidenEnc(int next_code, int width, int early) {
    return next_code >= (1 << width) - early && width < kMaxWidth;
}

// Decoder version: lags encoder by one install (first code
// after Clear doesn't install). Threshold is one less than
// the encoder's so the transition lines up by stream position
// even though next_code differs by one across the two sides.
bool ShouldWidenDec(int next_code, int width, int early) {
    return next_code >= (1 << width) - 1 - early && width < kMaxWidth;
}

}  // namespace

// -------------------------------------------------------------
// Public Encode
// -------------------------------------------------------------

std::vector<std::byte> Encode(std::span<const std::byte> input,
                              EarlyChange early_change) {
    const int early = static_cast<int>(early_change);  // 0 or 1
    BitPacker packer;
    Lookup lookup;
    int width = kInitWidth;
    int next_code = kInitNext;
    lookup.Clear();

    // Stream always begins with ClearCode per §7.4.4.2.
    packer.Push(kClearCode, width);

    if (!input.empty()) {
        int omega = static_cast<int>(input[0]) & 0xff;
        for (std::size_t i = 1; i < input.size(); ++i) {
            int byte = static_cast<int>(input[i]) & 0xff;
            int key = (omega << 8) | byte;
            int got = lookup.Get(key);
            if (got >= 0) {
                omega = got;
                continue;
            }
            // omega + byte is NOT in the table. Emit omega's
            // code, install omega+byte at next_code, reset
            // omega to the current byte as a one-char prefix.
            packer.Push(omega, width);
            if (next_code < kMaxCodes) {
                lookup.Put(key, next_code);
                ++next_code;
                if (ShouldWidenEnc(next_code, width, early)) ++width;
            } else {
                // Table full — emit ClearCode, reset everything.
                EmitClearAndReset(packer, width, next_code, lookup);
            }
            omega = byte;
        }
        // Final prefix is a code on its own.
        packer.Push(omega, width);
    }

    // EOD at the current width per spec.
    packer.Push(kEodCode, width);
    packer.Flush();
    return packer.Take();
}

// -------------------------------------------------------------
// Public Decode
// -------------------------------------------------------------

namespace {

// Walk the parent-pointer chain for `code` and append the
// reconstructed bytes (in order) to `out`. Returns the FIRST
// byte of the reconstructed string — needed for the KwKwK
// case where the caller then appends first-byte to prev_string.
std::uint8_t EmitString(const std::array<Entry, kMaxCodes>& table,
                        int code, std::vector<std::byte>& out) {
    // The chain is at most kMaxCodes long; stack buffer is fine.
    std::array<std::uint8_t, kMaxCodes> scratch{};
    std::size_t n = 0;
    int c = code;
    while (c >= 256) {
        scratch[n++] = table[c].last;
        c = table[c].parent;
    }
    scratch[n++] = static_cast<std::uint8_t>(c);
    // Reverse-emit so the first byte is at the front.
    for (std::size_t i = n; i > 0; --i) {
        out.push_back(static_cast<std::byte>(scratch[i - 1]));
    }
    return scratch[n - 1];
}

}  // namespace

std::vector<std::byte> Decode(std::span<const std::byte> input,
                              EarlyChange early_change) {
    const int early = static_cast<int>(early_change);  // 0 or 1
    std::vector<std::byte> out;
    if (input.empty()) {
        throw std::runtime_error(
            "lzw: empty input (expected at least ClearCode + EOD)");
    }
    BitUnpacker unpacker(input);
    std::array<Entry, kMaxCodes> table{};
    int width = kInitWidth;
    int next_code = kInitNext;
    int prev_code = -1;
    std::uint8_t prev_first = 0;  // first byte of prev_code's string

    while (true) {
        int code = unpacker.Pull(width);
        if (code < 0) {
            throw std::runtime_error(
                "lzw: missing EOD (truncated stream)");
        }
        if (code == kEodCode) break;
        if (code == kClearCode) {
            width = kInitWidth;
            next_code = kInitNext;
            prev_code = -1;
            continue;
        }
        if (prev_code < 0) {
            // First real code after a Clear. MUST be a literal
            // byte (< 256) or a pre-existing code.
            if (code >= next_code) {
                throw std::runtime_error(
                    "lzw: first code after Clear out of range");
            }
            prev_first = EmitString(table, code, out);
            prev_code = code;
            continue;
        }
        // Subsequent code: either in table (< next_code) OR the
        // KwKwK edge case (== next_code). Anything > next_code
        // is malformed.
        std::uint8_t first_byte;
        if (code < next_code) {
            first_byte = EmitString(table, code, out);
        } else if (code == next_code) {
            // KwKwK: string is prev_string + prev_first. Emit
            // prev_string then prev_first.
            std::size_t before = out.size();
            EmitString(table, prev_code, out);
            out.push_back(static_cast<std::byte>(prev_first));
            // First byte of this new string is the first byte
            // of prev_code's string, which equals what
            // EmitString just appended as out[before].
            first_byte = static_cast<std::uint8_t>(out[before]);
        } else {
            throw std::runtime_error(
                "lzw: code out of range (malformed stream)");
        }
        // Add prev_code + first_byte to table (if room). The
        // decoder lags the encoder by one insert (ClearCode +
        // first-real-code contribute no insert on this side),
        // so our widen threshold is `(1 << width) - 2` where
        // the encoder uses `(1 << width) - 1`. This offset
        // aligns the transition point by STREAM POSITION even
        // though next_code differs by one across the two sides.
        if (next_code < kMaxCodes) {
            table[next_code].parent = prev_code;
            table[next_code].last = first_byte;
            ++next_code;
            if (ShouldWidenDec(next_code, width, early)) {
                ++width;
            }
        }
        prev_code = code;
        prev_first = first_byte;
        // Re-read prev_first for the just-added entry so
        // KwKwK on the next iteration has the right first byte.
        // EmitString(prev_code, out) side-effects made out
        // grow; walk back to find the first byte. Cheaper to
        // recompute the first byte directly: for a literal,
        // first byte = code; for a table entry, follow parent
        // chain. Do the walk once here so the hot loop above
        // can stay simple.
        {
            int c = prev_code;
            while (c >= 256) c = table[c].parent;
            prev_first = static_cast<std::uint8_t>(c);
        }
    }
    return out;
}

}  // namespace foundation::lzw
