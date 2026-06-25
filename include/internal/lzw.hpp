#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace foundation::lzw {

// Off-by-one rule for the variable-width transition. PDF
// LZWDecode uses EarlyChange=1; TIFF Compression=5 (per the
// libtiff de-facto convention) uses EarlyChange=0. The two
// produce different bit-level streams; choose the one matching
// the consumer.
enum class EarlyChange { Pdf = 1, Tiff = 0 };

// Encode input as an LZW byte stream. Starts with ClearCode
// (256), ends with EOD (257), codes packed MSB-first, variable
// bit width growing 9→10→11→12 as the string table fills.
// Empty input encodes to a two-code ClearCode+EOD stream.
std::vector<std::byte> Encode(
    std::span<const std::byte> input,
    EarlyChange early_change = EarlyChange::Pdf);

// Decode an LZW byte stream back to its original plaintext.
// Handles the KwKwK case, ClearCode mid-stream, and EOD.
// Throws std::runtime_error on malformed input.
std::vector<std::byte> Decode(
    std::span<const std::byte> input,
    EarlyChange early_change = EarlyChange::Pdf);

}
