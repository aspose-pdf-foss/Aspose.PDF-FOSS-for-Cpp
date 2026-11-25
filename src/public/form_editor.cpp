#include <aspose/pdf/facades/form_editor.hpp>

#include <utility>
#include <vector>

#include <aspose/pdf/facades/font_style.hpp>
#include <aspose/pdf/forms/choice_field.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/vertical_alignment.hpp>

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
    using namespace Aspose::Pdf::Forms;
    if (fieldType == FieldType::MultiLineText)
        static_cast<TextBoxField*>(field.get())->Multiline(true);
    if (auto* radio = dynamic_cast<RadioButtonField*>(field.get())) {
        // One option per Items entry, RadioButtonItemSize square, laid out
        // from (llx, lly) — rightwards when RadioHoriz, else downwards —
        // RadioGap apart.
        const double size = radio_button_item_size_;
        for (std::size_t i = 0; i < items_.size(); ++i) {
            const double step = (size + radio_gap_) * static_cast<double>(i);
            const double x = radio_horiz_ ? llx + step : llx;
            const double y = radio_horiz_ ? lly : lly - step;
            radio->AddOption(items_[i],
                             Aspose::Pdf::Rectangle(x, y, x + size, y + size,
                                                    true));
        }
    } else if (auto* ch = dynamic_cast<ChoiceField*>(field.get())) {
        for (const auto& item : items_) ch->AddOption(item);
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

// ===== Field configuration ===================================================
// Each edit goes through the bound document's Field objects, so it persists
// on Save for new fields and for fields loaded from a file alike.

namespace {

using Aspose::Pdf::Forms::Field;

Field* FindField(Aspose::Pdf::Document* doc, const std::string& name) {
    if (doc == nullptr) return nullptr;
    for (Field* f : doc->Form().Fields())
        if (f != nullptr && f->PartialName() == name) return f;
    return nullptr;
}

// Standard-14 font name for a FormFieldFacade font; "" when it has none.
std::string FontStyleName(FontStyle s) {
    switch (s) {
        case FontStyle::Courier: return "Courier";
        case FontStyle::CourierBold: return "Courier-Bold";
        case FontStyle::CourierOblique: return "Courier-Oblique";
        case FontStyle::CourierBoldOblique: return "Courier-BoldOblique";
        case FontStyle::Helvetica: return "Helvetica";
        case FontStyle::HelveticaBold: return "Helvetica-Bold";
        case FontStyle::HelveticaOblique: return "Helvetica-Oblique";
        case FontStyle::HelveticaBoldOblique: return "Helvetica-BoldOblique";
        case FontStyle::TimesRoman: return "Times-Roman";
        case FontStyle::TimesBold: return "Times-Bold";
        case FontStyle::TimesItalic: return "Times-Italic";
        case FontStyle::TimesBoldItalic: return "Times-BoldItalic";
        default: return "";
    }
}

// A new field of `src`'s kind at `rect`, carrying its value, flags,
// appearance settings and options. nullptr for a radio group (its places
// are its options) or a kind without a single-rectangle form.
std::unique_ptr<Field> CloneField(const Field& src, Aspose::Pdf::Document& doc,
                                  const Aspose::Pdf::Rectangle& rect) {
    using namespace Aspose::Pdf::Forms;
    std::unique_ptr<Field> f;
    if (dynamic_cast<const RadioButtonField*>(&src) != nullptr) return nullptr;
    if (dynamic_cast<const SignatureField*>(&src) != nullptr)
        f = std::make_unique<SignatureField>(doc, rect);
    else if (auto* b = dynamic_cast<const ButtonField*>(&src)) {
        auto nb = std::make_unique<ButtonField>(doc, rect);
        nb->NormalCaption(b->NormalCaption());
        f = std::move(nb);
    } else if (auto* c = dynamic_cast<const CheckboxField*>(&src)) {
        auto nc = std::make_unique<CheckboxField>(doc, rect);
        nc->Checked(c->Checked());
        f = std::move(nc);
    } else if (auto* ch = dynamic_cast<const ChoiceField*>(&src)) {
        std::unique_ptr<ChoiceField> nch;
        if (dynamic_cast<const ComboBoxField*>(&src) != nullptr)
            nch = std::make_unique<ComboBoxField>(doc, rect);
        else
            nch = std::make_unique<ListBoxField>(doc, rect);
        auto& opts = const_cast<ChoiceField*>(ch)->Options();
        for (int i = 0; i < opts.Count(); ++i) nch->AddOption(opts[i].Value());
        nch->Selected(ch->Selected());
        nch->Value(const_cast<ChoiceField*>(ch)->Value());
        f = std::move(nch);
    } else if (auto* tx = dynamic_cast<const TextBoxField*>(&src)) {
        auto nt = std::make_unique<TextBoxField>(doc, rect);
        nt->Multiline(tx->Multiline());
        nt->ForceCombs(tx->ForceCombs());
        nt->MaxLen(tx->MaxLen());
        nt->TextVerticalAlignment(tx->TextVerticalAlignment());
        f = std::move(nt);
    } else {
        return nullptr;
    }
    if (dynamic_cast<const ChoiceField*>(&src) == nullptr)
        f->Value(src.Value());
    f->ReadOnly(src.ReadOnly());
    f->Required(src.Required());
    f->Exportable(src.Exportable());
    f->Flags(src.Flags());
    f->TextHorizontalAlignment(src.TextHorizontalAlignment());
    f->DefaultAppearance(src.DefaultAppearance());
    return f;
}

// `rect` moved so its lower-left corner is (x, y); size kept.
Aspose::Pdf::Rectangle PlacedAt(const Aspose::Pdf::Rectangle& rect, float x,
                                float y) {
    return Aspose::Pdf::Rectangle(x, y, x + rect.Width(), y + rect.Height(),
                                  true);
}

}  // namespace

bool FormEditor::SetFieldAttribute(const std::string& fieldName,
                                   PropertyFlag flag) {
    Field* f = FindField(EnsureDoc(), fieldName);
    if (f == nullptr) return false;
    switch (flag) {
        case PropertyFlag::ReadOnly: f->ReadOnly(true); return true;
        case PropertyFlag::Required: f->Required(true); return true;
        case PropertyFlag::NoExport: f->Exportable(false); return true;
        default: return false;
    }
}

bool FormEditor::SetFieldAppearance(
        const std::string& fieldName,
        Aspose::Pdf::Annotations::AnnotationFlags flag) {
    Field* f = FindField(EnsureDoc(), fieldName);
    if (f == nullptr) return false;
    f->Flags(flag);
    return true;
}

Aspose::Pdf::Annotations::AnnotationFlags FormEditor::GetFieldAppearance(
        const std::string& fieldName) {
    Field* f = FindField(EnsureDoc(), fieldName);
    return f == nullptr ? Aspose::Pdf::Annotations::AnnotationFlags{}
                        : f->Flags();
}

bool FormEditor::SetSubmitFlag(const std::string&, SubmitFormFlag) {
    return false;
}
bool FormEditor::SetSubmitUrl(const std::string&, const std::string&) {
    return false;
}

bool FormEditor::SetFieldLimit(const std::string& fieldName, int fieldLimit) {
    auto* t = dynamic_cast<Aspose::Pdf::Forms::TextBoxField*>(
        FindField(EnsureDoc(), fieldName));
    if (t == nullptr || fieldLimit < 0) return false;
    t->MaxLen(fieldLimit);
    return true;
}

bool FormEditor::SetFieldCombNumber(const std::string& fieldName,
                                    int combNumber) {
    auto* t = dynamic_cast<Aspose::Pdf::Forms::TextBoxField*>(
        FindField(EnsureDoc(), fieldName));
    if (t == nullptr || combNumber <= 0) return false;
    t->MaxLen(combNumber);
    t->ForceCombs(true);
    t->Multiline(false);
    return true;
}

bool FormEditor::MoveField(const std::string& fieldName, float llx, float lly,
                           float urx, float ury) {
    Field* f = FindField(EnsureDoc(), fieldName);
    if (f == nullptr) return false;
    f->Rect(Aspose::Pdf::Rectangle(llx, lly, urx, ury, true));
    return true;
}

bool FormEditor::SetFieldScript(const std::string&, const std::string&) {
    return false;
}
bool FormEditor::AddFieldScript(const std::string&, const std::string&) {
    return false;
}

bool FormEditor::Single2Multiple(const std::string& fieldName) {
    auto* t = dynamic_cast<Aspose::Pdf::Forms::TextBoxField*>(
        FindField(EnsureDoc(), fieldName));
    if (t == nullptr) return false;
    t->ForceCombs(false);
    t->Multiline(true);
    return true;
}

bool FormEditor::SetFieldAlignment(const std::string& fieldName,
                                   int alignment) {
    Field* f = FindField(EnsureDoc(), fieldName);
    if (f == nullptr) return false;
    using HA = Aspose::Pdf::HorizontalAlignment;
    switch (alignment) {
        case FormFieldFacade::AlignLeft: f->TextHorizontalAlignment(HA::Left); return true;
        case FormFieldFacade::AlignCenter: f->TextHorizontalAlignment(HA::Center); return true;
        case FormFieldFacade::AlignRight: f->TextHorizontalAlignment(HA::Right); return true;
        default: return false;
    }
}

bool FormEditor::SetFieldAlignmentV(const std::string& fieldName,
                                    int alignment) {
    auto* t = dynamic_cast<Aspose::Pdf::Forms::TextBoxField*>(
        FindField(EnsureDoc(), fieldName));
    if (t == nullptr) return false;
    using VA = Aspose::Pdf::VerticalAlignment;
    switch (alignment) {
        case FormFieldFacade::AlignTop: t->TextVerticalAlignment(VA::Top); return true;
        case FormFieldFacade::AlignMiddle: t->TextVerticalAlignment(VA::Center); return true;
        case FormFieldFacade::AlignBottom: t->TextVerticalAlignment(VA::Bottom); return true;
        default: return false;
    }
}

void FormEditor::ResetFacade() { facade_.Reset(); }
void FormEditor::ResetInnerFacade() { facade_.Reset(); }

void FormEditor::CopyInnerField(const std::string& fieldName,
                                const std::string& newFieldName,
                                int pageNum) {
    Aspose::Pdf::Document* doc = EnsureDoc();
    Field* src = FindField(doc, fieldName);
    if (src == nullptr) return;
    auto copy = CloneField(*src, *doc, src->Rect());
    if (!copy) return;
    doc->Form().Add(*copy, newFieldName, pageNum);
    owned_fields_.push_back(std::move(copy));
}

void FormEditor::CopyInnerField(const std::string& fieldName,
                                const std::string& newFieldName, int pageNum,
                                float abscissa, float ordinate) {
    Aspose::Pdf::Document* doc = EnsureDoc();
    Field* src = FindField(doc, fieldName);
    if (src == nullptr) return;
    auto copy = CloneField(*src, *doc, PlacedAt(src->Rect(), abscissa, ordinate));
    if (!copy) return;
    doc->Form().Add(*copy, newFieldName, pageNum);
    owned_fields_.push_back(std::move(copy));
}

void FormEditor::CopyOuterField(const std::string& srcFileName,
                                const std::string& fieldName) {
    Aspose::Pdf::Document other{srcFileName};
    Field* src = FindField(&other, fieldName);
    if (src == nullptr) return;
    const int page = other.FieldPageNumberInternal(*src);
    CopyOuterField(srcFileName, fieldName, page < 1 ? 1 : page);
}

void FormEditor::CopyOuterField(const std::string& srcFileName,
                                const std::string& fieldName, int pageNum) {
    Aspose::Pdf::Document* doc = EnsureDoc();
    if (doc == nullptr) return;
    Aspose::Pdf::Document other{srcFileName};
    Field* src = FindField(&other, fieldName);
    if (src == nullptr) return;
    auto copy = CloneField(*src, *doc, src->Rect());
    if (!copy) return;
    doc->Form().Add(*copy, fieldName, pageNum);
    owned_fields_.push_back(std::move(copy));
}

void FormEditor::CopyOuterField(const std::string& srcFileName,
                                const std::string& fieldName, int pageNum,
                                float abscissa, float ordinate) {
    Aspose::Pdf::Document* doc = EnsureDoc();
    if (doc == nullptr) return;
    Aspose::Pdf::Document other{srcFileName};
    Field* src = FindField(&other, fieldName);
    if (src == nullptr) return;
    auto copy =
        CloneField(*src, *doc, PlacedAt(src->Rect(), abscissa, ordinate));
    if (!copy) return;
    doc->Form().Add(*copy, fieldName, pageNum);
    owned_fields_.push_back(std::move(copy));
}

namespace {

// Apply the facade's set properties to one field: font and size (its
// DefaultAppearance), horizontal alignment, push-button caption, choice
// items, and position (Position = [llx, lly, urx, ury]).
void Decorate(Field& f, const FormFieldFacade& facade, bool font_set) {
    using namespace Aspose::Pdf::Forms;
    auto da = f.DefaultAppearance();
    bool da_changed = false;
    std::string font = facade.CustomFont();
    if (font.empty() && font_set) font = FontStyleName(facade.Font());
    if (!font.empty()) { da.FontName(font); da_changed = true; }
    if (facade.FontSize() > 0) { da.FontSize(facade.FontSize()); da_changed = true; }
    if (da_changed) {
        if (da.FontName().empty()) da.FontName("Helvetica");
        f.DefaultAppearance(da);
    }
    using HA = Aspose::Pdf::HorizontalAlignment;
    switch (facade.Alignment()) {
        case FormFieldFacade::AlignLeft: f.TextHorizontalAlignment(HA::Left); break;
        case FormFieldFacade::AlignCenter: f.TextHorizontalAlignment(HA::Center); break;
        case FormFieldFacade::AlignRight: f.TextHorizontalAlignment(HA::Right); break;
        default: break;
    }
    if (auto* b = dynamic_cast<ButtonField*>(&f); b && !facade.Caption().empty())
        b->NormalCaption(facade.Caption());
    if (auto* ch = dynamic_cast<ChoiceField*>(&f); ch && !facade.Items().empty()) {
        std::vector<std::string> existing;
        for (int i = 0; i < ch->Options().Count(); ++i)
            existing.push_back(ch->Options()[i].Value());
        for (const auto& e : existing) ch->DeleteOption(e);
        for (const auto& item : facade.Items()) ch->AddOption(item);
    }
    const auto& pos = facade.Position();
    if (pos.size() == 4)
        f.Rect(Aspose::Pdf::Rectangle(pos[0], pos[1], pos[2], pos[3], true));
}

// The FieldType a field is decorated as.
FieldType KindOf(const Field& f) {
    using namespace Aspose::Pdf::Forms;
    if (dynamic_cast<const RadioButtonField*>(&f)) return FieldType::Radio;
    if (dynamic_cast<const CheckboxField*>(&f)) return FieldType::CheckBox;
    if (dynamic_cast<const ButtonField*>(&f)) return FieldType::PushButton;
    if (dynamic_cast<const ComboBoxField*>(&f)) return FieldType::ComboBox;
    if (dynamic_cast<const ListBoxField*>(&f)) return FieldType::ListBox;
    if (dynamic_cast<const SignatureField*>(&f)) return FieldType::Signature;
    if (auto* t = dynamic_cast<const TextBoxField*>(&f))
        return t->Multiline() ? FieldType::MultiLineText : FieldType::Text;
    return FieldType::InvalidNameOrType;
}

}  // namespace

void FormEditor::DecorateField(const std::string& fieldName) {
    if (Field* f = FindField(EnsureDoc(), fieldName))
        Decorate(*f, facade_, facade_.font_set_);
}

void FormEditor::DecorateField(FieldType fieldType) {
    Aspose::Pdf::Document* doc = EnsureDoc();
    if (doc == nullptr) return;
    for (Field* f : doc->Form().Fields()) {
        if (f == nullptr) continue;
        const FieldType k = KindOf(*f);
        const bool match =
            k == fieldType || (fieldType == FieldType::Text &&
                               k == FieldType::MultiLineText);
        if (match) Decorate(*f, facade_, facade_.font_set_);
    }
}

void FormEditor::DecorateField() {
    Aspose::Pdf::Document* doc = EnsureDoc();
    if (doc == nullptr) return;
    for (Field* f : doc->Form().Fields())
        if (f != nullptr) Decorate(*f, facade_, facade_.font_set_);
}

void FormEditor::RenameField(const std::string& fieldName,
                             const std::string& newFieldName) {
    if (Field* f = FindField(EnsureDoc(), fieldName))
        f->PartialName(newFieldName);
}

void FormEditor::RemoveFieldAction(const std::string&) {}
void FormEditor::AddSubmitBtn(const std::string&, int, const std::string&,
                              const std::string&, float, float, float,
                              float) {}

void FormEditor::AddListItem(const std::string& fieldName,
                             const std::string& item) {
    if (auto* ch = dynamic_cast<Aspose::Pdf::Forms::ChoiceField*>(
            FindField(EnsureDoc(), fieldName)))
        ch->AddOption(item);
}

void FormEditor::AddListItem(const std::string& fieldName,
                             const std::vector<std::string>& items) {
    for (const auto& item : items) AddListItem(fieldName, item);
}

void FormEditor::DelListItem(const std::string& fieldName,
                             const std::string& item) {
    if (auto* ch = dynamic_cast<Aspose::Pdf::Forms::ChoiceField*>(
            FindField(EnsureDoc(), fieldName)))
        ch->DeleteOption(item);
}

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
