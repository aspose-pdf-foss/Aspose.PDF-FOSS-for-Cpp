#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "lexer.hpp"
#include "flate.hpp"

namespace foundation::xref {

enum class EntryKind {
    InUse,
    Compressed,
};

struct Entry {
    std::uint32_t id;
    std::uint16_t generation;
    EntryKind kind;
    std::uint64_t offset_or_stream_id;
    std::uint32_t index_in_stream;
};

struct Table {
    std::vector<Entry> entries;

    // Resolved trailer fields, captured while walking the startxref -> /Prev
    // chain (newest section wins). id == 0 means "absent". For a correct PDF,
    // root_id is always non-zero after Parse.
    std::uint32_t root_id  = 0;
    std::uint16_t root_gen = 0;
    std::uint32_t info_id  = 0;   // 0 = absent
    std::uint16_t info_gen = 0;
    std::uint64_t size     = 0;   // /Size (max across the chain)
};

Table Parse(std::span<const std::byte> input);
std::string ToCanonical(const Table& table);

}
