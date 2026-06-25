// =============================================================================
// redaction_apply_smoke_test — RedactionAnnotation::Redact() content removal.
// A redaction must REMOVE the underlying text (not merely cover it): after
// Redact() + Save, the redacted text is no longer extractable. Text outside the
// region survives; a Document-only annotation (no page) is a safe no-op.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

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
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "text_extractor" / "hello_world.pdf")
        .string();
}

std::string TmpOut(const std::string& tag) {
    return (std::filesystem::temp_directory_path() / ("redact_" + tag + ".pdf"))
        .string();
}

Rectangle FirstTextBox(Page& page) {
    TextFragmentAbsorber finder;  // match-all
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

// Redacting the text region removes the text from the saved document.
TEST(RedactionApplySmoke, RemovesTextInRegion) {
    const std::string out = TmpOut("removes");
    {
        Document doc{HelloPdf()};
        Page page = doc.Pages()[1];
        const Rectangle box = FirstTextBox(page);
        RedactionAnnotation r{page, box};
        r.OverlayText("REDACTED");
        r.Redact();
        doc.Save(out);
    }
    // The redacted text is no longer extractable.
    EXPECT_EQ(ExtractedText(out).find("Hello"), std::string::npos);
    std::filesystem::remove(out);
}

// A redaction region away from the text leaves the text intact.
TEST(RedactionApplySmoke, KeepsTextOutsideRegion) {
    const std::string out = TmpOut("keeps");
    {
        Document doc{HelloPdf()};
        Page page = doc.Pages()[1];
        const Rectangle box = FirstTextBox(page);
        // A region well to the right of the text — no overlap.
        RedactionAnnotation r{
            page, Rectangle{box.URX() + 100.0, box.LLY(), box.URX() + 150.0,
                            box.URY(), true}};
        r.Redact();
        doc.Save(out);
    }
    EXPECT_NE(ExtractedText(out).find("Hello"), std::string::npos);
    std::filesystem::remove(out);
}

// Redact() on a Document-bound annotation (no page) is a safe no-op.
TEST(RedactionApplySmoke, DocumentBoundNoOp) {
    Document doc{HelloPdf()};
    RedactionAnnotation r{doc};
    EXPECT_NO_THROW(r.Redact());
}
