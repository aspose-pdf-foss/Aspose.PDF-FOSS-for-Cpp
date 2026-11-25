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

}  // namespace foundation::field_appearance
