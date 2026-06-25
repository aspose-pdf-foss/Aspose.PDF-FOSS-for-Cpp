// =============================================================================
// page_collection_mutation_smoke_test — PageCollection Add / Insert /
// Delete (page-tree mutation). Each call rewrites the /Pages /Kids +
// /Count via incremental update and re-parses the tree, so Count()
// reflects the change immediately and persists through Save. Closes the
// pdflib Pages::add / remove / copy / move parity gap.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/text_absorber.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path Root() {
    return std::filesystem::path(__FILE__).parent_path().parent_path();
}
std::string HelloWorldPdf() {
    return (Root() / "pdfs" / "hello_world.pdf").string();
}
std::string TwoPagesPdf() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "text_extractor" / "two_pages.pdf").string();
}
std::string TempPath(const std::string& n) {
    return (std::filesystem::temp_directory_path() /
            ("aspose_pagemut_" + n)).string();
}
std::string PageText(Page& p) {
    Aspose::Pdf::Text::TextAbsorber abs;
    abs.Visit(p);
    return abs.Text();
}

}  // namespace

TEST(PageCollectionMutationSmoke, AddBlankSaveReload) {
    const std::string out = TempPath("add.pdf");
    {
        Document doc{HelloWorldPdf()};
        ASSERT_EQ(doc.Pages().Count(), 1u);
        Page added = doc.Pages().Add();
        EXPECT_EQ(doc.Pages().Count(), 2u);
        EXPECT_FLOAT_EQ(static_cast<float>(added.Rect().Width()), 612.0f);
        doc.Save(out);
    }
    Document re{out};
    EXPECT_EQ(re.Pages().Count(), 2u);
    std::filesystem::remove(out);
}

TEST(PageCollectionMutationSmoke, InsertBlankAtFront) {
    Document doc{TwoPagesPdf()};
    ASSERT_EQ(doc.Pages().Count(), 2u);
    doc.Pages().Insert(1);
    EXPECT_EQ(doc.Pages().Count(), 3u);
    // New blank page is now page 1; original page 1 ("Page one") is page 2.
    auto p2 = doc.Pages()[2];
    EXPECT_NE(PageText(p2).find("Page one"), std::string::npos);
}

TEST(PageCollectionMutationSmoke, DeletePageSaveReload) {
    const std::string out = TempPath("del.pdf");
    {
        Document doc{TwoPagesPdf()};
        ASSERT_EQ(doc.Pages().Count(), 2u);
        doc.Pages().Delete(1);  // drop "Page one"
        EXPECT_EQ(doc.Pages().Count(), 1u);
        auto p1 = doc.Pages()[1];
        EXPECT_NE(PageText(p1).find("Page two"), std::string::npos);
        doc.Save(out);
    }
    Document re{out};
    EXPECT_EQ(re.Pages().Count(), 1u);
    auto p1 = re.Pages()[1];
    EXPECT_NE(PageText(p1).find("Page two"), std::string::npos);
    std::filesystem::remove(out);
}

TEST(PageCollectionMutationSmoke, CopyPageSharesContent) {
    Document doc{TwoPagesPdf()};
    Page src = doc.Pages()[1];
    Page copy = doc.Pages().Add(src);  // append a copy of page 1
    EXPECT_EQ(doc.Pages().Count(), 3u);
    // The appended copy renders the same text as the source page.
    EXPECT_NE(PageText(copy).find("Page one"), std::string::npos);
}

TEST(PageCollectionMutationSmoke, DeleteMultipleAndAll) {
    Document doc{TwoPagesPdf()};
    doc.Pages().Add();
    doc.Pages().Add();
    ASSERT_EQ(doc.Pages().Count(), 4u);
    doc.Pages().Delete(std::vector<int>{1, 3});
    EXPECT_EQ(doc.Pages().Count(), 2u);
    doc.Pages().Delete();  // all
    EXPECT_EQ(doc.Pages().Count(), 0u);
}
