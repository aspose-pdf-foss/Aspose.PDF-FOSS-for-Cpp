// =============================================================================
// facades_pdf_file_info_smoke_test — beat Fa1 of the Facades cluster.
// PdfFileInfo extends SaveableFacade and wraps /Info dict access +
// file-level metadata (page count, encryption status, version, etc.).
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_file_info.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/password_type.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::string EncryptedPdf(const std::string& name) {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "encrypt_parser" / name).string();
}

}  // namespace

TEST(FacadesPdfFileInfoSmoke, DefaultsAndEnumValues) {
    EXPECT_EQ(static_cast<int>(PasswordType::None),         0);
    EXPECT_EQ(static_cast<int>(PasswordType::User),         1);
    EXPECT_EQ(static_cast<int>(PasswordType::Owner),        2);
    EXPECT_EQ(static_cast<int>(PasswordType::Inaccessible), 3);

    PdfFileInfo info;
    EXPECT_FALSE(info.IsPdfFile());
    EXPECT_FALSE(info.IsEncrypted());
    EXPECT_EQ(info.NumberOfPages(), 0);
    EXPECT_EQ(info.PasswordType(),  PasswordType::None);
    EXPECT_FALSE(info.HasOpenPassword());
    EXPECT_FALSE(info.HasEditPassword());
    EXPECT_FALSE(info.HasCollection());
    EXPECT_FALSE(info.UseStrictValidation());
}

TEST(FacadesPdfFileInfoSmoke, BindByFileLoadsMetadata) {
    PdfFileInfo info{HelloWorldPdf()};
    EXPECT_TRUE(info.IsPdfFile());
    EXPECT_EQ(info.InputFile(), HelloWorldPdf());
    EXPECT_GT(info.NumberOfPages(), 0);
    EXPECT_EQ(info.GetPdfVersion(), "1.4");

    // hello_world.pdf is a single US-Letter page.
    EXPECT_FLOAT_EQ(info.GetPageWidth(1),     612.0f);
    EXPECT_FLOAT_EQ(info.GetPageHeight(1),    792.0f);
    EXPECT_FLOAT_EQ(info.GetPageRotation(1),   0.0f);
}

TEST(FacadesPdfFileInfoSmoke, BindByDocumentAndInfoRoundtrip) {
    Document doc{HelloWorldPdf()};
    PdfFileInfo info{doc};

    info.Title("Hello PDF");
    info.Author("Jane Doe");
    info.Subject("Greeting");
    info.Keywords("hello,pdf,test");
    info.Creator("LibForge cpp smoke");
    info.CreationDate("D:20260530000000Z");
    info.ModDate("D:20260530000000Z");

    EXPECT_EQ(info.Title(),        "Hello PDF");
    EXPECT_EQ(info.Author(),       "Jane Doe");
    EXPECT_EQ(info.Subject(),      "Greeting");
    EXPECT_EQ(info.Keywords(),     "hello,pdf,test");
    EXPECT_EQ(info.Creator(),      "LibForge cpp smoke");
    EXPECT_EQ(info.CreationDate(), "D:20260530000000Z");
    EXPECT_EQ(info.ModDate(),      "D:20260530000000Z");
}

TEST(FacadesPdfFileInfoSmoke, GenericGetSetMetaInfo) {
    Document doc{HelloWorldPdf()};
    PdfFileInfo info{doc};
    info.SetMetaInfo("CustomKey", "CustomValue");
    EXPECT_EQ(info.GetMetaInfo("CustomKey"), "CustomValue");
    EXPECT_EQ(info.GetMetaInfo("Missing"),   "");
}

TEST(FacadesPdfFileInfoSmoke, SaveNewInfoWritesUnchangedDocumentVerbatim) {
    // hello_world.pdf has no /Info dict; SaveNewInfo without
    // /Info-dict mutations falls through to Document::Save's
    // fast-path (byte-identical copy). Edits on such a file are
    // covered by info_without_info_dict_smoke_test.
    PdfFileInfo info{HelloWorldPdf()};
    const std::string out =
        (std::filesystem::temp_directory_path() /
         "aspose_facade_filenfo_save.pdf").string();
    EXPECT_TRUE(info.SaveNewInfo(out));
    EXPECT_TRUE(std::filesystem::exists(out));
}

