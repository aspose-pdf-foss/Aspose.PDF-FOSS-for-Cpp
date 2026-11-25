#include "field_appearance.hpp"

#include <algorithm>
#include <sstream>

namespace foundation::field_appearance {

namespace {

// A 1pt black field border + white background, drawn in widget-local space,
// so every field is visibly outlined in any viewer (not reliant on the viewer
// painting /MK chrome).
void FieldChrome(std::ostringstream& cs, double w, double h) {
    cs << "1 1 1 rg 0 0 " << w << ' ' << h << " re f\n"     // white fill
       << "0 G 1 w 0.5 0.5 " << (w - 1.0) << ' ' << (h - 1.0)
       << " re S\n";                                       // black border
}

// Circle of radius r centred on (cx, cy) as four Bezier arcs, closed by `op`
// ("S" stroke or "f" fill).
void CirclePath(std::ostringstream& cs, double cx, double cy, double r,
                const char* op) {
    const double k = 0.5523;
    cs << (cx - r) << ' ' << cy << " m\n"
       << (cx - r) << ' ' << (cy + k * r) << ' ' << (cx - k * r) << ' '
       << (cy + r) << ' ' << cx << ' ' << (cy + r) << " c\n"
       << (cx + k * r) << ' ' << (cy + r) << ' ' << (cx + r) << ' '
       << (cy + k * r) << ' ' << (cx + r) << ' ' << cy << " c\n"
       << (cx + r) << ' ' << (cy - k * r) << ' ' << (cx + k * r) << ' '
       << (cy - r) << ' ' << cx << ' ' << (cy - r) << " c\n"
       << (cx - k * r) << ' ' << (cy - r) << ' ' << (cx - r) << ' '
       << (cy - k * r) << ' ' << (cx - r) << ' ' << cy << " c\n" << op << '\n';
}

}  // namespace

std::string EscapeLiteral(const std::string& s) {
    std::string o;
    o.reserve(s.size());
    for (char c : s) {
        if (c == '(' || c == ')' || c == '\\') o += '\\';
        o += c;
    }
    return o;
}

std::string TextBox(double w, double h, const std::string& value) {
    std::ostringstream cs;
    cs << "q\n";
    FieldChrome(cs, w, h);
    cs << "0 0 " << w << " " << h << " re W n\n";
    if (!value.empty()) {
        cs << "BT\n/" << kTextFontKey << " " << 12.0 << " Tf\n0 g\n"
           << "2 " << (h * 0.30) << " Td\n(" << EscapeLiteral(value)
           << ") Tj\nET\n";
    }
    cs << "Q\n";
    return cs.str();
}

std::string ListBox(double w, double h,
                    const std::vector<std::string>& options, int selected) {
    std::ostringstream cs;
    cs << "q\n";
    FieldChrome(cs, w, h);
    cs << "0 0 " << w << " " << h << " re W n\n";
    const double fs = 11.0, lh = fs * 1.3;
    double y = h - lh + 2.0;
    for (std::size_t i = 0; i < options.size() && y > -lh; ++i) {
        if (static_cast<int>(i) == selected) {
            cs << "0.6 0.75 0.9 rg 1 " << (y - 2.0) << ' ' << (w - 2.0)
               << ' ' << lh << " re f\n";  // selection highlight
        }
        cs << "BT\n/Helv " << fs << " Tf\n0 g\n2 " << y << " Td\n("
           << EscapeLiteral(options[i]) << ") Tj\nET\n";
        y -= lh;
    }
    cs << "Q\n";
    return cs.str();
}

std::string CheckBox(double w, double h, bool on) {
    std::ostringstream cs;
    cs << "q\n";
    FieldChrome(cs, w, h);  // always show the box outline
    if (on) {
        // Green vector check mark — drawn with strokes rather than the
        // ZapfDingbats glyph so it renders in any engine.
        cs << "0 0.5 0 RG " << std::max(1.0, std::min(w, h) * 0.13)
           << " w 1 J 1 j " << (w * 0.22) << ' ' << (h * 0.50) << " m "
           << (w * 0.42) << ' ' << (h * 0.27) << " l " << (w * 0.78) << ' '
           << (h * 0.74) << " l S\n";
    }
    cs << "Q\n";
    return cs.str();
}

std::string RadioButton(double w, double h, bool on) {
    std::ostringstream cs;
    const double cx = w / 2.0, cy = h / 2.0, rad = std::min(w, h) / 2.0;
    cs << "q\n0 g\n0.8 w\n";
    CirclePath(cs, cx, cy, rad - 1.0, "S");   // outer ring
    if (on) CirclePath(cs, cx, cy, rad * 0.5, "f");  // inner dot
    cs << "Q\n";
    return cs.str();
}

std::string PushButton(double w, double h, const std::string& caption) {
    const std::string cap = caption.empty() ? std::string("Button") : caption;
    std::ostringstream bc;
    bc << "q 0.18 0.20 0.45 rg 0 0 " << w << ' ' << h
       << " re f 1 1 1 rg BT /Helv 13 Tf "
       << (w / 2.0 - cap.size() * 3.6) << ' ' << (h / 2.0 - 5.0)
       << " Td (" << EscapeLiteral(cap) << ") Tj ET Q\n";
    return bc.str();
}

}  // namespace foundation::field_appearance
