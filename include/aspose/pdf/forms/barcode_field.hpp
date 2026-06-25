#pragma once

// =============================================================================
// Aspose::Pdf::Forms::BarcodeField — barcode form field (Aspose
// extension; canonical surface lives in Aspose.Pdf.Forms).
// Mirrors canonical Aspose.Pdf.Forms.BarcodeField; extends
// TextBoxField. All six canonical properties are read-only —
// they're derived from /MK dict + JS action wiring and v1
// returns the defaults set when AddBarcode (inherited from
// TextBoxField) was last called.
// =============================================================================

#include <string>

#include <aspose/pdf/forms/symbology.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Forms {

class BarcodeField : public TextBoxField {
public:
    BarcodeField(Aspose::Pdf::Page& page,
                 Aspose::Pdf::Rectangle rect);
    BarcodeField(Aspose::Pdf::Document& doc,
                 Aspose::Pdf::Rectangle rect);

    // ---- Properties (all read-only canonical) ----

    int Resolution() const noexcept;
    const std::string& Caption() const noexcept;
    Forms::Symbology Symbology() const noexcept;
    int XSymWidth() const noexcept;
    int XSymHeight() const noexcept;
    int ECC() const noexcept;

private:
    int resolution_ = 72;
    std::string caption_;
    Forms::Symbology symbology_ = Forms::Symbology::PDF417;
    int x_sym_width_ = 0;
    int x_sym_height_ = 0;
    int ecc_ = 5;
};

}  // namespace Aspose::Pdf::Forms
