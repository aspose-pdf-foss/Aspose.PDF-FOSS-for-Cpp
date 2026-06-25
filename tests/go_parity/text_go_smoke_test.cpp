// =============================================================================
// Go-parity: text_test.go → canonical C++ Text::TextAbsorber.
//
// Ported (canonical): per-page text extraction and whole-document extraction
// (Go's page.ExtractText / doc.ExtractText → TextAbsorber.Visit(Page/Document)
// + Text()).
// Skipped: the extraction-engine detail tests (TJ kerning, multiline, unknown
// font, Form XObject, horiz-scaling, Type0, with-layout / visual-order) — these
// drive Go's synthetic-PDF extraction internals and are covered by this lib's
// own text_extractor spec-oracle tests rather than the public TextAbsorber API.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/text_absorber.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Text::TextAbsorber;

std::string HelloPdf() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return (std::filesystem::path(env) / "hello_world.pdf").string();
    return (std::filesystem::path(__FILE__).parent_path().parent_path() /
            "fixtures" / "text_extractor" / "hello_world.pdf")
        .string();
}

}  // namespace

// TestExtractTextMinimal — per-page extraction.
TEST(GoText, ExtractTextPerPage) {
    Document doc{HelloPdf()};
    TextAbsorber absorber;
    absorber.Visit(doc.Pages()[1]);
    EXPECT_NE(absorber.Text().find("Hello"), std::string::npos);
}

// TestDocumentExtractText — whole-document extraction.
TEST(GoText, ExtractTextWholeDocument) {
    Document doc{HelloPdf()};
    TextAbsorber absorber;
    absorber.Visit(doc);
    EXPECT_NE(absorber.Text().find("Hello World"), std::string::npos);
}

// A fresh absorber yields empty text (matches Go's no-content path).
TEST(GoText, FreshAbsorberEmpty) {
    TextAbsorber absorber;
    EXPECT_TRUE(absorber.Text().empty());
}
