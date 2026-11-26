// =============================================================================
// pdf_file_editor_layout_smoke_test — PdfFileEditor page layout: booklet,
// N-up, margins, content resizing and page breaks. Source pages are
// hand-built 612 x 792 PDFs whose content is full-width black stripes at
// known heights, so a render shows where each page landed and how it was
// scaled.
// =============================================================================

#include <string>
#include <utility>
#include <vector>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/square_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_file_editor.hpp>
#include <aspose/pdf/page_size.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;
using namespace flatten_test;

namespace {

using Stripes = std::vector<std::pair<int, int>>;  // [y0, y1) per stripe

// A PDF whose page i (612 x 792) holds the black stripes pages[i].
void WriteStripes(const std::string& path, const std::vector<Stripes>& pages) {
    const int n = static_cast<int>(pages.size());
    std::string kids;
    for (int i = 0; i < n; ++i) kids += std::to_string(3 + 2 * i) + " 0 R ";
    std::vector<std::string> objs = {
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [" + kids + "] /Count " + std::to_string(n) +
            " >>"};
    std::map<int, std::string> bodies;
    for (int i = 0; i < n; ++i) {
        std::string body = "0 g\n";
        for (const auto& [y0, y1] : pages[static_cast<std::size_t>(i)])
            body += "0 " + std::to_string(y0) + " 612 " +
                    std::to_string(y1 - y0) + " re f\n";
        objs.push_back("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
                       "/Contents " + std::to_string(4 + 2 * i) + " 0 R >>");
        objs.push_back("<< /Length " + std::to_string(body.size()) + " >>");
        bodies[4 + 2 * i] = body;
    }
    WritePdf(path, objs, bodies);
}

bool Dark(const foundation::png_decoder::DecodedImage& img, double x,
          double y) {
    return CountPixels(img, x - 2, y - 2, x + 2, y + 2, 0.0,
                       [](int r, int g, int b) { return r + g + b < 200; }) > 0;
}

struct Out {
    std::string path;
    ~Out() { std::filesystem::remove(path); }
};

}  // namespace

TEST(PdfFileEditorLayout, BookletOrdersPagesForFolding) {
    const std::string src = TmpPath("booklet_src");
    Out out{TmpPath("booklet_out")};
    // Page k (1-based) holds a stripe at y in [100(k-1), 100k).
    WriteStripes(src, {{{0, 100}}, {{100, 200}}, {{200, 300}}});
    PdfFileEditor ed;
    ASSERT_TRUE(ed.MakeBooklet(src, out.path));
    Document doc{out.path};
    ASSERT_EQ(doc.Pages().Count(), 2u);  // 3 pages padded to 4: one sheet
    EXPECT_DOUBLE_EQ(doc.Pages()[1].Rect().Width(), 1224.0);
    EXPECT_DOUBLE_EQ(doc.Pages()[1].Rect().Height(), 792.0);
    // Front: [4 (blank), 1]; back: [2, 3].
    const auto front = Render(doc, 1);
    EXPECT_FALSE(Dark(front, 300, 50));   // left half: blank page 4
    EXPECT_TRUE(Dark(front, 900, 50));    // right half: page 1
    const auto back = Render(doc, 2);
    EXPECT_TRUE(Dark(back, 300, 150));    // left half: page 2
    EXPECT_TRUE(Dark(back, 900, 250));    // right half: page 3
    EXPECT_FALSE(Dark(back, 900, 150));
    std::filesystem::remove(src);
}

TEST(PdfFileEditorLayout, BookletOnGivenPageSize) {
    const std::string src = TmpPath("booklet_size_src");
    Out out{TmpPath("booklet_size_out")};
    WriteStripes(src, {{{0, 100}}, {{0, 100}}, {{0, 100}}, {{0, 100}}});
    PdfFileEditor ed;
    ASSERT_TRUE(ed.MakeBooklet(src, out.path, PageSize::A4()));
    Document doc{out.path};
    ASSERT_EQ(doc.Pages().Count(), 2u);
    EXPECT_NEAR(doc.Pages()[1].Rect().Width(), PageSize::A4().Width(), 0.01);
    EXPECT_NEAR(doc.Pages()[1].Rect().Height(), PageSize::A4().Height(), 0.01);
    std::filesystem::remove(src);
}

