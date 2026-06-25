#pragma once

// =============================================================================
// Aspose::Pdf::Facades::FormFieldFacade — appearance descriptor for a
// form field (border / font / colour / alignment / geometry / item
// lists). A pure value type consumed by FormEditor's Decorate / Get /
// SetFieldAppearance. Mirrors canonical
// Aspose.Pdf.Facades.FormFieldFacade.
//
// The 26 public static constants (BorderWidth* / BorderStyle* / Align* /
// CheckBoxStyle*) carry the canonical .NET values verbatim (dumped from
// the pinned Aspose.PDF 26.4.0 assembly).
//
// Phased drops:
//   * BorderColor / TextColor / BackgroundColor / BackgroudColor —
//     System.Drawing.Color cascade.
//   * Box — System.Drawing.Rectangle cascade (the .NET drawing rect,
//     not Aspose.Pdf.Rectangle).
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/facades/encoding_type.hpp>
#include <aspose/pdf/facades/font_style.hpp>

namespace Aspose::Pdf::Facades {

class FormFieldFacade {
public:
    // ---- Border width presets (float) ----
    static constexpr float BorderWidthUndified = -1.0f;  // canonical typo alias
    static constexpr float BorderWidthUndefined = -1.0f;
    static constexpr float BorderWidthThin = 1.0f;
    static constexpr float BorderWidthMedium = 2.0f;
    static constexpr float BorderWidthThick = 3.0f;

    // ---- Border style presets ----
    static constexpr int BorderStyleSolid = 0;
    static constexpr int BorderStyleDashed = 1;
    static constexpr int BorderStyleBeveled = 2;
    static constexpr int BorderStyleInset = 3;
    static constexpr int BorderStyleUnderline = 4;
    static constexpr int BorderStyleUndefined = 5;

    // ---- Alignment presets (horizontal + vertical share the int space) ----
    static constexpr int AlignLeft = 0;
    static constexpr int AlignCenter = 1;
    static constexpr int AlignRight = 2;
    static constexpr int AlignUndefined = 3;
    static constexpr int AlignJustified = 4;
    static constexpr int AlignTop = 0;
    static constexpr int AlignMiddle = 1;
    static constexpr int AlignBottom = 2;

    // ---- Check-box glyph presets (ZapfDingbats code points) ----
    static constexpr int CheckBoxStyleCircle = 108;
    static constexpr int CheckBoxStyleCheck = 52;
    static constexpr int CheckBoxStyleCross = 56;
    static constexpr int CheckBoxStyleDiamond = 117;
    static constexpr int CheckBoxStyleStar = 72;
    static constexpr int CheckBoxStyleSquare = 110;
    static constexpr int CheckBoxStyleUndefined = 32;

    FormFieldFacade() = default;

    // Reset every appearance attribute to its default.
    void Reset();

    int BorderStyle() const noexcept;
    void BorderStyle(int value) noexcept;
    float BorderWidth() const noexcept;
    void BorderWidth(float value) noexcept;

    FontStyle Font() const noexcept;
    void Font(FontStyle value) noexcept;
    const std::string& CustomFont() const noexcept;
    void CustomFont(std::string value);
    float FontSize() const noexcept;
    void FontSize(float value) noexcept;
    EncodingType TextEncoding() const noexcept;
    void TextEncoding(EncodingType value) noexcept;

    int Alignment() const noexcept;
    void Alignment(int value) noexcept;
    int Rotation() const noexcept;
    void Rotation(int value) noexcept;

    const std::string& Caption() const noexcept;
    void Caption(std::string value);
    int ButtonStyle() const noexcept;
    void ButtonStyle(int value) noexcept;

    const std::vector<float>& Position() const noexcept;
    void Position(std::vector<float> value);
    int PageNumber() const noexcept;
    void PageNumber(int value) noexcept;

    const std::vector<std::string>& Items() const noexcept;
    void Items(std::vector<std::string> value);
    const std::vector<std::vector<std::string>>& ExportItems() const noexcept;
    void ExportItems(std::vector<std::vector<std::string>> value);

private:
    int border_style_ = BorderStyleUndefined;
    float border_width_ = BorderWidthUndefined;
    FontStyle font_ = FontStyle{};
    std::string custom_font_;
    float font_size_ = 0.0f;
    EncodingType text_encoding_ = EncodingType{};
    int alignment_ = AlignUndefined;
    int rotation_ = 0;
    std::string caption_;
    int button_style_ = CheckBoxStyleUndefined;
    std::vector<float> position_;
    int page_number_ = 1;
    std::vector<std::string> items_;
    std::vector<std::vector<std::string>> export_items_;
};

}  // namespace Aspose::Pdf::Facades
