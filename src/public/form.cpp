#include <aspose/pdf/forms/form.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace Aspose::Pdf::Forms {

// ===== Form::FlattenSettings =================================================

bool Form::FlattenSettings::UpdateAppearances() const noexcept {
    return update_appearances_;
}
void Form::FlattenSettings::UpdateAppearances(bool v) noexcept {
    update_appearances_ = v;
}

bool Form::FlattenSettings::CallEvents() const noexcept {
    return call_events_;
}
void Form::FlattenSettings::CallEvents(bool v) noexcept {
    call_events_ = v;
}

bool Form::FlattenSettings::HideButtons() const noexcept {
    return hide_buttons_;
}
void Form::FlattenSettings::HideButtons(bool v) noexcept {
    hide_buttons_ = v;
}

bool Form::FlattenSettings::ApplyRedactions() const noexcept {
    return apply_redactions_;
}
void Form::FlattenSettings::ApplyRedactions(bool v) noexcept {
    apply_redactions_ = v;
}

// ===== Form ==================================================================

void Form::Delete(Field& field) {
    auto it = std::find(fields_.begin(), fields_.end(), &field);
    if (it != fields_.end()) fields_.erase(it);
}

void Form::Delete(const std::string& fieldName) {
    for (auto it = fields_.begin(); it != fields_.end(); ++it) {
        if (*it != nullptr && (*it)->PartialName() == fieldName) {
            fields_.erase(it);
            return;
        }
    }
}

void Form::Flatten() {
    // Structural flatten: drop every interactive field and flag Save to
    // remove the catalog /AcroForm. v1 does not render field appearances
    // into the page content stream (no appearance generator yet), so a
    // flattened field's value is removed rather than burned-in.
    fields_.clear();
    field_pages_.clear();
    flatten_requested_ = true;
}

void Form::Add(Field& field, int pageNumber) {
    field.Parent(&field);   // self-Parent default — re-bound below
    fields_.push_back(&field);
    field_pages_[&field] = pageNumber;
}

void Form::Add(Field& field) {
    fields_.push_back(&field);
}

Field* Form::Add(Field& field, const std::string& partialName,
                  int pageNumber) {
    field.PartialName(partialName);
    fields_.push_back(&field);
    field_pages_[&field] = pageNumber;
    return &field;
}

void Form::AddFieldAppearance(Field& /*field*/, int /*pageNumber*/,
                                Aspose::Pdf::Rectangle /*rect*/) {
    // v1 no-op — per-rendition appearance + page binding lands
    // with the F8 AcroForm save-through beat.
}

void Form::RemoveFieldAppearance(Field& /*field*/,
                                   int /*appearanceIndex*/) {
    // v1 no-op (see above).
}

void Form::MakeFormAnnotationsIndependent(
        Aspose::Pdf::Page& /*page*/) {
    // v1 no-op — detaches widget annotations from their Form
    // bindings so they survive Form.Delete(); lands with F8.
}

bool Form::HasField(const Field& field) const noexcept {
    return std::find(fields_.begin(), fields_.end(), &field) !=
           fields_.end();
}

bool Form::HasField(const std::string& fieldName) const noexcept {
    return HasField(fieldName, false);
}

bool Form::HasField(const std::string& fieldName,
                     bool /*searchChildren*/) const noexcept {
    for (const auto* f : fields_) {
        if (f != nullptr && f->PartialName() == fieldName) return true;
    }
    return false;
}

std::vector<Field*>
Form::GetFieldsInRect(Aspose::Pdf::Rectangle /*rect*/) const {
    // v1 stub returns empty — the rect-overlap test needs each
    // widget rendition's Rect, which v1 doesn't track in Form.
    return {};
}

bool Form::AutoRecalculate() const noexcept { return auto_recalculate_; }
void Form::AutoRecalculate(bool v) noexcept { auto_recalculate_ = v; }

bool Form::AutoRestoreForm() const noexcept { return auto_restore_form_; }
void Form::AutoRestoreForm(bool v) noexcept { auto_restore_form_ = v; }

int Form::Count() const noexcept {
    return static_cast<int>(fields_.size());
}

const Aspose::Pdf::Annotations::DefaultAppearance&
Form::DefaultAppearance() const noexcept { return default_appearance_; }
void Form::DefaultAppearance(
        Aspose::Pdf::Annotations::DefaultAppearance v) {
    default_appearance_ = std::move(v);
}

Forms::XFA& Form::XFA() noexcept { return xfa_; }
const Forms::XFA& Form::XFA() const noexcept { return xfa_; }

bool Form::IgnoreNeedsRendering() const noexcept {
    return ignore_needs_rendering_;
}
void Form::IgnoreNeedsRendering(bool v) noexcept {
    ignore_needs_rendering_ = v;
}

bool Form::RemovePermission() const noexcept { return remove_permission_; }
void Form::RemovePermission(bool v) noexcept { remove_permission_ = v; }

bool Form::EmulateRequierdGroups() const noexcept {
    return emulate_requierd_groups_;
}
void Form::EmulateRequierdGroups(bool v) noexcept {
    emulate_requierd_groups_ = v;
}

bool Form::NeedsRendering() const noexcept { return needs_rendering_; }

Forms::FormType Form::Type() const noexcept { return type_; }
void Form::Type(Forms::FormType v) noexcept { type_ = v; }

Aspose::Pdf::Annotations::WidgetAnnotation*
Form::operator[](const std::string& name) {
    for (auto* f : fields_) {
        if (f != nullptr && f->PartialName() == name) return f;
    }
    return nullptr;
}

Aspose::Pdf::Annotations::WidgetAnnotation*
Form::operator[](int index) {
    if (index < 0 ||
        static_cast<std::size_t>(index) >= fields_.size()) {
        return nullptr;
    }
    return fields_[static_cast<std::size_t>(index)];
}

bool Form::HasXfa() const noexcept {
    return !xfa_.FieldNames().empty();
}

std::vector<Field*> Form::Fields() const {
    return fields_;
}

void Form::CalculatedFields(std::vector<Field*> v) {
    calculated_fields_ = std::move(v);
}

bool Form::SignaturesExist() const noexcept { return signatures_exist_; }
void Form::SignaturesExist(bool v) noexcept { signatures_exist_ = v; }

bool Form::SignaturesAppendOnly() const noexcept {
    return signatures_append_only_;
}
void Form::SignaturesAppendOnly(bool v) noexcept {
    signatures_append_only_ = v;
}

}  // namespace Aspose::Pdf::Forms