TEST(PdfFileEditorLayout, NUpSideBySide) {
    const std::string a = TmpPath("nup_a");
    const std::string b = TmpPath("nup_b");
    Out out{TmpPath("nup_out")};
    WriteStripes(a, {{{0, 100}}});
    WriteStripes(b, {{{300, 400}}, {{0, 100}}});
    PdfFileEditor ed;
    ASSERT_TRUE(ed.MakeNUp(a, b, out.path));
    Document doc{out.path};
    ASSERT_EQ(doc.Pages().Count(), 2u);
    EXPECT_DOUBLE_EQ(doc.Pages()[1].Rect().Width(), 1224.0);
    const auto p1 = Render(doc, 1);
    EXPECT_TRUE(Dark(p1, 300, 50));     // a, page 1
    EXPECT_TRUE(Dark(p1, 900, 350));    // b, page 1
    EXPECT_FALSE(Dark(p1, 900, 50));
    const auto p2 = Render(doc, 2);     // a has no page 2
    EXPECT_FALSE(Dark(p2, 300, 50));
    EXPECT_TRUE(Dark(p2, 900, 50));
    std::filesystem::remove(a);
    std::filesystem::remove(b);
}

TEST(PdfFileEditorLayout, NUpStacked) {
    const std::string a = TmpPath("nupv_a");
    const std::string b = TmpPath("nupv_b");
    Out out{TmpPath("nupv_out")};
    WriteStripes(a, {{{0, 100}}});
    WriteStripes(b, {{{0, 100}}});
    PdfFileEditor ed;
    ASSERT_TRUE(ed.MakeNUp({a, b}, out.path, false));
    Document doc{out.path};
    ASSERT_EQ(doc.Pages().Count(), 1u);
    EXPECT_DOUBLE_EQ(doc.Pages()[1].Rect().Height(), 1584.0);
    const auto img = Render(doc, 1);
    EXPECT_TRUE(Dark(img, 300, 792 + 50));  // a on top
    EXPECT_TRUE(Dark(img, 300, 50));        // b below
    EXPECT_FALSE(Dark(img, 300, 792 + 150));
    std::filesystem::remove(a);
    std::filesystem::remove(b);
}

TEST(PdfFileEditorLayout, AddMarginsGrowsPageAndShiftsContent) {
    const std::string src = TmpPath("margins_src");
    Out out{TmpPath("margins_out")};
    WriteStripes(src, {{{0, 100}}});
    {
        // A square annotation moves with the content.
        Document d{src};
        auto page = d.Pages()[1];
        Annotations::SquareAnnotation sq{page, Rect(10, 10, 50, 50)};
        page.Annotations().Add(sq);
        d.Save(src + ".annot.pdf");
    }
    PdfFileEditor ed;
    ASSERT_TRUE(ed.AddMargins(src + ".annot.pdf", out.path, {1}, 50, 50, 0, 100));
    Document doc{out.path};
    EXPECT_DOUBLE_EQ(doc.Pages()[1].Rect().Width(), 712.0);
    EXPECT_DOUBLE_EQ(doc.Pages()[1].Rect().Height(), 892.0);
    const auto img = Render(doc, 1);
    EXPECT_TRUE(Dark(img, 300, 150));    // stripe moved up by 100
    EXPECT_FALSE(Dark(img, 300, 50));    // new bottom margin is blank
    EXPECT_FALSE(Dark(img, 20, 150));    // new left margin is blank
    const Parsed p = ParsePage(ReadFile(out.path), 0);
    for (const auto& a : AnnotDicts(p)) {
        if (NameOf(Get(a, "Subtype")) != "Square") continue;
        const auto* r = std::get_if<fo::Array>(&Get(a, "Rect")->v);
        ASSERT_NE(r, nullptr);
        auto num = [](const fo::Value& v) {
            if (auto* d = std::get_if<double>(&v.v)) return *d;
            return static_cast<double>(std::get<std::int64_t>(v.v));
        };
        EXPECT_DOUBLE_EQ(num(r->items[0]), 60.0);
        EXPECT_DOUBLE_EQ(num(r->items[1]), 110.0);
    }
    std::filesystem::remove(src);
    std::filesystem::remove(src + ".annot.pdf");
}

