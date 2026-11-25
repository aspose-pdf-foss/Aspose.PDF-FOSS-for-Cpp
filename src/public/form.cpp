#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/radio_button_field.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>
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
    // The form is empty for the user from here on; the fields move to
    // flatten_fields_ so Save still writes them (new fields with their /AP,
    // edited loaded fields with a fresh /AP), then burns every visible
    // widget into its page content and removes the catalog /AcroForm.
    flatten_fields_.insert(flatten_fields_.end(), fields_.begin(),
                           fields_.end());
    fields_.clear();
    flatten_requested_ = true;
}

void Form::Add(Field& field, int pageNumber) {
    field.Parent(&field);   // self-Parent default — re-bound below
    fields_.push_back(&field);
    field.owner_form_ = this;
    field_pages_[&field] = pageNumber;
}

void Form::Add(Field& field) {
    fields_.push_back(&field);
    field.owner_form_ = this;
}

void Form::FlattenField(Field& field) {
    auto it = std::find(fields_.begin(), fields_.end(), &field);
    if (it == fields_.end()) return;
    fields_.erase(it);
    flatten_single_.push_back(&field);
}

Field* Form::Add(Field& field, const std::string& partialName,
                  int pageNumber) {
    field.PartialName(partialName);
    fields_.push_back(&field);
    field.owner_form_ = this;
    field_pages_[&field] = pageNumber;
    return &field;
}

std::vector<std::uint32_t>
Form::RemainingLoadedWidgets(const Field& field) const {
    std::vector<std::uint32_t> ids;
    if (auto it = loaded_widget_ids_.find(&field);
            it != loaded_widget_ids_.end()) {
        ids = it->second;
    } else if (auto lit = loaded_ids_.find(&field); lit != loaded_ids_.end()) {
        ids.push_back(lit->second);  // field and widget in one object
    }
    if (auto rit = removed_widget_ids_.find(&field);
            rit != removed_widget_ids_.end()) {
        ids.erase(std::remove_if(ids.begin(), ids.end(),
                                 [&](std::uint32_t id) {
                                     return rit->second.count(id) != 0;
                                 }),
                  ids.end());
    }
    return ids;
}

void Form::AddFieldAppearance(Field& field, int pageNumber,
                              Aspose::Pdf::Rectangle rect) {
    if (std::find(fields_.begin(), fields_.end(), &field) == fields_.end())
        throw std::invalid_argument(
            "Aspose::Pdf::Forms::Form::AddFieldAppearance: the field is not "
            "in this form");
    if (dynamic_cast<RadioButtonField*>(&field) != nullptr)
        throw std::invalid_argument(
            "Aspose::Pdf::Forms::Form::AddFieldAppearance: a radio button "
            "field's places are its options; use AddOption");
    if (loaded_ids_.count(&field)) {
        added_renditions_[&field].push_back(Rendition{pageNumber, rect});
        return;
    }
    auto& list = renditions_[&field];
    if (list.empty()) {
        auto pit = field_pages_.find(&field);
        list.push_back(Rendition{pit == field_pages_.end() ? 0 : pit->second,
                                 field.Rect()});
    }
    list.push_back(Rendition{pageNumber, rect});
}

void Form::RemoveFieldAppearance(Field& field, int appearanceIndex) {
    if (std::find(fields_.begin(), fields_.end(), &field) == fields_.end())
        throw std::invalid_argument(
            "Aspose::Pdf::Forms::Form::RemoveFieldAppearance: the field is "
            "not in this form");
    const bool loaded = loaded_ids_.count(&field) != 0;
    const std::vector<std::uint32_t> remaining =
        loaded ? RemainingLoadedWidgets(field) : std::vector<std::uint32_t>{};
    std::size_t count = 0;
    if (loaded) {
        auto ait = added_renditions_.find(&field);
        count = remaining.size() +
                (ait == added_renditions_.end() ? 0 : ait->second.size());
    } else {
        auto rit = renditions_.find(&field);
        count = rit == renditions_.end() ? 1 : rit->second.size();
    }
    if (appearanceIndex < 1 || static_cast<std::size_t>(appearanceIndex) > count)
        throw std::out_of_range(
            "Aspose::Pdf::Forms::Form::RemoveFieldAppearance: appearance "
            "index " + std::to_string(appearanceIndex) + " out of range [1.." +
            std::to_string(count) + "]");
    if (count == 1)
        throw std::invalid_argument(
            "Aspose::Pdf::Forms::Form::RemoveFieldAppearance: cannot remove "
            "the field's only appearance");
    const auto idx = static_cast<std::size_t>(appearanceIndex - 1);
    if (loaded) {
        if (idx < remaining.size())
            removed_widget_ids_[&field].insert(remaining[idx]);
        else {
            auto& added = added_renditions_[&field];
            added.erase(added.begin() +
                        static_cast<std::ptrdiff_t>(idx - remaining.size()));
        }
        return;
    }
    auto& list = renditions_[&field];
    list.erase(list.begin() + static_cast<std::ptrdiff_t>(idx));
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
