// =============================================================================
// Go-parity: redact_apply_test.go / redact_apply_integration_test.go →
// canonical C++ RedactionAnnotation::Redact() (Go's doc.ApplyRedactions applies
// every redaction annotation; the canonical analogue is Redact() per
// annotation).
//
// Ported (canonical): redacted text is removed post-apply, text outside the
// region is preserved, OverlayText is painted (and extractable), no-overlay
// redaction still removes the text.
// Skipped: ValidateRedactions (invented linter), ApplyRedactions-removes-the-
// annotation (annotation excision beyond content removal not implemented),
// RegenerateAppearance (Go-internal), empty/stub and WriteTo-stream tests.
// Note: removal is whole-text-show granularity, so "preserved" text must live
// outside the region (not merely a different word in the same show).
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/redaction_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/text_absorber.hpp>
#include <aspose/pdf/text_fragment.hpp>
#include <aspose/pdf/text_fragment_absorber.hpp>
#include <aspose/pdf/text_fragment_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Annotations::RedactionAnnotation;
using Aspose::Pdf::Text::TextAbsorber;
using Aspose::Pdf::Text::TextFragmentAbsorber;

std::string HelloPdf() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return (std::filesystem::path(env) / "hello_world.pdf").string();
    return (std::filesystem::path(__FILE__).parent_path().parent_path() /
            "fixtures" / "text_extractor" / "hello_world.pdf")
        .string();
}
std::string TmpOut(const std::string& tag) {
    return (std::filesystem::temp_directory_path() / ("go_redact_" + tag + ".pdf"))
        .string();
}
Rectangle FirstTextBox(Page& page) {
    TextFragmentAbsorber finder;
    finder.Visit(page);
    return finder.TextFragments()[1].Rectangle();
}
std::string ExtractedText(const std::string& path) {
    Document doc{path};
    TextAbsorber a;
    a.Visit(doc);
    return a.Text();
}

}  // namespace

// TestApplyRedactionsRemovesText / TestApplyRedactionsTextExtractionPostApply
TEST(GoRedact, RemovesText) {
    const std::string out = TmpOut("rm");
    {
        Document doc{HelloPdf()};
        Page page = doc.Pages()[1];
        RedactionAnnotation r{page, FirstTextBox(page)};
        page.Annotations().Add(r);
        r.Redact();
        doc.Save(out);
    }
    EXPECT_EQ(ExtractedText(out).find("Hello"), std::string::npos);
    std::filesystem::remove(out);
}

// TestApplyRedactionsPreservesNonRedactedText — text outside the region stays.
TEST(GoRedact, PreservesTextOutsideRegion) {
    const std::string out = TmpOut("keep");
    {
        Document doc{HelloPdf()};
        Page page = doc.Pages()[1];
        const Rectangle box = FirstTextBox(page);
        RedactionAnnotation r{
            page, Rectangle{box.URX() + 80.0, box.LLY(), box.URX() + 140.0,
                            box.URY(), true}};
        r.Redact();
        doc.Save(out);
    }
    EXPECT_NE(ExtractedText(out).find("Hello"), std::string::npos);
    std::filesystem::remove(out);
}

// TestApplyRedactionsOverlayText — overlay text is painted over the box.
TEST(GoRedact, OverlayTextAppears) {
    const std::string out = TmpOut("overlay");
    {
        Document doc{HelloPdf()};
        Page page = doc.Pages()[1];
        RedactionAnnotation r{page, FirstTextBox(page)};
        r.OverlayText("CLASSIFIED");
        r.Redact();
        doc.Save(out);
    }
    const std::string text = ExtractedText(out);
    EXPECT_EQ(text.find("Hello"), std::string::npos);       // original removed
    EXPECT_NE(text.find("CLASSIFIED"), std::string::npos);  // overlay painted
    std::filesystem::remove(out);
}

// TestApplyRedactionsNoOverlayKeepsFill — no overlay still removes the text.
TEST(GoRedact, NoOverlayRemovesTextNoExtraText) {
    const std::string out = TmpOut("nofill");
    {
        Document doc{HelloPdf()};
        Page page = doc.Pages()[1];
        RedactionAnnotation r{page, FirstTextBox(page)};
        r.Redact();  // no OverlayText
        doc.Save(out);
    }
    const std::string text = ExtractedText(out);
    EXPECT_EQ(text.find("Hello"), std::string::npos);
    EXPECT_EQ(text.find("CLASSIFIED"), std::string::npos);
    std::filesystem::remove(out);
}
