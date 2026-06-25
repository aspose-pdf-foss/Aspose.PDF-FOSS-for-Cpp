// =============================================================================
// facades_pdf_file_editor_ops_smoke_test — PdfFileEditor real file ops
// (parity gap 3). Concatenate / Append / Insert deep-import the source
// pages' object graph (Document::ImportPagesFrom); Extract / Delete /
// Split reduce to PageCollection page removal. Closes pdflib's
// PdfFileEditor concatenate / extract / delete / split parity gap.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_file_editor.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/text_absorber.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

std::string HelloWorldPdf() {
    return (std::filesystem::path(__FILE__).parent_path().parent_path() /
            "pdfs" / "hello_world.pdf").string();
}
std::string TwoPagesPdf() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "text_extractor" / "two_pages.pdf").string();
}
std::string Tmp(const std::string& n) {
    return (std::filesystem::temp_directory_path() /
            ("aspose_fileeditor_" + n)).string();
}
std::string DocText(const std::string& path) {
    Document d{path};
    Aspose::Pdf::Text::TextAbsorber abs;
    abs.Visit(d);
    return abs.Text();
}

}  // namespace

TEST(PdfFileEditorOpsSmoke, ConcatenateTwoFiles) {
    const std::string out = Tmp("concat.pdf");
    PdfFileEditor ed;
    ASSERT_TRUE(ed.Concatenate(HelloWorldPdf(), TwoPagesPdf(), out));
    Document re{out};
    EXPECT_EQ(re.Pages().Count(), 3u);  // 1 + 2
    const std::string text = DocText(out);
    EXPECT_NE(text.find("Hello World"), std::string::npos);
    EXPECT_NE(text.find("Page one"), std::string::npos);
    EXPECT_NE(text.find("Page two"), std::string::npos);
    std::filesystem::remove(out);
}

TEST(PdfFileEditorOpsSmoke, ConcatenateManyFiles) {
    const std::string out = Tmp("concat_many.pdf");
    PdfFileEditor ed;
    ASSERT_TRUE(ed.Concatenate(
        std::vector<std::string>{HelloWorldPdf(), TwoPagesPdf(),
                                 HelloWorldPdf()},
        out));
    Document re{out};
    EXPECT_EQ(re.Pages().Count(), 4u);  // 1 + 2 + 1
    std::filesystem::remove(out);
}

TEST(PdfFileEditorOpsSmoke, ExtractPageRange) {
    const std::string out = Tmp("extract.pdf");
    PdfFileEditor ed;
    // Keep only page 2 of two_pages ("Page two").
    ASSERT_TRUE(ed.Extract(TwoPagesPdf(), 2, 2, out));
    Document re{out};
    EXPECT_EQ(re.Pages().Count(), 1u);
    EXPECT_NE(DocText(out).find("Page two"), std::string::npos);
    EXPECT_EQ(DocText(out).find("Page one"), std::string::npos);
    std::filesystem::remove(out);
}

TEST(PdfFileEditorOpsSmoke, DeletePages) {
    const std::string out = Tmp("delete.pdf");
    PdfFileEditor ed;
    ASSERT_TRUE(ed.Delete(TwoPagesPdf(), std::vector<int>{1}, out));
    Document re{out};
    EXPECT_EQ(re.Pages().Count(), 1u);
    EXPECT_NE(DocText(out).find("Page two"), std::string::npos);
    std::filesystem::remove(out);
}

TEST(PdfFileEditorOpsSmoke, AppendRange) {
    const std::string out = Tmp("append.pdf");
    PdfFileEditor ed;
    // Append page 1 ("Page one") of two_pages onto hello_world.
    ASSERT_TRUE(ed.Append(HelloWorldPdf(),
                          std::vector<std::string>{TwoPagesPdf()}, 1, 1,
                          out));
    Document re{out};
    EXPECT_EQ(re.Pages().Count(), 2u);  // 1 + 1
    const std::string text = DocText(out);
    EXPECT_NE(text.find("Hello World"), std::string::npos);
    EXPECT_NE(text.find("Page one"), std::string::npos);
    std::filesystem::remove(out);
}

TEST(PdfFileEditorOpsSmoke, SplitToPages) {
    // Copy two_pages to a temp so split files land in temp dir.
    const std::string src = Tmp("split_src.pdf");
    std::filesystem::copy_file(
        TwoPagesPdf(), src,
        std::filesystem::copy_options::overwrite_existing);
    PdfFileEditor ed;
    std::vector<std::string> files = ed.SplitToPages(src);
    ASSERT_EQ(files.size(), 2u);
    for (const auto& f : files) {
        EXPECT_TRUE(std::filesystem::exists(f));
        Document d{f};
        EXPECT_EQ(d.Pages().Count(), 1u);
        std::filesystem::remove(f);
    }
    std::filesystem::remove(src);
}