TEST(FacadesPdfFileInfoSmoke, ClearInfoResetsTitle) {
    Document doc{HelloWorldPdf()};
    PdfFileInfo info{doc};
    info.Title("Will be cleared");
    EXPECT_EQ(info.Title(), "Will be cleared");
    // ClearInfo wraps DocumentInfo::Clear(); reading back goes
    // through the same DocumentInfo accessor — after clear,
    // Title is empty.
    info.ClearInfo();
    EXPECT_TRUE(info.Title().empty());
}

TEST(FacadesPdfFileInfoSmoke, IsPdfFileFollowsBinding) {
    PdfFileInfo info;
    EXPECT_FALSE(info.IsPdfFile());
    info.BindPdf(HelloWorldPdf());
    EXPECT_TRUE(info.IsPdfFile());
    info.Close();
    EXPECT_FALSE(info.IsPdfFile());
}

TEST(FacadesPdfFileInfoSmoke, UseStrictValidationFlag) {
    PdfFileInfo info;
    EXPECT_FALSE(info.UseStrictValidation());
    info.UseStrictValidation(true);
    EXPECT_TRUE(info.UseStrictValidation());
}

TEST(FacadesPdfFileInfoSmoke, PageGeometryReflectsPageBoxes) {
    Document doc;
    Page p1 = doc.Pages().Add();
    p1.MediaBox(Rectangle(0, 0, 595, 842, true));
    p1.Rotate(Rotation::on90);
    Page p2 = doc.Pages().Add();
    p2.MediaBox(Rectangle(10, 20, 310, 420, true));

    PdfFileInfo info{doc};
    EXPECT_FLOAT_EQ(info.GetPageWidth(1),    595.0f);
    EXPECT_FLOAT_EQ(info.GetPageHeight(1),   842.0f);
    EXPECT_FLOAT_EQ(info.GetPageRotation(1),  90.0f);
    EXPECT_FLOAT_EQ(info.GetPageXOffset(1),    0.0f);
    EXPECT_FLOAT_EQ(info.GetPageYOffset(1),    0.0f);

    EXPECT_FLOAT_EQ(info.GetPageWidth(2),    300.0f);
    EXPECT_FLOAT_EQ(info.GetPageHeight(2),   400.0f);
    EXPECT_FLOAT_EQ(info.GetPageRotation(2),   0.0f);
    EXPECT_FLOAT_EQ(info.GetPageXOffset(2),   10.0f);
    EXPECT_FLOAT_EQ(info.GetPageYOffset(2),   20.0f);
}

TEST(FacadesPdfFileInfoSmoke, PageGeometrySurvivesSaveAndReopen) {
    const std::string out =
        (std::filesystem::temp_directory_path() /
         "aspose_facade_fileinfo_geometry.pdf").string();
    {
        Document doc;
        Page p = doc.Pages().Add();
        p.MediaBox(Rectangle(0, 0, 595, 842, true));
        p.Rotate(Rotation::on270);
        doc.Save(out);
    }
    {
        PdfFileInfo info{out};
        EXPECT_FLOAT_EQ(info.GetPageWidth(1),    595.0f);
        EXPECT_FLOAT_EQ(info.GetPageHeight(1),   842.0f);
        EXPECT_FLOAT_EQ(info.GetPageRotation(1), 270.0f);
    }
    std::filesystem::remove(out);
}

TEST(FacadesPdfFileInfoSmoke, PageGeometryOutOfRangeThrows) {
    PdfFileInfo info{HelloWorldPdf()};
    EXPECT_THROW(info.GetPageWidth(2), std::out_of_range);
    EXPECT_THROW(info.GetPageWidth(0), std::out_of_range);
}

TEST(FacadesPdfFileInfoSmoke, UnboundPageGeometryIsZero) {
    PdfFileInfo info;
    EXPECT_FLOAT_EQ(info.GetPageWidth(1),    0.0f);
    EXPECT_FLOAT_EQ(info.GetPageHeight(1),   0.0f);
    EXPECT_FLOAT_EQ(info.GetPageRotation(1), 0.0f);
}

TEST(FacadesPdfFileInfoSmoke, PasswordCtorOpensEncryptedFile) {
    PdfFileInfo info{EncryptedPdf("v2_r3_rc128.pdf"), "user"};
    EXPECT_TRUE(info.IsPdfFile());
    EXPECT_TRUE(info.IsEncrypted());
    EXPECT_GT(info.NumberOfPages(), 0);
}

TEST(FacadesPdfFileInfoSmoke, PasswordCtorWrongPasswordThrows) {
    EXPECT_THROW(PdfFileInfo(EncryptedPdf("v2_r3_rc128.pdf"), "wrong"),
                 std::runtime_error);
}
