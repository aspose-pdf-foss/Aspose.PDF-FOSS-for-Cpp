// =============================================================================
// facades_pdf_file_stamp_smoke_test — beat Fa13 of the Facades cluster.
// PdfFileStamp stamps headers / footers / page numbers onto a PDF. v1
// baseline: the content-stamping render path is deferred, so the Add*
// operations are stubs (the bound document saves through unchanged);
// the 8 position constants + properties are real.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_file_stamp.hpp>
#include <aspose/pdf/pdf_format.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::string TempPath(const std::string& name) {
    return (std::filesystem::temp_directory_path() /
            ("aspose_filestamp_" + name)).string();
}

}  // namespace

TEST(FacadesPdfFileStampSmoke, PositionConstants) {
    EXPECT_EQ(PdfFileStamp::PosBottomMiddle, 0);
    EXPECT_EQ(PdfFileStamp::PosBottomRight, 1);
    EXPECT_EQ(PdfFileStamp::PosUpperRight, 2);
    EXPECT_EQ(PdfFileStamp::PosSidesRight, 3);
    EXPECT_EQ(PdfFileStamp::PosUpperMiddle, 4);
    EXPECT_EQ(PdfFileStamp::PosBottomLeft, 5);
    EXPECT_EQ(PdfFileStamp::PosSidesLeft, 6);
    EXPECT_EQ(PdfFileStamp::PosUpperLeft, 7);
}

TEST(FacadesPdfFileStampSmoke, Defaults) {
    PdfFileStamp st;
    EXPECT_FALSE(st.OptimizeSize());
    EXPECT_FALSE(st.KeepSecurity());
    EXPECT_TRUE(st.InputFile().empty());
    EXPECT_TRUE(st.OutputFile().empty());
    EXPECT_FLOAT_EQ(st.PageNumberRotation(), 0.0f);
    EXPECT_EQ(st.StartingNumber(), 1);
    EXPECT_EQ(st.StampId(), 0);
    EXPECT_FLOAT_EQ(st.PageHeight(), 792.0f);
    EXPECT_FLOAT_EQ(st.PageWidth(), 612.0f);
}

TEST(FacadesPdfFileStampSmoke, PropertyRoundtrip) {
    PdfFileStamp st;
    st.OptimizeSize(true);
    st.KeepSecurity(true);
    st.InputFile("in.pdf");
    st.OutputFile("out.pdf");
    st.PageNumberRotation(90.0f);
    st.StartingNumber(5);
    st.StampId(42);
    st.ConvertTo(PdfFormat::PDF_A_1B);

    EXPECT_TRUE(st.OptimizeSize());
    EXPECT_TRUE(st.KeepSecurity());
    EXPECT_EQ(st.InputFile(), "in.pdf");
    EXPECT_EQ(st.OutputFile(), "out.pdf");
    EXPECT_FLOAT_EQ(st.PageNumberRotation(), 90.0f);
    EXPECT_EQ(st.StartingNumber(), 5);
    EXPECT_EQ(st.StampId(), 42);
}

TEST(FacadesPdfFileStampSmoke, StampStubsDoNotThrowAndSavePassesThrough) {
    Document doc{HelloWorldPdf()};
    PdfFileStamp st{doc};

    st.AddPageNumber("Page #");
    st.AddPageNumber("Page #", 1);
    st.AddPageNumber("Page #", 10.0f, 20.0f);
    st.AddPageNumber("Page #", 1, 0, 0, 100, 20);
    st.AddHeader("Header", 36.0f);
    st.AddHeader("Header", 36.0f, 36.0f, 36.0f);
    st.AddFooter("Footer", 36.0f);
    st.AddFooter("Footer", 36.0f, 36.0f, 36.0f);

    const std::string out = TempPath("save.pdf");
    st.Save(out);  // inherited SaveableFacade::Save — passthrough
    EXPECT_TRUE(std::filesystem::exists(out));
    std::filesystem::remove(out);
}

TEST(FacadesPdfFileStampSmoke, FileCtorBinds) {
    const std::string out = TempPath("filector.pdf");
    PdfFileStamp st{HelloWorldPdf(), out, /*keepSecurity=*/true};
    EXPECT_EQ(st.OutputFile(), out);
    EXPECT_TRUE(st.KeepSecurity());
    st.Save(out);
    EXPECT_TRUE(std::filesystem::exists(out));
    std::filesystem::remove(out);
}
