// =============================================================================
// facades_content_editor_delete_image_smoke_test — PdfContentEditor image
// removal (canonical DeleteImage). Embeds images, deletes them, and checks
// the per-page count drops + the change survives save/reload.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_content_editor.hpp>
#include <aspose/pdf/facades/pdf_extractor.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Facades::PdfContentEditor;
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
    return std::filesystem::temp_directory_path() / ("asp_delimg_" + tag);
}

// A real (decodable) PNG fixture from the png_decoder corpus.
std::string SamplePng() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "png_decoder" / "rgba_filter_none_8.png")
        .string();
}

// Count images via the public PdfExtractor image walk.
int CountImages(Document& doc) {
    PdfExtractor ex{doc};
    int n = 0;
    const auto sink = Tmp("imgsink.dat").string();
    while (ex.HasNextImage() && n < 100) {
        if (!ex.GetNextImage(sink)) break;
        ++n;
    }
    std::filesystem::remove(sink);
    return n;
}

}  // namespace

TEST(ContentEditorDeleteImage, DeleteOneReducesCount) {
    Document doc{HelloWorldPdf()};
    doc.Pages()[1].AddImage(SamplePng(), Rectangle(50, 50, 150, 150, false));
    doc.Pages()[1].AddImage(SamplePng(), Rectangle(200, 50, 300, 150, false));
    ASSERT_EQ(CountImages(doc), 2);

    PdfContentEditor editor{doc};
    editor.DeleteImage(1, std::vector<int>{1});  // remove the first image
    EXPECT_EQ(CountImages(doc), 1);

}

TEST(ContentEditorDeleteImage, DeleteAllNoArg) {
    Document doc{HelloWorldPdf()};
    doc.Pages()[1].AddImage(SamplePng(), Rectangle(50, 50, 150, 150, false));
    doc.Pages()[1].AddImage(SamplePng(), Rectangle(200, 50, 300, 150, false));
    ASSERT_EQ(CountImages(doc), 2);

    PdfContentEditor editor{doc};
    editor.DeleteImage();  // every image on every page
    EXPECT_EQ(CountImages(doc), 0);

}

TEST(ContentEditorDeleteImage, DeletePersistsAcrossReload) {
    const auto out = Tmp("out.pdf");

    {
        Document doc{HelloWorldPdf()};
        doc.Pages()[1].AddImage(SamplePng(),
                                Rectangle(50, 50, 150, 150, false));
        doc.Pages()[1].AddImage(SamplePng(),
                                Rectangle(200, 50, 300, 150, false));
        PdfContentEditor editor{doc};
        editor.DeleteImage(1, std::vector<int>{2});  // remove the second
        doc.Save(out.string());
    }

    Document re{out.string()};
    EXPECT_EQ(CountImages(re), 1);

    std::filesystem::remove(out);
}
