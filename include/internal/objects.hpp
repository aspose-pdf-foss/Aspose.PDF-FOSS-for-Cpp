#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "lexer.hpp"
#include "xref.hpp"

namespace foundation::objects {

enum class StringKind {
    Literal,
    Hex,
};

struct String {
    std::vector<std::byte> bytes;
    StringKind kind;
};

struct Ref {
    std::uint32_t id;
    std::uint16_t generation;
};

struct Value;

struct Array {
    std::vector<Value> items;
};

struct Dict {
    std::vector<std::pair<std::string, Value>> entries;
};

// Stream body — a byte view, matching the shared spec yaml's
// `stream {dict, body}` shape (v1.1). For parsed streams the
// span points into the parsed source buffer (no byte copy);
// for dirty-list entries the caller owns a vector whose
// lifetime must outlive the Stream value.
struct Stream {
    Dict header;
    std::span<const std::byte> body;
};

struct Value {
    std::variant<
        std::monostate,
        bool,
        std::int64_t,
        double,
        std::string,
        String,
        Array,
        Dict,
        Stream,
        Ref
    > v;
};

struct IndirectObject {
    std::uint32_t id;
    std::uint16_t generation;
    Value value;
};

struct Dump {
    std::vector<IndirectObject> objects;
};

Dump Parse(std::span<const std::byte> input);
std::string ToCanonical(const Dump& dump);

}
