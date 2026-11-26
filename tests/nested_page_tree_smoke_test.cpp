// =============================================================================
// nested_page_tree_smoke_test — adding, deleting and importing pages in a
// document whose /Pages tree is nested (an intermediate /Pages node carrying
// an inherited MediaBox): the tree is flattened first, the inherited
// attributes pinned on the pages.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_file_editor.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using flatten_test::TmpPath;

// Pages: 1, 2 under an intermediate node (inherited 200 x 200 media box),
// 3 directly under the root (300 x 300).
std::string NestedPdf(const std::string& tag) {
    const auto path = TmpPath(tag);
    flatten_test::WritePdf(path, {
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [3 0 R 6 0 R] /Count 3 >>",
        "<< /Type /Pages /Parent 2 0 R /Kids [4 0 R 5 0 R] /Count 2 /MediaBox [0 0 200 200] >>",
        "<< /Type /Page /Parent 3 0 R >>",
        "<< /Type /Page /Parent 3 0 R >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 300 300] >>",
    });
    return path;
}

std::vector<double> Widths(Document& doc) {
    std::vector<double> out;
    for (int i = 1; i <= static_cast<int>(doc.Pages().Count()); ++i)
        out.push_back(doc.Pages()[i].Rect().Width());
    return out;
}

}  // namespace

TEST(NestedPageTree, AddPage) {
    const auto out = TmpPath("nested_add_out");
    {
        Document doc{NestedPdf("nested_add")};
        doc.Pages().Add();
        doc.Save(out);
    }
    Document doc{out};
    ASSERT_EQ(doc.Pages().Count(), 4);
    const auto w = Widths(doc);
    EXPECT_DOUBLE_EQ(w[0], 200.0);  // inherited media box kept
    EXPECT_DOUBLE_EQ(w[1], 200.0);
    EXPECT_DOUBLE_EQ(w[2], 300.0);
}

TEST(NestedPageTree, DeletePage) {
    const auto out = TmpPath("nested_delete_out");
    {
        Document doc{NestedPdf("nested_delete")};
        doc.Pages().Delete(1);
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(Widths(doc), (std::vector<double>{200.0, 300.0}));
}

TEST(NestedPageTree, AppendIntoNestedDocument) {
    const auto out = TmpPath("nested_append_out");
    Facades::PdfFileEditor editor;
    ASSERT_TRUE(editor.Append(NestedPdf("nested_append"), {flatten_test::HelloWorldPdf()}, 1, 1, out));
    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), 4);
    EXPECT_DOUBLE_EQ(Widths(doc)[1], 200.0);
}
