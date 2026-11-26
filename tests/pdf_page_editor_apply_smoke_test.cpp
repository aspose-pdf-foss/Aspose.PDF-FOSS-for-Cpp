// =============================================================================
// pdf_page_editor_apply_smoke_test — PdfPageEditor::ApplyChanges on a
// 200 x 100 page holding a red 50 x 50 square at its lower-left corner:
// zoom + alignment, rotation, page size + MovePosition, ProcessPages,
// transitions / display duration, and rotation validation.
// =============================================================================

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_page_editor.hpp>
#include <aspose/pdf/page_size.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;
using flatten_test::RedPixels;
using flatten_test::Render;
using flatten_test::TmpPath;
namespace fo = foundation::objects;

const std::string kSquare = "1 0 0 rg 0 0 50 50 re f";

// `pages` pages of 200 x 100, each with the red square.
std::string SquarePdf(const std::string& tag, int pages = 1) {
    const auto path = TmpPath(tag);
    std::string kids;
    for (int i = 0; i < pages; ++i) kids += std::to_string(3 + 2 * i) + " 0 R ";
    std::vector<std::string> objs = {
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [" + kids + "] /Count " + std::to_string(pages) + " >>"};
    std::map<int, std::string> streams;
    for (int i = 0; i < pages; ++i) {
        const int page_id = 3 + 2 * i;
        objs.push_back("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 100] /Contents " +
                       std::to_string(page_id + 1) + " 0 R >>");
        objs.push_back("<< /Length " + std::to_string(kSquare.size()) + " >>");
        streams[page_id + 1] = kSquare;
    }
    flatten_test::WritePdf(path, objs, streams);
    return path;
}

// Apply `setup` to a fresh editor on `src`, save to `out`, reopen.
template <class F>
std::string Applied(const std::string& src, const std::string& out, F setup) {
    Document doc{src};
    PdfPageEditor editor{doc};
    setup(editor);
    editor.ApplyChanges();
    doc.Save(out);
    return out;
}

}  // namespace

TEST(PdfPageEditorApply, ZoomCenteredKeepsPageSize) {
    const auto out = Applied(SquarePdf("pe_zoom"), TmpPath("pe_zoom_out"), [](PdfPageEditor& e) {
        e.Zoom(0.5f);
        e.HorizontalAlignment(Aspose::Pdf::HorizontalAlignment::Center);
        e.VerticalAlignmentType(Aspose::Pdf::VerticalAlignment::Center);
    });
    Document doc{out};
    PdfPageEditor reader{doc};
    EXPECT_FLOAT_EQ(reader.GetPageSize(1).Width(), 200.0f);
    EXPECT_FLOAT_EQ(reader.GetPageSize(1).Height(), 100.0f);
    const auto img = Render(doc, 1);
    // The page shrinks to 100 x 50 centred at (50, 25): the square is 25 x 25.
    EXPECT_GT(RedPixels(img, 52, 27, 73, 48), 300);
    EXPECT_EQ(RedPixels(img, 0, 0, 20, 20), 0);
    EXPECT_EQ(RedPixels(img, 80, 55, 120, 95), 0);
}

TEST(PdfPageEditorApply, RotationTurnsContentAndPage) {
    const auto out = Applied(SquarePdf("pe_rot"), TmpPath("pe_rot_out"),
                             [](PdfPageEditor& e) { e.Rotation(90); });
    Document doc{out};
    PdfPageEditor reader{doc};
    EXPECT_FLOAT_EQ(reader.GetPageSize(1).Width(), 100.0f);
    EXPECT_FLOAT_EQ(reader.GetPageSize(1).Height(), 200.0f);
    EXPECT_EQ(reader.GetPageRotation(1), 0);  // content turned, /Rotate kept
    const auto img = Render(doc, 1);
    // Clockwise quarter turn: the lower-left square goes to the upper left.
    EXPECT_GT(RedPixels(img, 2, 152, 48, 198), 1500);
    EXPECT_EQ(RedPixels(img, 0, 0, 50, 50), 0);
}

TEST(PdfPageEditorApply, PageSizeAndMovePosition) {
    const auto out = Applied(SquarePdf("pe_move"), TmpPath("pe_move_out"), [](PdfPageEditor& e) {
        e.PageSize(Aspose::Pdf::PageSize(400.0f, 300.0f));
        e.MovePosition(10.0f, 20.0f);
    });
    Document doc{out};
    PdfPageEditor reader{doc};
    EXPECT_FLOAT_EQ(reader.GetPageSize(1).Width(), 400.0f);
    EXPECT_FLOAT_EQ(reader.GetPageSize(1).Height(), 300.0f);
    const auto img = Render(doc, 1);
    EXPECT_GT(RedPixels(img, 12, 22, 58, 68), 1500);   // bottom-left aligned, moved
    EXPECT_EQ(RedPixels(img, 0, 0, 8, 18), 0);
}

TEST(PdfPageEditorApply, OnlyProcessPagesChange) {
    const auto out = Applied(SquarePdf("pe_pages", 2), TmpPath("pe_pages_out"),
                             [](PdfPageEditor& e) {
                                 e.ProcessPages({2});
                                 e.Rotation(90);
                             });
    Document doc{out};
    PdfPageEditor reader{doc};
    EXPECT_FLOAT_EQ(reader.GetPageSize(1).Width(), 200.0f);
    EXPECT_FLOAT_EQ(reader.GetPageSize(2).Width(), 100.0f);
}

TEST(PdfPageEditorApply, TransitionAndDisplayDuration) {
    const auto out = Applied(SquarePdf("pe_trans"), TmpPath("pe_trans_out"), [](PdfPageEditor& e) {
        e.TransitionType(PdfPageEditor::BLINDH);
        e.TransitionDuration(3);
        e.DisplayDuration(5);
    });
    const auto page = flatten_test::ParsePage(flatten_test::ReadFile(out), 0);
    const auto trans = flatten_test::ResolveDict(page.dump, flatten_test::Get(page.page, "Trans"));
    EXPECT_EQ(flatten_test::NameOf(flatten_test::Get(trans, "S")), "Blinds");
    EXPECT_EQ(flatten_test::NameOf(flatten_test::Get(trans, "Dm")), "H");
    const auto* d = flatten_test::Get(trans, "D");
    ASSERT_NE(d, nullptr);
    EXPECT_DOUBLE_EQ(std::get<double>(d->v), 3.0);
    EXPECT_NE(flatten_test::Get(page.page, "Dur"), nullptr);
    // Content untouched: nothing but transitions was staged.
    Document doc{out};
    EXPECT_GT(RedPixels(Render(doc, 1), 2, 2, 48, 48), 1500);
}

TEST(PdfPageEditorApply, RotationMustBeAMultipleOf90) {
    Document doc{SquarePdf("pe_bad_rot")};
    PdfPageEditor editor{doc};
    editor.Rotation(45);
    EXPECT_THROW(editor.ApplyChanges(), std::invalid_argument);
}
