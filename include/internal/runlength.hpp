#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <stdexcept>

namespace foundation::runlength {

// Encode ``input`` as a PDF RunLengthDecode stream. The output
// ALWAYS ends with a 0x80 end-of-data marker; empty input
// therefore encodes to a single 0x80 byte. The encoder is free
// to choose any valid segmentation (literal vs repeat, where
// to split runs > 128) — any valid encoding round-trips
// through Decode to the original input.
//
// Throws std::runtime_error only on internal failure — any
// byte sequence is valid RunLength input.
std::vector<std::byte> Encode(std::span<const std::byte> input);

// Decode a PDF RunLengthDecode stream. Reads segments until
// the 0x80 EOD marker or end of input. A truncated segment
// (payload bytes missing for the declared length) is
// malformed; a missing EOD is malformed (the caller must pass
// a complete stream).
//
// Empty input is rejected as a malformed stream (cannot
// determine whether EOD was intended). Decode of a single
// 0x80 byte returns an empty vector — the round-trip of
// Encode of empty input.
//
// Throws std::runtime_error on any malformed input, naming
// the offending segment's offset in the message.
std::vector<std::byte> Decode(std::span<const std::byte> input);

}
