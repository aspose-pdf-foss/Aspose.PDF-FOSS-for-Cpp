#pragma once

// =============================================================================
// Aspose::Pdf::Facades::FormEditor — AcroForm field editor (add / remove /
// copy / decorate / configure form fields). Mirrors canonical
// Aspose.Pdf.Facades.FormEditor; extends SaveableFacade.
//
// Input-file -> output-file (SrcFileName / DestFileName) or bound
// Document. AddField and RemoveField are REAL — they construct the
// concrete Field for the requested FieldType and route through the bound
// document's AcroForm (Document::Form().Add / Delete), persisting on
// Save via the AcroForm save-through. The remaining field-configuration
// operations (Set*/Copy*/Decorate*/list-item/script/alignment) are v1
// stubs.
//
// Phased drops:
//   * Stream ctors / SrcStream / DestStream — Stream cascade.
// =============================================================================

#include <memory>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_flags.hpp>
#include <aspose/pdf/facades/field_type.hpp>
#include <aspose/pdf/facades/form_field_facade.hpp>
#include <aspose/pdf/facades/property_flag.hpp>
#include <aspose/pdf/facades/saveable_facade.hpp>
#include <aspose/pdf/facades/submit_form_flag.hpp>
#include <aspose/pdf/pdf_format.hpp>

namespace Aspose::Pdf {
class Document;
namespace Forms { class Field; }
}

// MSVC C4250: BindPdf/Close are inherited from Facade by dominance over
// the virtual IFacade base also reached via ISaveableFacade — intended.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

namespace Aspose::Pdf::Facades {

class FormEditor : public SaveableFacade {
public:
    FormEditor() noexcept = default;
    FormEditor(const std::string& srcFileName,
               const std::string& destFileName);
    explicit FormEditor(Aspose::Pdf::Document& document);
    FormEditor(Aspose::Pdf::Document& document,
               const std::string& destFileName);
    ~FormEditor() override;

    // Save the edited document to DestFileName.
    void Save();

    // ---- Field add / remove (real, via Document::Form) ----

    bool AddField(FieldType fieldType, const std::string& fieldName,
                  int pageNum, float llx, float lly, float urx, float ury);
    bool AddField(FieldType fieldType, const std::string& fieldName,
                  const std::string& fieldId, int pageNum,
                  float llx, float lly, float urx, float ury);
    void RemoveField(const std::string& fieldName);

    // ---- Field configuration (v1 stubs) ----

    bool SetFieldAttribute(const std::string& fieldName, PropertyFlag flag);
    bool SetFieldAppearance(
        const std::string& fieldName,
        Aspose::Pdf::Annotations::AnnotationFlags flag);
    Aspose::Pdf::Annotations::AnnotationFlags GetFieldAppearance(
        const std::string& fieldName);
    bool SetSubmitFlag(const std::string& fieldName, SubmitFormFlag flag);
    bool SetSubmitUrl(const std::string& fieldName, const std::string& url);
    bool SetFieldLimit(const std::string& fieldName, int fieldLimit);
    bool SetFieldCombNumber(const std::string& fieldName, int combNumber);
    bool MoveField(const std::string& fieldName,
                   float llx, float lly, float urx, float ury);
    bool SetFieldScript(const std::string& fieldName,
                        const std::string& script);
    bool AddFieldScript(const std::string& fieldName,
                        const std::string& script);
    bool Single2Multiple(const std::string& fieldName);
    bool SetFieldAlignment(const std::string& fieldName, int alignment);
    bool SetFieldAlignmentV(const std::string& fieldName, int alignment);

    void ResetFacade();
    void ResetInnerFacade();
    void CopyInnerField(const std::string& fieldName,
                        const std::string& newFieldName, int pageNum);
    // Copy field `fieldName` as `newFieldName` onto page `pageNum`, at the
    // same rectangle or with its lower-left corner at (abscissa, ordinate).
    // Radio groups are not copied.
    void CopyInnerField(const std::string& fieldName,
                        const std::string& newFieldName, int pageNum,
                        float abscissa, float ordinate);
    // Copy field `fieldName` from the PDF `srcFileName` into this document:
    // onto its page in the source, or `pageNum`; at its rectangle or with
    // its lower-left corner at (abscissa, ordinate).
    void CopyOuterField(const std::string& srcFileName,
                        const std::string& fieldName);
    void CopyOuterField(const std::string& srcFileName,
                        const std::string& fieldName, int pageNum);
    void CopyOuterField(const std::string& srcFileName,
                        const std::string& fieldName, int pageNum,
                        float abscissa, float ordinate);
    void DecorateField(const std::string& fieldName);
    void DecorateField(FieldType fieldType);
    void DecorateField();
    void RenameField(const std::string& fieldName,
                     const std::string& newFieldName);
    void RemoveFieldAction(const std::string& fieldName);
    void AddSubmitBtn(const std::string& fieldName, int pageNum,
                      const std::string& buttonName, const std::string& url,
                      float llx, float lly, float urx, float ury);
    void AddListItem(const std::string& fieldName, const std::string& item);
    void AddListItem(const std::string& fieldName,
                     const std::vector<std::string>& items);
    void DelListItem(const std::string& fieldName, const std::string& item);

    // ---- Properties ----

    const std::string& SrcFileName() const;
    void SrcFileName(std::string value);
    const std::string& DestFileName() const;
    void DestFileName(std::string value);
    void ConvertTo(Aspose::Pdf::PdfFormat value);
    const std::vector<std::string>& Items() const;
    void Items(std::vector<std::string> value);
    const std::vector<std::vector<std::string>>& ExportItems() const;
    void ExportItems(std::vector<std::vector<std::string>> value);
    const FormFieldFacade& Facade() const;
    void Facade(FormFieldFacade value);
    float RadioGap() const noexcept;
    void RadioGap(float value) noexcept;
    bool RadioHoriz() const noexcept;
    void RadioHoriz(bool value) noexcept;
    double RadioButtonItemSize() const noexcept;
    void RadioButtonItemSize(double value) noexcept;
    SubmitFormFlag SubmitFlag() const noexcept;
    void SubmitFlag(SubmitFormFlag value) noexcept;

private:
    // Resolve the working document: a bound Document, or lazily loaded
    // from SrcFileName. Returns nullptr when neither is available.
    Aspose::Pdf::Document* EnsureDoc();

    std::string src_file_;
    std::string dest_file_;
    std::vector<std::unique_ptr<Aspose::Pdf::Forms::Field>> owned_fields_;
    std::vector<std::string> items_;
    std::vector<std::vector<std::string>> export_items_;
    FormFieldFacade facade_;
    Aspose::Pdf::PdfFormat convert_to_ = Aspose::Pdf::PdfFormat::v_1_7;
    float radio_gap_ = 1.0f;
    bool radio_horiz_ = false;
    double radio_button_item_size_ = 20.0;
    SubmitFormFlag submit_flag_ = SubmitFormFlag::Html;
};

}  // namespace Aspose::Pdf::Facades

#ifdef _MSC_VER
#pragma warning(pop)
#endif
