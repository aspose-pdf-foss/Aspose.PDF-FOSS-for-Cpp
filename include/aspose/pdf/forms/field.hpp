#pragma once

// =============================================================================
// Aspose::Pdf::Forms::Field — abstract base for AcroForm form
// fields (PDF §12.7.3). Mirrors canonical Aspose.Pdf.Forms.Field
// (Aspose.PDF 26.4.0); extends Aspose::Pdf::Annotations::WidgetAnnotation
// AND interface ICollection<WidgetAnnotation> — a Field IS a
// widget (it carries one default rendition) AND owns the list of
// widget renditions across pages when shared.
//
// Phased drops on this beat (drops.yaml + translations):
//   * CopyTo(Field[], int) — self-referential abstract-array drop
//   * CopyTo(WidgetAnnotation[], int) — abstract-array semantic
//   * GetEnumerator — IEnumerator translations cascade
//   * IsSynchronized + SyncRoot — System.Collections legacy
//   * ExecuteFieldJavaScript(JavascriptAction) — JavascriptAction
//     cascade
//   * ExportValueToJson(Stream, bool) — Stream cascade
//   * ImportValueFromJson(Stream) — Stream cascade
//   * ImportValueFromJson(Stream, string) — Stream cascade
// =============================================================================

#include <cstddef>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/widget_annotation.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Forms {

class Field : public Aspose::Pdf::Annotations::WidgetAnnotation {
public:
    explicit Field(Aspose::Pdf::Document& doc);

    // ---- Methods ----

    // Recompute calc-order field values (PDF §12.7.5.7 /CO entry).
    // v1 returns true unconditionally — calc-order propagation
    // lands when the Form save-through beat (F8) wires the
    // Catalog /AcroForm /CO entries.
    bool Recalculate();

    // Flatten the field into the page content stream (removes
    // the field annotation; underlying content stays).
    void Flatten();

    // Move the field's Rect so its lower-left corner is at
    // `point`. Width/height preserved.
    void SetPosition(Aspose::Pdf::Point point);

    // ---- Properties — naming ----

    const std::string& PartialName() const noexcept;
    void PartialName(std::string value);

    const std::string& AlternateName() const noexcept;
    void AlternateName(std::string value);

    const std::string& MappingName() const noexcept;
    void MappingName(std::string value);

    // ---- Properties — value (string form per PDF spec /V) ----

    const std::string& Value() const noexcept;
    void Value(std::string value);

    // ---- Properties — collection of widget renditions ----

    // Number of widget renditions in this field (1 for a normal
    // field; N for a shared field with N page placements).
    int Count() const noexcept;

    // True iff this field carries multiple widget renditions
    // (Count > 1).
    bool IsGroup() const noexcept;

    // Widget rendition by name (canonical /T entry of each
    // widget). Throws std::out_of_range on miss.
    Aspose::Pdf::Annotations::WidgetAnnotation&
        operator[](const std::string& name);
    // Widget rendition by 0-based index.
    Aspose::Pdf::Annotations::WidgetAnnotation&
        operator[](int index);

    // ---- Properties — placement ----

    // Annotation index on the field's page. AnnotationIndex is
    // 1-based and read/write per canonical.
    int AnnotationIndex() const noexcept;
    void AnnotationIndex(int value) noexcept;

    // Page index this field appears on (1-based). Read-only.
    int PageIndex() const noexcept;

    // True iff the field has multiple widget renditions sharing
    // the same field name (PDF §12.7.3.2 — kids array on a non-
    // terminal field).
    bool IsSharedField() const noexcept;
    void IsSharedField(bool value) noexcept;

    bool FitIntoRectangle() const noexcept;
    void FitIntoRectangle(bool value) noexcept;

    double MaxFontSize() const noexcept;
    void MaxFontSize(double value) noexcept;

    double MinFontSize() const noexcept;
    void MinFontSize(double value) noexcept;

    int TabOrder() const noexcept;
    void TabOrder(int value) noexcept;

protected:
    Field() noexcept = default;

    friend class TextBoxField;

private:
    std::string partial_name_;
    std::string alternate_name_;
    std::string mapping_name_;
    std::string value_;
    int annotation_index_ = 1;
    int page_index_ = 1;
    bool is_shared_field_ = false;
    bool fit_into_rectangle_ = false;
    double max_font_size_ = 0.0;
    double min_font_size_ = 0.0;
    int tab_order_ = 0;

    // Owned widget renditions when the field is shared across
    // pages. The "default" rendition is always this Field itself
    // (Field IS a WidgetAnnotation); kids beyond that are stored
    // here as non-owning pointers. v1 doesn't yet expose Add for
    // these — the storage is in place for the F8 save-through
    // beat to populate.
    std::vector<Aspose::Pdf::Annotations::WidgetAnnotation*> kids_;
};

}  // namespace Aspose::Pdf::Forms
