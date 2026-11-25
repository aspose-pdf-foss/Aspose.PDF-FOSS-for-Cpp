#pragma once

// =============================================================================
// foundation::field_appearance — appearance-stream (/AP) content generators
// for AcroForm widgets (PDF §12.5.5, §12.7.4). Each function returns the body
// of a Form XObject in widget-local space [0 0 w h]; the caller wraps it in a
// stream with a /BBox of [0 0 w h] and a font resource named by the k*FontKey
// constants below. Shared by the AcroForm save path and Form::Flatten.
// =============================================================================

#include <string>
#include <vector>

namespace foundation::field_appearance {

// Font resource used by text-bearing appearances (text, list, button).
inline constexpr const char* kTextFontKey = "Helv";
inline constexpr const char* kTextBaseFont = "Helvetica";
// Font resource declared on check-box appearances (the mark itself is drawn
// with vector strokes, so the font is never selected).
inline constexpr const char* kCheckFontKey = "ZaDb";
inline constexpr const char* kCheckBaseFont = "ZapfDingbats";

// Escape '(', ')' and '\' for a PDF literal string.
std::string EscapeLiteral(const std::string& s);

// Single-line text field: white box with a 1pt black border and `value` in
// /Helv 12 (no text drawn when `value` is empty).
std::string TextBox(double w, double h, const std::string& value);

// List box: one option per line, the `selected` row (0-based; -1 = none)
// highlighted.
std::string ListBox(double w, double h,
                    const std::vector<std::string>& options, int selected);

// Check box: the box outline, plus a green check mark when `on`.
std::string CheckBox(double w, double h, bool on);

// Radio button: an outer ring, plus a filled inner dot when `on`.
std::string RadioButton(double w, double h, bool on);

// Push button: filled box with a centred white caption ("Button" if empty).
std::string PushButton(double w, double h, const std::string& caption);

// ---- Text layout -------------------------------------------------------------

// Glyph advance widths (1000-unit em) for laying out single-byte text:
// a simple font's /FirstChar + /Widths when given, else the Standard-14
// font `base_font`.
struct FontMetrics {
    std::string base_font = "Helvetica";  // Standard-14 name
    int first_char = 0;
    std::vector<double> widths;
    double missing_width = 500.0;
    double Width(unsigned char c) const;
};

// A Standard-14 face for a /DA font name or a font family name.
struct Standard14Face {
    std::string key;        // conventional resource name, e.g. "HeBo"
    std::string base_font;  // e.g. "Helvetica-Bold"
};
// Helv / Arial* / Helvetica* -> Helvetica, TiRo / Times* -> Times,
// Cour / Courier* -> Courier; bold / italic from the name; anything else
// -> Helvetica.
Standard14Face MapFontName(const std::string& name);

// The font, size and colour operator of a /DA string ("/Helv 0 Tf 0 g").
struct ParsedDa {
    std::string font_key;   // "" when no Tf
    double size = 0.0;      // 0 = auto
    std::string color = "0 g";
};
ParsedDa ParseDa(const std::string& da);

// Width of `text` at `size` points.
double TextWidth(const std::string& text, double size, const FontMetrics& m);

// Break `text` into lines no wider than `max_width` at `size`: at spaces,
// at '\n', and inside a word longer than a line.
std::vector<std::string> WrapText(const std::string& text, double max_width,
                                  double size, const FontMetrics& m);

// How a text field's value is laid out.
struct TextLayoutStyle {
    std::string font_key = kTextFontKey;  // font resource name
    FontMetrics metrics;
    double size = 12.0;          // 0 = auto
    std::string color = "0 g";   // fill-colour operator
    int quadding = 0;            // 0 left, 1 centre, 2 right
    bool multiline = false;
    bool comb = false;           // with max_len > 0, single-line only
    int max_len = 0;
};

// Text field: white box with a 1pt black border and `value` laid out per
// `style` (wrapped when multi-line, one cell per character for comb),
// clipped to the box.
std::string TextField(double w, double h, const std::string& value,
                      const TextLayoutStyle& style);

}  // namespace foundation::field_appearance