TEST(PdfFileEditorLayout, AddMarginsPctOnlyListedPages) {
    const std::string src = TmpPath("marginspct_src");
    Out out{TmpPath("marginspct_out")};
    WriteStripes(src, {{{0, 100}}, {{0, 100}}});
    PdfFileEditor ed;
    ASSERT_TRUE(ed.AddMarginsPct(src, out.path, {2}, 10, 10, 10, 10));
    Document doc{out.path};
    EXPECT_DOUBLE_EQ(doc.Pages()[1].Rect().Width(), 612.0);
    EXPECT_NEAR(doc.Pages()[2].Rect().Width(), 612.0 * 1.2, 0.01);
    EXPECT_NEAR(doc.Pages()[2].Rect().Height(), 792.0 * 1.2, 0.01);
    std::filesystem::remove(src);
}

TEST(PdfFileEditorLayout, ResizeContentsPctCentresScaledContent) {
    const std::string src = TmpPath("resizepct_src");
    Out out{TmpPath("resizepct_out")};
    WriteStripes(src, {{{0, 100}}});
    PdfFileEditor ed;
    ASSERT_TRUE(ed.ResizeContentsPct(src, out.path, 50, 50));
    Document doc{out.path};
    EXPECT_DOUBLE_EQ(doc.Pages()[1].Rect().Width(), 612.0);
    // Scaled to 306 x 396 at (153, 198): the stripe is y 198..248.
    const auto img = Render(doc, 1);
    EXPECT_TRUE(Dark(img, 300, 220));
    EXPECT_FALSE(Dark(img, 50, 220));
    EXPECT_FALSE(Dark(img, 300, 50));
    std::filesystem::remove(src);
}

TEST(PdfFileEditorLayout, ResizeContentsWithParameters) {
    const std::string src = TmpPath("resize_src");
    Out out{TmpPath("resize_out")};
    WriteStripes(src, {{{0, 100}}});
    PdfFileEditor::ContentsResizeParameters params;
    params.NewPageWidth(PdfFileEditor::ContentsResizeValue::Units(400));
    params.LeftMargin(PdfFileEditor::ContentsResizeValue::Units(0));
    params.RightMargin(PdfFileEditor::ContentsResizeValue::Units(0));
    PdfFileEditor ed;
    ASSERT_TRUE(ed.ResizeContents(src, out.path, {1}, params));
    Document doc{out.path};
    EXPECT_DOUBLE_EQ(doc.Pages()[1].Rect().Width(), 400.0);
    EXPECT_DOUBLE_EQ(doc.Pages()[1].Rect().Height(), 792.0);
    const auto img = Render(doc, 1);
    EXPECT_TRUE(Dark(img, 390, 50));   // stripe squeezed to the new width
    EXPECT_FALSE(Dark(img, 390, 150));
    std::filesystem::remove(src);
}

TEST(PdfFileEditorLayout, AddPageBreakSplitsPage) {
    const std::string src = TmpPath("break_src");
    Out out{TmpPath("break_out")};
    WriteStripes(src, {{{0, 100}, {600, 700}}, {{300, 400}}});
    PdfFileEditor ed;
    ASSERT_TRUE(ed.AddPageBreak(src, out.path, {PdfFileEditor::PageBreak(1, 400)}));
    Document doc{out.path};
    ASSERT_EQ(doc.Pages().Count(), 3u);
    const auto top = Render(doc, 1);
    EXPECT_TRUE(Dark(top, 300, 650));    // upper part stays
    EXPECT_FALSE(Dark(top, 300, 50));    // lower part removed
    const auto bottom = Render(doc, 2);
    EXPECT_TRUE(Dark(bottom, 300, 392 + 50));  // lower part at the top
    EXPECT_FALSE(Dark(bottom, 300, 650));
    EXPECT_FALSE(Dark(bottom, 300, 50));
    const auto last = Render(doc, 3);    // the old page 2 follows
    EXPECT_TRUE(Dark(last, 300, 350));
    std::filesystem::remove(src);
}

TEST(PdfFileEditorLayout, TryVariantsReturnFalseOnBadInput) {
    Out out{TmpPath("try_out")};
    PdfFileEditor ed;
    EXPECT_FALSE(ed.TryMakeBooklet("no_such_file.pdf", out.path));
    EXPECT_FALSE(ed.TryMakeNUp("no_such_a.pdf", "no_such_b.pdf", out.path));
}
