#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
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

    // Special members are user-declared here and defined out-of-line in
    // objects.cpp (where Value/Dict/Array are complete). This breaks the
    // recursive-type instantiation cycle
    //   Value -> Dict -> std::vector<std::pair<std::string, Value>> -> Value
    // which, under clang + libstdc++, otherwise eagerly evaluates
    // completeness traits on the still-incomplete Value and fails to
    // compile (gcc defers the same instantiation, so it slips through).
    Value();
    Value(const Value&);
    Value(Value&&) noexcept;
    Value& operator=(const Value&);
    Value& operator=(Value&&) noexcept;
    ~Value();

    // Preserve the aggregate-style ergonomics this type had before the
    // special members above were declared: `Value{x}` for any variant
    // alternative x. Excluded for Value itself so copy/move are chosen.
    template <class T,
              class = std::enable_if_t<
                  !std::is_same_v<std::remove_cvref_t<T>, Value>>>
    Value(T&& x) : v(std::forward<T>(x)) {}
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
