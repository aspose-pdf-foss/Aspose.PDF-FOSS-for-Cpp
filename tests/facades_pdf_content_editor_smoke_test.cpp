// =============================================================================
// facades_pdf_content_editor_smoke_test — beat Fa14 of the Facades
// cluster (cluster close). PdfContentEditor edits content / links /
// stamps / images / text. v1 baseline: the content-edit + text-replace
// render paths are deferred, so the editor operations are stubs
// (ReplaceText returns false; void editors no-op); the 6 document-action
// trigger constants + the bound-document passthrough are real.
// =============================================================================

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
    EXPECT_EQ(editor.GetViewerPreference(), 0);
}

TEST(FacadesPdfContentEditorSmoke, EditorStubsDoNotThrow) {
    Document doc{HelloWorldPdf()};
    PdfContentEditor editor{doc};

    editor.AddDocumentAttachment("file.dat", "desc");
    editor.DeleteAttachments();
    editor.AddDocumentAdditionalAction(PdfContentEditor::DocumentOpen,
                                       "app.alert('hi')");
    editor.RemoveDocumentOpenAction();
    editor.ChangeViewerPreference(1);
    editor.ReplaceImage(1, 1, "img.png");
    editor.DeleteImage(1, std::vector<int>{1, 2});
    editor.DeleteImage();
    editor.DeleteStamp(1, std::vector<int>{1});
    editor.DeleteStampByIds(std::vector<int>{1, 2});
    editor.DeleteStampByIds(1, std::vector<int>{1});
    editor.DeleteStampById(1, 2);
    editor.DeleteStampById(3);
    editor.HideStampById(1, 2);
    editor.ShowStampById(1, 2);
    editor.MoveStampById(1, 2, 10.0, 20.0);
    editor.MoveStamp(1, 2, 10.0, 20.0);
    SUCCEED();
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
