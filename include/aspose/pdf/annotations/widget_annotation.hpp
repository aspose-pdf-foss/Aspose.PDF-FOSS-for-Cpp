#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::WidgetAnnotation — interactive form
// widget annotation (PDF /Subtype /Widget per §12.5.6.19).
// Mirrors canonical Aspose.Pdf.Annotations.WidgetAnnotation;
// extends Annotation directly.
//
// Phased drops:
//   * ExportToJson(Stream, ExportFieldsToJsonOptions)
//   * ExportToJson(string, ExportFieldsToJsonOptions) —
//     ExportFieldsToJsonOptions / FieldSerializationResult cascade
//   * OnActivated (PdfAction) — PdfAction cascade
//   * Actions (AnnotationActionCollection) — collection cascade
//   * Parent (Forms.Field) — Forms namespace not yet in cpp catalog
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/default_appearance.hpp>
#include <aspose/pdf/annotations/highlighting_mode.hpp>

namespace Aspose::Pdf {
class Document;
namespace Forms { class Field; }
}

namespace Aspose::Pdf::Annotations {

class WidgetAnnotation : public Annotation {
public:
    explicit WidgetAnnotation(Aspose::Pdf::Document& doc);

protected:
    // Default ctor for subclasses (Forms::Field / Forms::TextBoxField
    // etc.) that need the no-arg path canonical requires on the
    // Field-family ctors. Not part of the public Widget surface.
    WidgetAnnotation() noexcept = default;

public:

    void Accept(AnnotationSelector& visitor) override;

    // Returns the canonical "on" appearance state name for a
    // checkable widget (typically the PDF name /Yes); empty for
    // widgets that aren't checkable.
    std::string GetCheckedStateName() const;

    // ---- Properties ----

    HighlightingMode Highlighting() const noexcept;
    void Highlighting(HighlightingMode value) noexcept;

    const Annotations::DefaultAppearance& DefaultAppearance() const noexcept;
    void DefaultAppearance(Annotations::DefaultAppearance value) noexcept;

    bool ReadOnly() const noexcept;
    void ReadOnly(bool value) noexcept;

    bool Required() const noexcept;
    void Required(bool value) noexcept;

    bool Exportable() const noexcept;
    void Exportable(bool value) noexcept;

    // Owning Field (PDF §12.7.3.2 — terminal field's widget points
    // up to its Field via /Parent). nullptr when this widget isn't
    // bound to a Field. Set internally by Forms::Form::Add. Land
    // F7 (was previously dropped behind the Aspose.Pdf.Forms.Field
    // translations cascade until the Field class shipped at F2).
    Forms::Field* Parent() const noexcept;
    void Parent(Forms::Field* value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Document* document_ = nullptr;
    HighlightingMode highlighting_ = HighlightingMode::Invert;
    Annotations::DefaultAppearance default_appearance_{};
    Aspose::Pdf::Forms::Field* parent_ = nullptr;
    bool read_only_ = false;
    bool required_ = false;
    bool exportable_ = true;
};

}  // namespace Aspose::Pdf::Annotations
