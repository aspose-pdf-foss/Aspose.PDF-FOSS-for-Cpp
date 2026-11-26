#include <aspose/pdf/facades/pdf_annotation_editor.hpp>

#include "_pending.hpp"

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

namespace Aspose::Pdf::Facades {

PdfAnnotationEditor::PdfAnnotationEditor(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

// ===== Import / modify — not implemented =====================================
// XFDF / FDF annotation import and bulk modification throw rather than
// silently doing nothing.

void PdfAnnotationEditor::ImportAnnotationsFromXfdf(const std::string&) {
    PENDING("annotation-xfdf-import");
}
void PdfAnnotationEditor::ImportAnnotationsFromFdf(const std::string&) {
    PENDING("annotation-fdf-import");
}
void PdfAnnotationEditor::ImportAnnotationFromXfdf(const std::string&) {
    PENDING("annotation-xfdf-import");
}
void PdfAnnotationEditor::ImportAnnotationFromXfdf(
    const std::string&,
    const std::vector<Aspose::Pdf::Annotations::AnnotationType>&) {
    PENDING("annotation-xfdf-import");
}
void PdfAnnotationEditor::ImportAnnotations(
    const std::vector<std::string>&,
    const std::vector<Aspose::Pdf::Annotations::AnnotationType>&) {
    PENDING("annotation-pdf-import");
}
void PdfAnnotationEditor::ImportAnnotations(
    const std::vector<std::string>&) {
    PENDING("annotation-pdf-import");
}

void PdfAnnotationEditor::ModifyAnnotations(
    int, int, Aspose::Pdf::Annotations::Annotation&) {
    PENDING("annotation-modify");
}
void PdfAnnotationEditor::ModifyAnnotationsAuthor(
    int, int, const std::string&, const std::string&) {
    PENDING("annotation-modify");
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

// ===== Flatten ===============================================================
// Staged on the bound document and applied by its next Save: appearances
// are drawn into the page content and the annotations removed. Works on the
// file's annotations, including subtypes the annotation collection does not
// model; one without an appearance is left in place.

void PdfAnnotationEditor::FlatteningAnnotations() {
    FlatteningAnnotations(Aspose::Pdf::Forms::Form::FlattenSettings{});
}

void PdfAnnotationEditor::FlatteningAnnotations(
        const Aspose::Pdf::Forms::Form::FlattenSettings& flattenSettings) {
    if (document_ == nullptr) return;
    // Every form field and every annotation of every page.
    Aspose::Pdf::Forms::Form& form = document_->Form();
    form.flatten_hide_buttons_ = flattenSettings.HideButtons();
    form.Flatten();
    Aspose::Pdf::Document::FlattenRequest::Rule rule;
    rule.hide_buttons = flattenSettings.HideButtons();
    document_->pending_flatten_rules_.push_back(std::move(rule));
}

void PdfAnnotationEditor::FlatteningAnnotations(
        int start, int end,
        const std::vector<Aspose::Pdf::Annotations::AnnotationType>&
            annotType) {
    if (document_ == nullptr) return;
    Aspose::Pdf::Document::FlattenRequest::Rule rule;
    rule.first_page = start;
    rule.last_page = end;
    for (const auto t : annotType) {
        const std::string name = AnnotTypeName(t);
        if (!name.empty()) rule.subtypes.insert(name);
    }
    // A type list with nothing this library can name selects nothing.
    if (!annotType.empty() && rule.subtypes.empty()) return;
    document_->pending_flatten_rules_.push_back(std::move(rule));
}

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
