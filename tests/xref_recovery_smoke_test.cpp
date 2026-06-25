// =============================================================================
// xref_recovery_smoke_test — xref v3 recovery + /Length body bounds.
//
// These exercise behaviour the two_oracle_agreement freeze corpus cannot:
// qpdf and mutool diverge on broken-xref recovery, so the agreement gate
// excludes those cases. Fixtures live in tests/fixtures/xref_recovery
// (generate_fixtures.py); see that script for each shape.
// =============================================================================

#include "xref.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace {

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("XREF_RECOVERY_FIXTURE_DIR")) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path()
        / "fixtures" / "xref_recovery";
}

std::vector<std::byte> ReadFile(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    EXPECT_TRUE(f) << "missing fixture " << p;
    std::vector<std::byte> bytes(static_cast<std::size_t>(f.tellg()));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(bytes.data()),
           static_cast<std::streamsize>(bytes.size()));
    return bytes;
}

const foundation::xref::Entry* Find(const foundation::xref::Table& t,
                                    std::uint32_t id) {
    for (const auto& e : t.entries) {
        if (e.id == id) return &e;
    }
    return nullptr;
}

// The three direct objects all resolve to in-use entries whose offset
// lands on the matching `N 0 obj` header, and /Root is recovered — for
// both the stale-startxref and the bad-type-byte shapes.
void ExpectClassicalRecovery(const char* name) {
    const auto bytes = ReadFile(FixtureRoot() / name);
    ASSERT_FALSE(bytes.empty());
    const std::string text(reinterpret_cast<const char*>(bytes.data()),
                           bytes.size());

    const auto table = foundation::xref::Parse(
        std::span<const std::byte>(bytes.data(), bytes.size()));

    EXPECT_EQ(table.root_id, 1u) << name;
    for (std::uint32_t id = 1; id <= 3; ++id) {
        const auto* e = Find(table, id);
        ASSERT_NE(e, nullptr) << name << " missing object " << id;
        EXPECT_EQ(e->kind, foundation::xref::EntryKind::InUse);
        const std::string header = std::to_string(id) + " 0 obj";
        EXPECT_EQ(e->offset_or_stream_id, text.find(header))
            << name << " object " << id << " offset";
    }
}

TEST(XrefRecoverySmoke, StaleStartxrefReconstructs) {
    ExpectClassicalRecovery("recon_stale.pdf");
}

TEST(XrefRecoverySmoke, BadTypeByteReconstructs) {
    ExpectClassicalRecovery("recon_badtype.pdf");
}

// The /XRef stream's FlateDecode body ends in 0x0A. Honouring /Length
// keeps it; the old scan-and-trim truncated it. The two objects packed
// in the /ObjStm are visible only through the decoded stream (whole-file
// reconstruction cannot see ObjStm-packed objects), so their presence as
// Compressed entries proves the /Length-bounded stream decode succeeded.
TEST(XrefRecoverySmoke, LengthBoundedStreamKeepsTrailingByte) {
    const auto bytes = ReadFile(FixtureRoot() / "length_objstm.pdf");
    ASSERT_FALSE(bytes.empty());

    const auto table = foundation::xref::Parse(
        std::span<const std::byte>(bytes.data(), bytes.size()));

    EXPECT_EQ(table.root_id, 1u);
    for (std::uint32_t id : {1u, 2u}) {
        const auto* e = Find(table, id);
        ASSERT_NE(e, nullptr) << "compressed object " << id << " missing";
        EXPECT_EQ(e->kind, foundation::xref::EntryKind::Compressed)
            << "object " << id << " should be ObjStm-compressed";
        EXPECT_EQ(e->offset_or_stream_id, 4u) << "container stream id";
    }
    EXPECT_EQ(Find(table, 1u)->index_in_stream, 0u);
    EXPECT_EQ(Find(table, 2u)->index_in_stream, 1u);
}

}  // namespace
