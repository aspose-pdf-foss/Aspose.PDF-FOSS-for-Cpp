#include "field_appearance.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>

#include "standard14_metrics.hpp"

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
    return TextField(w, h, value, TextLayoutStyle{});
}

// ---- Text layout -------------------------------------------------------------

double FontMetrics::Width(unsigned char c) const {
    if (widths.empty()) {
        const char ch[2] = {static_cast<char>(c), '\0'};
        const int wdt = c < 0x80 ? foundation::standard14_metrics::GetCharWidth(
                                       base_font, std::string_view(ch, 1))
                                 : 0;
        return wdt > 0 ? wdt : missing_width;
    }
    const int i = static_cast<int>(c) - first_char;
    if (i >= 0 && static_cast<std::size_t>(i) < widths.size() && widths[i] > 0)
        return widths[static_cast<std::size_t>(i)];
    return missing_width;
}

Standard14Face MapFontName(const std::string& name) {
    std::string n;
    for (char c : name)
        n += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    auto has = [&](const char* s) { return n.find(s) != std::string::npos; };
    // Conventional Acrobat resource names first.
    if (n == "helv") return {"Helv", "Helvetica"};
    if (n == "hebo") return {"HeBo", "Helvetica-Bold"};
    if (n == "tiro") return {"TiRo", "Times-Roman"};
    if (n == "tibo") return {"TiBo", "Times-Bold"};
    if (n == "tiit") return {"TiIt", "Times-Italic"};
    if (n == "tibi") return {"TiBI", "Times-BoldItalic"};
    if (n == "cour") return {"Cour", "Courier"};
    if (n == "cobo") return {"CoBo", "Courier-Bold"};
    const bool bold = has("bold") || has("black") || has("heavy");
    const bool italic = has("italic") || has("oblique");
    if (has("times") || has("tiro") || (has("serif") && !has("sans"))) {
        if (bold && italic) return {"TiBI", "Times-BoldItalic"};
        if (bold) return {"TiBo", "Times-Bold"};
        if (italic) return {"TiIt", "Times-Italic"};
        return {"TiRo", "Times-Roman"};
    }
    if (has("cour") || has("mono")) {
        if (bold && italic) return {"CoBO", "Courier-BoldOblique"};
        if (bold) return {"CoBo", "Courier-Bold"};
        if (italic) return {"CoOb", "Courier-Oblique"};
        return {"Cour", "Courier"};
    }
    if (bold && italic) return {"HeBO", "Helvetica-BoldOblique"};
    if (bold) return {"HeBo", "Helvetica-Bold"};
    if (italic) return {"HeOb", "Helvetica-Oblique"};
    return {"Helv", "Helvetica"};
}

ParsedDa ParseDa(const std::string& da) {
    ParsedDa out;
    std::vector<std::string> tokens;
    std::istringstream in(da);
    for (std::string t; in >> t;) tokens.push_back(t);
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const std::string& op = tokens[i];
        if (op == "Tf" && i >= 2 && !tokens[i - 2].empty() &&
            tokens[i - 2][0] == '/') {
            out.font_key = tokens[i - 2].substr(1);
            try { out.size = std::stod(tokens[i - 1]); } catch (...) {}
        } else if ((op == "g" && i >= 1) || (op == "rg" && i >= 3) ||
                   (op == "k" && i >= 4)) {
            const std::size_t n = op == "g" ? 1 : op == "rg" ? 3 : 4;
            std::string c;
            for (std::size_t j = i - n; j < i; ++j) c += tokens[j] + " ";
            out.color = c + op;
        }
    }
    return out;
}

double TextWidth(const std::string& text, double size, const FontMetrics& m) {
    double total = 0.0;
    for (char c : text) total += m.Width(static_cast<unsigned char>(c));
    return total * size / 1000.0;
}

