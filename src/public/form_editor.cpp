#include <aspose/pdf/facades/form_editor.hpp>

#include <utility>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/forms/barcode_field.hpp>
#include <aspose/pdf/forms/button_field.hpp>
#include <aspose/pdf/forms/checkbox_field.hpp>
#include <aspose/pdf/forms/combo_box_field.hpp>
#include <aspose/pdf/forms/date_field.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/list_box_field.hpp>
#include <aspose/pdf/forms/number_field.hpp>
#include <aspose/pdf/forms/radio_button_field.hpp>
#include <aspose/pdf/forms/signature_field.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>

namespace Aspose::Pdf::Facades {

namespace {

// Build the concrete Field for a requested FieldType at the given rect.
// Returns nullptr for types with no simple single-rect construction
// (InvalidNameOrType).
std::unique_ptr<Aspose::Pdf::Forms::Field> MakeField(
    FieldType type, Aspose::Pdf::Document& doc,
    const Aspose::Pdf::Rectangle& rect) {
    using namespace Aspose::Pdf::Forms;
    switch (type) {
        case FieldType::Text:
        case FieldType::MultiLineText:
            return std::make_unique<TextBoxField>(doc, rect);
        case FieldType::ComboBox:
            return std::make_unique<ComboBoxField>(doc, rect);
        case FieldType::ListBox:
            return std::make_unique<ListBoxField>(doc, rect);
        case FieldType::Radio:
            // RadioButtonField has no single-rect ctor (options carry
            // their own rects); construct field-only.
            return std::make_unique<RadioButtonField>(doc);
        case FieldType::CheckBox:
            return std::make_unique<CheckboxField>(doc, rect);
        case FieldType::PushButton:
        case FieldType::Image:
            return std::make_unique<ButtonField>(doc, rect);
        case FieldType::Barcode:
            return std::make_unique<BarcodeField>(doc, rect);
        case FieldType::Signature:
            return std::make_unique<SignatureField>(doc, rect);
        case FieldType::Numeric:
            return std::make_unique<NumberField>(doc, rect);
        case FieldType::DateTime:
            return std::make_unique<DateField>(doc, rect);
        case FieldType::InvalidNameOrType:
        default:
            return nullptr;
    }
}

}  // namespace

FormEditor::FormEditor(const std::string& srcFileName,
                       const std::string& destFileName)
    : src_file_(srcFileName), dest_file_(destFileName) {}

FormEditor::FormEditor(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

FormEditor::FormEditor(Aspose::Pdf::Document& document,
                       const std::string& destFileName)
    : dest_file_(destFileName) {
    BindPdf(document);
}

FormEditor::~FormEditor() = default;

Aspose::Pdf::Document* FormEditor::EnsureDoc() {
    if (document_ == nullptr && !src_file_.empty()) {
        SaveableFacade::BindPdf(src_file_);  // loads + owns + sets document_
    }
    return document_;
}

void FormEditor::Save() {
    Aspose::Pdf::Document* doc = EnsureDoc();
    if (doc != nullptr && !dest_file_.empty()) {
        doc->Save(dest_file_);
    }
}

// ===== Field add / remove (real) =============================================

bool FormEditor::AddField(FieldType fieldType, const std::string& fieldName,
                          int pageNum, float llx, float lly, float urx,
                          float ury) {
    Aspose::Pdf::Document* doc = EnsureDoc();
    if (doc == nullptr) {
        return false;
    }
    auto field = MakeField(fieldType, *doc,
                           Aspose::Pdf::Rectangle(llx, lly, urx, ury, true));
    if (!field) {
        return false;
    }
    doc->Form().Add(*field, fieldName, pageNum);
    owned_fields_.push_back(std::move(field));
    return true;
}

bool FormEditor::AddField(FieldType fieldType, const std::string& fieldName,
                          const std::string& /*fieldId*/, int pageNum,
                          float llx, float lly, float urx, float ury) {
    return AddField(fieldType, fieldName, pageNum, llx, lly, urx, ury);
}

void FormEditor::RemoveField(const std::string& fieldName) {
    Aspose::Pdf::Document* doc = EnsureDoc();
    if (doc != nullptr) {
        doc->Form().Delete(fieldName);
    }
}

// ===== Field configuration — v1 stubs ========================================
// Per-field attribute editing (appearance / scripts / limits / alignment /
// copy / decorate / list items) needs the field-property write path wired
// through the AcroForm; deferred to a follow-on beat.

bool FormEditor::SetFieldAttribute(const std::string&, PropertyFlag) {
    return false;
}
bool FormEditor::SetFieldAppearance(
    const std::string&, Aspose::Pdf::Annotations::AnnotationFlags) {
    return false;
}
Aspose::Pdf::Annotations::AnnotationFlags FormEditor::GetFieldAppearance(
    const std::string&) {
    return Aspose::Pdf::Annotations::AnnotationFlags{};
}
bool FormEditor::SetSubmitFlag(const std::string&, SubmitFormFlag) {
    return false;
}
bool FormEditor::SetSubmitUrl(const std::string&, const std::string&) {
    return false;
}
bool FormEditor::SetFieldLimit(const std::string&, int) { return false; }
bool FormEditor::SetFieldCombNumber(const std::string&, int) { return false; }
bool FormEditor::MoveField(const std::string&, float, float, float, float) {
    return false;
}
bool FormEditor::SetFieldScript(const std::string&, const std::string&) {
    return false;
}
bool FormEditor::AddFieldScript(const std::string&, const std::string&) {
    return false;
}
bool FormEditor::Single2Multiple(const std::string&) { return false; }
bool FormEditor::SetFieldAlignment(const std::string&, int) { return false; }
bool FormEditor::SetFieldAlignmentV(const std::string&, int) { return false; }

void FormEditor::ResetFacade() { facade_.Reset(); }
void FormEditor::ResetInnerFacade() { facade_.Reset(); }
void FormEditor::CopyInnerField(const std::string&, const std::string&, int) {}
void FormEditor::CopyInnerField(const std::string&, const std::string&, int,
                                float, float) {}
void FormEditor::CopyOuterField(const std::string&, const std::string&) {}
void FormEditor::CopyOuterField(const std::string&, const std::string&, int) {}
void FormEditor::CopyOuterField(const std::string&, const std::string&, int,
                                float, float) {}
void FormEditor::DecorateField(const std::string&) {}
void FormEditor::DecorateField(FieldType) {}
void FormEditor::DecorateField() {}
void FormEditor::RenameField(const std::string&, const std::string&) {}
void FormEditor::RemoveFieldAction(const std::string&) {}
void FormEditor::AddSubmitBtn(const std::string&, int, const std::string&,
                              const std::string&, float, float, float,
                              float) {}
void FormEditor::AddListItem(const std::string&, const std::string&) {}
void FormEditor::AddListItem(const std::string&,
                             const std::vector<std::string>&) {}
void FormEditor::DelListItem(const std::string&, const std::string&) {}

// ===== Properties ============================================================

const std::string& FormEditor::SrcFileName() const { return src_file_; }
void FormEditor::SrcFileName(std::string value) {
    src_file_ = std::move(value);
}
const std::string& FormEditor::DestFileName() const { return dest_file_; }
void FormEditor::DestFileName(std::string value) {
    dest_file_ = std::move(value);
}
void FormEditor::ConvertTo(Aspose::Pdf::PdfFormat value) {
    convert_to_ = value;
}
const std::vector<std::string>& FormEditor::Items() const { return items_; }
void FormEditor::Items(std::vector<std::string> value) {
    items_ = std::move(value);
}
const std::vector<std::vector<std::string>>& FormEditor::ExportItems() const {
    return export_items_;
}
void FormEditor::ExportItems(std::vector<std::vector<std::string>> value) {
    export_items_ = std::move(value);
}
const FormFieldFacade& FormEditor::Facade() const { return facade_; }
void FormEditor::Facade(FormFieldFacade value) { facade_ = std::move(value); }
float FormEditor::RadioGap() const noexcept { return radio_gap_; }
void FormEditor::RadioGap(float value) noexcept { radio_gap_ = value; }
bool FormEditor::RadioHoriz() const noexcept { return radio_horiz_; }
void FormEditor::RadioHoriz(bool value) noexcept { radio_horiz_ = value; }
double FormEditor::RadioButtonItemSize() const noexcept {
    return radio_button_item_size_;
}
void FormEditor::RadioButtonItemSize(double value) noexcept {
    radio_button_item_size_ = value;
}
SubmitFormFlag FormEditor::SubmitFlag() const noexcept { return submit_flag_; }
void FormEditor::SubmitFlag(SubmitFormFlag value) noexcept {
    submit_flag_ = value;
}

}  // namespace Aspose::Pdf::Facades
