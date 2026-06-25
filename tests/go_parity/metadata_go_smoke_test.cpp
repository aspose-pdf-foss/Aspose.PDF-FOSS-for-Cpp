// =============================================================================
// Go-parity: metadata_test.go → canonical C++ Document::Info (DocumentInfo).
//
// Ported (canonical): read /Info fields, set + round-trip, custom-field
// add/round-trip, replace, clear. Go asserts the marketing fixture's exact
// strings ("Untitled", "Acrobat Editor 9.0", …); here the intent is asserted
// against this lib's with_info.pdf fixture (fields present / round-trip).
// Skipped: TestDocumentMetadataAfterAppend (Document.Append merge — not a
// confirmed canonical op in the cpp subset).
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_info.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path MetaDir() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return std::filesystem::path(env) / "fixtures" / "metadata";
    return std::filesystem::path(__FILE__).parent_path().parent_path() /
           "fixtures" / "metadata";
}

std::string WithInfo() { return (MetaDir() / "with_info.pdf").string(); }

std::string TmpOut(const std::string& tag) {
    return (std::filesystem::temp_directory_path() / ("go_meta_" + tag + ".pdf"))
        .string();
}

}  // namespace

// TestDocumentMetadataFields / TestDocumentMetadata — /Info fields are read.
TEST(GoMetadata, ReadInfoFields) {
    Document doc{WithInfo()};
    DocumentInfo& info = doc.Info();
    EXPECT_FALSE(info.Title().empty());
    EXPECT_FALSE(info.Producer().empty());
}

// TestSetMetadataRoundTrip — set the named fields and read them back.
TEST(GoMetadata, SetFieldsRoundTrip) {
    const std::string out = TmpOut("set");
    {
        Document doc{WithInfo()};
        DocumentInfo& info = doc.Info();
        info.Title("Go Parity Title");
        info.Author("Go Parity Author");
        info.Subject("Go Parity Subject");
        info.Keywords("a; b; c");
        doc.Save(out);
    }
    Document doc{out};
    DocumentInfo& info = doc.Info();
    EXPECT_EQ(info.Title(), "Go Parity Title");
    EXPECT_EQ(info.Author(), "Go Parity Author");
    EXPECT_EQ(info.Subject(), "Go Parity Subject");
    EXPECT_EQ(info.Keywords(), "a; b; c");
    std::filesystem::remove(out);
}

// TestSetMetadataReplaces — re-setting a field replaces the prior value.
TEST(GoMetadata, SetReplaces) {
    Document doc{WithInfo()};
    DocumentInfo& info = doc.Info();
    info.Title("first");
    info.Title("second");
    EXPECT_EQ(info.Title(), "second");
}

// TestMetadataCustomFieldsRoundTrip / TestSetMetadataCustomFields — custom keys.
TEST(GoMetadata, CustomFieldsRoundTrip) {
    const std::string out = TmpOut("custom");
    {
        Document doc{WithInfo()};
        doc.Info().Add("GoParityKey", "GoParityValue");
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(doc.Info()["GoParityKey"], "GoParityValue");
    std::filesystem::remove(out);
}

// TestClearMetadata — Clear() empties the named fields.
TEST(GoMetadata, ClearEmptiesFields) {
    Document doc{WithInfo()};
    DocumentInfo& info = doc.Info();
    info.Title("to be cleared");
    info.Clear();
    EXPECT_TRUE(info.Title().empty());
}
