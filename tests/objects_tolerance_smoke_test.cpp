// =============================================================================
// objects_tolerance_smoke_test — objects-layer tolerance of non-conforming
// input that poppler/mutool accept but a strict parser rejects (task_06 A4).
//
// Each fixture is built inline with an unusable startxref so the xref layer
// reconstructs the object set by scanning `N G obj` headers; objects::Parse
// then parses them. Verified (by reverting the fixes) to throw before:
//   invalid hex digit in name / duplicate dictionary key /
//   expected 'endstream' after stream body / invalid integer literal /
//   expected 'endobj' keyword.
// =============================================================================

#include "objects.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace {

using foundation::objects::Dump;
using foundation::objects::IndirectObject;
using foundation::objects::Value;

std::vector<std::byte> Pdf(const std::vector<std::string>& bodies) {
    std::string s = "%PDF-1.5\n";
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        s += std::to_string(i + 1) + " 0 obj\n" + bodies[i] + "\nendobj\n";
    }
    // Bogus startxref → the xref layer reconstructs by scanning headers.
    s += "trailer\n<</Root 1 0 R>>\nstartxref\n0\n%%EOF\n";
    std::vector<std::byte> out(s.size());
    for (std::size_t i = 0; i < s.size(); ++i)
        out[i] = static_cast<std::byte>(s[i]);
    return out;
}

const IndirectObject* Obj(const Dump& d, std::uint32_t id) {
    for (const auto& o : d.objects)
        if (o.id == id) return &o;
    return nullptr;
}

Dump Parse(const std::vector<std::byte>& pdf) {
    return foundation::objects::Parse(
        std::span<const std::byte>(pdf.data(), pdf.size()));
}

// `#` not followed by two hex digits is kept literally (not rejected).
TEST(ObjectsToleranceSmoke, HashNotHexIsLiteral) {
    const auto d = Parse(Pdf({"<< /K /a#_b >>"}));
    const auto* o = Obj(d, 1);
    ASSERT_NE(o, nullptr);
    const auto* dict = std::get_if<foundation::objects::Dict>(&o->value.v);
    ASSERT_NE(dict, nullptr);
    ASSERT_EQ(dict->entries.size(), 1u);
    const auto* name = std::get_if<std::string>(&dict->entries[0].second.v);
    ASSERT_NE(name, nullptr);
    EXPECT_EQ(*name, "a#_b");
}

// A duplicate key keeps the last occurrence rather than throwing.
TEST(ObjectsToleranceSmoke, DuplicateKeyLastWins) {
    const auto d = Parse(Pdf({"<< /A 1 /A 2 >>"}));
    const auto* o = Obj(d, 1);
    ASSERT_NE(o, nullptr);
    const auto* dict = std::get_if<foundation::objects::Dict>(&o->value.v);
    ASSERT_NE(dict, nullptr);
    ASSERT_EQ(dict->entries.size(), 1u);
    EXPECT_EQ(dict->entries[0].first, "A");
    const auto* iv = std::get_if<std::int64_t>(&dict->entries[0].second.v);
    ASSERT_NE(iv, nullptr);
    EXPECT_EQ(*iv, 2);
}

// A wrong direct /Length recovers the true body by scanning for endstream.
TEST(ObjectsToleranceSmoke, WrongLengthScansForEndstream) {
    const auto d = Parse(Pdf({"<< /Length 3 >>\nstream\nHELLOWORLD\nendstream"}));
    const auto* o = Obj(d, 1);
    ASSERT_NE(o, nullptr);
    const auto* st = std::get_if<foundation::objects::Stream>(&o->value.v);
    ASSERT_NE(st, nullptr);
    const std::string body(reinterpret_cast<const char*>(st->body.data()),
                           st->body.size());
    EXPECT_EQ(body, "HELLOWORLD");
}

// An integer literal too large for int64 parses as a real, not an error.
TEST(ObjectsToleranceSmoke, OverflowIntegerBecomesReal) {
    const auto d = Parse(Pdf({"[ 340282346638528859811704183484516925440 ]"}));
    const auto* o = Obj(d, 1);
    ASSERT_NE(o, nullptr);
    const auto* arr = std::get_if<foundation::objects::Array>(&o->value.v);
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(arr->items.size(), 1u);
    EXPECT_NE(std::get_if<double>(&arr->items[0].v), nullptr);
}

// One malformed object is skipped; the surrounding objects still load.
TEST(ObjectsToleranceSmoke, MalformedObjectIsSkipped) {
    const auto d = Parse(Pdf({
        "<< /Type /Catalog >>",   // 1: valid
        "<< /A 1 /A",             // 2: unterminated dict -> throws -> skipped
        "<< /Type /Page >>",      // 3: valid
    }));
    EXPECT_NE(Obj(d, 1), nullptr) << "object 1 should load";
    EXPECT_EQ(Obj(d, 2), nullptr) << "malformed object 2 should be skipped";
    EXPECT_NE(Obj(d, 3), nullptr) << "object 3 should load";
}

}  // namespace
