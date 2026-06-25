// =============================================================================
// facades_pdf_file_stamp_draw_smoke_test — PdfFileStamp real headers /
// footers / page numbers (parity gap 7). AddHeader/AddFooter/
// AddPageNumber draw Helvetica text into each page's content stream via
// Document::ApplyTextStamps; the drawn text is extractable after Save.
// Closes pdflib's header/footer rendering parity gap.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_file_stamp.hpp>
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
            ("aspose_stampdraw_" + n)).string();
}
std::string DocText(const std::string& path) {
    Document d{path};
    Aspose::Pdf::Text::TextAbsorber abs;
    abs.Visit(d);
    return abs.Text();
}

}  // namespace

TEST(PdfFileStampDrawSmoke, HeaderAppearsInExtractedText) {
    const std::string out = Tmp("header.pdf");
    {
        Document doc{TwoPagesPdf()};
        PdfFileStamp st{doc};
        st.AddHeader("CONFIDENTIAL", 36.0f);
        st.Save(out);
    }
    EXPECT_NE(DocText(out).find("CONFIDENTIAL"), std::string::npos);
    std::filesystem::remove(out);
}

TEST(PdfFileStampDrawSmoke, FooterAppearsInExtractedText) {
    const std::string out = Tmp("footer.pdf");
    {
        Document doc{TwoPagesPdf()};
        PdfFileStamp st{doc};
        st.AddFooter("Draft v2", 36.0f);
        st.Save(out);
    }
    EXPECT_NE(DocText(out).find("Draft v2"), std::string::npos);
    std::filesystem::remove(out);
}

TEST(PdfFileStampDrawSmoke, PageNumbersPerPage) {
    const std::string out = Tmp("pagenum.pdf");
    {
        Document doc{TwoPagesPdf()};
        PdfFileStamp st{doc};
        st.AddPageNumber("Page #");
        st.Save(out);
    }
    const std::string text = DocText(out);
    EXPECT_NE(text.find("Page 1"), std::string::npos);
    EXPECT_NE(text.find("Page 2"), std::string::npos);
    // Original page content survives.
    EXPECT_NE(text.find("Page one"), std::string::npos);
    std::filesystem::remove(out);
}

TEST(PdfFileStampDrawSmoke, StartingNumberHonored) {
    const std::string out = Tmp("startnum.pdf");
    {
        Document doc{TwoPagesPdf()};
        PdfFileStamp st{doc};
        st.AddPageNumber("#", 100);
        st.Save(out);
    }
    const std::string text = DocText(out);
    EXPECT_NE(text.find("100"), std::string::npos);
    EXPECT_NE(text.find("101"), std::string::npos);
    std::filesystem::remove(out);
}
