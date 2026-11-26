#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PdfAnnotationEditor — annotation-level editor
// for the bound PDF (import / export / extract / modify / delete /
// flatten / redact). Mirrors canonical
// Aspose.Pdf.Facades.PdfAnnotationEditor; extends SaveableFacade.
//
// v1 capabilities:
//   * Deletion is REAL — DeleteAnnotations() clears every page's
//     annotation collection; DeleteAnnotation(name) removes the
//     uniquely-named annotation. Both run through the foundation
//     AnnotationCollection on each Page.
//   * Import (XFDF / FDF), export, modify, flatten, redact and
//     delete-by-subtype are v1 stubs — the XFDF/FDF serialisation
//     foundation + the annotation-flatten render path land in
//     follow-on beats.
//
// Phased drops:
//   * Every Stream-based overload (Import/Export with a Stream or
//     Stream[] parameter) — Stream cascade.
//   * ModifyAnnotations(int, int, System.Enum, Annotation) — the
//     untyped System.Enum parameter is non-idiomatic; the
//     (int, int, Annotation) overload is the idiomatic surface.
//   * ExtractAnnotations ×2 — return IList<Annotation> (abstract
//     element); the C++ idiom iterates the page collections
//     directly (drops.yaml).
//   * RedactArea — System.Drawing.Color cascade.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/facades/saveable_facade.hpp>
#include <aspose/pdf/forms/form.hpp>

namespace Aspose::Pdf {
class Document;
namespace Annotations { class Annotation; }
}

// MSVC C4250: BindPdf/Close are inherited from Facade by dominance over
// the virtual IFacade base also reached via ISaveableFacade — intended.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

namespace Aspose::Pdf::Facades {

class PdfAnnotationEditor : public SaveableFacade {
public:
    PdfAnnotationEditor() noexcept = default;
    explicit PdfAnnotationEditor(Aspose::Pdf::Document& document);

    // ---- Import ----
    // Annotations from an XFDF file (ISO 19444-1: page, rect, colour,
    // author, contents, popups, quad points, ink, line, vertices, …), an FDF
    // file, or other PDFs (page N onto page N; form widgets are skipped).
    // The annotation types, when given, filter what is imported. The bound
    // document is updated at once. Throws std::runtime_error for an
    // unreadable or malformed file.

    void ImportAnnotationsFromXfdf(const std::string& xfdfFile);
    void ImportAnnotationsFromFdf(const std::string& fdfFile);
    void ImportAnnotationFromXfdf(const std::string& xfdfFile);
    void ImportAnnotationFromXfdf(
        const std::string& xfdfFile,
        const std::vector<Aspose::Pdf::Annotations::AnnotationType>& annotType);
    void ImportAnnotations(
        const std::vector<std::string>& annotFile,
        const std::vector<Aspose::Pdf::Annotations::AnnotationType>& annotType);
    void ImportAnnotations(const std::vector<std::string>& annotFile);

    // ---- Modify (pages start..end, 1-based) ----
    // ModifyAnnotations copies the colour, contents, author and subject
    // (and a text annotation's Open) of `annotation` onto every annotation
    // of the same type; ModifyAnnotationsAuthor renames an author.

    void ModifyAnnotations(int start, int end,
                           Aspose::Pdf::Annotations::Annotation& annotation);
    void ModifyAnnotationsAuthor(int start, int end,
                                 const std::string& srcAuthor,
                                 const std::string& desAuthor);

    // ---- Flatten (v1 stubs — render path deferred) ----

    void FlatteningAnnotations();
    void FlatteningAnnotations(
        const Aspose::Pdf::Forms::Form::FlattenSettings& flattenSettings);
    void FlatteningAnnotations(
        int start, int end,
        const std::vector<Aspose::Pdf::Annotations::AnnotationType>& annotType);

    // ---- Delete ----

    // Clear every page's annotation collection (real).
    void DeleteAnnotations();
    // Delete annotations of the given subtype name (v1 stub —
    // needs the AnnotationType<->/Subtype name table).
    void DeleteAnnotations(const std::string& annotType);
    // Delete the uniquely-named annotation across all pages (real).
    void DeleteAnnotation(const std::string& annotName);
};

}  // namespace Aspose::Pdf::Facades

#ifdef _MSC_VER
#pragma warning(pop)
#endif
