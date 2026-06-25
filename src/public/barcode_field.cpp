#include <aspose/pdf/forms/barcode_field.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

BarcodeField::BarcodeField(Aspose::Pdf::Page& page,
                            Aspose::Pdf::Rectangle rect)
    : TextBoxField(page, std::move(rect)) {}

BarcodeField::BarcodeField(Aspose::Pdf::Document& doc,
                            Aspose::Pdf::Rectangle rect)
    : TextBoxField(doc, std::move(rect)) {}

int BarcodeField::Resolution() const noexcept { return resolution_; }
const std::string& BarcodeField::Caption() const noexcept {
    return caption_;
}
Forms::Symbology BarcodeField::Symbology() const noexcept {
    return symbology_;
}
int BarcodeField::XSymWidth() const noexcept { return x_sym_width_; }
int BarcodeField::XSymHeight() const noexcept { return x_sym_height_; }
int BarcodeField::ECC() const noexcept { return ecc_; }

}  // namespace Aspose::Pdf::Forms