std::vector<std::string> WrapText(const std::string& text, double max_width,
                                  double size, const FontMetrics& m) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (true) {  // one paragraph per '\n'
        const std::size_t nl = text.find('\n', start);
        const std::string para =
            text.substr(start, nl == std::string::npos ? std::string::npos
                                                       : nl - start);
        std::string line;
        std::istringstream words(para);
        auto push_word = [&](std::string word) {
            const std::string candidate = line.empty() ? word : line + " " + word;
            if (TextWidth(candidate, size, m) <= max_width) {
                line = candidate;
                return;
            }
            if (!line.empty()) {
                lines.push_back(line);
                line.clear();
            }
            // A word wider than the line is cut by characters.
            while (TextWidth(word, size, m) > max_width && word.size() > 1) {
                std::size_t k = 1;
                while (k < word.size() &&
                       TextWidth(word.substr(0, k + 1), size, m) <= max_width)
                    ++k;
                lines.push_back(word.substr(0, k));
                word = word.substr(k);
            }
            line = word;
        };
        for (std::string word; words >> word;) push_word(word);
        lines.push_back(line);
        if (nl == std::string::npos) break;
        start = nl + 1;
    }
    return lines;
}

std::string TextField(double w, double h, const std::string& value,
                      const TextLayoutStyle& st) {
    std::ostringstream cs;
    cs << "q\n";
    FieldChrome(cs, w, h);
    cs << "0 0 " << w << " " << h << " re W n\n";
    if (value.empty()) {
        cs << "Q\n";
        return cs.str();
    }
    constexpr double kPad = 2.0, kMinSize = 4.0, kLeading = 1.15;
    const double inner_w = std::max(1.0, w - 2 * kPad);
    const double inner_h = std::max(1.0, h - 2 * kPad);
    const bool comb = st.comb && st.max_len > 0 && !st.multiline;
    const std::string text =
        comb && value.size() > static_cast<std::size_t>(st.max_len)
            ? value.substr(0, static_cast<std::size_t>(st.max_len))
            : value;

    double size = st.size;
    std::vector<std::string> lines;
    if (st.multiline) {
        if (size <= 0.0) {  // auto: 12pt, shrunk until the lines fit
            size = 12.0;
            while (size > kMinSize) {
                lines = WrapText(text, inner_w, size, st.metrics);
                if (lines.size() * size * kLeading <= inner_h) break;
                size -= 0.5;
            }
        }
        lines = WrapText(text, inner_w, size, st.metrics);
    } else {
        if (size <= 0.0) {  // auto: fit the height, then the width
            size = inner_h * 0.8;
            const double cell = comb ? w / st.max_len : 0.0;
            const double need =
                comb ? (TextWidth("W", 1.0, st.metrics))
                     : TextWidth(text, 1.0, st.metrics);
            const double room = comb ? cell - 1.0 : inner_w;
            if (need > 0.0) size = std::min(size, room / need);
            size = std::max(size, kMinSize);
        }
        lines.push_back(text);
    }

    cs << "BT\n/" << st.font_key << " " << size << " Tf\n" << st.color << "\n";
    if (comb) {
        // One character per cell, centred in it.
        const double cell = w / st.max_len;
        const double y = (h - size * 0.72) / 2.0;
        double prev_x = 0.0, prev_y = 0.0;
        for (std::size_t i = 0; i < text.size(); ++i) {
            const std::string ch(1, text[i]);
            const double x = cell * static_cast<double>(i) +
                             (cell - TextWidth(ch, size, st.metrics)) / 2.0;
            cs << (x - prev_x) << ' ' << (y - prev_y) << " Td\n("
               << EscapeLiteral(ch) << ") Tj\n";
            prev_x = x;
            prev_y = y;
        }
    } else {
        const double lh = size * kLeading;
        double y = st.multiline ? h - kPad - size * 0.85
                                : (h - size * 0.72) / 2.0;
        double prev_x = 0.0, prev_y = 0.0;
        for (const auto& line : lines) {
            const double tw = TextWidth(line, size, st.metrics);
            double x = kPad;
            if (st.quadding == 1) x = (w - tw) / 2.0;
            else if (st.quadding == 2) x = w - kPad - tw;
            cs << (x - prev_x) << ' ' << (y - prev_y) << " Td\n("
               << EscapeLiteral(line) << ") Tj\n";
            prev_x = x;
            prev_y = y;
            y -= lh;
        }
    }
    cs << "ET\nQ\n";
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
