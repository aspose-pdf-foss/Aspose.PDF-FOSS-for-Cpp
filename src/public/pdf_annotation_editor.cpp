#include <aspose/pdf/facades/pdf_annotation_editor.hpp>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

namespace Aspose::Pdf::Facades {

PdfAnnotationEditor::PdfAnnotationEditor(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

// ===== Import — v1 stubs =====================================================
// XFDF / FDF parsing (the annotation-interchange formats) lands in a
// follow-on beat. The surface compiles + binds; callers see a no-op
// import until then.

void PdfAnnotationEditor::ImportAnnotationsFromXfdf(const std::string&) {}
void PdfAnnotationEditor::ImportAnnotationsFromFdf(const std::string&) {}
void PdfAnnotationEditor::ImportAnnotationFromXfdf(const std::string&) {}
void PdfAnnotationEditor::ImportAnnotationFromXfdf(
    const std::string&,
    const std::vector<Aspose::Pdf::Annotations::AnnotationType>&) {}
void PdfAnnotationEditor::ImportAnnotations(
    const std::vector<std::string>&,
    const std::vector<Aspose::Pdf::Annotations::AnnotationType>&) {}
void PdfAnnotationEditor::ImportAnnotations(
    const std::vector<std::string>&) {}

// ===== Modify — v1 stubs =====================================================

void PdfAnnotationEditor::ModifyAnnotations(
    int, int, Aspose::Pdf::Annotations::Annotation&) {}
void PdfAnnotationEditor::ModifyAnnotationsAuthor(
    int, int, const std::string&, const std::string&) {}

// ===== Flatten — v1 stubs ====================================================
// Flattening renders annotation appearances into the page content
// stream; the render path lands with the appearance-generation beat.

void PdfAnnotationEditor::FlatteningAnnotations() {}
void PdfAnnotationEditor::FlatteningAnnotations(
    const Aspose::Pdf::Forms::Form::FlattenSettings&) {}
void PdfAnnotationEditor::FlatteningAnnotations(
    int, int,
    const std::vector<Aspose::Pdf::Annotations::AnnotationType>&) {}

// ===== Delete ================================================================

void PdfAnnotationEditor::DeleteAnnotations() {
    if (document_ == nullptr) {
        return;
    }
    Aspose::Pdf::PageCollection& pages = document_->Pages();
    const int count = static_cast<int>(pages.Count());
    for (int i = 1; i <= count; ++i) {
        pages[i].Annotations().Delete();
    }
}

namespace {

// Canonical annotation-type name (AnnotationType.ToString()) for the
// delete-by-type filter.
const char* AnnotTypeName(Aspose::Pdf::Annotations::AnnotationType t) {
    using AT = Aspose::Pdf::Annotations::AnnotationType;
    switch (t) {
        case AT::Text:           return "Text";
        case AT::Circle:         return "Circle";
        case AT::Polygon:        return "Polygon";
        case AT::PolyLine:       return "PolyLine";
        case AT::Line:           return "Line";
        case AT::Square:         return "Square";
        case AT::FreeText:       return "FreeText";
        case AT::Highlight:      return "Highlight";
        case AT::Underline:      return "Underline";
        case AT::StrikeOut:      return "StrikeOut";
        case AT::Squiggly:       return "Squiggly";
        case AT::Link:           return "Link";
        case AT::Ink:            return "Ink";
        case AT::Stamp:          return "Stamp";
        case AT::Caret:          return "Caret";
        case AT::Redaction:      return "Redaction";
        case AT::Watermark:      return "Watermark";
        case AT::FileAttachment: return "FileAttachment";
        case AT::Popup:          return "Popup";
        case AT::Widget:         return "Widget";
        default:                 return "";
    }
}

}  // namespace

void PdfAnnotationEditor::DeleteAnnotations(const std::string& annotType) {
    if (document_ == nullptr) {
        return;
    }
    Aspose::Pdf::PageCollection& pages = document_->Pages();
    const int count = static_cast<int>(pages.Count());
    for (int p = 1; p <= count; ++p) {
        auto& annots = pages[p].Annotations();
        for (int i = annots.Count() - 1; i >= 0; --i) {
            if (annotType == AnnotTypeName(annots[i].AnnotationType())) {
                annots.Delete(i);
            }
        }
    }
}

void PdfAnnotationEditor::DeleteAnnotation(const std::string& annotName) {
    if (document_ == nullptr) {
        return;
    }
    Aspose::Pdf::PageCollection& pages = document_->Pages();
    const int pageCount = static_cast<int>(pages.Count());
    for (int p = 1; p <= pageCount; ++p) {
        Aspose::Pdf::Page page = pages[p];
        Aspose::Pdf::Annotations::AnnotationCollection& annots =
            page.Annotations();
        const int annotCount = annots.Count();
        // AnnotationCollection is 0-based (cf. PageCollection's
        // 1-based indexing).
        for (int a = 0; a < annotCount; ++a) {
            if (annots[a].Name() == annotName) {
                annots.Delete(a);
                return;
            }
        }
    }
}

}  // namespace Aspose::Pdf::Facades
