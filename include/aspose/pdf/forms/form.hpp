#pragma once

// =============================================================================
// Aspose::Pdf::Forms::Form — main entry point for AcroForm (and
// XFA-wrapper) interaction (PDF §12.7.3 — interactive forms).
// Mirrors canonical Aspose.Pdf.Forms.Form (Aspose.PDF 26.4.0);
// extends System.Object + implements
// ICollection<WidgetAnnotation>.
//
// Field instances are added through `Add(field, …)` and stored as
// non-owning pointers. Callers own the Field lifetime.
//
// Phased drops (this beat):
//   * CopyTo(Field[], int) — abstract-array semantic
//   * IsSynchronized + SyncRoot — System.Collections legacy
//   * GetEnumerator — IEnumerator translations cascade
//   * AssignXfa(XmlDocument) — XmlDocument cascade
//   * ExportToJson(Stream, ...) / ImportFromJson(Stream)  — Stream + ExportFieldsToJsonOptions cascades
//   * DefaultResources (Aspose.Pdf.Resources) — Resources cascade
//   * CalculatedFields (IEnumerable<Field> setter-only) — Field-
//     collection idiom mismatch
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/default_appearance.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/form_type.hpp>
#include <aspose/pdf/forms/xfa.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Forms {

class Form {
public:
    // ---- Nested types ----

    // SignDependentElementsRenderingModes — controls how a Form's
    // sign-dependent widgets render when the doc is converted
    // (e.g. to PDF/A or via a Document.Save fast-path).
    enum class SignDependentElementsRenderingModes : int {
        RenderFormAsUnsigned = 0,
        RenderFormAsSigned = 1,
    };

    // FlattenSettings — bundle of toggles consumed by
    // Form::Flatten. Constructed with canonical defaults
    // (UpdateAppearances=true, CallEvents=true, HideButtons=false,
    // ApplyRedactions=false).
    class FlattenSettings {
    public:
        FlattenSettings() noexcept = default;

        bool UpdateAppearances() const noexcept;
        void UpdateAppearances(bool value) noexcept;

        bool CallEvents() const noexcept;
        void CallEvents(bool value) noexcept;

        bool HideButtons() const noexcept;
        void HideButtons(bool value) noexcept;

        bool ApplyRedactions() const noexcept;
        void ApplyRedactions(bool value) noexcept;

    private:
        bool update_appearances_ = true;
        bool call_events_ = true;
        bool hide_buttons_ = false;
        bool apply_redactions_ = false;
    };

    Form() noexcept = default;

    // ---- Public field (canonical declares this as a field, not a property) ----

    SignDependentElementsRenderingModes
        SignDependentElementsRenderingModeWhenConverted =
            SignDependentElementsRenderingModes::RenderFormAsUnsigned;

    // ---- Mutation ----

    void Delete(Field& field);
    void Delete(const std::string& fieldName);

    void Flatten();

    void Add(Field& field, int pageNumber);
    void Add(Field& field);
    Field* Add(Field& field, const std::string& partialName,
               int pageNumber);

    void AddFieldAppearance(Field& field, int pageNumber,
                             Aspose::Pdf::Rectangle rect);
    void RemoveFieldAppearance(Field& field, int appearanceIndex);

    void MakeFormAnnotationsIndependent(Aspose::Pdf::Page& page);

    // ---- Query ----

    bool HasField(const Field& field) const noexcept;
    bool HasField(const std::string& fieldName) const noexcept;
    bool HasField(const std::string& fieldName,
                  bool searchChildren) const noexcept;

    std::vector<Field*> GetFieldsInRect(
        Aspose::Pdf::Rectangle rect) const;

    // ---- Properties ----

    bool AutoRecalculate() const noexcept;
    void AutoRecalculate(bool value) noexcept;

    bool AutoRestoreForm() const noexcept;
    void AutoRestoreForm(bool value) noexcept;

    int Count() const noexcept;

    const Aspose::Pdf::Annotations::DefaultAppearance&
        DefaultAppearance() const noexcept;
    void DefaultAppearance(
        Aspose::Pdf::Annotations::DefaultAppearance value);

    Forms::XFA& XFA() noexcept;
    const Forms::XFA& XFA() const noexcept;

    bool IgnoreNeedsRendering() const noexcept;
    void IgnoreNeedsRendering(bool value) noexcept;

    bool RemovePermission() const noexcept;
    void RemovePermission(bool value) noexcept;

    bool EmulateRequierdGroups() const noexcept;
    void EmulateRequierdGroups(bool value) noexcept;

    bool NeedsRendering() const noexcept;

    Forms::FormType Type() const noexcept;
    void Type(Forms::FormType value) noexcept;

    Aspose::Pdf::Annotations::WidgetAnnotation*
        operator[](const std::string& name);
    Aspose::Pdf::Annotations::WidgetAnnotation*
        operator[](int index);

    bool HasXfa() const noexcept;

    // Fields returns the registered Field pointers as a vector
    // (canonical Field[] → std::vector<Field*>).
    std::vector<Field*> Fields() const;

    // CalculatedFields is canonical setter-only (write-shadow on
    // the read-only-ish base). v1 stores the list and exposes it
    // back through Fields() iteration.
    void CalculatedFields(std::vector<Field*> value);

    bool SignaturesExist() const noexcept;
    void SignaturesExist(bool value) noexcept;

    bool SignaturesAppendOnly() const noexcept;
    void SignaturesAppendOnly(bool value) noexcept;

private:
    // Document populates these on load-on-open (Document::Form()): fields
    // read back from an existing /AcroForm are owned here, their raw
    // pointers registered in fields_, and their source PDF object id kept
    // in loaded_ids_ so a re-save reuses the original object (lossless —
    // loaded fields are not rewritten unless removed).
    friend class Aspose::Pdf::Document;
    std::vector<std::unique_ptr<Field>> owned_fields_;
    std::map<const Field*, std::uint32_t> loaded_ids_;
    // Snapshot of each loaded field's effective value at load time, so Save
    // can detect a value edit and re-emit only the changed field objects.
    std::map<const Field*, std::string> loaded_values_;
    std::size_t loaded_count_ = 0;
    // Set by Flatten() — Save removes the catalog /AcroForm.
    bool flatten_requested_ = false;

    std::vector<Field*> fields_;
    // 1-based page number a newly-added field's widget belongs on, recorded
    // by the Add(field, pageNumber) overloads. Save reads this to emit the
    // widget's /P parent-page ref and to link it into that page's /Annots so
    // viewers render the field. Fields added through the page-less Add(field)
    // overload are absent here and keep the v1 unbound behaviour.
    std::map<const Field*, int> field_pages_;
    std::vector<Field*> calculated_fields_;
    Aspose::Pdf::Annotations::DefaultAppearance default_appearance_{};
    Forms::XFA xfa_;
    Forms::FormType type_ = Forms::FormType::Standard;
    bool auto_recalculate_ = false;
    bool auto_restore_form_ = false;
    bool ignore_needs_rendering_ = false;
    bool remove_permission_ = false;
    bool emulate_requierd_groups_ = false;
    bool needs_rendering_ = false;
    bool signatures_exist_ = false;
    bool signatures_append_only_ = false;
};

}  // namespace Aspose::Pdf::Forms
