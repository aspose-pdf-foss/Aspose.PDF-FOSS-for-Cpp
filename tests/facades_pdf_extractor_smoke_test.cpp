// =============================================================================
// facades_pdf_extractor_smoke_test — beat Fa3 of the Facades cluster.
// PdfExtractor extends Facade and extracts text / images / attachments.
// v1 text extraction is REAL (wired through the foundation TextAbsorber,
// honouring StartPage/EndPage); image + attachment extraction are stubs.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_extractor.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path() / "fixtures";
}

std::string TwoPagesPdf() {
    return (FixtureRoot() / "text_extractor" / "two_pages.pdf").string();
}

std::string ReadFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string TempOut(const std::string& name) {
    return (std::filesystem::temp_directory_path() / name).string();
}

}  // namespace

TEST(FacadesPdfExtractorSmoke, PropertyDefaultsAndRoundtrip) {
    PdfExtractor ex;
    EXPECT_EQ(ex.StartPage(), 1);
    EXPECT_EQ(ex.EndPage(), 0);
    EXPECT_EQ(ex.ExtractTextMode(), 0);
    EXPECT_EQ(ex.Resolution(), 150);
    EXPECT_TRUE(ex.Password().empty());
    EXPECT_FALSE(ex.IsBidi());

    ex.StartPage(2);
    ex.EndPage(5);
    ex.ExtractTextMode(1);
    ex.Resolution(300);
    ex.Password("secret");
    EXPECT_EQ(ex.StartPage(), 2);
    EXPECT_EQ(ex.EndPage(), 5);
    EXPECT_EQ(ex.ExtractTextMode(), 1);
    EXPECT_EQ(ex.Resolution(), 300);
    EXPECT_EQ(ex.Password(), "secret");
}

TEST(FacadesPdfExtractorSmoke, BindAndExtractText) {
    Document doc{TwoPagesPdf()};
    PdfExtractor ex{doc};
    ex.ExtractText();

    const std::string out = TempOut("aspose_extractor_all.txt");
    ex.GetText(out);
    ASSERT_TRUE(std::filesystem::exists(out));
    const std::string text = ReadFile(out);
    EXPECT_NE(text.find("Page one"), std::string::npos);
    EXPECT_NE(text.find("Page two"), std::string::npos);
}

TEST(FacadesPdfExtractorSmoke, GetTextLazilyExtracts) {
    // GetText without a prior ExtractText() still works — extraction
    // runs lazily on first read.
    PdfExtractor ex;
    ex.BindPdf(TwoPagesPdf());
    const std::string out = TempOut("aspose_extractor_lazy.txt");
    ex.GetText(out);
    const std::string text = ReadFile(out);
    EXPECT_NE(text.find("Page one"), std::string::npos);
    EXPECT_NE(text.find("Page two"), std::string::npos);
}

TEST(FacadesPdfExtractorSmoke, PageByPageCursor) {
    Document doc{TwoPagesPdf()};
    PdfExtractor ex{doc};

    ASSERT_TRUE(ex.HasNextPageText());
    const std::string p1 = TempOut("aspose_extractor_p1.txt");
    ex.GetNextPageText(p1);
    EXPECT_NE(ReadFile(p1).find("Page one"), std::string::npos);

    ASSERT_TRUE(ex.HasNextPageText());
    const std::string p2 = TempOut("aspose_extractor_p2.txt");
    ex.GetNextPageText(p2);
    EXPECT_NE(ReadFile(p2).find("Page two"), std::string::npos);

    EXPECT_FALSE(ex.HasNextPageText());
}

TEST(FacadesPdfExtractorSmoke, StartPageEndPageRange) {
    Document doc{TwoPagesPdf()};
    PdfExtractor ex{doc};
    ex.StartPage(2);  // page 2 only — resets the extraction pass
    ex.ExtractText();

    const std::string out = TempOut("aspose_extractor_range.txt");
    ex.GetText(out);
    const std::string text = ReadFile(out);
    EXPECT_EQ(text.find("Page one"), std::string::npos);
    EXPECT_NE(text.find("Page two"), std::string::npos);
}

TEST(FacadesPdfExtractorSmoke, ImageAndAttachmentStubs) {
    Document doc{TwoPagesPdf()};
    PdfExtractor ex{doc};

    // Image extraction — v1 stubs: no images surfaced.
    EXPECT_FALSE(ex.HasNextImage());
    EXPECT_FALSE(ex.GetNextImage(TempOut("aspose_extractor_img.bin")));
    ex.ExtractImage();  // no-op, must not throw

    // Attachment extraction — v1 stubs: empty.
    EXPECT_TRUE(ex.GetAttachNames().empty());
    EXPECT_TRUE(ex.GetAttachmentInfo().empty());
    ex.ExtractAttachment();                 // no-op
    ex.ExtractAttachment("anything.dat");   // no-op
    ex.GetAttachment(TempOut("aspose_extractor_att.bin"));  // no-op
}
