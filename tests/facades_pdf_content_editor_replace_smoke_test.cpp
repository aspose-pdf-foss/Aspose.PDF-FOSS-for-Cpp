// =============================================================================
// facades_pdf_content_editor_replace_smoke_test — PdfContentEditor real
// text replace (parity gap 5). ReplaceText decodes the page content
// stream, substitutes the literal (src) text operand with (dest), and
// rewrites it uncompressed. Closes pdflib's replaceAll/replaceFirst gap.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_content_editor.hpp>
#include <aspose/pdf/text_absorber.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

std::string TwoPagesPdf() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "text_extractor" / "two_pages.pdf").string();
}
std::string Tmp(const std::string& n) {
    return (std::filesystem::temp_directory_path() /
            ("aspose_replace_" + n)).string();
}
std::string DocText(const std::string& path) {
    Document d{path};
    Aspose::Pdf::Text::TextAbsorber abs;
    abs.Visit(d);
    return abs.Text();
}

}  // namespace

TEST(PdfContentEditorReplaceSmoke, ReplaceAllPages) {
    const std::string out = Tmp("all.pdf");
    bool ok = false;
    {
        Document doc{TwoPagesPdf()};
        PdfContentEditor ed{doc};
        ok = ed.ReplaceText("Page one", "REPLACED");
        ed.Save(out);
    }
    EXPECT_TRUE(ok);
    const std::string text = DocText(out);
    EXPECT_NE(text.find("REPLACED"), std::string::npos);
    EXPECT_EQ(text.find("Page one"), std::string::npos);
    // The other page is untouched.
    EXPECT_NE(text.find("Page two"), std::string::npos);
    std::filesystem::remove(out);
}

TEST(PdfContentEditorReplaceSmoke, ReplaceMissingReturnsFalse) {
    Document doc{TwoPagesPdf()};
    PdfContentEditor ed{doc};
    EXPECT_FALSE(ed.ReplaceText("NoSuchString12345", "X"));
}

TEST(PdfContentEditorReplaceSmoke, ReplaceOnSpecificPage) {
    const std::string out = Tmp("page2.pdf");
    {
        Document doc{TwoPagesPdf()};
        PdfContentEditor ed{doc};
        // Replace on page 2 only ("Page two").
        EXPECT_TRUE(ed.ReplaceText("Page two", "SECOND", 2));
        ed.Save(out);
    }
    const std::string text = DocText(out);
    EXPECT_NE(text.find("SECOND"), std::string::npos);
    EXPECT_EQ(text.find("Page two"), std::string::npos);
    EXPECT_NE(text.find("Page one"), std::string::npos);
    std::filesystem::remove(out);
}
