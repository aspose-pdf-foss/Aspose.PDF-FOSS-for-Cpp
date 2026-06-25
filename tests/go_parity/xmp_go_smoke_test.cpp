// =============================================================================
// Go-parity: xmp_test.go → canonical C++ Document::Metadata (Metadata +
// XmpValue).
//
// Ported (canonical): XMP key add + round-trip through Save, Clear.
// Skipped: SyncInfoToXMP (invented Go convenience) and external-packet parse
// assertions tied to a Go-specific fixture.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/metadata.hpp>
#include <aspose/pdf/xmp_value.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path PdfDir() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return std::filesystem::path(env);
    return std::filesystem::path(__FILE__)
        .parent_path()
        .parent_path()
        .parent_path() /
        "pdfs";
}
std::string HelloWorld() { return (PdfDir() / "hello_world.pdf").string(); }
std::string TmpOut(const std::string& tag) {
    return (std::filesystem::temp_directory_path() / ("go_xmp_" + tag + ".pdf"))
        .string();
}

}  // namespace

// TestXMPRoundTrip — an XMP key set on the metadata survives save/reload.
TEST(GoXmp, RoundTrip) {
    const std::string out = TmpOut("rt");
    {
        Document doc{HelloWorld()};
        doc.Metadata().Add("dc:title", XmpValue(std::string("Go Parity")));
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_TRUE(doc.Metadata().ContainsKey("dc:title"));
    std::filesystem::remove(out);
}

// TestXMPClear — Clear empties the metadata bag.
TEST(GoXmp, Clear) {
    Document doc{HelloWorld()};
    Metadata& meta = doc.Metadata();
    meta.Add("dc:creator", XmpValue(std::string("X")));
    ASSERT_GE(meta.Count(), 1);
    meta.Clear();
    EXPECT_EQ(meta.Count(), 0);
}
