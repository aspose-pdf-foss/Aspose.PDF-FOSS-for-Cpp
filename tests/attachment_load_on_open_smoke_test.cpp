// =============================================================================
// attachment_load_on_open_smoke_test — Document::EmbeddedFiles() reads the
// catalog's /Names /EmbeddedFiles name tree back (load-on-open), and
// PdfExtractor extracts the embedded bytes. Verifies attach → save →
// reopen → read-back + extract, delete-on-disk, and lossless re-save.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/embedded_file_collection.hpp>
#include <aspose/pdf/facades/pdf_extractor.hpp>
#include <aspose/pdf/file_specification.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Facades::PdfExtractor;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::filesystem::path Tmp(const std::string& tag) {
    return std::filesystem::temp_directory_path() / ("asp_att_loo_" + tag);
}

void WriteFile(const std::filesystem::path& p, const std::string& data) {
    std::ofstream(p, std::ios::binary).write(data.data(),
                                             static_cast<std::streamsize>(
                                                 data.size()));
}

std::string ReadFile(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in),
                       std::istreambuf_iterator<char>());
}

// Build a PDF with one embedded attachment (payload bytes) at `out`.
void MakeAttachedPdf(const std::filesystem::path& out,
                     const std::filesystem::path& src,
                     const std::string& payload) {
    WriteFile(src, payload);
    Document doc{HelloWorldPdf()};
    FileSpecification fs(src.string(), "a note");
    doc.EmbeddedFiles().Add("payload.bin", fs);
    doc.Save(out.string());
}

}  // namespace

TEST(AttachmentLoadOnOpen, RoundTripCountAndExtract) {
    const auto src = Tmp("src.bin");
    const auto out = Tmp("attached.pdf");
    const std::string payload = "embedded attachment payload \x01\x02\xFF end";
    MakeAttachedPdf(out, src, payload);

    Document re{out.string()};
    ASSERT_EQ(re.EmbeddedFiles().Count(), 1);

    PdfExtractor ex{re};
    auto names = ex.GetAttachNames();
    ASSERT_EQ(names.size(), 1u);

    const auto extracted = Tmp("extracted.bin");
    ex.GetAttachment(extracted.string());  // first attachment
    EXPECT_EQ(ReadFile(extracted), payload);

    std::filesystem::remove(src);
    std::filesystem::remove(out);
    std::filesystem::remove(extracted);
}

TEST(AttachmentLoadOnOpen, DeleteAllOnDiskClears) {
    const auto src = Tmp("src2.bin");
    const auto out = Tmp("attached2.pdf");
    const auto out2 = Tmp("cleared2.pdf");
    MakeAttachedPdf(out, src, "to be removed");

    {
        Document re{out.string()};
        ASSERT_EQ(re.EmbeddedFiles().Count(), 1);
        re.EmbeddedFiles().Delete();  // remove all
        EXPECT_EQ(re.EmbeddedFiles().Count(), 0);
        re.Save(out2.string());
    }

    Document re2{out2.string()};
    EXPECT_EQ(re2.EmbeddedFiles().Count(), 0);

    std::filesystem::remove(src);
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}

TEST(AttachmentLoadOnOpen, ReadOnlyResaveIsLossless) {
    const auto src = Tmp("src3.bin");
    const auto out = Tmp("attached3.pdf");
    const auto out2 = Tmp("resaved3.pdf");
    const std::string payload = "stable payload 123";
    MakeAttachedPdf(out, src, payload);

    {
        Document re{out.string()};
        ASSERT_EQ(re.EmbeddedFiles().Count(), 1);
        re.Save(out2.string());  // no changes
    }

    Document re2{out2.string()};
    ASSERT_EQ(re2.EmbeddedFiles().Count(), 1);
    PdfExtractor ex{re2};
    const auto extracted = Tmp("extracted3.bin");
    ex.GetAttachment(extracted.string());
    EXPECT_EQ(ReadFile(extracted), payload);

    std::filesystem::remove(src);
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
    std::filesystem::remove(extracted);
}
