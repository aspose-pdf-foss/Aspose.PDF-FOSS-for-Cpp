// =============================================================================
// facades_pdf_content_editor_smoke_test — beat Fa14 of the Facades
// cluster (cluster close). PdfContentEditor edits content / links /
// stamps / images / text. v1 baseline: the content-edit + text-replace
// render paths are deferred, so the editor operations are stubs
// (ReplaceText returns false; void editors no-op); the 6 document-action
// trigger constants + the bound-document passthrough are real.
// =============================================================================

#include <stdexcept>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_content_editor.hpp>

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

TEST(FacadesPdfContentEditorSmoke, DocumentActionConstants) {
    EXPECT_EQ(PdfContentEditor::DocumentOpen, "DO");
    EXPECT_EQ(PdfContentEditor::DocumentClose, "WC");
    EXPECT_EQ(PdfContentEditor::DocumentWillSave, "WS");
    EXPECT_EQ(PdfContentEditor::DocumentSaved, "DS");
    EXPECT_EQ(PdfContentEditor::DocumentWillPrint, "WP");
    EXPECT_EQ(PdfContentEditor::DocumentPrinted, "DP");
}

TEST(FacadesPdfContentEditorSmoke, ReplaceTextStubsReturnFalse) {
    Document doc{HelloWorldPdf()};
    PdfContentEditor editor{doc};
    EXPECT_FALSE(editor.ReplaceText("foo", "bar"));
    EXPECT_FALSE(editor.ReplaceText("foo", 1, "bar"));
    EXPECT_FALSE(editor.ReplaceText("foo", "bar", 1));
    EXPECT_THROW(editor.GetViewerPreference(), std::logic_error);
}

TEST(FacadesPdfContentEditorSmoke, UnimplementedEditorsThrow) {
    Document doc{HelloWorldPdf()};
    PdfContentEditor editor{doc};

    EXPECT_THROW(editor.AddDocumentAttachment("file.dat", "desc"), std::logic_error);
    EXPECT_THROW(editor.DeleteAttachments(), std::logic_error);
    EXPECT_THROW(editor.AddDocumentAdditionalAction(PdfContentEditor::DocumentOpen,
                                                    "app.alert('hi')"),
                 std::logic_error);
    EXPECT_THROW(editor.RemoveDocumentOpenAction(), std::logic_error);
    EXPECT_THROW(editor.ChangeViewerPreference(1), std::logic_error);
    EXPECT_THROW(editor.ReplaceImage(1, 1, "img.png"), std::logic_error);
    EXPECT_THROW(editor.DeleteStamp(1, std::vector<int>{1}), std::logic_error);
    EXPECT_THROW(editor.DeleteStampByIds(std::vector<int>{1, 2}), std::logic_error);
    EXPECT_THROW(editor.DeleteStampByIds(1, std::vector<int>{1}), std::logic_error);
    EXPECT_THROW(editor.DeleteStampById(1, 2), std::logic_error);
    EXPECT_THROW(editor.DeleteStampById(3), std::logic_error);
    EXPECT_THROW(editor.HideStampById(1, 2), std::logic_error);
    EXPECT_THROW(editor.ShowStampById(1, 2), std::logic_error);
    EXPECT_THROW(editor.MoveStampById(1, 2, 10.0, 20.0), std::logic_error);
    EXPECT_THROW(editor.MoveStamp(1, 2, 10.0, 20.0), std::logic_error);

    // Image deletion works.
    editor.DeleteImage(1, std::vector<int>{1, 2});
    editor.DeleteImage();
}

TEST(FacadesPdfContentEditorSmoke, SavePassesThrough) {
    Document doc{HelloWorldPdf()};
    PdfContentEditor editor{doc};
    editor.ReplaceText("x", "y");  // no-op
    const std::string out =
        (std::filesystem::temp_directory_path() /
         "aspose_contenteditor_save.pdf").string();
    editor.Save(out);  // inherited SaveableFacade::Save — passthrough
    EXPECT_TRUE(std::filesystem::exists(out));
    std::filesystem::remove(out);
}
