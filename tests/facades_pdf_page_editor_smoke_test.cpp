// =============================================================================
// facades_pdf_page_editor_smoke_test — beat Fa11 of the Facades cluster.
// PdfPageEditor stages page-level edits (move/rotate/zoom/align +
// transitions). GetPages is real (page count); the transform operations
// + per-page geometry are v1 stubs (canonical defaults). 16 page-
// transition constants are pinned to canonical values.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/alignment_type.hpp>
#include <aspose/pdf/facades/pdf_page_editor.hpp>
#include <aspose/pdf/facades/vertical_alignment_type.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/page_size.hpp>
#include <aspose/pdf/vertical_alignment.hpp>

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

}  // namespace

TEST(FacadesPdfPageEditorSmoke, TransitionConstants) {
    EXPECT_EQ(PdfPageEditor::SPLITVOUT, 1);
    EXPECT_EQ(PdfPageEditor::SPLITHIN, 4);
    EXPECT_EQ(PdfPageEditor::INBOX, 7);
    EXPECT_EQ(PdfPageEditor::DISSOLVE, 13);
    EXPECT_EQ(PdfPageEditor::DGLITTER, 16);
}

TEST(FacadesPdfPageEditorSmoke, GetPagesReal) {
    Document doc{HelloWorldPdf()};
    PdfPageEditor editor{doc};
    EXPECT_EQ(editor.GetPages(), static_cast<int>(doc.Pages().Count()));
    EXPECT_GT(editor.GetPages(), 0);
}

TEST(FacadesPdfPageEditorSmoke, GeometryStubs) {
    Document doc{HelloWorldPdf()};
    PdfPageEditor editor{doc};
    auto sz = editor.GetPageSize(1);
    EXPECT_FLOAT_EQ(sz.Width(), 612.0f);
    EXPECT_FLOAT_EQ(sz.Height(), 792.0f);
    EXPECT_EQ(editor.GetPageRotation(1), 0);
    editor.MovePosition(10, 20);  // no-throw
    editor.ApplyChanges();        // no-throw
}

TEST(FacadesPdfPageEditorSmoke, UnboundGetPagesZero) {
    PdfPageEditor editor;
    EXPECT_EQ(editor.GetPages(), 0);
}

TEST(FacadesPdfPageEditorSmoke, PropertyRoundtrip) {
    PdfPageEditor editor;
    editor.TransitionDuration(5);
    editor.TransitionType(PdfPageEditor::BLINDV);
    editor.DisplayDuration(3);
    editor.ProcessPages(std::vector<int>{1, 3, 5});
    editor.Rotation(90);
    editor.Zoom(1.5f);
    editor.PageSize(PageSize::A4());
    editor.Alignment(AlignmentType::Center());
    editor.HorizontalAlignment(HorizontalAlignment::Left);
    editor.VerticalAlignment(VerticalAlignmentType::Bottom());
    editor.VerticalAlignmentType(VerticalAlignment::Top);

    EXPECT_EQ(editor.TransitionDuration(), 5);
    EXPECT_EQ(editor.TransitionType(), PdfPageEditor::BLINDV);
    EXPECT_EQ(editor.DisplayDuration(), 3);
    ASSERT_EQ(editor.ProcessPages().size(), 3u);
    EXPECT_EQ(editor.ProcessPages()[1], 3);
    EXPECT_EQ(editor.Rotation(), 90);
    EXPECT_FLOAT_EQ(editor.Zoom(), 1.5f);
    EXPECT_FLOAT_EQ(editor.PageSize().Width(), PageSize::A4().Width());
    EXPECT_EQ(editor.Alignment(), AlignmentType::Center());
    EXPECT_EQ(editor.HorizontalAlignment(), HorizontalAlignment::Left);
    EXPECT_EQ(editor.VerticalAlignment(), VerticalAlignmentType::Bottom());
    EXPECT_EQ(editor.VerticalAlignmentType(), VerticalAlignment::Top);
}
