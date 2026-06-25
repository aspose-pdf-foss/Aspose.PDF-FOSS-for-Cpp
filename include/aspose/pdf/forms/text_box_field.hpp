#pragma once

// =============================================================================
// Aspose::Pdf::Forms::TextBoxField — single-line / multi-line text
// input form field (PDF §12.7.4.3 — text fields). Mirrors
// canonical Aspose.Pdf.Forms.TextBoxField (Aspose.PDF 26.4.0);
// extends Field.
//
// Phased drops on this beat:
//   * AddImage(System.Drawing.Image) — System.Drawing.Image
//     cascade
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/vertical_alignment.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Forms {

class TextBoxField : public Field {
public:
    TextBoxField();
    explicit TextBoxField(Aspose::Pdf::Document& doc);
    TextBoxField(Aspose::Pdf::Document& doc, Aspose::Pdf::Rectangle rect);
    TextBoxField(Aspose::Pdf::Page& page,
                 Aspose::Pdf::Rectangle rect);
    TextBoxField(Aspose::Pdf::Page& page,
                 std::vector<Aspose::Pdf::Rectangle> rects);

    // ---- Methods ----

    // Encode `code` as a 1D/2D barcode and stamp it as the
    // appearance. v1 stores `code` in the field state — actual
    // barcode rasterization is deferred.
    void AddBarcode(const std::string& code);

    // ---- Properties — TextBox-specific ----

    bool Multiline() const noexcept;
    void Multiline(bool value) noexcept;

    bool SpellCheck() const noexcept;
    void SpellCheck(bool value) noexcept;

    bool Scrollable() const noexcept;
    void Scrollable(bool value) noexcept;

    bool ForceCombs() const noexcept;
    void ForceCombs(bool value) noexcept;

    int MaxLen() const noexcept;
    void MaxLen(int value) noexcept;

    Aspose::Pdf::VerticalAlignment TextVerticalAlignment() const noexcept;
    void TextVerticalAlignment(
        Aspose::Pdf::VerticalAlignment value) noexcept;

    // Value is also declared on Field; TextBoxField re-declares it
    // per canonical (the typed-string convention is the same — no
    // semantic divergence in v1).
    const std::string& Value() const noexcept;
    void Value(std::string value);

private:
    std::vector<Aspose::Pdf::Rectangle> rects_;
    std::string barcode_;
    bool multiline_ = false;
    bool spell_check_ = true;
    bool scrollable_ = true;
    bool force_combs_ = false;
    int max_len_ = 0;
    Aspose::Pdf::VerticalAlignment text_vertical_alignment_ =
        Aspose::Pdf::VerticalAlignment::Top;
};

}  // namespace Aspose::Pdf::Forms
