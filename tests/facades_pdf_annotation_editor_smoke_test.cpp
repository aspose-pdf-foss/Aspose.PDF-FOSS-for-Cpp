// =============================================================================
// facades_pdf_annotation_editor_smoke_test — beat Fa4 of the Facades
// cluster. PdfAnnotationEditor extends SaveableFacade and edits the
// bound document's annotations. v1 deletion is REAL (runs through the
// per-page AnnotationCollection); import/export/modify/flatten/
// delete-by-subtype are stubs.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/circle_annotation.hpp>
#include <aspose/pdf/annotations/text_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_annotation_editor.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;
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

TEST(FacadesPdfAnnotationEditorSmoke, DeleteAllAnnotationsClearsEveryPage) {
    Document doc{HelloWorldPdf()};
    TextAnnotation a{doc};
    CircleAnnotation b{doc};
    doc.Pages()[1].Annotations().Add(a);
    doc.Pages()[1].Annotations().Add(b);
    ASSERT_EQ(doc.Pages()[1].Annotations().Count(), 2);

    PdfAnnotationEditor editor{doc};
    editor.DeleteAnnotations();
    EXPECT_EQ(doc.Pages()[1].Annotations().Count(), 0);
}

TEST(FacadesPdfAnnotationEditorSmoke, DeleteAnnotationByNameRemovesOne) {
    Document doc{HelloWorldPdf()};
    TextAnnotation a{doc};
    TextAnnotation b{doc};
    a.Name("first");
    b.Name("second");
    doc.Pages()[1].Annotations().Add(a);
    doc.Pages()[1].Annotations().Add(b);
    ASSERT_EQ(doc.Pages()[1].Annotations().Count(), 2);

    PdfAnnotationEditor editor{doc};
    editor.DeleteAnnotation("first");

    auto& annots = doc.Pages()[1].Annotations();
    ASSERT_EQ(annots.Count(), 1);
    EXPECT_EQ(annots[0].Name(), "second");
}

TEST(FacadesPdfAnnotationEditorSmoke, DeleteAnnotationUnknownNameNoOp) {
    Document doc{HelloWorldPdf()};
    TextAnnotation a{doc};
    a.Name("only");
    doc.Pages()[1].Annotations().Add(a);

    PdfAnnotationEditor editor{doc};
    editor.DeleteAnnotation("missing");
    EXPECT_EQ(doc.Pages()[1].Annotations().Count(), 1);
}

TEST(FacadesPdfAnnotationEditorSmoke, StubsDoNotThrow) {
    Document doc{HelloWorldPdf()};
    PdfAnnotationEditor editor{doc};

    const std::vector<AnnotationType> types{AnnotationType::Text};
    const std::vector<std::string> files{"a.xfdf"};

    // Import / modify / flatten / delete-by-subtype are v1 stubs:
    // they must bind + run without throwing on a bound document.
    editor.ImportAnnotationsFromXfdf("a.xfdf");
    editor.ImportAnnotationsFromFdf("a.fdf");
    editor.ImportAnnotationFromXfdf("a.xfdf");
    editor.ImportAnnotationFromXfdf("a.xfdf", types);
    editor.ImportAnnotations(files, types);
    editor.ImportAnnotations(files);
    editor.ModifyAnnotationsAuthor(1, 1, "old", "new");
    editor.FlatteningAnnotations();
    editor.FlatteningAnnotations(1, 1, types);
    editor.DeleteAnnotations("Text");  // delete-by-subtype stub

    // ModifyAnnotations(start, end, Annotation&) stub.
    TextAnnotation tmpl{doc};
    editor.ModifyAnnotations(1, 1, tmpl);

    SUCCEED();
}

TEST(FacadesPdfAnnotationEditorSmoke, UnboundDeleteIsSafe) {
    PdfAnnotationEditor editor;  // nothing bound
    editor.DeleteAnnotations();
    editor.DeleteAnnotation("anything");
    SUCCEED();
}
