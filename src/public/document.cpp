#include <aspose/pdf/document.hpp>
#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/caret_annotation.hpp>
#include <aspose/pdf/annotations/circle_annotation.hpp>
#include <aspose/pdf/annotations/explicit_destination.hpp>
#include <aspose/pdf/annotations/explicit_destination_type.hpp>
#include <aspose/pdf/annotations/fit_b_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_bh_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_bv_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_h_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_r_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_v_explicit_destination.hpp>
#include <aspose/pdf/annotations/go_to_action.hpp>
#include <aspose/pdf/annotations/go_to_uri_action.hpp>
#include <aspose/pdf/annotations/highlight_annotation.hpp>
#include <aspose/pdf/annotations/named_destination.hpp>
#include <aspose/pdf/annotations/xyz_explicit_destination.hpp>
#include <aspose/pdf/annotations/link_annotation.hpp>
#include <aspose/pdf/annotations/redaction_annotation.hpp>
#include <aspose/pdf/annotations/square_annotation.hpp>
#include <aspose/pdf/annotations/squiggly_annotation.hpp>
#include <aspose/pdf/annotations/stamp_annotation.hpp>
#include <aspose/pdf/annotations/strike_out_annotation.hpp>
#include <aspose/pdf/annotations/text_annotation.hpp>
#include <aspose/pdf/annotations/underline_annotation.hpp>
#include <aspose/pdf/annotations/line_annotation.hpp>
#include <aspose/pdf/annotations/ink_annotation.hpp>
#include <aspose/pdf/annotations/free_text_annotation.hpp>
#include <aspose/pdf/annotations/watermark_annotation.hpp>
#include <aspose/pdf/document_info.hpp>
#include <aspose/pdf/embedded_file_collection.hpp>
#include <aspose/pdf/named_destination_collection.hpp>
#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/cell.hpp>
#include <aspose/pdf/cells.hpp>
#include <aspose/pdf/drawing/circle.hpp>
#include <aspose/pdf/drawing/ellipse.hpp>
#include <aspose/pdf/drawing/graph.hpp>
#include <aspose/pdf/drawing/line.hpp>
#include <aspose/pdf/drawing/rectangle.hpp>
#include <aspose/pdf/drawing/shape.hpp>
#include <aspose/pdf/graph_info.hpp>
#include <aspose/pdf/outline_collection.hpp>
#include <aspose/pdf/outline_item_collection.hpp>
#include <aspose/pdf/outlines.hpp>
#include <aspose/pdf/page_label_collection.hpp>
#include <aspose/pdf/paragraphs.hpp>
#include <aspose/pdf/resources.hpp>
#include <aspose/pdf/row.hpp>
#include <aspose/pdf/rows.hpp>
#include <aspose/pdf/table.hpp>
#include <aspose/pdf/artifact.hpp>
#include <aspose/pdf/artifact_collection.hpp>
#include <aspose/pdf/floating_box.hpp>
#include <aspose/pdf/load_options.hpp>
#include <aspose/pdf/svg_load_options.hpp>
#include <aspose/pdf/x_image.hpp>
#include <aspose/pdf/x_image_collection.hpp>
#include <aspose/pdf/forms/button_field.hpp>
#include <aspose/pdf/forms/checkbox_field.hpp>
#include <aspose/pdf/forms/combo_box_field.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/file_select_box_field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/list_box_field.hpp>
#include <aspose/pdf/forms/password_box_field.hpp>
#include <aspose/pdf/forms/radio_button_field.hpp>
#include <aspose/pdf/forms/rich_text_box_field.hpp>
#include <aspose/pdf/forms/signature_field.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/metadata.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>

#include "crypt_filter.hpp"
#include "encrypt_parser.hpp"
#include "encrypt_writer.hpp"
#include "flate.hpp"
#include "objects.hpp"
#include "flate.hpp"
#include "content_stream.hpp"
#include "digest.hpp"
#include "outlines.hpp"
#include "pages_tree.hpp"
#include "pkcs7_sign.hpp"
#include "png_decoder.hpp"
#include "truetype.hpp"
#include "pdf_writer_incremental.hpp"
#include "trailer.hpp"
#include "xref.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <set>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ios>
#include <random>
#include <span>
#include <stdexcept>
#include <sstream>
#include <string>
#include <system_error>
#include <variant>

namespace Aspose::Pdf {

namespace {

// PDF-string encoder shared by the metadata flush path. UTF-8 input;
// emits PDFDocEncoding (ASCII subset) when the value is pure ASCII,
// otherwise UTF-16BE with a BOM prefix per PDF 32000-1:2008 §7.9.2.2.
std::vector<std::byte> EncodePdfString(const std::string& s) {
    bool ascii_only = true;
    for (char c : s) {
        if (static_cast<unsigned char>(c) >= 0x80) {
            ascii_only = false;
            break;
        }
    }
    std::vector<std::byte> out;
    if (ascii_only) {
        out.reserve(s.size());
        for (char c : s) {
            out.push_back(static_cast<std::byte>(c));
        }
        return out;
    }
    out.push_back(static_cast<std::byte>(0xFE));
    out.push_back(static_cast<std::byte>(0xFF));
    for (std::size_t i = 0; i < s.size(); ) {
        unsigned char c0 = static_cast<unsigned char>(s[i]);
        std::uint32_t cp;
        if (c0 < 0x80) {
            cp = c0;
            i += 1;
        } else if ((c0 & 0xE0) == 0xC0 && i + 1 < s.size()) {
            cp = ((c0 & 0x1Fu) << 6) |
                 (static_cast<unsigned char>(s[i + 1]) & 0x3Fu);
            i += 2;
        } else if ((c0 & 0xF0) == 0xE0 && i + 2 < s.size()) {
            cp = ((c0 & 0x0Fu) << 12) |
                 ((static_cast<unsigned char>(s[i + 1]) & 0x3Fu) << 6) |
                 (static_cast<unsigned char>(s[i + 2]) & 0x3Fu);
            i += 3;
        } else if ((c0 & 0xF8) == 0xF0 && i + 3 < s.size()) {
            cp = ((c0 & 0x07u) << 18) |
                 ((static_cast<unsigned char>(s[i + 1]) & 0x3Fu) << 12) |
                 ((static_cast<unsigned char>(s[i + 2]) & 0x3Fu) << 6) |
                 (static_cast<unsigned char>(s[i + 3]) & 0x3Fu);
            i += 4;
        } else {
            cp = 0xFFFD;
            i += 1;
        }
        if (cp <= 0xFFFF) {
            out.push_back(static_cast<std::byte>((cp >> 8) & 0xFF));
            out.push_back(static_cast<std::byte>(cp & 0xFF));
        } else {
            cp -= 0x10000;
            std::uint32_t high = 0xD800 + (cp >> 10);
            std::uint32_t low  = 0xDC00 + (cp & 0x3FF);
            out.push_back(static_cast<std::byte>((high >> 8) & 0xFF));
            out.push_back(static_cast<std::byte>(high & 0xFF));
            out.push_back(static_cast<std::byte>((low >> 8) & 0xFF));
            out.push_back(static_cast<std::byte>(low & 0xFF));
        }
    }
    return out;
}

std::vector<std::byte> ReadAll(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::system_error(
            std::error_code(errno, std::generic_category()),
            "Aspose::Pdf::Document: cannot open '" + filename + "' for reading");
    }
    const auto end = file.tellg();
    if (end < 0) {
        throw std::runtime_error(
            "Aspose::Pdf::Document: failed to size '" + filename + "'");
    }
    std::vector<std::byte> buffer(static_cast<std::size_t>(end));
    file.seekg(0, std::ios::beg);
    if (!buffer.empty() &&
        !file.read(reinterpret_cast<char*>(buffer.data()),
                   static_cast<std::streamsize>(buffer.size()))) {
        throw std::system_error(
            std::error_code(errno, std::generic_category()),
            "Aspose::Pdf::Document: read failed on '" + filename + "'");
    }
    return buffer;
}

void WriteAll(const std::string& filename,
              std::span<const std::byte> bytes) {
    std::ofstream out(filename, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::system_error(
            std::error_code(errno, std::generic_category()),
            "Aspose::Pdf::Document: cannot open '" + filename +
                "' for writing");
    }
    if (!bytes.empty() &&
        !out.write(reinterpret_cast<const char*>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()))) {
        throw std::system_error(
            std::error_code(errno, std::generic_category()),
            "Aspose::Pdf::Document: write failed on '" + filename + "'");
    }
}

}  // namespace

namespace {

// Minimal empty-PDF byte sequence — a /Pages tree with /Count 0
// and /Kids[]. Same shape TS+csharp peers seed via the matching
// Document() no-arg ctor; not byte-identical across libs (offsets
// recompute) but semantically identical (parses to a 0-page
// document).
std::vector<std::byte> BuildEmptyPdf() {
    auto encode = [](const std::string& s) {
        std::vector<std::byte> out;
        out.reserve(s.size());
        for (char c : s) {
            out.push_back(static_cast<std::byte>(
                static_cast<unsigned char>(c)));
        }
        return out;
    };
    auto pad10 = [](std::size_t n) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%010zu", n);
        return std::string(buf);
    };
    auto header = encode("%PDF-1.7\n");
    auto obj1 = encode("1 0 obj\n<</Type/Catalog/Pages 2 0 R>>\nendobj\n");
    auto obj2 = encode("2 0 obj\n<</Type/Pages/Count 0/Kids[]>>\nendobj\n");
    std::size_t obj1_off = header.size();
    std::size_t obj2_off = obj1_off + obj1.size();
    std::size_t xref_off = obj2_off + obj2.size();
    std::string xref_str =
        "xref\n0 3\n"
        "0000000000 65535 f \n" +
        pad10(obj1_off) + " 00000 n \n" +
        pad10(obj2_off) + " 00000 n \n"
        "trailer\n<</Size 3/Root 1 0 R>>\nstartxref\n" +
        std::to_string(xref_off) + "\n"
        "%%EOF\n";
    auto xref = encode(xref_str);
    std::vector<std::byte> total;
    total.reserve(header.size() + obj1.size() + obj2.size() + xref.size());
    total.insert(total.end(), header.begin(), header.end());
    total.insert(total.end(), obj1.begin(), obj1.end());
    total.insert(total.end(), obj2.begin(), obj2.end());
    total.insert(total.end(), xref.begin(), xref.end());
    return total;
}

// ---- Minimal SVG → PDF importer (S-1) --------------------------------------
// Sizes a single page to the SVG width/height (or viewBox) and renders the
// basic vector primitives. Text, transforms, curves, gradients and CSS
// styling are deferred (see SvgLoadOptions).

// Value of attribute `name` on an element's tag text, or "".
std::string SvgAttr(const std::string& el, const std::string& name) {
    std::size_t pos = 0;
    while ((pos = el.find(name, pos)) != std::string::npos) {
        const bool boundary =
            pos == 0 ||
            std::isspace(static_cast<unsigned char>(el[pos - 1]));
        std::size_t q = pos + name.size();
        while (q < el.size() &&
               std::isspace(static_cast<unsigned char>(el[q])))
            ++q;
        if (boundary && q < el.size() && el[q] == '=') {
            ++q;
            while (q < el.size() &&
                   std::isspace(static_cast<unsigned char>(el[q])))
                ++q;
            if (q < el.size() && (el[q] == '"' || el[q] == '\'')) {
                const char quote = el[q++];
                const std::size_t end = el.find(quote, q);
                if (end != std::string::npos) return el.substr(q, end - q);
            }
        }
        pos += name.size();
    }
    return {};
}

double SvgNum(const std::string& s, double dflt = 0.0) {
    if (s.empty()) return dflt;
    try {
        return std::stod(s);
    } catch (...) {
        return dflt;
    }
}

// Parse an SVG colour ("#rgb" / "#rrggbb" / a few named / "none") into 0..1
// RGB. Returns false for "none" / unrecognised.
bool SvgColor(std::string c, double& r, double& g, double& b) {
    auto trim = [](std::string& s) {
        while (!s.empty() &&
               std::isspace(static_cast<unsigned char>(s.front())))
            s.erase(s.begin());
        while (!s.empty() &&
               std::isspace(static_cast<unsigned char>(s.back())))
            s.pop_back();
    };
    trim(c);
    for (char& ch : c) ch = static_cast<char>(std::tolower(
                           static_cast<unsigned char>(ch)));
    if (c.empty() || c == "none" || c == "transparent") return false;
    if (c[0] == '#') {
        std::string h = c.substr(1);
        auto hx = [](const std::string& s) {
            return static_cast<double>(std::stoi(s, nullptr, 16));
        };
        try {
            if (h.size() == 3) {
                r = hx(std::string(2, h[0])) / 255.0;
                g = hx(std::string(2, h[1])) / 255.0;
                b = hx(std::string(2, h[2])) / 255.0;
                return true;
            }
            if (h.size() == 6) {
                r = hx(h.substr(0, 2)) / 255.0;
                g = hx(h.substr(2, 2)) / 255.0;
                b = hx(h.substr(4, 2)) / 255.0;
                return true;
            }
        } catch (...) {
            return false;
        }
        return false;
    }
    struct Named {
        const char* name;
        double r, g, b;
    };
    static const Named kNamed[] = {
        {"black", 0, 0, 0},       {"white", 1, 1, 1},
        {"red", 1, 0, 0},         {"green", 0, 0.5, 0},
        {"lime", 0, 1, 0},        {"blue", 0, 0, 1},
        {"yellow", 1, 1, 0},      {"gray", 0.5, 0.5, 0.5},
        {"grey", 0.5, 0.5, 0.5},  {"silver", 0.75, 0.75, 0.75},
        {"orange", 1, 0.65, 0},   {"purple", 0.5, 0, 0.5},
    };
    for (const auto& n : kNamed) {
        if (c == n.name) {
            r = n.r;
            g = n.g;
            b = n.b;
            return true;
        }
    }
    return false;
}

std::vector<std::byte> BuildSvgPdf(const std::string& svg) {
    auto encode = [](const std::string& s) {
        std::vector<std::byte> out;
        out.reserve(s.size());
        for (char c : s)
            out.push_back(static_cast<std::byte>(
                static_cast<unsigned char>(c)));
        return out;
    };

    // Root <svg ...> tag → page size.
    std::string root;
    {
        const std::size_t s = svg.find("<svg");
        if (s != std::string::npos) {
            const std::size_t e = svg.find('>', s);
            root = svg.substr(s, e == std::string::npos ? std::string::npos
                                                        : e - s);
        }
    }
    double page_w = SvgNum(SvgAttr(root, "width"), 0.0);
    double page_h = SvgNum(SvgAttr(root, "height"), 0.0);
    if (page_w <= 0.0 || page_h <= 0.0) {
        std::istringstream vb(SvgAttr(root, "viewBox"));
        double vx, vy, vw, vh;
        if (vb >> vx >> vy >> vw >> vh && vw > 0 && vh > 0) {
            page_w = vw;
            page_h = vh;
        }
    }
    if (page_w <= 0.0) page_w = 612.0;
    if (page_h <= 0.0) page_h = 792.0;

    std::string ops;
    auto paint = [&](const std::string& el, bool strokeShape) {
        double r = 0, g = 0, b = 0;
        const std::string fillA = SvgAttr(el, "fill");
        const std::string strokeA = SvgAttr(el, "stroke");
        const double sw = SvgNum(SvgAttr(el, "stroke-width"), 1.0);
        bool hasFill =
            !strokeShape && SvgColor(fillA.empty() ? "black" : fillA, r, g, b);
        double fr = r, fg = g, fb = b;
        double sr = 0, sg = 0, sb = 0;
        bool hasStroke = SvgColor(strokeA, sr, sg, sb);
        if (strokeShape && !hasStroke) {  // a line must be visible
            hasStroke = true;
            sr = sg = sb = 0;
        }
        char buf[128];
        if (hasFill) {
            std::snprintf(buf, sizeof(buf), "%.4f %.4f %.4f rg ", fr, fg, fb);
            ops += buf;
        }
        if (hasStroke) {
            std::snprintf(buf, sizeof(buf), "%.4f %.4f %.4f RG %.2f w ", sr, sg,
                          sb, sw);
            ops += buf;
        }
        ops += hasFill && hasStroke ? "B\n" : (hasFill ? "f\n" : "S\n");
    };
    auto ellipse = [&](double cx, double cy, double rx, double ry) {
        const double k = 0.5522847498307936;
        char b[256];
        std::snprintf(b, sizeof(b), "%.2f %.2f m ", cx + rx, cy);
        ops += b;
        std::snprintf(b, sizeof(b), "%.2f %.2f %.2f %.2f %.2f %.2f c ",
                      cx + rx, cy + k * ry, cx + k * rx, cy + ry, cx, cy + ry);
        ops += b;
        std::snprintf(b, sizeof(b), "%.2f %.2f %.2f %.2f %.2f %.2f c ",
                      cx - k * rx, cy + ry, cx - rx, cy + k * ry, cx - rx, cy);
        ops += b;
        std::snprintf(b, sizeof(b), "%.2f %.2f %.2f %.2f %.2f %.2f c ",
                      cx - rx, cy - k * ry, cx - k * rx, cy - ry, cx, cy - ry);
        ops += b;
        std::snprintf(b, sizeof(b), "%.2f %.2f %.2f %.2f %.2f %.2f c ",
                      cx + k * rx, cy - ry, cx + rx, cy - k * ry, cx + rx, cy);
        ops += b;
    };

    // Scan every element tag and render the shapes we support.
    std::size_t i = 0;
    while ((i = svg.find('<', i)) != std::string::npos) {
        const std::size_t e = svg.find('>', i);
        if (e == std::string::npos) break;
        const std::string el = svg.substr(i, e - i);
        i = e + 1;
        auto starts = [&](const char* t) {
            return el.rfind(std::string("<") + t, 0) == 0 &&
                   (el.size() == std::strlen(t) + 1 ||
                    !std::isalnum(static_cast<unsigned char>(
                        el[std::strlen(t) + 1])));
        };
        char buf[256];
        if (starts("rect")) {
            const double x = SvgNum(SvgAttr(el, "x"));
            const double y = SvgNum(SvgAttr(el, "y"));
            const double w = SvgNum(SvgAttr(el, "width"));
            const double h = SvgNum(SvgAttr(el, "height"));
            if (w > 0 && h > 0) {
                ops += "q ";
                std::snprintf(buf, sizeof(buf), "%.2f %.2f %.2f %.2f re ", x,
                              page_h - (y + h), w, h);
                ops += buf;
                paint(el, false);
                ops += "Q\n";
            }
        } else if (starts("line")) {
            const double x1 = SvgNum(SvgAttr(el, "x1"));
            const double y1 = SvgNum(SvgAttr(el, "y1"));
            const double x2 = SvgNum(SvgAttr(el, "x2"));
            const double y2 = SvgNum(SvgAttr(el, "y2"));
            ops += "q ";
            std::snprintf(buf, sizeof(buf), "%.2f %.2f m %.2f %.2f l ", x1,
                          page_h - y1, x2, page_h - y2);
            ops += buf;
            paint(el, true);
            ops += "Q\n";
        } else if (starts("circle")) {
            const double cx = SvgNum(SvgAttr(el, "cx"));
            const double cy = SvgNum(SvgAttr(el, "cy"));
            const double rr = SvgNum(SvgAttr(el, "r"));
            if (rr > 0) {
                ops += "q ";
                ellipse(cx, page_h - cy, rr, rr);
                paint(el, false);
                ops += "Q\n";
            }
        } else if (starts("ellipse")) {
            const double cx = SvgNum(SvgAttr(el, "cx"));
            const double cy = SvgNum(SvgAttr(el, "cy"));
            const double rx = SvgNum(SvgAttr(el, "rx"));
            const double ry = SvgNum(SvgAttr(el, "ry"));
            if (rx > 0 && ry > 0) {
                ops += "q ";
                ellipse(cx, page_h - cy, rx, ry);
                paint(el, false);
                ops += "Q\n";
            }
        } else if (starts("polyline") || starts("polygon")) {
            const bool closed = starts("polygon");
            std::istringstream ps(SvgAttr(el, "points"));
            std::string tok;
            bool first = true;
            std::string path;
            while (ps >> tok) {
                const std::size_t comma = tok.find(',');
                if (comma == std::string::npos) continue;
                const double px = SvgNum(tok.substr(0, comma));
                const double py = SvgNum(tok.substr(comma + 1));
                std::snprintf(buf, sizeof(buf), "%.2f %.2f %s ", px,
                              page_h - py, first ? "m" : "l");
                path += buf;
                first = false;
            }
            if (!first) {
                ops += "q ";
                ops += path;
                if (closed) ops += "h ";
                paint(el, !closed);
                ops += "Q\n";
            }
        } else if (starts("path")) {
            const std::string d = SvgAttr(el, "d");
            std::string path;
            double cx = 0, cy = 0;
            std::size_t p = 0;
            bool any = false;
            auto readNum = [&](double& out) -> bool {
                while (p < d.size() &&
                       (std::isspace(static_cast<unsigned char>(d[p])) ||
                        d[p] == ','))
                    ++p;
                const std::size_t st = p;
                while (p < d.size() &&
                       (std::isdigit(static_cast<unsigned char>(d[p])) ||
                        d[p] == '.' || d[p] == '-' || d[p] == '+' ||
                        d[p] == 'e' || d[p] == 'E'))
                    ++p;
                if (p == st) return false;
                out = SvgNum(d.substr(st, p - st));
                return true;
            };
            while (p < d.size()) {
                const char cmd = d[p];
                if (std::isalpha(static_cast<unsigned char>(cmd))) {
                    ++p;
                    double nx, ny;
                    if (cmd == 'M' || cmd == 'L') {
                        if (readNum(nx) && readNum(ny)) {
                            cx = nx;
                            cy = ny;
                            std::snprintf(buf, sizeof(buf), "%.2f %.2f %s ", cx,
                                          page_h - cy, cmd == 'M' ? "m" : "l");
                            path += buf;
                            any = true;
                        }
                    } else if (cmd == 'H') {
                        if (readNum(nx)) {
                            cx = nx;
                            std::snprintf(buf, sizeof(buf), "%.2f %.2f l ", cx,
                                          page_h - cy);
                            path += buf;
                        }
                    } else if (cmd == 'V') {
                        if (readNum(ny)) {
                            cy = ny;
                            std::snprintf(buf, sizeof(buf), "%.2f %.2f l ", cx,
                                          page_h - cy);
                            path += buf;
                        }
                    } else if (cmd == 'Z' || cmd == 'z') {
                        path += "h ";
                    }
                    // Unsupported commands (curves/arcs) are skipped.
                } else {
                    ++p;
                }
            }
            if (any) {
                ops += "q ";
                ops += path;
                paint(el, false);
                ops += "Q\n";
            }
        }
    }

    // Assemble a single-page PDF.
    auto pad10 = [](std::size_t n) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%010zu", n);
        return std::string(buf);
    };
    char mb[128];
    std::snprintf(mb, sizeof(mb), "0 0 %.2f %.2f", page_w, page_h);
    const std::string stream_body = ops;
    const std::string obj1s = "1 0 obj\n<</Type/Catalog/Pages 2 0 R>>\nendobj\n";
    const std::string obj2s =
        "2 0 obj\n<</Type/Pages/Count 1/Kids[3 0 R]>>\nendobj\n";
    const std::string obj3s = "3 0 obj\n<</Type/Page/Parent 2 0 R/MediaBox[" +
                              std::string(mb) +
                              "]/Contents 4 0 R/Resources<<>>>>\nendobj\n";
    const std::string obj4s = "4 0 obj\n<</Length " +
                              std::to_string(stream_body.size()) + ">>\nstream\n" +
                              stream_body + "\nendstream\nendobj\n";
    const std::string header = "%PDF-1.7\n";
    const std::size_t o1 = header.size();
    const std::size_t o2 = o1 + obj1s.size();
    const std::size_t o3 = o2 + obj2s.size();
    const std::size_t o4 = o3 + obj3s.size();
    const std::size_t xref_off = o4 + obj4s.size();
    const std::string xref =
        "xref\n0 5\n0000000000 65535 f \n" + pad10(o1) + " 00000 n \n" +
        pad10(o2) + " 00000 n \n" + pad10(o3) + " 00000 n \n" + pad10(o4) +
        " 00000 n \ntrailer\n<</Size 5/Root 1 0 R>>\nstartxref\n" +
        std::to_string(xref_off) + "\n%%EOF\n";
    return encode(header + obj1s + obj2s + obj3s + obj4s + xref);
}

}  // namespace

Document::Document() {
    bytes_ = BuildEmptyPdf();
    auto bytes_view = std::span<const std::byte>(bytes_.data(), bytes_.size());
    info_id_ = foundation::xref::Parse(bytes_view).info_id;
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(bytes_view));
}

Document::Document(const std::string& filename) {
    bytes_ = ReadAll(filename);
    source_filename_ = filename;
    auto bytes_view = std::span<const std::byte>(bytes_.data(), bytes_.size());
    info_id_ = foundation::xref::Parse(bytes_view).info_id;
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(bytes_view));
}

Document::Document(const std::string& filename,
                   const Aspose::Pdf::LoadOptions& options) {
    // v1: SvgLoadOptions drives an SVG → PDF import; any other LoadOptions
    // loads the file as a PDF.
    if (dynamic_cast<const Aspose::Pdf::SvgLoadOptions*>(&options) != nullptr) {
        std::ifstream f(filename, std::ios::binary);
        const std::string svg((std::istreambuf_iterator<char>(f)),
                              std::istreambuf_iterator<char>());
        bytes_ = BuildSvgPdf(svg);
    } else {
        bytes_ = ReadAll(filename);
    }
    source_filename_ = filename;
    auto bytes_view = std::span<const std::byte>(bytes_.data(), bytes_.size());
    info_id_ = foundation::xref::Parse(bytes_view).info_id;
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(bytes_view));
}

namespace {

// Slice off the trailing PDF section that corresponds to the
// LAST `startxref/%%EOF` pair. The
// foundation::trailer::Parse uses a forward scan inside the last
// 4 KiB; on a PDF with an incremental update (two or more
// `startxref/%%EOF` blocks within 4 KiB of the file's end) it
// returns the FIRST one, which points at the original — pre-
// update — trailer and misses any /Encrypt entry added by the
// update. Slicing past every earlier `%%EOF` ensures only the
// last `startxref` survives in the input the parser sees.
//
// Byte offsets inside indirect objects are NOT affected — those
// continue to be resolved against the full bytes_ buffer when
// foundation::objects::Parse runs.
std::span<const std::byte> LastTrailerSlice(
        std::span<const std::byte> bytes) {
    static constexpr std::string_view kEof = "%%EOF";
    // Walk backwards finding `%%EOF` occurrences. The slice
    // starts JUST AFTER the second-to-last one.
    std::size_t found_count = 0;
    std::size_t second_to_last = 0;
    std::size_t i = bytes.size();
    while (i >= kEof.size()) {
        --i;
        if (i + kEof.size() > bytes.size()) continue;
        if (std::memcmp(bytes.data() + i,
                         kEof.data(), kEof.size()) == 0) {
            ++found_count;
            if (found_count == 2) {
                second_to_last = i + kEof.size();
                break;
            }
        }
        if (i == 0) break;
    }
    if (found_count >= 2) {
        return bytes.subspan(second_to_last);
    }
    return bytes;
}

// Manual scan for /ID[0] in the trailer dictionary. Required for
// V<5 encrypt-parser authentication (Algorithm 2 + Algorithm 5
// both consume /ID[0]). Returns an empty vector if /ID is not
// present (V=5 ignores file_id; an empty span is the canonical
// input there).
//
// Scans backwards for the LAST `trailer` keyword in the source,
// then forward for /ID followed by `[`, then a hex-string `<...>`
// (the standard pikepdf / qpdf emission). Literal-string /ID
// values (rare in the wild) are not handled — they'd return
// empty and V<5 authentication would then fail with a clear
// "file_id required" error from encrypt_parser.
std::vector<std::byte> ExtractFileIdFromTrailer(
        std::span<const std::byte> bytes) {
    static constexpr char kTrailer[] = "trailer";
    static constexpr std::size_t kTrailerLen = 7;
    auto is_ws = [](std::byte b) {
        const auto c = static_cast<unsigned char>(b);
        return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f';
    };
    auto is_hex = [](std::byte b) {
        const auto c = static_cast<unsigned char>(b);
        return (c >= '0' && c <= '9') ||
               (c >= 'a' && c <= 'f') ||
               (c >= 'A' && c <= 'F');
    };
    auto nyb = [](std::byte b) -> int {
        const auto c = static_cast<unsigned char>(b);
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        return 10 + (c - 'A');
    };

    // Last trailer keyword.
    std::size_t trailer_off = std::string::npos;
    if (bytes.size() < kTrailerLen) return {};
    for (std::size_t i = bytes.size() - kTrailerLen + 1; i > 0; ) {
        --i;
        if (std::memcmp(bytes.data() + i, kTrailer, kTrailerLen) == 0) {
            // boundary check — char before must be whitespace or
            // start-of-file; char after must be whitespace.
            const bool before_ok =
                (i == 0) || is_ws(bytes[i - 1]);
            const bool after_ok =
                (i + kTrailerLen >= bytes.size())
                || is_ws(bytes[i + kTrailerLen]);
            if (before_ok && after_ok) {
                trailer_off = i + kTrailerLen;
                break;
            }
        }
    }
    if (trailer_off == std::string::npos) return {};

    // Scan for `/ID` inside the trailer dict.
    std::size_t pos = trailer_off;
    while (pos + 3 <= bytes.size()) {
        if (static_cast<unsigned char>(bytes[pos])     == '/' &&
            static_cast<unsigned char>(bytes[pos + 1]) == 'I' &&
            static_cast<unsigned char>(bytes[pos + 2]) == 'D') {
            const std::size_t after = pos + 3;
            const bool boundary =
                (after >= bytes.size())
                || is_ws(bytes[after])
                || static_cast<unsigned char>(bytes[after]) == '['
                || static_cast<unsigned char>(bytes[after]) == '/';
            if (boundary) {
                pos = after;
                break;
            }
        }
        ++pos;
    }
    if (pos >= bytes.size()) return {};

    // Skip to `[`.
    while (pos < bytes.size() &&
           static_cast<unsigned char>(bytes[pos]) != '[') ++pos;
    if (pos >= bytes.size()) return {};
    ++pos;

    // Skip whitespace, expect `<` (hex string).
    while (pos < bytes.size() && is_ws(bytes[pos])) ++pos;
    if (pos >= bytes.size() ||
        static_cast<unsigned char>(bytes[pos]) != '<') {
        return {};
    }
    ++pos;

    // Collect hex digits up to `>`.
    std::vector<std::byte> out;
    out.reserve(16);
    int hi = -1;
    while (pos < bytes.size() &&
           static_cast<unsigned char>(bytes[pos]) != '>') {
        if (is_hex(bytes[pos])) {
            const int v = nyb(bytes[pos]);
            if (hi < 0) {
                hi = v;
            } else {
                out.push_back(static_cast<std::byte>((hi << 4) | v));
                hi = -1;
            }
        }
        ++pos;
    }
    if (hi >= 0) {
        // Trailing odd hex digit per PDF spec — pad with low nibble 0.
        out.push_back(static_cast<std::byte>(hi << 4));
    }
    return out;
}

// Helpers to project objects::Value variants into concrete types.

const foundation::objects::Dict* AsDict(
        const foundation::objects::Value& v) {
    return std::get_if<foundation::objects::Dict>(&v.v);
}

const foundation::objects::String* AsString(
        const foundation::objects::Value& v) {
    return std::get_if<foundation::objects::String>(&v.v);
}

const std::int64_t* AsInt(const foundation::objects::Value& v) {
    return std::get_if<std::int64_t>(&v.v);
}

const bool* AsBool(const foundation::objects::Value& v) {
    return std::get_if<bool>(&v.v);
}

const foundation::objects::Dict* FindEncryptDict(
        const foundation::objects::Dump& dump,
        std::uint32_t encrypt_id) {
    for (const auto& obj : dump.objects) {
        if (obj.id == encrypt_id) {
            return AsDict(obj.value);
        }
    }
    return nullptr;
}

const foundation::objects::Value* FindEntry(
        const foundation::objects::Dict& d,
        const std::string& key) {
    for (const auto& [k, v] : d.entries) {
        if (k == key) return &v;
    }
    return nullptr;
}

// Build an encrypt_parser::EncryptDict carrier from the parsed
// /Encrypt dictionary entries.
foundation::encrypt_parser::EncryptDict BuildEncryptDict(
        const foundation::objects::Dict& d) {
    foundation::encrypt_parser::EncryptDict enc{};
    if (auto* p = FindEntry(d, "V");      p && AsInt(*p)) enc.v = static_cast<int>(*AsInt(*p));
    if (auto* p = FindEntry(d, "R");      p && AsInt(*p)) enc.r = static_cast<int>(*AsInt(*p));
    if (auto* p = FindEntry(d, "Length"); p && AsInt(*p)) {
        enc.length_bits = static_cast<int>(*AsInt(*p));
    } else if (enc.v == 1) {
        enc.length_bits = 40;
    } else if (enc.v == 4) {
        enc.length_bits = 128;
    } else if (enc.v == 5) {
        enc.length_bits = 256;
    }
    if (auto* p = FindEntry(d, "P"); p && AsInt(*p)) {
        enc.p = static_cast<std::int32_t>(*AsInt(*p));
    }
    if (auto* p = FindEntry(d, "O");     p && AsString(*p)) enc.o     = AsString(*p)->bytes;
    if (auto* p = FindEntry(d, "U");     p && AsString(*p)) enc.u     = AsString(*p)->bytes;
    if (auto* p = FindEntry(d, "OE");    p && AsString(*p)) enc.oe    = AsString(*p)->bytes;
    if (auto* p = FindEntry(d, "UE");    p && AsString(*p)) enc.ue    = AsString(*p)->bytes;
    if (auto* p = FindEntry(d, "Perms"); p && AsString(*p)) enc.perms = AsString(*p)->bytes;
    enc.encrypt_metadata = true;
    if (auto* p = FindEntry(d, "EncryptMetadata"); p && AsBool(*p)) {
        enc.encrypt_metadata = *AsBool(*p);
    }
    return enc;
}

std::vector<std::byte> StringToBytes(const std::string& s) {
    std::vector<std::byte> out(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        out[i] = static_cast<std::byte>(
            static_cast<unsigned char>(s[i]));
    }
    return out;
}

}  // namespace

Document::Document(const std::string& filename,
                   const std::string& password) {
    bytes_ = ReadAll(filename);
    source_filename_ = filename;
    auto view = std::span<const std::byte>(bytes_.data(), bytes_.size());

    // Trailer parsing is performed against the slice past every
    // earlier `%%EOF` so the foundation::trailer::Parse
    // sees only the LAST startxref — see LastTrailerSlice's doc
    // comment for the underlying defect.
    const auto trailer_view = LastTrailerSlice(view);
    const auto trailer_info = foundation::trailer::Parse(trailer_view);
    info_id_ = trailer_info.info_id;

    if (trailer_info.encrypt_id != 0) {
        // Resolve /Encrypt dict via objects::Parse and authenticate
        // the supplied password. objects::Parse runs on the FULL
        // buffer because xref offsets resolve into the original
        // (non-sliced) bytes.
        const auto dump = foundation::objects::Parse(view);
        const auto* enc_dict = FindEncryptDict(dump, trailer_info.encrypt_id);
        if (enc_dict == nullptr) {
            throw std::runtime_error(
                "Aspose::Pdf::Document: trailer references /Encrypt "
                "object id " + std::to_string(trailer_info.encrypt_id)
                + " but that object was not found or is not a dictionary");
        }

        auto enc = BuildEncryptDict(*enc_dict);
        const auto file_id = ExtractFileIdFromTrailer(trailer_view);
        const auto pwd = StringToBytes(password);

        const auto auth = foundation::encrypt_parser::Authenticate(
            enc,
            std::span<const std::byte>(pwd.data(), pwd.size()),
            std::span<const std::byte>(file_id.data(), file_id.size()));
        if (!auth.authenticated) {
            throw std::runtime_error(
                "Aspose::Pdf::Document: invalid password — neither "
                "the user (/U) nor owner (/O) entry of the "
                "trailer's /Encrypt dictionary matched");
        }
        is_encrypted_ = true;
        file_key_ = auth.file_key;
    }

    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(view));
}

bool Document::IsEncrypted() const noexcept {
    return is_encrypted_;
}

void Document::Decrypt() {
    // v1: flag-only flip — the in-memory bytes_ and the /Encrypt
    // dictionary in the trailer are NOT rewritten here (full
    // decrypt-then-save-as-plaintext lands alongside the write-
    // path crypto integration in Document::Encrypt). The
    // recovered file_key_ is preserved so consumers that wire
    // through the key in future beats can still see plaintext.
    is_encrypted_ = false;
}

namespace {

// CryptoAlgorithm + usePdf20 -> (V, R, length_bits, crypt_filter::Method).
struct AlgoVariant {
    int v;
    int r;
    int length_bits;
    foundation::crypt_filter::Method method;
};

AlgoVariant ResolveVariant(CryptoAlgorithm a, bool use_pdf20) {
    switch (a) {
        case CryptoAlgorithm::RC4x40:
            return {1, 2, 40,  foundation::crypt_filter::Method::V2};
        case CryptoAlgorithm::RC4x128:
            return {2, 3, 128, foundation::crypt_filter::Method::V2};
        case CryptoAlgorithm::AESx128:
            return {4, 4, 128, foundation::crypt_filter::Method::AESV2};
        case CryptoAlgorithm::AESx256:
            return use_pdf20
                ? AlgoVariant{5, 6, 256, foundation::crypt_filter::Method::AESV3}
                : AlgoVariant{5, 5, 256, foundation::crypt_filter::Method::AESV3};
    }
    throw std::runtime_error(
        "Aspose::Pdf::Document::Encrypt: unsupported CryptoAlgorithm");
}

// Fill `out` with CSPRNG bytes via std::random_device. Used for
// /ID generation, salt buffers, and AES-CBC per-object IVs.
void FillRandomBytes(std::span<std::byte> out) {
    std::random_device rd;
    for (std::size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<std::byte>(
            rd() & 0xFF);
    }
}

// Walk a Value tree, re-encrypting every String in-place AND
// replacing every Stream's body with a freshly-encrypted blob.
// New byte storage is appended to `owned_bufs` so the resulting
// span-typed Stream bodies survive past the call.
void EncryptValueInPlace(
        foundation::objects::Value& v,
        std::uint32_t obj_id,
        std::uint16_t generation,
        foundation::crypt_filter::Method method,
        std::span<const std::byte> file_key,
        std::vector<std::vector<std::byte>>& owned_bufs) {
    if (auto* s = std::get_if<foundation::objects::String>(&v.v)) {
        std::array<std::byte, 16> iv_buf{};
        if (method != foundation::crypt_filter::Method::V2) {
            FillRandomBytes(std::span<std::byte>(iv_buf));
        }
        const auto iv_span = (method == foundation::crypt_filter::Method::V2)
            ? std::span<const std::byte>{}
            : std::span<const std::byte>(iv_buf);
        s->bytes = foundation::crypt_filter::Encrypt(
            method, file_key, obj_id, generation,
            std::span<const std::byte>(s->bytes.data(), s->bytes.size()),
            iv_span);
        return;
    }
    if (auto* st = std::get_if<foundation::objects::Stream>(&v.v)) {
        std::array<std::byte, 16> iv_buf{};
        if (method != foundation::crypt_filter::Method::V2) {
            FillRandomBytes(std::span<std::byte>(iv_buf));
        }
        const auto iv_span = (method == foundation::crypt_filter::Method::V2)
            ? std::span<const std::byte>{}
            : std::span<const std::byte>(iv_buf);
        auto encrypted = foundation::crypt_filter::Encrypt(
            method, file_key, obj_id, generation,
            st->body, iv_span);
        owned_bufs.push_back(std::move(encrypted));
        st->body = std::span<const std::byte>(
            owned_bufs.back().data(),
            owned_bufs.back().size());
        // Walk into the stream's header dict too — its strings
        // (e.g. /Filter names are NAMES, not strings, so safe;
        // /DecodeParms can have nested dicts though, walked
        // recursively).
        for (auto& [k, sub] : st->header.entries) {
            EncryptValueInPlace(sub, obj_id, generation,
                                  method, file_key, owned_bufs);
        }
        return;
    }
    if (auto* a = std::get_if<foundation::objects::Array>(&v.v)) {
        for (auto& item : a->items) {
            EncryptValueInPlace(item, obj_id, generation,
                                  method, file_key, owned_bufs);
        }
        return;
    }
    if (auto* d = std::get_if<foundation::objects::Dict>(&v.v)) {
        for (auto& [k, sub] : d->entries) {
            EncryptValueInPlace(sub, obj_id, generation,
                                  method, file_key, owned_bufs);
        }
        return;
    }
    // Everything else (null / bool / int / real / name / ref)
    // doesn't carry encryptable content.
}

// Build the /Encrypt indirect object's Dict value from a
// populated EncryptDict (encrypt_writer output). All entries
// are caller-owned plaintext per PDF §7.6.2.
foundation::objects::Value BuildEncryptDictValue(
        const foundation::encrypt_parser::EncryptDict& enc) {
    foundation::objects::Dict d;
    auto add_int = [&](const char* k, int v) {
        foundation::objects::Value val;
        val.v = static_cast<std::int64_t>(v);
        d.entries.emplace_back(k, std::move(val));
    };
    auto add_name = [&](const char* k, std::string n) {
        foundation::objects::Value val;
        val.v = std::move(n);  // string -> Name on the wire
        d.entries.emplace_back(k, std::move(val));
    };
    auto add_hex = [&](const char* k,
                       const std::vector<std::byte>& bytes) {
        foundation::objects::String s;
        s.bytes = bytes;
        s.kind = foundation::objects::StringKind::Hex;
        foundation::objects::Value val;
        val.v = std::move(s);
        d.entries.emplace_back(k, std::move(val));
    };
    auto add_bool = [&](const char* k, bool b) {
        foundation::objects::Value val;
        val.v = b;
        d.entries.emplace_back(k, std::move(val));
    };

    add_name("Filter", "Standard");
    add_int("V", enc.v);
    add_int("R", enc.r);
    add_int("Length", enc.length_bits);
    add_int("P", enc.p);
    add_hex("O", enc.o);
    add_hex("U", enc.u);
    if (enc.v >= 5) {
        add_hex("OE", enc.oe);
        add_hex("UE", enc.ue);
        add_hex("Perms", enc.perms);
    }
    if (!enc.encrypt_metadata) {
        add_bool("EncryptMetadata", false);
    }

    foundation::objects::Value out;
    out.v = std::move(d);
    return out;
}

// Project a Permissions bitfield onto a signed-32-bit /P value
// per ISO 32000-1 §7.6.3.2 Table 22. Bits the canonical enum
// doesn't define (1-2, 7-8, 13+) are set to 1 — that's the PDF
// convention for "reserved / must be 1".
std::int32_t BuildPValue(Permissions p) {
    std::int32_t v = static_cast<std::int32_t>(0xFFFFF0C0u);
    // ^ bits 1, 2 set, 3-6 cleared, 7-8 set, 9-12 cleared,
    //   13+ all set (top half of the signed int).
    v |= static_cast<std::int32_t>(p);
    return v;
}

// Project a DocumentPrivilege onto a Permissions bitfield. See
// the header for the per-flag mapping notes.
Permissions PrivilegeToPermissions(
        const Aspose::Pdf::Facades::DocumentPrivilege& pr) {
    Permissions p = static_cast<Permissions>(0);
    if (pr.AllowPrint()) {
        p |= Permissions::PrintDocument | Permissions::PrintingQuality;
    } else if (pr.AllowDegradedPrinting()) {
        p |= Permissions::PrintDocument;
    }
    if (pr.AllowModifyContents())
        p |= Permissions::ModifyContent;
    if (pr.AllowCopy())
        p |= Permissions::ExtractContent;
    if (pr.AllowModifyAnnotations())
        p |= Permissions::ModifyTextAnnotations;
    if (pr.AllowFillIn())
        p |= Permissions::FillForm;
    if (pr.AllowScreenReaders())
        p |= Permissions::ExtractContentWithDisabilities;
    if (pr.AllowAssembly())
        p |= Permissions::AssembleDocument;
    return p;
}

}  // namespace

int Document::Permissions() const {
    if (encrypt_requested_) return BuildPValue(pending_permissions_);

    auto view = std::span<const std::byte>(bytes_.data(), bytes_.size());
    try {
        const auto trailer_view = LastTrailerSlice(view);
        const auto trailer_info = foundation::trailer::Parse(trailer_view);
        if (trailer_info.encrypt_id != 0) {
            const auto dump = foundation::objects::Parse(view);
            const auto* enc = FindEncryptDict(dump, trailer_info.encrypt_id);
            if (enc != nullptr) {
                for (const auto& kv : enc->entries) {
                    if (kv.first != "P") continue;
                    if (const auto* n = std::get_if<std::int64_t>(&kv.second.v))
                        return static_cast<int>(*n);
                }
            }
        }
    } catch (const std::exception&) {
    }

    // Not encrypted — every operation is permitted.
    const Aspose::Pdf::Permissions all =
        Aspose::Pdf::Permissions::PrintDocument |
        Aspose::Pdf::Permissions::ModifyContent |
        Aspose::Pdf::Permissions::ExtractContent |
        Aspose::Pdf::Permissions::ModifyTextAnnotations |
        Aspose::Pdf::Permissions::FillForm |
        Aspose::Pdf::Permissions::ExtractContentWithDisabilities |
        Aspose::Pdf::Permissions::AssembleDocument |
        Aspose::Pdf::Permissions::PrintingQuality;
    return BuildPValue(all);
}

void Document::Optimize() { optimize_requested_ = true; }
void Document::OptimizeResources() { optimize_requested_ = true; }

namespace {

// Collect every indirect-object id referenced by `v` (recursing dicts, arrays
// and stream headers).
void CollectObjectRefs(const foundation::objects::Value& v,
                       std::vector<std::uint32_t>& out) {
    if (const auto* r = std::get_if<foundation::objects::Ref>(&v.v)) {
        out.push_back(r->id);
    } else if (const auto* a =
                   std::get_if<foundation::objects::Array>(&v.v)) {
        for (const auto& it : a->items) CollectObjectRefs(it, out);
    } else if (const auto* d =
                   std::get_if<foundation::objects::Dict>(&v.v)) {
        for (const auto& kv : d->entries) CollectObjectRefs(kv.second, out);
    } else if (const auto* s =
                   std::get_if<foundation::objects::Stream>(&v.v)) {
        for (const auto& kv : s->header.entries) CollectObjectRefs(kv.second, out);
    }
}

// PDF name / dict-key escaping (matches the canonical writer).
std::string EscapeName(const std::string& name) {
    std::string out;
    for (unsigned char c : name) {
        if ((c >= 0x21 && c <= 0x7E) && c != '#' && c != '<' && c != '>' &&
            c != '(' && c != ')' && c != '[' && c != ']' && c != '{' &&
            c != '}' && c != '/' && c != '%') {
            out.push_back(static_cast<char>(c));
        } else {
            char buf[4];
            std::snprintf(buf, sizeof(buf), "#%02X", c);
            out += buf;
        }
    }
    return out;
}

std::string SerializeInline(const foundation::objects::Value& v);

std::string SerializeDictInline(const foundation::objects::Dict& dict) {
    if (dict.entries.empty()) return "<< >>";
    std::string out = "<<";
    for (const auto& kv : dict.entries) {
        out += " /" + EscapeName(kv.first) + " " + SerializeInline(kv.second);
    }
    out += " >>";
    return out;
}

std::string SerializeInline(const foundation::objects::Value& v) {
    namespace o = foundation::objects;
    if (std::holds_alternative<std::monostate>(v.v)) return "null";
    if (const auto* b = std::get_if<bool>(&v.v)) return *b ? "true" : "false";
    if (const auto* n = std::get_if<std::int64_t>(&v.v))
        return std::to_string(*n);
    if (const auto* d = std::get_if<double>(&v.v)) {
        std::ostringstream oss;
        oss << *d;
        return oss.str();
    }
    if (const auto* name = std::get_if<std::string>(&v.v))
        return "/" + EscapeName(*name);
    if (const auto* s = std::get_if<o::String>(&v.v)) {
        std::string out = "<";
        char buf[3];
        for (std::byte by : s->bytes) {
            std::snprintf(buf, sizeof(buf), "%02X",
                          static_cast<unsigned char>(by));
            out += buf;
        }
        out += ">";
        return out;
    }
    if (const auto* a = std::get_if<o::Array>(&v.v)) {
        if (a->items.empty()) return "[ ]";
        std::string out = "[";
        for (const auto& it : a->items) out += " " + SerializeInline(it);
        out += " ]";
        return out;
    }
    if (const auto* dd = std::get_if<o::Dict>(&v.v))
        return SerializeDictInline(*dd);
    if (const auto* r = std::get_if<o::Ref>(&v.v))
        return std::to_string(r->id) + " " + std::to_string(r->generation) +
               " R";
    return "null";  // a bare Stream never appears inline
}

}  // namespace

std::vector<std::byte> Document::CompactDocument(
        const std::vector<std::byte>& working) const {
    auto span = std::span<const std::byte>(working.data(), working.size());
    foundation::trailer::Trailer tb;
    foundation::objects::Dump dump;
    try {
        tb = foundation::trailer::Parse(LastTrailerSlice(span));
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return working;
    }
    if (tb.root_id == 0) return working;
    // Compaction rewrites the trailer; a single-section trailer can't carry the
    // /Encrypt + /ID needed by an encrypted document, so skip those.
    if (tb.encrypt_id != 0 || encrypt_requested_) return working;

    // Latest object per id (later occurrences override earlier — incremental
    // semantics).
    std::map<std::uint32_t, const foundation::objects::IndirectObject*> by_id;
    for (const auto& obj : dump.objects) by_id[obj.id] = &obj;

    // Reachability from the catalog (+ Info + Encrypt) following every Ref.
    std::set<std::uint32_t> reachable;
    std::vector<std::uint32_t> stack;
    auto visit = [&](std::uint32_t id) {
        if (id != 0 && by_id.count(id) && reachable.insert(id).second)
            stack.push_back(id);
    };
    visit(tb.root_id);
    if (tb.info_id != 0) visit(tb.info_id);
    if (tb.encrypt_id != 0) visit(tb.encrypt_id);
    while (!stack.empty()) {
        const std::uint32_t id = stack.back();
        stack.pop_back();
        std::vector<std::uint32_t> refs;
        CollectObjectRefs(by_id[id]->value, refs);
        for (std::uint32_t r : refs) visit(r);
    }
    if (reachable.empty()) return working;

    // Serialise a fresh single-section PDF.
    std::string out = "%PDF-1.7\n%\xE2\xE3\xCF\xD3\n";
    std::map<std::uint32_t, std::size_t> offsets;
    for (std::uint32_t id : reachable) {  // std::set iterates ascending
        const auto* obj = by_id[id];
        offsets[id] = out.size();
        out += std::to_string(id) + " " +
               std::to_string(obj->generation) + " obj\n";
        if (const auto* st =
                std::get_if<foundation::objects::Stream>(&obj->value.v)) {
            foundation::objects::Dict hdr;
            for (const auto& kv : st->header.entries) {
                if (kv.first != "Length") hdr.entries.push_back(kv);
            }
            foundation::objects::Value len;
            len.v = static_cast<std::int64_t>(st->body.size());
            hdr.entries.emplace_back("Length", std::move(len));
            out += SerializeDictInline(hdr);
            out += "\nstream\n";
            out.append(reinterpret_cast<const char*>(st->body.data()),
                       st->body.size());
            out += "\nendstream";
        } else {
            out += SerializeInline(obj->value);
        }
        out += "\nendobj\n";
    }

    const std::uint32_t size = reachable.empty() ? 1 : (*reachable.rbegin() + 1);
    const std::size_t xref_offset = out.size();
    out += "xref\n0 " + std::to_string(size) + "\n";

    // Free list: object 0 heads it; every id in [1,size) not reachable is free,
    // chained ascending, last pointing back to 0.
    std::vector<std::uint32_t> free_ids;
    for (std::uint32_t id = 1; id < size; ++id) {
        if (!reachable.count(id)) free_ids.push_back(id);
    }
    char buf[32];
    {
        const std::uint32_t next = free_ids.empty() ? 0 : free_ids.front();
        std::snprintf(buf, sizeof(buf), "%010u 65535 f \n", next);
        out += buf;
    }
    std::size_t fi = 0;
    for (std::uint32_t id = 1; id < size; ++id) {
        if (reachable.count(id)) {
            std::snprintf(buf, sizeof(buf), "%010zu %05u n \n",
                          offsets[id], by_id[id]->generation);
        } else {
            ++fi;
            const std::uint32_t next =
                fi < free_ids.size() ? free_ids[fi] : 0;
            std::snprintf(buf, sizeof(buf), "%010u 65535 f \n", next);
        }
        out += buf;
    }

    out += "trailer\n<< /Size " + std::to_string(size) + " /Root " +
           std::to_string(tb.root_id) + " 0 R";
    if (tb.info_id != 0 && reachable.count(tb.info_id))
        out += " /Info " + std::to_string(tb.info_id) + " 0 R";
    out += " >>\nstartxref\n" + std::to_string(xref_offset) + "\n%%EOF\n";

    std::vector<std::byte> result(out.size());
    for (std::size_t i = 0; i < out.size(); ++i)
        result[i] = static_cast<std::byte>(static_cast<unsigned char>(out[i]));
    return result;
}

void Document::Encrypt(const std::string& userPassword,
                       const std::string& ownerPassword,
                       Aspose::Pdf::Permissions permissions,
                       CryptoAlgorithm cryptoAlgorithm) {
    Encrypt(userPassword, ownerPassword, permissions,
            cryptoAlgorithm, false);
}

void Document::Encrypt(const std::string& userPassword,
                       const std::string& ownerPassword,
                       Aspose::Pdf::Permissions permissions,
                       CryptoAlgorithm cryptoAlgorithm,
                       bool usePdf20) {
    encrypt_requested_ = true;
    pending_user_password_ = userPassword;
    pending_owner_password_ = ownerPassword;
    pending_permissions_ = permissions;
    pending_algorithm_ = cryptoAlgorithm;
    pending_use_pdf20_ = usePdf20;
}

void Document::Encrypt(
        const std::string& userPassword,
        const std::string& ownerPassword,
        const Aspose::Pdf::Facades::DocumentPrivilege& privileges,
        CryptoAlgorithm cryptoAlgorithm) {
    Encrypt(userPassword, ownerPassword, privileges,
            cryptoAlgorithm, false);
}

void Document::Encrypt(
        const std::string& userPassword,
        const std::string& ownerPassword,
        const Aspose::Pdf::Facades::DocumentPrivilege& privileges,
        CryptoAlgorithm cryptoAlgorithm,
        bool usePdf20) {
    Encrypt(userPassword, ownerPassword,
            PrivilegeToPermissions(privileges),
            cryptoAlgorithm, usePdf20);
}

Document::Document(Document&&) noexcept = default;
Document& Document::operator=(Document&&) noexcept = default;
Document::~Document() = default;

void Document::Save(const std::string& outputFileName) const {
    const bool metadata_dirty = metadata_ && metadata_->Dirty();
    bool annots_dirty = false;
    for (const auto& slot : page_annotations_) {
        // Dirty when the page has annotations to (re)write, OR when loaded
        // annotations were removed (Count can drop to 0 on delete-all —
        // loaded_count_ still reflects that the page had annotations).
        if (slot && (slot->Count() > 0 || slot->loaded_count_ > 0)) {
            annots_dirty = true;
            break;
        }
    }
    const bool embedded_dirty =
        embedded_files_ && (embedded_files_->Count() > 0 ||
                            embedded_files_->loaded_count_ > 0);
    const bool named_dests_dirty =
        named_destinations_ && (named_destinations_->Count() > 0 ||
                                named_destinations_->loaded_count_ > 0);
    const bool page_labels_dirty =
        page_labels_ && (!page_labels_->ranges_.empty() ||
                         page_labels_->loaded_count_ > 0);
    bool paragraphs_dirty = false;
    for (const auto& slot : page_paragraphs_)
        if (slot && slot->Count() > 0) paragraphs_dirty = true;
    // Images are dirty when a loaded page's collection has staged adds
    // (XImageCollection::Add) or had loaded images deleted (the surviving
    // loaded set is smaller than what was loaded).
    bool images_dirty = false;
    for (const auto& slot : page_resources_) {
        if (!slot) continue;
        const XImageCollection& coll = slot->images_;
        std::size_t surviving_loaded = 0;
        bool has_new = false;
        bool has_replaced = false;
        for (const auto& im : coll.images_) {
            if (im->is_new_)
                has_new = true;
            else {
                ++surviving_loaded;
                if (im->is_replaced_) has_replaced = true;
            }
        }
        if (has_new || has_replaced ||
            surviving_loaded < coll.loaded_names_.size()) {
            images_dirty = true;
            break;
        }
    }
    // Artifacts (watermarks) are dirty when any loaded page has artifacts to
    // render.
    bool artifacts_dirty = false;
    for (const auto& slot : page_artifacts_)
        if (slot && slot->Count() > 0) artifacts_dirty = true;
    // Dirty when the form has fields to (re)write, OR loaded fields were
    // removed / value-edited (Count can drop below loaded_count_), OR a
    // flatten was requested.
    const bool acroform_dirty =
        form_ && (form_->Count() > 0 || form_->loaded_count_ > 0 ||
                  form_->flatten_requested_);
    const bool text_edits_dirty = !pending_text_edits_.empty();
    const bool redactions_dirty = !pending_redactions_.empty();
    // Sync the public outline tree (if it was accessed) into the staged-write
    // form so AppendOutlinesUpdate persists it. Takes precedence over the
    // PdfBookmarkEditor staging path.
    if (outlines_collection_) {
        std::vector<OutlineNode> nodes;
        for (int i = 1; i <= outlines_collection_->Count(); ++i)
            nodes.push_back(OutlineNodeFromItem((*outlines_collection_)[i]));
        if (!nodes.empty() || outlines_dirty_) {
            staged_outlines_ = std::move(nodes);
            outlines_dirty_ = true;
        }
    }
    if (!dirty_ && !metadata_dirty && !annots_dirty && !embedded_dirty
        && !named_dests_dirty && !page_labels_dirty && !acroform_dirty
        && !page_geom_dirty_ && !outlines_dirty_ && !encrypt_requested_
        && !text_edits_dirty && !optimize_requested_ && !paragraphs_dirty
        && !images_dirty && !artifacts_dirty && !redactions_dirty) {
        WriteAll(outputFileName,
                 std::span<const std::byte>(bytes_.data(), bytes_.size()));
        return;
    }

    std::vector<std::byte> working(bytes_.begin(), bytes_.end());

    if (dirty_) {
        if (info_id_ == 0) {
            throw std::runtime_error(
                "Aspose::Pdf::Document::Save: source document has "
                "no /Info dictionary; info edits cannot be flushed "
                "via incremental update without a /Info object to "
                "replace");
        }
        foundation::objects::Dict info_dict;
        info_dict.entries.reserve(info_entries_.size());
        for (const auto& [k, v] : info_entries_) {
            foundation::objects::String pdf_str;
            pdf_str.bytes = EncodePdfString(v);
            pdf_str.kind = foundation::objects::StringKind::Literal;
            foundation::objects::Value val;
            val.v = std::move(pdf_str);
            info_dict.entries.emplace_back(k, std::move(val));
        }
        foundation::pdf_writer_incremental::DirtyObject info_dirty;
        info_dirty.id = info_id_;
        info_dirty.generation = 0;
        info_dirty.value.v = std::move(info_dict);
        working = foundation::pdf_writer_incremental::AppendIncremental(
            std::span<const std::byte>(working.data(), working.size()),
            std::span<const foundation::pdf_writer_incremental::DirtyObject>(
                &info_dirty, 1));
    }

    if (metadata_dirty) {
        working = AppendMetadataUpdate(working);
    }

    if (annots_dirty) {
        working = AppendAnnotationsUpdate(working);
    }

    if (embedded_dirty) {
        working = AppendEmbeddedFilesUpdate(working);
    }

    if (named_dests_dirty) {
        working = AppendNamedDestinationsUpdate(working);
    }

    if (page_labels_dirty) {
        working = AppendPageLabelsUpdate(working);
    }

    if (paragraphs_dirty) {
        working = AppendParagraphsUpdate(working);
    }

    if (images_dirty) {
        working = AppendImagesUpdate(working);
    }

    if (artifacts_dirty) {
        working = AppendArtifactsUpdate(working);
    }

    if (acroform_dirty) {
        working = AppendAcroFormUpdate(working);
    }

    if (text_edits_dirty) {
        working = AppendTextEditsUpdate(working);
    }

    if (redactions_dirty) {
        working = AppendRedactionsUpdate(working);
    }

    if (page_geom_dirty_) {
        working = AppendPageGeometryUpdate(working);
    }

    if (outlines_dirty_) {
        working = AppendOutlinesUpdate(working);
    }

    if (encrypt_requested_) {
        // Resolve the algorithm variant and source the randomness
        // for /ID, salt, and per-object IVs.
        const auto variant = ResolveVariant(
            pending_algorithm_, pending_use_pdf20_);

        std::vector<std::byte> file_id(16);
        FillRandomBytes(std::span<std::byte>(file_id));

        const std::size_t salt_len = (variant.r < 5) ? 16 : 68;
        std::vector<std::byte> salt(salt_len);
        FillRandomBytes(std::span<std::byte>(salt));

        const std::int32_t p_value = BuildPValue(pending_permissions_);

        // Lower the user/owner passwords to raw byte spans.
        std::vector<std::byte> user_pwd(pending_user_password_.size());
        for (std::size_t i = 0; i < pending_user_password_.size(); ++i) {
            user_pwd[i] = static_cast<std::byte>(
                static_cast<unsigned char>(pending_user_password_[i]));
        }
        std::vector<std::byte> owner_pwd(pending_owner_password_.size());
        for (std::size_t i = 0; i < pending_owner_password_.size(); ++i) {
            owner_pwd[i] = static_cast<std::byte>(
                static_cast<unsigned char>(pending_owner_password_[i]));
        }

        // Compute /O, /U, /OE, /UE, /Perms + the file encryption
        // key via the encrypt_writer foundation primitive.
        auto out_enc = foundation::encrypt_writer::BuildEncryptDict(
            std::span<const std::byte>(user_pwd.data(), user_pwd.size()),
            std::span<const std::byte>(owner_pwd.data(), owner_pwd.size()),
            p_value,
            variant.v,
            variant.r,
            variant.length_bits,
            std::span<const std::byte>(file_id.data(), file_id.size()),
            /*encrypt_metadata=*/true,
            std::span<const std::byte>(salt.data(), salt.size()));

        // Parse the working buffer's indirect objects, encrypt
        // each object's strings + streams in place, and build the
        // dirty list.
        const auto dump = foundation::objects::Parse(
            std::span<const std::byte>(working.data(), working.size()));

        // Allocate a fresh id for the /Encrypt object — one past
        // the maximum object id observed in the current dump.
        std::uint32_t encrypt_id = 0;
        for (const auto& o : dump.objects) {
            if (o.id > encrypt_id) encrypt_id = o.id;
        }
        ++encrypt_id;

        std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;
        dirty.reserve(dump.objects.size() + 1);
        std::vector<std::vector<std::byte>> owned_bufs;

        for (const auto& obj : dump.objects) {
            foundation::pdf_writer_incremental::DirtyObject d;
            d.id = obj.id;
            d.generation = obj.generation;
            d.value = obj.value;
            EncryptValueInPlace(d.value, d.id, d.generation,
                                  variant.method,
                                  std::span<const std::byte>(
                                      out_enc.file_key.data(),
                                      out_enc.file_key.size()),
                                  owned_bufs);
            dirty.push_back(std::move(d));
        }

        // The /Encrypt indirect object itself — PLAINTEXT per
        // PDF §7.6.2.
        {
            foundation::pdf_writer_incremental::DirtyObject d;
            d.id = encrypt_id;
            d.generation = 0;
            d.value = BuildEncryptDictValue(out_enc.encrypt_dict);
            dirty.push_back(std::move(d));
        }

        foundation::pdf_writer_incremental::AppendOptions options;
        options.has_encrypt_ref = true;
        options.encrypt_id = encrypt_id;
        options.encrypt_gen = 0;
        options.has_file_id = true;
        options.file_id_first = file_id;
        options.file_id_second = file_id;

        working = foundation::pdf_writer_incremental::AppendIncremental(
            std::span<const std::byte>(working.data(), working.size()),
            std::span<const foundation::pdf_writer_incremental::DirtyObject>(
                dirty.data(), dirty.size()),
            options);
    }

    if (optimize_requested_) {
        working = CompactDocument(working);
    }

    WriteAll(outputFileName,
             std::span<const std::byte>(working.data(), working.size()));
}

void Document::Save() const {
    if (!source_filename_.has_value()) {
        throw std::runtime_error(
            "Aspose::Pdf::Document::Save: no source filename to save to. "
            "This Document was constructed without a filename; use "
            "Save(outputFileName) instead.");
    }
    Save(*source_filename_);
}

void Document::SetTitle(const std::string& title) {
    Info().Title(title);
}

DocumentInfo& Document::Info() {
    if (!info_) {
        info_ = std::make_unique<DocumentInfo>(*this);
    }
    return *info_;
}

namespace {

const foundation::objects::Value* DictGet(
        const foundation::objects::Dict& d,
        std::string_view key) {
    for (const auto& [k, v] : d.entries) {
        if (k == key) return &v;
    }
    return nullptr;
}

const foundation::objects::IndirectObject* FindObject(
        const foundation::objects::Dump& dump, std::uint32_t id) {
    for (const auto& obj : dump.objects) {
        if (obj.id == id) return &obj;
    }
    return nullptr;
}

bool HasFlateFilter(const foundation::objects::Dict& header) {
    auto* filter = DictGet(header, "Filter");
    if (filter == nullptr) return false;
    if (auto* name = std::get_if<std::string>(&filter->v)) {
        return *name == "FlateDecode";
    }
    if (auto* arr = std::get_if<foundation::objects::Array>(&filter->v)) {
        for (const auto& it : arr->items) {
            if (auto* n = std::get_if<std::string>(&it.v)) {
                if (*n == "FlateDecode") return true;
            }
        }
    }
    return false;
}

std::vector<std::byte> ExtractXmpPacket(
        std::span<const std::byte> source,
        std::uint32_t root_id) {
    auto dump = foundation::objects::Parse(source);
    auto* catalog = FindObject(dump, root_id);
    if (catalog == nullptr) return {};
    auto* cat_dict = std::get_if<foundation::objects::Dict>(&catalog->value.v);
    if (cat_dict == nullptr) return {};
    auto* meta_entry = DictGet(*cat_dict, "Metadata");
    if (meta_entry == nullptr) return {};
    auto* meta_ref = std::get_if<foundation::objects::Ref>(&meta_entry->v);
    if (meta_ref == nullptr) return {};
    auto* meta_obj = FindObject(dump, meta_ref->id);
    if (meta_obj == nullptr) return {};
    auto* stream = std::get_if<foundation::objects::Stream>(&meta_obj->value.v);
    if (stream == nullptr) return {};
    (void)source;  // body span now carries the slice directly
    if (HasFlateFilter(stream->header)) {
        return foundation::flate::Decode(stream->body);
    }
    return std::vector<std::byte>(
        stream->body.begin(), stream->body.end());
}

}  // namespace

Aspose::Pdf::Metadata& Document::Metadata() {
    if (!metadata_) {
        auto bytes_view = std::span<const std::byte>(
            bytes_.data(), bytes_.size());
        auto root_id = foundation::trailer::Parse(bytes_view).root_id;
        if (root_id != 0) {
            auto packet = ExtractXmpPacket(bytes_view, root_id);
            metadata_ = std::unique_ptr<Aspose::Pdf::Metadata>(
                new Aspose::Pdf::Metadata(
                    std::span<const std::byte>(
                        packet.data(), packet.size())));
        } else {
            metadata_ = std::unique_ptr<Aspose::Pdf::Metadata>(
                new Aspose::Pdf::Metadata());
        }
    }
    return *metadata_;
}

std::vector<std::byte> Document::AppendMetadataUpdate(
        const std::vector<std::byte>& working) const {
    if (!metadata_) return working;

    auto working_span = std::span<const std::byte>(
        working.data(), working.size());

    foundation::trailer::Trailer trailer_bundle;
    try {
        trailer_bundle = foundation::trailer::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }
    if (trailer_bundle.root_id == 0) return working;

    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }

    const foundation::objects::IndirectObject* catalog = nullptr;
    for (const auto& obj : dump.objects) {
        if (obj.id == trailer_bundle.root_id) {
            catalog = &obj;
            break;
        }
    }
    if (catalog == nullptr) return working;
    const auto* catalog_dict =
        std::get_if<foundation::objects::Dict>(&catalog->value.v);
    if (catalog_dict == nullptr) return working;

    // Decide the stream object id: reuse existing /Metadata Ref
    // or allocate trailer.size (next free id).
    std::uint32_t stream_id = 0;
    std::uint16_t stream_gen = 0;
    bool catalog_dirty;
    const foundation::objects::Ref* existing_meta = nullptr;
    for (const auto& kv : catalog_dict->entries) {
        if (kv.first == "Metadata") {
            existing_meta = std::get_if<foundation::objects::Ref>(
                &kv.second.v);
            break;
        }
    }
    if (existing_meta != nullptr) {
        stream_id = existing_meta->id;
        stream_gen = existing_meta->generation;
        catalog_dirty = false;
    } else {
        stream_id = trailer_bundle.size;
        stream_gen = 0;
        catalog_dirty = true;
    }

    auto xmp_body = metadata_->ToXmpPacket();
    foundation::objects::Stream stream_value;
    {
        foundation::objects::Value type_val;
        type_val.v = std::string("Metadata");
        stream_value.header.entries.emplace_back("Type",
            std::move(type_val));
        foundation::objects::Value subtype_val;
        subtype_val.v = std::string("XML");
        stream_value.header.entries.emplace_back("Subtype",
            std::move(subtype_val));
        stream_value.body = std::span<const std::byte>(
            xmp_body.data(), xmp_body.size());
    }

    std::vector<foundation::pdf_writer_incremental::DirtyObject>
        dirty;
    {
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = stream_id;
        d.generation = stream_gen;
        d.value.v = std::move(stream_value);
        dirty.push_back(std::move(d));
    }
    if (catalog_dirty) {
        foundation::objects::Dict updated_catalog = *catalog_dict;
        foundation::objects::Ref meta_ref{stream_id, stream_gen};
        foundation::objects::Value meta_val;
        meta_val.v = meta_ref;
        updated_catalog.entries.emplace_back("Metadata",
            std::move(meta_val));
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = catalog->id;
        d.generation = catalog->generation;
        d.value.v = std::move(updated_catalog);
        dirty.push_back(std::move(d));
    }

    return foundation::pdf_writer_incremental::AppendIncremental(
        working_span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
}

PageCollection& Document::Pages() {
    if (!pages_) {
        pages_ = std::unique_ptr<PageCollection>(new PageCollection(*this));
    }
    return *pages_;
}

EmbeddedFileCollection& Document::EmbeddedFiles() {
    if (!embedded_files_) {
        embedded_files_ = std::make_unique<EmbeddedFileCollection>();
        LoadEmbeddedFiles(*embedded_files_);
    }
    return *embedded_files_;
}

NamedDestinationCollection& Document::NamedDestinations() {
    if (!named_destinations_) {
        named_destinations_ = std::make_unique<NamedDestinationCollection>();
        LoadNamedDestinations(*named_destinations_);
    }
    return *named_destinations_;
}

OutlineCollection& Document::Outlines() {
    if (!outlines_collection_) {
        outlines_collection_ = std::make_unique<OutlineCollection>();
        LoadOutlines(*outlines_collection_);
    }
    return *outlines_collection_;
}

PageLabelCollection& Document::PageLabels() {
    if (!page_labels_) {
        page_labels_ = std::make_unique<PageLabelCollection>();
        LoadPageLabels(*page_labels_);
    }
    return *page_labels_;
}

Aspose::Pdf::Forms::Form& Document::Form() {
    if (!form_) {
        form_ = std::make_unique<Aspose::Pdf::Forms::Form>();
        LoadAcroFormFields(*form_);
    }
    return *form_;
}

namespace {

// Map an AnnotationType enum to the PDF /Subtype name per
// ISO 32000-1 §12.5.6. Returns nullptr for types whose canonical
// /Subtype is not in scope for v1 baseline write-through (the
// printer-mark subclasses, PDF3D, TrapNet, Unknown).
const char* PdfSubtypeName(
        Aspose::Pdf::Annotations::AnnotationType t) noexcept {
    using AT = Aspose::Pdf::Annotations::AnnotationType;
    switch (t) {
        case AT::Text:            return "Text";
        case AT::Link:            return "Link";
        case AT::FreeText:        return "FreeText";
        case AT::Line:            return "Line";
        case AT::Square:          return "Square";
        case AT::Circle:          return "Circle";
        case AT::Polygon:         return "Polygon";
        case AT::PolyLine:        return "PolyLine";
        case AT::Highlight:       return "Highlight";
        case AT::Underline:       return "Underline";
        case AT::Squiggly:        return "Squiggly";
        case AT::StrikeOut:       return "StrikeOut";
        case AT::Stamp:           return "Stamp";
        case AT::Caret:           return "Caret";
        case AT::Ink:             return "Ink";
        case AT::Popup:           return "Popup";
        case AT::FileAttachment:  return "FileAttachment";
        case AT::Sound:           return "Sound";
        case AT::Movie:           return "Movie";
        case AT::Widget:          return "Widget";
        case AT::Screen:          return "Screen";
        case AT::Watermark:       return "Watermark";
        case AT::Redaction:       return "Redact";  // PDF spec /Subtype name
        // v1 baseline doesn't emit printer-mark subclasses,
        // PDF3D, TrapNet, RichMedia, or Unknown — subtype-specific
        // entries (/Page /MarkStyle /etc.) needed before round-trip
        // is meaningful.
        default:                  return nullptr;
    }
}

// Build the v1 baseline /Annot dictionary for a concrete
// Annotation. /Type /Subtype /Rect /Contents /P. Returns nullopt
// for annotations whose subtype is out of v1 scope.
std::optional<foundation::objects::Dict> BuildAnnotDict(
        const Aspose::Pdf::Annotations::Annotation& a,
        foundation::objects::Ref page_ref) {
    const char* subtype = PdfSubtypeName(a.AnnotationType());
    if (subtype == nullptr) return std::nullopt;

    foundation::objects::Dict d;

    {
        foundation::objects::Value v;
        v.v = std::string("Annot");
        d.entries.emplace_back("Type", std::move(v));
    }
    {
        foundation::objects::Value v;
        v.v = std::string(subtype);
        d.entries.emplace_back("Subtype", std::move(v));
    }
    {
        const auto& rect = a.Rect();
        foundation::objects::Array rect_arr;
        rect_arr.items.reserve(4);
        for (double d_value :
                 {rect.LLX(), rect.LLY(), rect.URX(), rect.URY()}) {
            foundation::objects::Value v;
            v.v = d_value;
            rect_arr.items.push_back(std::move(v));
        }
        foundation::objects::Value v;
        v.v = std::move(rect_arr);
        d.entries.emplace_back("Rect", std::move(v));
    }
    if (!a.Contents().empty()) {
        foundation::objects::String pdf_str;
        pdf_str.bytes = EncodePdfString(a.Contents());
        pdf_str.kind = foundation::objects::StringKind::Literal;
        foundation::objects::Value v;
        v.v = std::move(pdf_str);
        d.entries.emplace_back("Contents", std::move(v));
    }
    {
        foundation::objects::Value v;
        v.v = page_ref;
        d.entries.emplace_back("P", std::move(v));
    }
    return d;
}

}  // namespace

namespace {
// Defined later in this TU; used by the appearance-stream emitters below.
foundation::objects::Value PageNameValue(const char* name);
}  // namespace

std::vector<std::byte> Document::AppendAnnotationsUpdate(
        const std::vector<std::byte>& working) const {
    auto working_span = std::span<const std::byte>(
        working.data(), working.size());

    foundation::trailer::Trailer trailer_bundle;
    try {
        trailer_bundle = foundation::trailer::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }
    if (trailer_bundle.size == 0) return working;

    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }

    std::uint32_t next_id = trailer_bundle.size;
    for (const auto& o : dump.objects) next_id = std::max(next_id, o.id + 1);
    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;

    // ---- Annotation appearance-stream support -------------------------------
    // Markup/figure annotations get a pre-rendered /AP so viewers draw them
    // (Acrobat does not synthesise markup appearances). Bodies are parked for
    // stable addresses (Stream.body is a non-owning span).
    std::vector<std::unique_ptr<std::vector<std::byte>>> ap_bodies;
    auto emit_annot_ap =
        [&](double llx, double lly, double urx, double ury,
            const std::string& content, bool need_font,
            bool need_extgstate) -> std::uint32_t {
        foundation::objects::Dict resources;
        if (need_font) {
            foundation::objects::Dict font_inner;
            font_inner.entries.emplace_back("Type", PageNameValue("Font"));
            font_inner.entries.emplace_back("Subtype", PageNameValue("Type1"));
            font_inner.entries.emplace_back("BaseFont",
                                            PageNameValue("Helvetica"));
            foundation::objects::Dict fonts;
            { foundation::objects::Value v; v.v = std::move(font_inner);
              fonts.entries.emplace_back("Helv", std::move(v)); }
            foundation::objects::Value v; v.v = std::move(fonts);
            resources.entries.emplace_back("Font", std::move(v));
        }
        if (need_extgstate) {
            foundation::objects::Dict gs;
            gs.entries.emplace_back("Type", PageNameValue("ExtGState"));
            gs.entries.emplace_back("BM", PageNameValue("Multiply"));
            foundation::objects::Dict ext;
            { foundation::objects::Value v; v.v = std::move(gs);
              ext.entries.emplace_back("GS0", std::move(v)); }
            foundation::objects::Value v; v.v = std::move(ext);
            resources.entries.emplace_back("ExtGState", std::move(v));
        }
        foundation::objects::Array bbox;
        for (double dv : {llx, lly, urx, ury}) {
            foundation::objects::Value v; v.v = dv; bbox.items.push_back(std::move(v));
        }
        foundation::objects::Dict header;
        header.entries.emplace_back("Type", PageNameValue("XObject"));
        header.entries.emplace_back("Subtype", PageNameValue("Form"));
        { foundation::objects::Value v; v.v = static_cast<std::int64_t>(1);
          header.entries.emplace_back("FormType", std::move(v)); }
        { foundation::objects::Value v; v.v = std::move(bbox);
          header.entries.emplace_back("BBox", std::move(v)); }
        { foundation::objects::Value v; v.v = std::move(resources);
          header.entries.emplace_back("Resources", std::move(v)); }
        ap_bodies.push_back(std::make_unique<std::vector<std::byte>>(
            reinterpret_cast<const std::byte*>(content.data()),
            reinterpret_cast<const std::byte*>(content.data()) +
                content.size()));
        foundation::objects::Stream st;
        st.header = std::move(header);
        st.body = std::span<const std::byte>(ap_bodies.back()->data(),
                                             ap_bodies.back()->size());
        const std::uint32_t id = next_id++;
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = id; d.generation = 0; d.value.v = std::move(st);
        dirty.push_back(std::move(d));
        return id;
    };

    for (std::size_t leaf_idx = 0;
         leaf_idx < page_annotations_.size(); ++leaf_idx) {
        const auto& slot = page_annotations_[leaf_idx];
        if (!slot) continue;
        if (leaf_idx >= tree_->leaves.size()) continue;

        const std::uint32_t page_id = tree_->leaves[leaf_idx].id;
        const foundation::objects::Ref page_ref{page_id, 0};

        // Partition the collection into loaded (already-persisted) and
        // newly-added annotations. Loaded annotations are preserved by
        // their original /Annots refs — never rewritten (lossless).
        std::set<std::uint32_t> present_loaded;
        std::vector<const Aspose::Pdf::Annotations::Annotation*> new_annots;
        for (int i = 0; i < slot->Count(); ++i) {
            const auto* ptr = &(*slot)[i];
            auto it = slot->loaded_ids_.find(ptr);
            if (it != slot->loaded_ids_.end())
                present_loaded.insert(it->second);
            else
                new_annots.push_back(ptr);
        }
        const bool removed_loaded =
            present_loaded.size() < slot->loaded_count_;

        // Page unchanged — leave its /Annots untouched.
        if (new_annots.empty() && !removed_loaded) continue;

        const auto* page_obj = FindObject(dump, page_id);
        const auto* page_dict_in =
            page_obj ? std::get_if<foundation::objects::Dict>(
                           &page_obj->value.v)
                     : nullptr;
        if (page_dict_in == nullptr) continue;

        // Surviving loaded-annotation refs, in original order.
        foundation::objects::Array annots_arr;
        if (const auto* a = DictGet(*page_dict_in, "Annots")) {
            if (const auto* arr =
                    std::get_if<foundation::objects::Array>(&a->v)) {
                for (const auto& it : arr->items) {
                    const auto* ref =
                        std::get_if<foundation::objects::Ref>(&it.v);
                    if (ref && present_loaded.count(ref->id))
                        annots_arr.items.push_back(it);
                }
            }
        }

        // Emit each newly-added annotation as a new object.
        for (const auto* ptr : new_annots) {
            auto annot_dict = BuildAnnotDict(*ptr, page_ref);
            if (!annot_dict) continue;

            // Link navigation — serialise the activated action / destination
            // so the hyperlink actually jumps. A GoToURIAction becomes
            // /A << /S /URI /URI (…) >>; an in-document GoToAction (or a
            // Destination set directly on the link) becomes a /Dest array
            // [<dest-page-ref> <verb> <ops…>] resolved against the page tree.
            namespace ANav = Aspose::Pdf::Annotations;
            if (const auto* link =
                    dynamic_cast<const ANav::LinkAnnotation*>(ptr)) {
                // Suppress the viewer-drawn link box: a Link with no /Border or
                // /BS defaults to /Border [0 0 1] (a 1-pt frame). Emit a
                // zero-width border so the hyperlink is invisible chrome.
                {
                    foundation::objects::Array b;
                    for (int n = 0; n < 3; ++n) {
                        foundation::objects::Value v;
                        v.v = static_cast<std::int64_t>(0);
                        b.items.push_back(std::move(v));
                    }
                    foundation::objects::Value bv; bv.v = std::move(b);
                    annot_dict->entries.emplace_back("Border", std::move(bv));
                }
                // Resolve a 1-based page number → page object ref.
                auto dest_page_ref =
                    [&](int pn, foundation::objects::Ref& out) -> bool {
                    if (pn < 1 ||
                        static_cast<std::size_t>(pn) > tree_->leaves.size())
                        return false;
                    out = foundation::objects::Ref{
                        tree_->leaves[static_cast<std::size_t>(pn - 1)].id, 0};
                    return true;
                };
                // Build a /Dest array from an ExplicitDestination (page + the
                // typed verb/operands parsed from its ToString()).
                auto build_dest =
                    [&](const ANav::ExplicitDestination& ed,
                        foundation::objects::Array& dest) -> bool {
                    foundation::objects::Ref pr;
                    if (!dest_page_ref(ed.PageNumber(), pr)) return false;
                    foundation::objects::Value prv; prv.v = pr;
                    dest.items.push_back(std::move(prv));
                    std::istringstream iss(ed.ToString());
                    std::string page_tok, verb;
                    iss >> page_tok >> verb;
                    foundation::objects::Value vv;
                    vv.v = verb.empty() ? std::string("Fit") : verb;
                    dest.items.push_back(std::move(vv));
                    double op;
                    while (iss >> op) {
                        foundation::objects::Value ov; ov.v = op;
                        dest.items.push_back(std::move(ov));
                    }
                    return true;
                };
                const ANav::IAppointment* dest_app = link->Destination();
                const ANav::PdfAction* act = link->Action();
                if (const auto* uri =
                        dynamic_cast<const ANav::GoToURIAction*>(act)) {
                    foundation::objects::Dict a;
                    a.entries.emplace_back("S", foundation::objects::Value{
                                                    std::string("URI")});
                    foundation::objects::String us;
                    us.bytes = EncodePdfString(uri->URI());
                    us.kind = foundation::objects::StringKind::Literal;
                    foundation::objects::Value uv; uv.v = std::move(us);
                    a.entries.emplace_back("URI", std::move(uv));
                    foundation::objects::Value av; av.v = std::move(a);
                    annot_dict->entries.emplace_back("A", std::move(av));
                } else {
                    // GoToAction → its ExplicitDestination, else a Destination
                    // set directly on the link.
                    const ANav::ExplicitDestination* ed = nullptr;
                    if (const auto* g =
                            dynamic_cast<const ANav::GoToAction*>(act))
                        ed = dynamic_cast<const ANav::ExplicitDestination*>(
                            g->Destination());
                    if (ed == nullptr)
                        ed = dynamic_cast<const ANav::ExplicitDestination*>(
                            dest_app);
                    foundation::objects::Array dest;
                    if (ed != nullptr && build_dest(*ed, dest)) {
                        foundation::objects::Value dv; dv.v = std::move(dest);
                        annot_dict->entries.emplace_back("Dest",
                                                         std::move(dv));
                    }
                }
            }

            // Subtype geometry + colour + a pre-rendered /AP appearance so the
            // annotation paints in a viewer (conventional colours — the public
            // Color type exposes no RGB getter to read the authored value).
            {
                namespace AN = Aspose::Pdf::Annotations;
                const AN::AnnotationType t = ptr->AnnotationType();
                const auto& r = ptr->Rect();
                const double llx = r.LLX(), lly = r.LLY();
                const double urx = r.URX(), ury = r.URY();
                const double w = urx - llx, h = ury - lly;
                auto push_num = [](foundation::objects::Array& a, double v) {
                    foundation::objects::Value x; x.v = v; a.items.push_back(std::move(x));
                };
                auto set_arr = [&](const char* key, foundation::objects::Array a) {
                    foundation::objects::Value v; v.v = std::move(a);
                    annot_dict->entries.emplace_back(key, std::move(v));
                };
                auto color_arr = [&](double cr, double cg, double cb) {
                    foundation::objects::Array a; push_num(a, cr); push_num(a, cg);
                    push_num(a, cb); return a;
                };
                auto quad = [&]() {
                    foundation::objects::Array a;
                    for (double v : {llx, ury, urx, ury, llx, lly, urx, lly})
                        push_num(a, v);
                    set_arr("QuadPoints", std::move(a));
                };
                std::ostringstream cs;
                bool has_ap = true, need_font = false, need_extg = false;
                // Stroke/fill colour: the annotation's authored Color when set
                // (alpha > 0), else a per-type default.
                const Aspose::Pdf::Color ac = ptr->Color();
                const bool has_c = ac.A() > 0.0;
                double cr = 1.0, cg = 0.0, cb = 0.0;  // default red
                auto use_color = [&](double dr, double dg, double db) {
                    if (has_c) { cr = ac.r_; cg = ac.g_; cb = ac.b_; }
                    else { cr = dr; cg = dg; cb = db; }
                };
                auto rgb = [](double a, double b, double c) {
                    std::ostringstream o; o << a << ' ' << b << ' ' << c;
                    return o.str();
                };
                auto esc = [&](const std::string& s) {
                    for (char c : s) {
                        if (c == '(' || c == ')' || c == '\\') cs << '\\';
                        cs << c;
                    }
                };
                auto bezier_ellipse = [&](double cx, double cy, double rx,
                                          double ry) {
                    const double k = 0.5523;
                    cs << cx << ' ' << (cy + ry) << " m "
                       << (cx + k * rx) << ' ' << (cy + ry) << ' '
                       << (cx + rx) << ' ' << (cy + k * ry) << ' '
                       << (cx + rx) << ' ' << cy << " c "
                       << (cx + rx) << ' ' << (cy - k * ry) << ' '
                       << (cx + k * rx) << ' ' << (cy - ry) << ' '
                       << cx << ' ' << (cy - ry) << " c "
                       << (cx - k * rx) << ' ' << (cy - ry) << ' '
                       << (cx - rx) << ' ' << (cy - k * ry) << ' '
                       << (cx - rx) << ' ' << cy << " c "
                       << (cx - rx) << ' ' << (cy + k * ry) << ' '
                       << (cx - k * rx) << ' ' << (cy + ry) << ' '
                       << cx << ' ' << (cy + ry) << " c ";
                };
                switch (t) {
                    case AN::AnnotationType::Highlight:
                        use_color(1, 1, 0); need_extg = true; quad();
                        cs << "q /GS0 gs " << rgb(cr, cg, cb) << " rg " << llx
                           << ' ' << lly << ' ' << w << ' ' << h << " re f Q\n";
                        break;
                    case AN::AnnotationType::Underline:
                    case AN::AnnotationType::Squiggly:
                        use_color(0, 0, 1); quad();
                        cs << "q " << rgb(cr, cg, cb) << " rg " << llx << ' '
                           << lly << ' ' << w << " 1.5 re f Q\n";
                        break;
                    case AN::AnnotationType::StrikeOut:
                        use_color(1, 0, 0); quad();
                        cs << "q " << rgb(cr, cg, cb) << " rg " << llx << ' '
                           << (lly + h * 0.5) << ' ' << w << " 1.5 re f Q\n";
                        break;
                    case AN::AnnotationType::Square:
                        use_color(1, 0, 0);
                        cs << "q 1 1 0.6 rg " << rgb(cr, cg, cb) << " RG 1.5 w "
                           << (llx + 1) << ' ' << (lly + 1) << ' ' << (w - 2)
                           << ' ' << (h - 2) << " re B Q\n";  // fill + stroke
                        break;
                    case AN::AnnotationType::Circle:
                        use_color(0, 0.6, 0);
                        cs << "q " << rgb(cr, cg, cb) << " RG 1.5 w [3 3] 0 d ";
                        bezier_ellipse((llx + urx) / 2, (lly + ury) / 2,
                                       w / 2 - 1, h / 2 - 1);
                        cs << "S Q\n";  // dashed, no fill
                        break;
                    case AN::AnnotationType::Line:
                        if (auto* la = dynamic_cast<const AN::LineAnnotation*>(ptr)) {
                            use_color(0, 0, 1);
                            const auto& s = la->Starting();
                            const auto& e = la->Ending();
                            foundation::objects::Array a;
                            push_num(a, s.X()); push_num(a, s.Y());
                            push_num(a, e.X()); push_num(a, e.Y());
                            set_arr("L", std::move(a));
                            const double dx = e.X() - s.X(), dy = e.Y() - s.Y();
                            const double len = std::max(
                                1e-3, std::sqrt(dx * dx + dy * dy));
                            const double ux = dx / len, uy = dy / len, az = 8.0;
                            auto head = [&](double px, double py, double vx,
                                            double vy) {
                                cs << (px + (vx * 0.866 - vy * 0.5) * az) << ' '
                                   << (py + (vy * 0.866 + vx * 0.5) * az)
                                   << " m " << px << ' ' << py << " l "
                                   << (px + (vx * 0.866 + vy * 0.5) * az) << ' '
                                   << (py + (vy * 0.866 - vx * 0.5) * az)
                                   << " l S\n";
                            };
                            cs << "q " << rgb(cr, cg, cb) << " RG 1.5 w "
                               << s.X() << ' ' << s.Y() << " m " << e.X() << ' '
                               << e.Y() << " l S\n";
                            head(s.X(), s.Y(), ux, uy);
                            head(e.X(), e.Y(), -ux, -uy);
                            cs << "Q\n";
                        } else has_ap = false;
                        break;
                    case AN::AnnotationType::Ink:
                        if (auto* ia = dynamic_cast<const AN::InkAnnotation*>(ptr)) {
                            use_color(0.6, 0, 0.6);
                            const auto& strokes = ia->InkList();
                            foundation::objects::Array outer;
                            cs << "q " << rgb(cr, cg, cb) << " RG 1.5 w ";
                            for (const auto& st : strokes) {
                                foundation::objects::Array inner;
                                for (std::size_t i = 0; i < st.size(); ++i) {
                                    push_num(inner, st[i].X());
                                    push_num(inner, st[i].Y());
                                    cs << st[i].X() << ' ' << st[i].Y()
                                       << (i == 0 ? " m " : " l ");
                                }
                                foundation::objects::Value iv;
                                iv.v = std::move(inner);
                                outer.items.push_back(std::move(iv));
                            }
                            cs << "S Q\n";
                            set_arr("InkList", std::move(outer));
                        } else has_ap = false;
                        break;
                    case AN::AnnotationType::FreeText:
                        use_color(0, 0, 0); need_font = true;
                        cs << "q 1 1 0.7 rg " << llx << ' ' << lly << ' ' << w
                           << ' ' << h << " re f 0 0 0 RG 1 w " << (llx + 0.5)
                           << ' ' << (lly + 0.5) << ' ' << (w - 1) << ' '
                           << (h - 1) << " re S " << rgb(cr, cg, cb)
                           << " rg BT /Helv 11 Tf " << (llx + 6) << ' '
                           << (lly + h / 2 - 4) << " Td (";
                        esc(ptr->Contents());
                        cs << ") Tj ET Q\n";
                        break;
                    case AN::AnnotationType::Stamp: {
                        use_color(0, 0.5, 0); need_font = true;
                        const std::string label =
                            ptr->Contents().empty() ? "APPROVED" : ptr->Contents();
                        cs << "q " << rgb(cr, cg, cb) << " RG 2 w " << (llx + 1)
                           << ' ' << (lly + 1) << ' ' << (w - 2) << ' '
                           << (h - 2) << " re S " << rgb(cr, cg, cb)
                           << " rg BT /Helv " << (h * 0.42) << " Tf "
                           << (llx + w * 0.12) << ' ' << (lly + h * 0.32)
                           << " Td (";
                        esc(label);
                        cs << ") Tj ET Q\n";
                        break;
                    }
                    default:
                        has_ap = false;  // Text(sticky)/Link/FileAttachment
                        break;
                }
                if (has_ap) {
                    set_arr("C", color_arr(cr, cg, cb));
                    const std::uint32_t apid = emit_annot_ap(
                        llx, lly, urx, ury, cs.str(), need_font, need_extg);
                    foundation::objects::Dict apd;
                    foundation::objects::Value nref;
                    nref.v = foundation::objects::Ref{apid, 0};
                    apd.entries.emplace_back("N", std::move(nref));
                    foundation::objects::Value apv;
                    apv.v = std::move(apd);
                    annot_dict->entries.emplace_back("AP", std::move(apv));
                }
            }

            const std::uint32_t annot_id = next_id++;
            foundation::pdf_writer_incremental::DirtyObject d;
            d.id = annot_id;
            d.generation = 0;
            d.value.v = std::move(*annot_dict);
            dirty.push_back(std::move(d));
            foundation::objects::Value rv;
            rv.v = foundation::objects::Ref{annot_id, 0};
            annots_arr.items.push_back(std::move(rv));
        }

        foundation::objects::Dict updated = *page_dict_in;
        for (auto it = updated.entries.begin();
             it != updated.entries.end();) {
            if (it->first == "Annots")
                it = updated.entries.erase(it);
            else
                ++it;
        }
        foundation::objects::Value annots_val;
        annots_val.v = std::move(annots_arr);
        updated.entries.emplace_back("Annots", std::move(annots_val));

        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = page_id;
        d.generation = page_obj->generation;
        d.value.v = std::move(updated);
        dirty.push_back(std::move(d));
    }

    if (dirty.empty()) return working;

    return foundation::pdf_writer_incremental::AppendIncremental(
        working_span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
}

// ===== Page geometry =========================================================

Aspose::Pdf::Rectangle Document::GetPageRectInternal(
        std::size_t leaf_index) const {
    if (auto it = pending_mediabox_.find(leaf_index);
            it != pending_mediabox_.end()) {
        const auto& b = it->second;
        return Aspose::Pdf::Rectangle(b[0], b[1], b[2], b[3], false);
    }
    if (tree_ && leaf_index < tree_->leaves.size()) {
        const auto& leaf = tree_->leaves[leaf_index];
        return Aspose::Pdf::Rectangle(0.0, 0.0, leaf.width, leaf.height,
                                      false);
    }
    return Aspose::Pdf::Rectangle(0.0, 0.0, 0.0, 0.0, false);
}

Aspose::Pdf::Rectangle Document::GetPageCropBoxInternal(
        std::size_t leaf_index) const {
    if (auto it = pending_cropbox_.find(leaf_index);
            it != pending_cropbox_.end()) {
        const auto& b = it->second;
        return Aspose::Pdf::Rectangle(b[0], b[1], b[2], b[3], false);
    }
    // CropBox defaults to MediaBox when not explicitly set.
    return GetPageRectInternal(leaf_index);
}

int Document::GetPageRotationInternal(std::size_t leaf_index) const {
    if (auto it = pending_rotation_.find(leaf_index);
            it != pending_rotation_.end()) {
        return it->second;
    }
    if (tree_ && leaf_index < tree_->leaves.size()) {
        return static_cast<int>(tree_->leaves[leaf_index].rotation);
    }
    return 0;
}

void Document::SetPageRectInternal(std::size_t leaf_index,
                                   const Aspose::Pdf::Rectangle& rect) {
    pending_mediabox_[leaf_index] = {rect.LLX(), rect.LLY(), rect.URX(),
                                     rect.URY()};
    page_geom_dirty_ = true;
}

void Document::SetPageCropBoxInternal(std::size_t leaf_index,
                                      const Aspose::Pdf::Rectangle& rect) {
    pending_cropbox_[leaf_index] = {rect.LLX(), rect.LLY(), rect.URX(),
                                    rect.URY()};
    page_geom_dirty_ = true;
}

namespace {

// Read a named box array (e.g. /TrimBox) from a page dict in `dump`. Returns
// the four [llx lly urx ury] doubles when present.
std::optional<std::array<double, 4>> ReadPageBox(
        const foundation::objects::Dump& dump, std::uint32_t page_id,
        const char* key) {
    const auto* page = FindObject(dump, page_id);
    const auto* dict =
        page ? std::get_if<foundation::objects::Dict>(&page->value.v) : nullptr;
    if (dict == nullptr) return std::nullopt;
    const auto* v = DictGet(*dict, key);
    const auto* arr =
        v ? std::get_if<foundation::objects::Array>(&v->v) : nullptr;
    if (arr == nullptr || arr->items.size() < 4) return std::nullopt;
    std::array<double, 4> box{};
    for (int i = 0; i < 4; ++i) {
        const auto& iv = arr->items[static_cast<std::size_t>(i)].v;
        if (const auto* d = std::get_if<double>(&iv))
            box[static_cast<std::size_t>(i)] = *d;
        else if (const auto* n = std::get_if<std::int64_t>(&iv))
            box[static_cast<std::size_t>(i)] = static_cast<double>(*n);
        else
            return std::nullopt;
    }
    return box;
}

}  // namespace

// Resolve a Trim/Bleed/Art box: staged write → the page dict entry from file →
// the CropBox default.
Aspose::Pdf::Rectangle Document::ResolvePageBoxInternal(
        std::size_t leaf_index,
        const std::map<std::size_t, std::array<double, 4>>& pending,
        const char* key) const {
    if (auto it = pending.find(leaf_index); it != pending.end()) {
        const auto& b = it->second;
        return Aspose::Pdf::Rectangle(b[0], b[1], b[2], b[3], false);
    }
    if (tree_ && leaf_index < tree_->leaves.size()) {
        auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
        try {
            auto dump = foundation::objects::Parse(span);
            if (auto box =
                    ReadPageBox(dump, tree_->leaves[leaf_index].id, key)) {
                return Aspose::Pdf::Rectangle((*box)[0], (*box)[1], (*box)[2],
                                              (*box)[3], false);
            }
        } catch (const std::exception&) {
        }
    }
    return GetPageCropBoxInternal(leaf_index);
}

Aspose::Pdf::Rectangle Document::GetPageTrimBoxInternal(
        std::size_t leaf_index) const {
    return ResolvePageBoxInternal(leaf_index, pending_trimbox_, "TrimBox");
}

void Document::SetPageTrimBoxInternal(std::size_t leaf_index,
                                      const Aspose::Pdf::Rectangle& rect) {
    pending_trimbox_[leaf_index] = {rect.LLX(), rect.LLY(), rect.URX(),
                                    rect.URY()};
    page_geom_dirty_ = true;
}

Aspose::Pdf::Rectangle Document::GetPageBleedBoxInternal(
        std::size_t leaf_index) const {
    return ResolvePageBoxInternal(leaf_index, pending_bleedbox_, "BleedBox");
}

void Document::SetPageBleedBoxInternal(std::size_t leaf_index,
                                       const Aspose::Pdf::Rectangle& rect) {
    pending_bleedbox_[leaf_index] = {rect.LLX(), rect.LLY(), rect.URX(),
                                     rect.URY()};
    page_geom_dirty_ = true;
}

Aspose::Pdf::Rectangle Document::GetPageArtBoxInternal(
        std::size_t leaf_index) const {
    return ResolvePageBoxInternal(leaf_index, pending_artbox_, "ArtBox");
}

void Document::SetPageArtBoxInternal(std::size_t leaf_index,
                                     const Aspose::Pdf::Rectangle& rect) {
    pending_artbox_[leaf_index] = {rect.LLX(), rect.LLY(), rect.URX(),
                                   rect.URY()};
    page_geom_dirty_ = true;
}

void Document::SetPageRotationInternal(std::size_t leaf_index, int degrees) {
    int norm = degrees % 360;
    if (norm < 0) norm += 360;
    pending_rotation_[leaf_index] = norm;
    page_geom_dirty_ = true;
}

namespace {

// Build a /MediaBox-style 4-number array [llx lly urx ury].
foundation::objects::Value BuildRectArray(const std::array<double, 4>& b) {
    foundation::objects::Array arr;
    arr.items.reserve(4);
    for (double d_value : b) {
        foundation::objects::Value v;
        v.v = d_value;
        arr.items.push_back(std::move(v));
    }
    foundation::objects::Value out;
    out.v = std::move(arr);
    return out;
}

}  // namespace

std::vector<std::byte> Document::AppendPageGeometryUpdate(
        const std::vector<std::byte>& working) const {
    auto working_span = std::span<const std::byte>(
        working.data(), working.size());

    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }
    if (!tree_) return working;

    std::vector<std::size_t> leaves;
    auto note = [&leaves](std::size_t idx) {
        if (std::find(leaves.begin(), leaves.end(), idx) == leaves.end())
            leaves.push_back(idx);
    };
    for (const auto& kv : pending_mediabox_) note(kv.first);
    for (const auto& kv : pending_cropbox_) note(kv.first);
    for (const auto& kv : pending_trimbox_) note(kv.first);
    for (const auto& kv : pending_bleedbox_) note(kv.first);
    for (const auto& kv : pending_artbox_) note(kv.first);
    for (const auto& kv : pending_rotation_) note(kv.first);

    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;
    for (std::size_t leaf_idx : leaves) {
        if (leaf_idx >= tree_->leaves.size()) continue;
        const std::uint32_t page_id = tree_->leaves[leaf_idx].id;

        const foundation::objects::IndirectObject* page_obj = nullptr;
        for (const auto& obj : dump.objects) {
            if (obj.id == page_id) { page_obj = &obj; break; }
        }
        if (page_obj == nullptr) continue;
        const auto* page_dict_in =
            std::get_if<foundation::objects::Dict>(&page_obj->value.v);
        if (page_dict_in == nullptr) continue;

        foundation::objects::Dict updated = *page_dict_in;
        auto strip = [&updated](const char* key) {
            for (auto it = updated.entries.begin();
                 it != updated.entries.end();) {
                if (it->first == key) it = updated.entries.erase(it);
                else ++it;
            }
        };

        if (auto it = pending_mediabox_.find(leaf_idx);
                it != pending_mediabox_.end()) {
            strip("MediaBox");
            updated.entries.emplace_back("MediaBox",
                                         BuildRectArray(it->second));
        }
        if (auto it = pending_cropbox_.find(leaf_idx);
                it != pending_cropbox_.end()) {
            strip("CropBox");
            updated.entries.emplace_back("CropBox",
                                         BuildRectArray(it->second));
        }
        if (auto it = pending_trimbox_.find(leaf_idx);
                it != pending_trimbox_.end()) {
            strip("TrimBox");
            updated.entries.emplace_back("TrimBox",
                                         BuildRectArray(it->second));
        }
        if (auto it = pending_bleedbox_.find(leaf_idx);
                it != pending_bleedbox_.end()) {
            strip("BleedBox");
            updated.entries.emplace_back("BleedBox",
                                         BuildRectArray(it->second));
        }
        if (auto it = pending_artbox_.find(leaf_idx);
                it != pending_artbox_.end()) {
            strip("ArtBox");
            updated.entries.emplace_back("ArtBox",
                                         BuildRectArray(it->second));
        }
        if (auto it = pending_rotation_.find(leaf_idx);
                it != pending_rotation_.end()) {
            strip("Rotate");
            foundation::objects::Value v;
            v.v = static_cast<std::int64_t>(it->second);
            updated.entries.emplace_back("Rotate", std::move(v));
        }

        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = page_id;
        d.generation = page_obj->generation;
        d.value.v = std::move(updated);
        dirty.push_back(std::move(d));
    }

    if (dirty.empty()) return working;
    return foundation::pdf_writer_incremental::AppendIncremental(
        working_span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
}

// ===== Page-tree mutation ====================================================

namespace {

std::uint32_t FindPagesRootId(const foundation::objects::Dump& dump,
                              std::uint32_t catalog_id) {
    for (const auto& obj : dump.objects) {
        if (obj.id != catalog_id) continue;
        const auto* d =
            std::get_if<foundation::objects::Dict>(&obj.value.v);
        if (d == nullptr) return 0;
        for (const auto& kv : d->entries) {
            if (kv.first == "Pages") {
                if (auto* r = std::get_if<foundation::objects::Ref>(
                        &kv.second.v))
                    return r->id;
            }
        }
    }
    return 0;
}

const foundation::objects::IndirectObject* FindPageObj(
        const foundation::objects::Dump& dump, std::uint32_t id) {
    for (const auto& obj : dump.objects) {
        if (obj.id == id) return &obj;
    }
    return nullptr;
}

foundation::objects::Value PageNameValue(const char* name) {
    foundation::objects::Value v;
    v.v = std::string(name);
    return v;
}

}  // namespace

std::size_t Document::AddPageInternal(int insertAt1Based, bool copy,
                                      std::size_t srcLeaf, double width,
                                      double height) {
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    auto trailer_bundle = foundation::trailer::Parse(span);
    auto dump = foundation::objects::Parse(span);
    if (trailer_bundle.root_id == 0)
        throw std::runtime_error(
            "Aspose::Pdf::PageCollection: document has no catalog");

    const std::uint32_t root_id =
        FindPagesRootId(dump, trailer_bundle.root_id);
    const auto* root_obj = root_id ? FindPageObj(dump, root_id) : nullptr;
    const auto* root_dict =
        root_obj ? std::get_if<foundation::objects::Dict>(&root_obj->value.v)
                 : nullptr;
    if (root_dict == nullptr)
        throw std::runtime_error(
            "Aspose::Pdf::PageCollection: no /Pages tree root");

    const foundation::objects::Array* kids_in = nullptr;
    for (const auto& kv : root_dict->entries) {
        if (kv.first == "Kids")
            kids_in = std::get_if<foundation::objects::Array>(&kv.second.v);
    }
    if (kids_in == nullptr ||
        kids_in->items.size() != tree_->leaves.size())
        throw std::runtime_error(
            "Aspose::Pdf::PageCollection: nested /Pages tree mutation "
            "not supported in v1 (flat tree required)");

    std::uint32_t add_maxid = 0;
    for (const auto& o : dump.objects) add_maxid = std::max(add_maxid, o.id);
    const std::uint32_t new_id =
        std::max(trailer_bundle.size, add_maxid + 1);
    const foundation::objects::Ref root_ref{root_id, 0};

    foundation::objects::Dict page;
    page.entries.emplace_back("Type", PageNameValue("Page"));
    {
        foundation::objects::Value pv;
        pv.v = root_ref;
        page.entries.emplace_back("Parent", std::move(pv));
    }
    {
        double w = width, h = height;
        if (copy && srcLeaf < tree_->leaves.size()) {
            w = tree_->leaves[srcLeaf].width;
            h = tree_->leaves[srcLeaf].height;
        }
        foundation::objects::Array mb;
        for (double d_value : {0.0, 0.0, w, h}) {
            foundation::objects::Value v;
            v.v = d_value;
            mb.items.push_back(std::move(v));
        }
        foundation::objects::Value mbv;
        mbv.v = std::move(mb);
        page.entries.emplace_back("MediaBox", std::move(mbv));
    }
    if (copy && srcLeaf < tree_->leaves.size()) {
        const auto* src = FindPageObj(dump, tree_->leaves[srcLeaf].id);
        const auto* src_dict =
            src ? std::get_if<foundation::objects::Dict>(&src->value.v)
                : nullptr;
        if (src_dict != nullptr) {
            for (const char* key :
                 {"Contents", "Resources", "Rotate", "CropBox"}) {
                for (const auto& kv : src_dict->entries) {
                    if (kv.first == key) {
                        page.entries.emplace_back(key, kv.second);
                        break;
                    }
                }
            }
        }
    } else {
        foundation::objects::Value res;
        res.v = foundation::objects::Dict{};
        page.entries.emplace_back("Resources", std::move(res));
    }

    foundation::objects::Array kids = *kids_in;
    std::size_t pos =
        (insertAt1Based <= 0 ||
         static_cast<std::size_t>(insertAt1Based) > kids.items.size())
            ? kids.items.size()
            : static_cast<std::size_t>(insertAt1Based - 1);
    foundation::objects::Value new_ref;
    new_ref.v = foundation::objects::Ref{new_id, 0};
    kids.items.insert(kids.items.begin() + static_cast<std::ptrdiff_t>(pos),
                      std::move(new_ref));

    foundation::objects::Dict updated_root = *root_dict;
    for (auto it = updated_root.entries.begin();
         it != updated_root.entries.end();) {
        if (it->first == "Kids" || it->first == "Count")
            it = updated_root.entries.erase(it);
        else
            ++it;
    }
    {
        foundation::objects::Value kv;
        kv.v = std::move(kids);
        updated_root.entries.emplace_back("Kids", std::move(kv));
        foundation::objects::Value cv;
        cv.v = static_cast<std::int64_t>(kids_in->items.size() + 1);
        updated_root.entries.emplace_back("Count", std::move(cv));
    }

    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;
    {
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = new_id;
        d.generation = 0;
        d.value.v = std::move(page);
        dirty.push_back(std::move(d));
    }
    {
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = root_id;
        d.generation = root_obj->generation;
        d.value.v = std::move(updated_root);
        dirty.push_back(std::move(d));
    }

    bytes_ = foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(
            std::span<const std::byte>(bytes_.data(), bytes_.size())));

    // Re-index the staged, per-leaf write state for the page just inserted
    // at `pos`. The leaves at index >= pos shift up by one; the new page's
    // own slot starts empty. For an append (pos == old leaf count) nothing
    // shifts, so previously staged annotations / geometry / paragraphs on
    // earlier pages survive the Add (they used to be discarded here, which
    // dropped any annotation or SetPageSize staged before a later
    // Pages().Add()).
    auto shift_slots = [pos](auto& vec) {
        if (pos < vec.size())
            vec.insert(vec.begin() + static_cast<std::ptrdiff_t>(pos),
                       nullptr);
    };
    shift_slots(page_annotations_);
    shift_slots(page_paragraphs_);
    shift_slots(page_resources_);
    shift_slots(page_artifacts_);

    auto shift_keys = [pos](auto& m) {
        std::decay_t<decltype(m)> shifted;
        for (auto& [leaf, value] : m)
            shifted.emplace(leaf >= pos ? leaf + 1 : leaf, std::move(value));
        m = std::move(shifted);
    };
    shift_keys(pending_mediabox_);
    shift_keys(pending_cropbox_);
    shift_keys(pending_trimbox_);
    shift_keys(pending_bleedbox_);
    shift_keys(pending_artbox_);
    shift_keys(pending_rotation_);
    page_geom_dirty_ =
        !(pending_mediabox_.empty() && pending_cropbox_.empty() &&
          pending_trimbox_.empty() && pending_bleedbox_.empty() &&
          pending_artbox_.empty() && pending_rotation_.empty());
    return pos;
}

void Document::DeletePagesInternal(std::vector<int> pageNumbers1Based) {
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    auto trailer_bundle = foundation::trailer::Parse(span);
    auto dump = foundation::objects::Parse(span);
    const std::uint32_t root_id =
        FindPagesRootId(dump, trailer_bundle.root_id);
    const auto* root_obj = root_id ? FindPageObj(dump, root_id) : nullptr;
    const auto* root_dict =
        root_obj ? std::get_if<foundation::objects::Dict>(&root_obj->value.v)
                 : nullptr;
    if (root_dict == nullptr)
        throw std::runtime_error(
            "Aspose::Pdf::PageCollection: no /Pages tree root");
    const foundation::objects::Array* kids_in = nullptr;
    for (const auto& kv : root_dict->entries)
        if (kv.first == "Kids")
            kids_in = std::get_if<foundation::objects::Array>(&kv.second.v);
    if (kids_in == nullptr ||
        kids_in->items.size() != tree_->leaves.size())
        throw std::runtime_error(
            "Aspose::Pdf::PageCollection: nested /Pages tree mutation "
            "not supported in v1 (flat tree required)");

    std::vector<bool> drop(kids_in->items.size(), false);
    for (int n : pageNumbers1Based) {
        if (n >= 1 && static_cast<std::size_t>(n) <= drop.size())
            drop[static_cast<std::size_t>(n - 1)] = true;
    }
    foundation::objects::Array kids;
    for (std::size_t i = 0; i < kids_in->items.size(); ++i)
        if (!drop[i]) kids.items.push_back(kids_in->items[i]);

    const std::size_t new_count = kids.items.size();
    foundation::objects::Dict updated_root = *root_dict;
    for (auto it = updated_root.entries.begin();
         it != updated_root.entries.end();) {
        if (it->first == "Kids" || it->first == "Count")
            it = updated_root.entries.erase(it);
        else
            ++it;
    }
    {
        foundation::objects::Value kv;
        kv.v = std::move(kids);
        updated_root.entries.emplace_back("Kids", std::move(kv));
        foundation::objects::Value cv;
        cv.v = static_cast<std::int64_t>(new_count);
        updated_root.entries.emplace_back("Count", std::move(cv));
    }

    foundation::pdf_writer_incremental::DirtyObject d;
    d.id = root_id;
    d.generation = root_obj->generation;
    d.value.v = std::move(updated_root);
    bytes_ = foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            &d, 1));
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(
            std::span<const std::byte>(bytes_.data(), bytes_.size())));
    page_annotations_.clear();
    pending_mediabox_.clear();
    pending_cropbox_.clear();
    pending_rotation_.clear();
    page_geom_dirty_ = false;
}

void Document::DeleteAllPagesInternal() {
    std::vector<int> all;
    for (std::size_t i = 0; i < tree_->leaves.size(); ++i)
        all.push_back(static_cast<int>(i + 1));
    if (!all.empty()) DeletePagesInternal(std::move(all));
}

namespace {

// Gather every indirect-object id referenced (one level) within a Value.
void CollectRefs(const foundation::objects::Value& v,
                 std::vector<std::uint32_t>& out) {
    if (auto* r = std::get_if<foundation::objects::Ref>(&v.v)) {
        out.push_back(r->id);
    } else if (auto* a = std::get_if<foundation::objects::Array>(&v.v)) {
        for (const auto& item : a->items) CollectRefs(item, out);
    } else if (auto* d = std::get_if<foundation::objects::Dict>(&v.v)) {
        for (const auto& kv : d->entries) CollectRefs(kv.second, out);
    } else if (auto* s = std::get_if<foundation::objects::Stream>(&v.v)) {
        for (const auto& kv : s->header.entries) CollectRefs(kv.second, out);
    }
}

// Deep-copy a Value, rewriting each Ref id through `remap` (refs absent
// from the map are left as-is). Stream bodies are shared by span.
foundation::objects::Value RemapValue(
        const foundation::objects::Value& in,
        const std::map<std::uint32_t, std::uint32_t>& remap) {
    foundation::objects::Value out;
    if (auto* r = std::get_if<foundation::objects::Ref>(&in.v)) {
        auto it = remap.find(r->id);
        out.v = foundation::objects::Ref{
            it != remap.end() ? it->second : r->id, 0};
    } else if (auto* a = std::get_if<foundation::objects::Array>(&in.v)) {
        foundation::objects::Array arr;
        arr.items.reserve(a->items.size());
        for (const auto& item : a->items)
            arr.items.push_back(RemapValue(item, remap));
        out.v = std::move(arr);
    } else if (auto* d = std::get_if<foundation::objects::Dict>(&in.v)) {
        foundation::objects::Dict dict;
        dict.entries.reserve(d->entries.size());
        for (const auto& kv : d->entries)
            dict.entries.emplace_back(kv.first, RemapValue(kv.second, remap));
        out.v = std::move(dict);
    } else if (auto* s = std::get_if<foundation::objects::Stream>(&in.v)) {
        foundation::objects::Stream st;
        for (const auto& kv : s->header.entries)
            st.header.entries.emplace_back(kv.first,
                                           RemapValue(kv.second, remap));
        st.body = s->body;  // shared span into the source buffer
        out.v = std::move(st);
    } else {
        out = in;
    }
    return out;
}

}  // namespace

void Document::ImportPagesFrom(const Document& src,
                               const std::vector<int>& srcPages1Based,
                               int insertAt1Based) {
    if (!tree_ || !src.tree_) return;

    auto dspan = std::span<const std::byte>(bytes_.data(), bytes_.size());
    auto dtrailer = foundation::trailer::Parse(dspan);
    auto ddump = foundation::objects::Parse(dspan);
    const std::uint32_t droot_id =
        FindPagesRootId(ddump, dtrailer.root_id);
    const auto* droot_obj = droot_id ? FindPageObj(ddump, droot_id) : nullptr;
    const auto* droot_dict =
        droot_obj
            ? std::get_if<foundation::objects::Dict>(&droot_obj->value.v)
            : nullptr;
    if (droot_dict == nullptr)
        throw std::runtime_error(
            "Aspose::Pdf::PdfFileEditor: destination has no /Pages root");
    const foundation::objects::Array* dkids_in = nullptr;
    for (const auto& kv : droot_dict->entries)
        if (kv.first == "Kids")
            dkids_in = std::get_if<foundation::objects::Array>(&kv.second.v);
    if (dkids_in == nullptr ||
        dkids_in->items.size() != tree_->leaves.size())
        throw std::runtime_error(
            "Aspose::Pdf::PdfFileEditor: nested /Pages tree merge not "
            "supported in v1 (flat tree required)");

    auto sspan =
        std::span<const std::byte>(src.bytes_.data(), src.bytes_.size());
    auto sdump = foundation::objects::Parse(sspan);

    std::vector<std::uint32_t> src_page_ids;
    std::set<std::uint32_t> is_page;
    for (int n : srcPages1Based) {
        if (n >= 1 && static_cast<std::size_t>(n) <= src.tree_->leaves.size()) {
            const std::uint32_t id = src.tree_->leaves[n - 1].id;
            src_page_ids.push_back(id);
            is_page.insert(id);
        }
    }
    if (src_page_ids.empty()) return;

    // BFS the reachable object graph (index-grown worklist).
    std::vector<std::uint32_t> order;
    std::set<std::uint32_t> seen;
    auto add = [&](std::uint32_t id) {
        if (seen.insert(id).second) order.push_back(id);
    };
    for (std::uint32_t id : src_page_ids) add(id);
    for (std::size_t i = 0; i < order.size(); ++i) {
        const std::uint32_t id = order[i];
        const auto* obj = FindPageObj(sdump, id);
        if (obj == nullptr) continue;
        std::vector<std::uint32_t> refs;
        const auto* pd =
            std::get_if<foundation::objects::Dict>(&obj->value.v);
        if (is_page.count(id) && pd != nullptr) {
            for (const auto& kv : pd->entries) {
                if (kv.first == "Parent" || kv.first == "Annots") continue;
                CollectRefs(kv.second, refs);
            }
        } else {
            CollectRefs(obj->value, refs);
        }
        for (std::uint32_t r : refs) add(r);
    }

    // Next free id = max(trailer /Size, highest existing id + 1). The
    // /Size guard guards against a stale trailer size across stacked
    // incremental updates (the writer leaves /Size unbumped in some
    // xref-stream cases, which is harmless for reading but would cause
    // an id collision here).
    std::uint32_t maxid = 0;
    for (const auto& o : ddump.objects) maxid = std::max(maxid, o.id);
    std::uint32_t next = std::max(dtrailer.size, maxid + 1);
    std::map<std::uint32_t, std::uint32_t> remap;
    for (std::uint32_t id : order) remap[id] = next++;

    const foundation::objects::Ref droot_ref{droot_id, 0};
    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;
    for (std::uint32_t id : order) {
        const auto* obj = FindPageObj(sdump, id);
        if (obj == nullptr) continue;
        foundation::objects::Value remapped = RemapValue(obj->value, remap);
        if (is_page.count(id)) {
            auto* pd = std::get_if<foundation::objects::Dict>(&remapped.v);
            if (pd != nullptr) {
                for (auto it = pd->entries.begin();
                     it != pd->entries.end();) {
                    if (it->first == "Parent" || it->first == "Annots")
                        it = pd->entries.erase(it);
                    else
                        ++it;
                }
                foundation::objects::Value pv;
                pv.v = droot_ref;
                pd->entries.emplace_back("Parent", std::move(pv));
            }
        }
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = remap[id];
        d.generation = 0;
        d.value = std::move(remapped);
        dirty.push_back(std::move(d));
    }

    foundation::objects::Array dkids = *dkids_in;
    std::size_t pos =
        (insertAt1Based <= 0 ||
         static_cast<std::size_t>(insertAt1Based) > dkids.items.size())
            ? dkids.items.size()
            : static_cast<std::size_t>(insertAt1Based - 1);
    std::vector<foundation::objects::Value> new_refs;
    for (std::uint32_t pid : src_page_ids) {
        foundation::objects::Value v;
        v.v = foundation::objects::Ref{remap[pid], 0};
        new_refs.push_back(std::move(v));
    }
    dkids.items.insert(
        dkids.items.begin() + static_cast<std::ptrdiff_t>(pos),
        std::make_move_iterator(new_refs.begin()),
        std::make_move_iterator(new_refs.end()));

    foundation::objects::Dict updated_root = *droot_dict;
    for (auto it = updated_root.entries.begin();
         it != updated_root.entries.end();) {
        if (it->first == "Kids" || it->first == "Count")
            it = updated_root.entries.erase(it);
        else
            ++it;
    }
    const std::size_t new_count = dkids.items.size();
    {
        foundation::objects::Value kv;
        kv.v = std::move(dkids);
        updated_root.entries.emplace_back("Kids", std::move(kv));
        foundation::objects::Value cv;
        cv.v = static_cast<std::int64_t>(new_count);
        updated_root.entries.emplace_back("Count", std::move(cv));
    }
    {
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = droot_id;
        d.generation = droot_obj->generation;
        d.value.v = std::move(updated_root);
        dirty.push_back(std::move(d));
    }

    bytes_ = foundation::pdf_writer_incremental::AppendIncremental(
        dspan,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(
            std::span<const std::byte>(bytes_.data(), bytes_.size())));
    page_annotations_.clear();
    pending_mediabox_.clear();
    pending_cropbox_.clear();
    pending_rotation_.clear();
    page_geom_dirty_ = false;
}

// ===== Document outline (/Outlines) ==========================================

void Document::SetStagedOutlines(std::vector<OutlineNode> nodes) {
    staged_outlines_ = std::move(nodes);
    outlines_dirty_ = true;
}

void Document::ClearStagedOutlines() {
    staged_outlines_.clear();
    outlines_dirty_ = true;
}

std::vector<std::pair<int, std::string>> Document::ParseOutlineItems() const {
    std::vector<std::pair<int, std::string>> out;
    try {
        auto parsed = foundation::outlines::Parse(
            std::span<const std::byte>(bytes_.data(), bytes_.size()));
        for (const auto& it : parsed.items)
            out.emplace_back(static_cast<int>(it.depth), it.title);
    } catch (const std::exception&) {
        // forgiving: no outline → empty
    }
    return out;
}

namespace {

int CountOutlineTotal(const std::vector<Document::OutlineNode>& nodes) {
    int n = 0;
    for (const auto& node : nodes)
        n += 1 + CountOutlineTotal(node.children);
    return n;
}

struct OutlineLevel {
    std::uint32_t first = 0;
    std::uint32_t last = 0;
    int count = 0;
};

OutlineLevel BuildOutlineLevel(
        const std::vector<Document::OutlineNode>& nodes,
        foundation::objects::Ref parent, std::uint32_t& next_id,
        std::vector<foundation::pdf_writer_incremental::DirtyObject>& dirty,
        const std::vector<foundation::pages_tree::LeafPage>& leaves) {
    OutlineLevel result;
    if (nodes.empty()) return result;
    std::vector<std::uint32_t> ids;
    ids.reserve(nodes.size());
    for (std::size_t i = 0; i < nodes.size(); ++i) ids.push_back(next_id++);
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        foundation::objects::Dict item;
        {
            foundation::objects::String t;
            t.bytes = EncodePdfString(node.title);
            t.kind = foundation::objects::StringKind::Literal;
            foundation::objects::Value tv;
            tv.v = std::move(t);
            item.entries.emplace_back("Title", std::move(tv));
        }
        {
            foundation::objects::Value pv;
            pv.v = parent;
            item.entries.emplace_back("Parent", std::move(pv));
        }
        if (i > 0) {
            foundation::objects::Value v;
            v.v = foundation::objects::Ref{ids[i - 1], 0};
            item.entries.emplace_back("Prev", std::move(v));
        }
        if (i + 1 < nodes.size()) {
            foundation::objects::Value v;
            v.v = foundation::objects::Ref{ids[i + 1], 0};
            item.entries.emplace_back("Next", std::move(v));
        }
        // /F text-style flags (PDF §12.3.3: bit 1 Italic, bit 2 Bold).
        if (node.italic || node.bold) {
            std::int64_t flags = (node.italic ? 1 : 0) | (node.bold ? 2 : 0);
            foundation::objects::Value fv;
            fv.v = flags;
            item.entries.emplace_back("F", std::move(fv));
        }
        // Navigation target — priority: URI action, then named destination,
        // then explicit destination (typed verb), else a plain /Fit on `page`.
        if (!node.uri.empty()) {
            foundation::objects::Dict a;
            foundation::objects::Value sv;
            sv.v = std::string("URI");
            a.entries.emplace_back("S", std::move(sv));
            foundation::objects::String us;
            us.bytes = EncodePdfString(node.uri);
            us.kind = foundation::objects::StringKind::Literal;
            foundation::objects::Value uv;
            uv.v = std::move(us);
            a.entries.emplace_back("URI", std::move(uv));
            foundation::objects::Value av;
            av.v = std::move(a);
            item.entries.emplace_back("A", std::move(av));
        } else if (!node.named_dest.empty()) {
            foundation::objects::String ns;
            ns.bytes = EncodePdfString(node.named_dest);
            ns.kind = foundation::objects::StringKind::Literal;
            foundation::objects::Value dv;
            dv.v = std::move(ns);
            item.entries.emplace_back("Dest", std::move(dv));
        } else if (node.page >= 1 &&
                   static_cast<std::size_t>(node.page) <= leaves.size()) {
            foundation::objects::Array dest;
            foundation::objects::Value pr;
            pr.v = foundation::objects::Ref{leaves[node.page - 1].id, 0};
            dest.items.push_back(std::move(pr));
            foundation::objects::Value verb;
            verb.v = node.dest_verb.empty() ? std::string("Fit")
                                            : node.dest_verb;
            dest.items.push_back(std::move(verb));
            for (double op : node.dest_ops) {
                foundation::objects::Value ov;
                ov.v = op;
                dest.items.push_back(std::move(ov));
            }
            foundation::objects::Value dv;
            dv.v = std::move(dest);
            item.entries.emplace_back("Dest", std::move(dv));
        }
        if (!node.children.empty()) {
            OutlineLevel child = BuildOutlineLevel(
                node.children, foundation::objects::Ref{ids[i], 0}, next_id,
                dirty, leaves);
            foundation::objects::Value fv;
            fv.v = foundation::objects::Ref{child.first, 0};
            item.entries.emplace_back("First", std::move(fv));
            foundation::objects::Value lv;
            lv.v = foundation::objects::Ref{child.last, 0};
            item.entries.emplace_back("Last", std::move(lv));
            // /Count — negative when the node is collapsed (closed).
            foundation::objects::Value cv;
            cv.v = static_cast<std::int64_t>(node.open ? child.count
                                                       : -child.count);
            item.entries.emplace_back("Count", std::move(cv));
        }
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = ids[i];
        d.generation = 0;
        d.value.v = std::move(item);
        dirty.push_back(std::move(d));
    }
    result.first = ids.front();
    result.last = ids.back();
    result.count = static_cast<int>(nodes.size());
    return result;
}

}  // namespace

std::vector<std::byte> Document::AppendOutlinesUpdate(
        const std::vector<std::byte>& working) const {
    auto span = std::span<const std::byte>(working.data(), working.size());
    foundation::trailer::Trailer trailer_bundle;
    foundation::objects::Dump dump;
    try {
        trailer_bundle = foundation::trailer::Parse(span);
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return working;
    }
    if (trailer_bundle.root_id == 0) return working;
    const auto* catalog = FindPageObj(dump, trailer_bundle.root_id);
    const auto* catalog_dict =
        catalog ? std::get_if<foundation::objects::Dict>(&catalog->value.v)
                : nullptr;
    if (catalog_dict == nullptr) return working;

    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;
    foundation::objects::Dict updated_catalog = *catalog_dict;
    for (auto it = updated_catalog.entries.begin();
         it != updated_catalog.entries.end();) {
        if (it->first == "Outlines")
            it = updated_catalog.entries.erase(it);
        else
            ++it;
    }

    if (!staged_outlines_.empty()) {
        std::uint32_t maxid = 0;
        for (const auto& o : dump.objects) maxid = std::max(maxid, o.id);
        std::uint32_t next_id = std::max(trailer_bundle.size, maxid + 1);
        const std::uint32_t root_outline_id = next_id++;
        const std::vector<foundation::pages_tree::LeafPage> empty_leaves;
        const auto& leaves = tree_ ? tree_->leaves : empty_leaves;
        OutlineLevel top = BuildOutlineLevel(
            staged_outlines_, foundation::objects::Ref{root_outline_id, 0},
            next_id, dirty, leaves);

        foundation::objects::Dict root;
        {
            foundation::objects::Value tv;
            tv.v = std::string("Outlines");
            root.entries.emplace_back("Type", std::move(tv));
        }
        {
            foundation::objects::Value v;
            v.v = foundation::objects::Ref{top.first, 0};
            root.entries.emplace_back("First", std::move(v));
        }
        {
            foundation::objects::Value v;
            v.v = foundation::objects::Ref{top.last, 0};
            root.entries.emplace_back("Last", std::move(v));
        }
        {
            foundation::objects::Value v;
            v.v = static_cast<std::int64_t>(
                CountOutlineTotal(staged_outlines_));
            root.entries.emplace_back("Count", std::move(v));
        }
        foundation::pdf_writer_incremental::DirtyObject rd;
        rd.id = root_outline_id;
        rd.generation = 0;
        rd.value.v = std::move(root);
        dirty.push_back(std::move(rd));

        foundation::objects::Value ov;
        ov.v = foundation::objects::Ref{root_outline_id, 0};
        updated_catalog.entries.emplace_back("Outlines", std::move(ov));
    }

    foundation::pdf_writer_incremental::DirtyObject cd;
    cd.id = trailer_bundle.root_id;
    cd.generation = catalog->generation;
    cd.value.v = std::move(updated_catalog);
    dirty.push_back(std::move(cd));

    return foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
}

// ===== Text stamps (content-stream writer) ===================================

int Document::PageCountInternal() const {
    return tree_ ? static_cast<int>(tree_->leaves.size()) : 0;
}
double Document::PageHeightInternal(std::size_t leaf) const {
    return (tree_ && leaf < tree_->leaves.size())
               ? tree_->leaves[leaf].height
               : 0.0;
}
double Document::PageWidthInternal(std::size_t leaf) const {
    return (tree_ && leaf < tree_->leaves.size())
               ? tree_->leaves[leaf].width
               : 0.0;
}

namespace {

std::string FmtNum(double v) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f", v);
    return buf;
}

// Escape a string for a PDF content-stream literal: ( ) \ get
// backslash-escaped; non-ASCII bytes are dropped (v1 ASCII text).
std::string EscapeContentLiteral(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (c == '\\' || c == '(' || c == ')') {
            out.push_back('\\');
            out.push_back(c);
        } else if (u >= 0x20 && u < 0x7F) {
            out.push_back(c);
        }
    }
    return out;
}

// Resolve a Value that is a Dict or a Ref-to-Dict into a copy of the
// dict (empty if neither).
foundation::objects::Dict ResolveDictCopy(
        const foundation::objects::Value& v,
        const foundation::objects::Dump& dump) {
    if (auto* d = std::get_if<foundation::objects::Dict>(&v.v)) return *d;
    if (auto* r = std::get_if<foundation::objects::Ref>(&v.v)) {
        if (const auto* obj = FindPageObj(dump, r->id)) {
            if (auto* d =
                    std::get_if<foundation::objects::Dict>(&obj->value.v))
                return *d;
        }
    }
    return {};
}

}  // namespace

void Document::ApplyTextStamps(const std::vector<TextStampReq>& stamps) {
    if (stamps.empty() || !tree_) return;
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    auto trailer_bundle = foundation::trailer::Parse(span);
    auto dump = foundation::objects::Parse(span);

    std::uint32_t maxid = 0;
    for (const auto& o : dump.objects) maxid = std::max(maxid, o.id);
    std::uint32_t next = std::max(trailer_bundle.size, maxid + 1);

    // Shared Helvetica font object (/AspHelv in each page's /Font).
    const std::uint32_t font_id = next++;
    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;
    {
        foundation::objects::Dict font;
        font.entries.emplace_back("Type", PageNameValue("Font"));
        font.entries.emplace_back("Subtype", PageNameValue("Type1"));
        font.entries.emplace_back("BaseFont", PageNameValue("Helvetica"));
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = font_id;
        d.generation = 0;
        d.value.v = std::move(font);
        dirty.push_back(std::move(d));
    }

    // Group stamps by leaf, in leaf order.
    std::vector<std::size_t> leaves_order;
    for (const auto& s : stamps) {
        if (std::find(leaves_order.begin(), leaves_order.end(), s.leaf) ==
            leaves_order.end())
            leaves_order.push_back(s.leaf);
    }

    // Content-stream bodies must outlive the AppendIncremental call
    // (Stream.body is a non-owning span).
    std::vector<std::vector<std::byte>> bodies;
    bodies.reserve(leaves_order.size());

    for (std::size_t leaf : leaves_order) {
        if (leaf >= tree_->leaves.size()) continue;
        const std::uint32_t page_id = tree_->leaves[leaf].id;
        const auto* page_obj = FindPageObj(dump, page_id);
        const auto* page_dict =
            page_obj
                ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                : nullptr;
        if (page_dict == nullptr) continue;

        std::string body;
        for (const auto& s : stamps) {
            if (s.leaf != leaf) continue;
            body += "BT /AspHelv " + FmtNum(s.size) + " Tf " +
                    FmtNum(s.x) + " " + FmtNum(s.y) + " Td (" +
                    EscapeContentLiteral(s.text) + ") Tj ET\n";
        }
        std::vector<std::byte> body_bytes;
        body_bytes.reserve(body.size());
        for (char c : body)
            body_bytes.push_back(static_cast<std::byte>(c));
        bodies.push_back(std::move(body_bytes));

        const std::uint32_t stream_id = next++;
        {
            foundation::objects::Stream sv;
            sv.body = std::span<const std::byte>(bodies.back().data(),
                                                 bodies.back().size());
            foundation::pdf_writer_incremental::DirtyObject d;
            d.id = stream_id;
            d.generation = 0;
            d.value.v = std::move(sv);
            dirty.push_back(std::move(d));
        }

        foundation::objects::Dict updated = *page_dict;

        // --- /Contents: append the new stream ref ---
        foundation::objects::Array contents;
        const foundation::objects::Value* existing_contents = nullptr;
        for (const auto& kv : page_dict->entries)
            if (kv.first == "Contents") existing_contents = &kv.second;
        if (existing_contents != nullptr) {
            if (auto* arr = std::get_if<foundation::objects::Array>(
                    &existing_contents->v)) {
                contents = *arr;
            } else if (std::get_if<foundation::objects::Ref>(
                           &existing_contents->v)) {
                contents.items.push_back(*existing_contents);
            }
        }
        {
            foundation::objects::Value sref;
            sref.v = foundation::objects::Ref{stream_id, 0};
            contents.items.push_back(std::move(sref));
        }
        for (auto it = updated.entries.begin(); it != updated.entries.end();) {
            if (it->first == "Contents")
                it = updated.entries.erase(it);
            else
                ++it;
        }
        {
            foundation::objects::Value cv;
            cv.v = std::move(contents);
            updated.entries.emplace_back("Contents", std::move(cv));
        }

        // --- /Resources /Font /AspHelv → font ref (inline merge) ---
        foundation::objects::Dict resources;
        for (const auto& kv : page_dict->entries)
            if (kv.first == "Resources")
                resources = ResolveDictCopy(kv.second, dump);
        foundation::objects::Dict fonts;
        for (const auto& kv : resources.entries)
            if (kv.first == "Font") fonts = ResolveDictCopy(kv.second, dump);
        for (auto it = fonts.entries.begin(); it != fonts.entries.end();) {
            if (it->first == "AspHelv")
                it = fonts.entries.erase(it);
            else
                ++it;
        }
        {
            foundation::objects::Value fref;
            fref.v = foundation::objects::Ref{font_id, 0};
            fonts.entries.emplace_back("AspHelv", std::move(fref));
        }
        for (auto it = resources.entries.begin();
             it != resources.entries.end();) {
            if (it->first == "Font")
                it = resources.entries.erase(it);
            else
                ++it;
        }
        {
            foundation::objects::Value fv;
            fv.v = std::move(fonts);
            resources.entries.emplace_back("Font", std::move(fv));
        }
        for (auto it = updated.entries.begin(); it != updated.entries.end();) {
            if (it->first == "Resources")
                it = updated.entries.erase(it);
            else
                ++it;
        }
        {
            foundation::objects::Value rv;
            rv.v = std::move(resources);
            updated.entries.emplace_back("Resources", std::move(rv));
        }

        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = page_id;
        d.generation = page_obj->generation;
        d.value.v = std::move(updated);
        dirty.push_back(std::move(d));
    }

    bytes_ = foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(
            std::span<const std::byte>(bytes_.data(), bytes_.size())));
    page_annotations_.clear();
    pending_mediabox_.clear();
    pending_cropbox_.clear();
    pending_rotation_.clear();
    page_geom_dirty_ = false;
}

// ===== Table paragraph rendering (Page.Paragraphs) ===========================

namespace {

// Append a single stroked edge (black) when its GraphInfo has a width.
void DrawEdge(std::string& body, const Aspose::Pdf::GraphInfo& g, double x1,
              double y1, double x2, double y2) {
    if (g.LineWidth() <= 0.0f) return;
    body += "q " + FmtNum(g.LineWidth()) + " w 0 0 0 RG " + FmtNum(x1) + " " +
            FmtNum(y1) + " m " + FmtNum(x2) + " " + FmtNum(y2) + " l S Q\n";
}

// Emit an ellipse outline (4 cubic Béziers) into the content body.
void EmitEllipse(std::string& body, double cx, double cy, double rx,
                 double ry) {
    const double k = 0.5522847498307936;
    auto pt = [](double a, double b) {
        return FmtNum(a) + " " + FmtNum(b) + " ";
    };
    body += pt(cx + rx, cy) + "m ";
    body += pt(cx + rx, cy + k * ry) + pt(cx + k * rx, cy + ry) +
            pt(cx, cy + ry) + "c ";
    body += pt(cx - k * rx, cy + ry) + pt(cx - rx, cy + k * ry) +
            pt(cx - rx, cy) + "c ";
    body += pt(cx - rx, cy - k * ry) + pt(cx - k * rx, cy - ry) +
            pt(cx, cy - ry) + "c ";
    body += pt(cx + k * rx, cy - ry) + pt(cx + rx, cy - k * ry) +
            pt(cx + rx, cy) + "c ";
}

// Render a graph's shapes (Line / Rectangle / Circle / Ellipse), offset by the
// graph's placement, stroked in black with each shape's line width.
void DrawGraphShapes(std::string& body,
                     const Aspose::Pdf::Drawing::Graph& graph) {
    const double gx = graph.Left();
    const double gy = graph.Top();
    for (const auto& sp : graph.Shapes()) {
        if (sp == nullptr) continue;
        const float lw = sp->GraphInfo().LineWidth();
        const double w = lw > 0 ? lw : 1.0;
        body += "q " + FmtNum(w) + " w 0 0 0 RG ";
        if (const auto* ln =
                dynamic_cast<const Aspose::Pdf::Drawing::Line*>(sp.get())) {
            const auto& p = ln->PositionArray();
            for (std::size_t i = 0; i + 1 < p.size(); i += 2) {
                body += FmtNum(gx + p[i]) + " " + FmtNum(gy + p[i + 1]) +
                        (i == 0 ? " m " : " l ");
            }
            body += "S";
        } else if (const auto* rc = dynamic_cast<
                       const Aspose::Pdf::Drawing::Rectangle*>(sp.get())) {
            body += FmtNum(gx + rc->Left()) + " " + FmtNum(gy + rc->Bottom()) +
                    " " + FmtNum(rc->Width()) + " " + FmtNum(rc->Height()) +
                    " re S";
        } else if (const auto* ci = dynamic_cast<
                       const Aspose::Pdf::Drawing::Circle*>(sp.get())) {
            EmitEllipse(body, gx + ci->PosX(), gy + ci->PosY(), ci->Radius(),
                        ci->Radius());
            body += "S";
        } else if (const auto* el = dynamic_cast<
                       const Aspose::Pdf::Drawing::Ellipse*>(sp.get())) {
            EmitEllipse(body, gx + el->Left() + el->Width() / 2.0,
                        gy + el->Bottom() + el->Height() / 2.0,
                        el->Width() / 2.0, el->Height() / 2.0);
            body += "S";
        }
        body += " Q\n";
    }
}

double PageMediaHeight(const foundation::objects::Dict& page_dict) {
    for (const auto& kv : page_dict.entries) {
        if (kv.first != "MediaBox") continue;
        if (const auto* a =
                std::get_if<foundation::objects::Array>(&kv.second.v)) {
            if (a->items.size() >= 4) {
                const auto& v = a->items[3].v;
                if (const auto* d = std::get_if<double>(&v)) return *d;
                if (const auto* n = std::get_if<std::int64_t>(&v))
                    return static_cast<double>(*n);
            }
        }
    }
    return 792.0;
}

// Draw a FloatingBox's border frame. Placement: the box's Margin offsets it
// from the top-left (defaulting to a 72-pt inset when unset); its child
// paragraphs / background fill are deferred (the lib strokes in black).
void DrawFloatingBox(std::string& body, const Aspose::Pdf::FloatingBox& fb,
                     double page_h) {
    const double w = fb.Width();
    const double h = fb.Height();
    if (w <= 0.0 || h <= 0.0) return;
    const auto& m = fb.Margin();
    const double x = m.Left() > 0.0 ? m.Left() : 72.0;
    const double top = page_h - (m.Top() > 0.0 ? m.Top() : 72.0);
    const double bottom = top - h;
    const double right = x + w;
    const Aspose::Pdf::BorderInfo& b = fb.Border();
    DrawEdge(body, b.Left(), x, bottom, x, top);
    DrawEdge(body, b.Right(), right, bottom, right, top);
    DrawEdge(body, b.Top(), x, top, right, top);
    DrawEdge(body, b.Bottom(), x, bottom, right, bottom);
}

// Forward declaration — defined with the image-embed helpers further down;
// used by the table renderer for cell background images (C-6).
bool DecodeImageForXObject(const std::vector<std::byte>& file_bytes, int& w,
                           int& h, std::vector<std::byte>& body,
                           const char*& filter, const char*& colorspace);

}  // namespace

std::vector<std::byte> Document::AppendParagraphsUpdate(
        const std::vector<std::byte>& working) const {
    bool any = false;
    for (const auto& slot : page_paragraphs_)
        if (slot && slot->Count() > 0) any = true;
    if (!any || !tree_) return working;

    auto span = std::span<const std::byte>(working.data(), working.size());
    foundation::trailer::Trailer tb;
    foundation::objects::Dump dump;
    try {
        tb = foundation::trailer::Parse(span);
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return working;
    }

    std::uint32_t maxid = 0;
    for (const auto& o : dump.objects) maxid = std::max(maxid, o.id);
    std::uint32_t next = std::max(tb.size, maxid + 1);
    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;

    // Shared Helvetica font (/AspHelv).
    const std::uint32_t font_id = next++;
    {
        foundation::objects::Dict font;
        font.entries.emplace_back("Type", PageNameValue("Font"));
        font.entries.emplace_back("Subtype", PageNameValue("Type1"));
        font.entries.emplace_back("BaseFont", PageNameValue("Helvetica"));
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = font_id;
        d.generation = 0;
        d.value.v = std::move(font);
        dirty.push_back(std::move(d));
    }

    // Continuation pages created for broken tables: their refs accumulate
    // here and are appended to the /Pages root /Kids in one update below.
    const std::uint32_t paras_root_id = FindPagesRootId(dump, tb.root_id);
    std::vector<foundation::objects::Value> continuation_kids;

    std::vector<std::vector<std::byte>> bodies;
    for (std::size_t leaf = 0; leaf < page_paragraphs_.size(); ++leaf) {
        const auto& slot = page_paragraphs_[leaf];
        if (!slot || slot->Count() == 0) continue;
        if (leaf >= tree_->leaves.size()) continue;
        const std::uint32_t page_id = tree_->leaves[leaf].id;
        const auto* page_obj = FindPageObj(dump, page_id);
        const auto* page_dict =
            page_obj ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                     : nullptr;
        if (page_dict == nullptr) continue;
        const double page_h = PageMediaHeight(*page_dict);

        std::string body;
        // Image XObjects created for cell backgrounds on this page (name→id),
        // merged into the page's /Resources /XObject below.
        std::vector<std::pair<std::string, std::uint32_t>> page_xobjs;
        for (int pi = 1; pi <= slot->Count(); ++pi) {
            if (const auto* graph =
                    dynamic_cast<const Aspose::Pdf::Drawing::Graph*>(
                        &(*slot)[pi])) {
                DrawGraphShapes(body, *graph);
                continue;
            }
            if (const auto* fb = dynamic_cast<const Aspose::Pdf::FloatingBox*>(
                    &(*slot)[pi])) {
                DrawFloatingBox(body, *fb, page_h);
                continue;
            }
            const auto* table = dynamic_cast<const Table*>(&(*slot)[pi]);
            if (table == nullptr) continue;

            const double x = table->Left() != 0.0f ? table->Left() : 72.0;
            const double y_top =
                table->Top() != 0.0f ? table->Top() : page_h - 72.0;
            std::vector<double> cols;
            {
                std::istringstream iss(table->ColumnWidths());
                double w;
                while (iss >> w) cols.push_back(w);
            }
            const double pad = 2.0;
            double fs = table->DefaultCellTextState().FontSize();
            if (fs <= 0.0f) fs = 10.0;
            const double bottom_margin = 72.0;

            const Rows& rows = table->Rows();
            const int row_count = rows.Count();
            const int rep = std::max(
                0, std::min(table->RepeatingRowsCount(), row_count));

            auto rowHeight = [&](int ri) {
                const Row& row = rows[ri];
                return row.FixedRowHeight() > 0.0
                           ? row.FixedRowHeight()
                           : std::max(row.MinRowHeight(), fs * 1.4 + 2.0 * pad);
            };
            // Resolve a cell's background-image bytes: the in-memory bytes
            // (BackgroundImage) take precedence over a file path
            // (BackgroundImageFile).
            auto cellImageBytes =
                [&](const Cell& cell) -> std::vector<std::byte> {
                if (!cell.background_image_.empty())
                    return cell.background_image_;
                if (cell.background_image_file_.empty())
                    return {};
                std::ifstream f(cell.background_image_file_, std::ios::binary);
                if (!f) return {};
                const std::string raw(
                    (std::istreambuf_iterator<char>(f)),
                    std::istreambuf_iterator<char>());
                std::vector<std::byte> b(raw.size());
                for (std::size_t i = 0; i < raw.size(); ++i)
                    b[i] = static_cast<std::byte>(
                        static_cast<unsigned char>(raw[i]));
                return b;
            };

            // Draw row `ri` with its top edge at `rowTop` into `target`;
            // widen `maxRight`; return the row height. When `xobjs` is
            // non-null, cell background images are embedded as XObjects and
            // their (name, id) recorded for the page-resource merge.
            auto drawRow =
                [&](std::string& target, int ri, double rowTop,
                    double& maxRight,
                    std::vector<std::pair<std::string, std::uint32_t>>* xobjs)
                -> double {
                const Row& row = rows[ri];
                const Cells& cs = row.Cells();
                const double row_h = rowHeight(ri);
                double cl = x;
                int col = 0;
                for (int ci = 1; ci <= cs.Count(); ++ci) {
                    const Cell& cell = cs[ci];
                    const int span_n = std::max(1, cell.ColSpan());
                    double cw = 0.0;
                    for (int k = 0; k < span_n; ++k) {
                        cw += (col + k < static_cast<int>(cols.size()))
                                  ? cols[static_cast<std::size_t>(col + k)]
                                  : 80.0;
                    }
                    const double cb = rowTop - row_h;
                    // Background image (behind border + text).
                    if (xobjs != nullptr) {
                        const std::vector<std::byte> img = cellImageBytes(cell);
                        int iw = 0, ih = 0;
                        std::vector<std::byte> ibody;
                        const char* ifilter = nullptr;
                        const char* icolor = "DeviceRGB";
                        if (!img.empty() &&
                            DecodeImageForXObject(img, iw, ih, ibody, ifilter,
                                                  icolor)) {
                            const std::string iname =
                                "CellImg" + std::to_string(xobjs->size());
                            const std::uint32_t img_id = next++;
                            foundation::objects::Stream sv;
                            sv.header.entries.emplace_back(
                                "Type", PageNameValue("XObject"));
                            sv.header.entries.emplace_back(
                                "Subtype", PageNameValue("Image"));
                            auto numE = [&](const char* k, int v) {
                                foundation::objects::Value vv;
                                vv.v = static_cast<std::int64_t>(v);
                                sv.header.entries.emplace_back(k,
                                                               std::move(vv));
                            };
                            numE("Width", iw);
                            numE("Height", ih);
                            sv.header.entries.emplace_back(
                                "ColorSpace", PageNameValue(icolor));
                            numE("BitsPerComponent", 8);
                            sv.header.entries.emplace_back(
                                "Filter", PageNameValue(ifilter));
                            bodies.push_back(std::move(ibody));
                            sv.body = std::span<const std::byte>(
                                bodies.back().data(), bodies.back().size());
                            foundation::pdf_writer_incremental::DirtyObject d;
                            d.id = img_id;
                            d.generation = 0;
                            d.value.v = std::move(sv);
                            dirty.push_back(std::move(d));
                            xobjs->emplace_back(iname, img_id);
                            target += "q " + FmtNum(cw) + " 0 0 " +
                                      FmtNum(row_h) + " " + FmtNum(cl) + " " +
                                      FmtNum(cb) + " cm /" + iname + " Do Q\n";
                        }
                    }
                    if (!cell.IsNoBorder()) {
                        const BorderInfo& cbrd = cell.Border();
                        const bool cell_has =
                            cbrd.Left().LineWidth() > 0 ||
                            cbrd.Right().LineWidth() > 0 ||
                            cbrd.Top().LineWidth() > 0 ||
                            cbrd.Bottom().LineWidth() > 0;
                        const BorderInfo& bi =
                            cell_has ? cbrd : table->DefaultCellBorder();
                        DrawEdge(target, bi.Left(), cl, cb, cl, cb + row_h);
                        DrawEdge(target, bi.Right(), cl + cw, cb, cl + cw,
                                 cb + row_h);
                        DrawEdge(target, bi.Top(), cl, cb + row_h, cl + cw,
                                 cb + row_h);
                        DrawEdge(target, bi.Bottom(), cl, cb, cl + cw, cb);
                    }
                    if (!cell.text_.empty()) {
                        const double tx = cl + pad;
                        const double ty = cb + row_h - pad - fs * 0.8;
                        target += "BT /AspHelv " + FmtNum(fs) + " Tf " +
                                  FmtNum(tx) + " " + FmtNum(ty) + " Td (" +
                                  EscapeContentLiteral(cell.text_) + ") Tj ET\n";
                    }
                    cl += cw;
                    col += span_n;
                }
                maxRight = std::max(maxRight, cl);
                return row_h;
            };
            auto drawOuterBorder = [&](std::string& target, double segTop,
                                       double segBottom, double maxRight) {
                if (!table->IsBordersIncluded()) return;
                const BorderInfo& tbo = table->Border();
                DrawEdge(target, tbo.Left(), x, segBottom, x, segTop);
                DrawEdge(target, tbo.Right(), maxRight, segBottom, maxRight,
                         segTop);
                DrawEdge(target, tbo.Top(), x, segTop, maxRight, segTop);
                DrawEdge(target, tbo.Bottom(), x, segBottom, maxRight,
                         segBottom);
            };

            // First segment renders into the page's shared body.
            double y = y_top;
            double max_right = x;
            int ri = 1;
            for (; ri <= row_count; ++ri) {
                const double row_h = rowHeight(ri);
                if (table->IsBroken() && ri > 1 &&
                    (y - row_h) < bottom_margin)
                    break;  // overflow → continue on a new page
                drawRow(body, ri, y, max_right, &page_xobjs);
                y -= row_h;
            }
            drawOuterBorder(body, y_top, y, max_right);

            // Continuation pages for the remaining rows (broken tables only).
            const foundation::objects::Value* mediabox = nullptr;
            for (const auto& kv : page_dict->entries)
                if (kv.first == "MediaBox") mediabox = &kv.second;
            while (table->IsBroken() && ri <= row_count &&
                   paras_root_id != 0) {
                std::string cbody;
                double cmax_right = x;
                const double cseg_top = page_h - 72.0;
                double cy = cseg_top;
                for (int h = 1; h <= rep; ++h)
                    cy -= drawRow(cbody, h, cy, cmax_right, nullptr);
                int rows_this_seg = 0;
                for (; ri <= row_count; ++ri) {
                    const double row_h = rowHeight(ri);
                    if (rows_this_seg > 0 && (cy - row_h) < bottom_margin)
                        break;
                    drawRow(cbody, ri, cy, cmax_right, nullptr);
                    cy -= row_h;
                    ++rows_this_seg;
                }
                drawOuterBorder(cbody, cseg_top, cy, cmax_right);

                std::vector<std::byte> cbytes;
                cbytes.reserve(cbody.size());
                for (char ch : cbody)
                    cbytes.push_back(static_cast<std::byte>(ch));
                bodies.push_back(std::move(cbytes));
                const std::uint32_t cstream_id = next++;
                {
                    foundation::objects::Stream sv;
                    sv.body = std::span<const std::byte>(bodies.back().data(),
                                                         bodies.back().size());
                    foundation::pdf_writer_incremental::DirtyObject d;
                    d.id = cstream_id;
                    d.generation = 0;
                    d.value.v = std::move(sv);
                    dirty.push_back(std::move(d));
                }
                const std::uint32_t cpage_id = next++;
                {
                    foundation::objects::Dict pg;
                    pg.entries.emplace_back("Type", PageNameValue("Page"));
                    pg.entries.emplace_back(
                        "Parent", foundation::objects::Value{
                                      foundation::objects::Ref{paras_root_id,
                                                               0}});
                    if (mediabox != nullptr)
                        pg.entries.emplace_back("MediaBox", *mediabox);
                    pg.entries.emplace_back(
                        "Contents", foundation::objects::Value{
                                        foundation::objects::Ref{cstream_id,
                                                                 0}});
                    foundation::objects::Dict fnt;
                    fnt.entries.emplace_back(
                        "AspHelv", foundation::objects::Value{
                                       foundation::objects::Ref{font_id, 0}});
                    foundation::objects::Dict res;
                    res.entries.emplace_back(
                        "Font", foundation::objects::Value{std::move(fnt)});
                    pg.entries.emplace_back(
                        "Resources", foundation::objects::Value{std::move(res)});
                    foundation::pdf_writer_incremental::DirtyObject d;
                    d.id = cpage_id;
                    d.generation = 0;
                    d.value.v = std::move(pg);
                    dirty.push_back(std::move(d));
                }
                continuation_kids.push_back(foundation::objects::Value{
                    foundation::objects::Ref{cpage_id, 0}});
            }
        }
        if (body.empty()) continue;

        std::vector<std::byte> body_bytes;
        body_bytes.reserve(body.size());
        for (char c : body) body_bytes.push_back(static_cast<std::byte>(c));
        bodies.push_back(std::move(body_bytes));

        const std::uint32_t stream_id = next++;
        {
            foundation::objects::Stream sv;
            sv.body = std::span<const std::byte>(bodies.back().data(),
                                                 bodies.back().size());
            foundation::pdf_writer_incremental::DirtyObject d;
            d.id = stream_id;
            d.generation = 0;
            d.value.v = std::move(sv);
            dirty.push_back(std::move(d));
        }

        foundation::objects::Dict updated = *page_dict;
        foundation::objects::Array contents;
        const foundation::objects::Value* existing = nullptr;
        for (const auto& kv : page_dict->entries)
            if (kv.first == "Contents") existing = &kv.second;
        if (existing != nullptr) {
            if (auto* arr =
                    std::get_if<foundation::objects::Array>(&existing->v))
                contents = *arr;
            else if (std::get_if<foundation::objects::Ref>(&existing->v))
                contents.items.push_back(*existing);
        }
        contents.items.push_back(
            foundation::objects::Value{foundation::objects::Ref{stream_id, 0}});
        for (auto it = updated.entries.begin(); it != updated.entries.end();) {
            if (it->first == "Contents")
                it = updated.entries.erase(it);
            else
                ++it;
        }
        updated.entries.emplace_back(
            "Contents", foundation::objects::Value{std::move(contents)});

        foundation::objects::Dict resources;
        for (const auto& kv : page_dict->entries)
            if (kv.first == "Resources")
                resources = ResolveDictCopy(kv.second, dump);
        foundation::objects::Dict fonts;
        for (const auto& kv : resources.entries)
            if (kv.first == "Font") fonts = ResolveDictCopy(kv.second, dump);
        for (auto it = fonts.entries.begin(); it != fonts.entries.end();) {
            if (it->first == "AspHelv")
                it = fonts.entries.erase(it);
            else
                ++it;
        }
        fonts.entries.emplace_back(
            "AspHelv", foundation::objects::Value{
                           foundation::objects::Ref{font_id, 0}});
        for (auto it = resources.entries.begin();
             it != resources.entries.end();) {
            if (it->first == "Font")
                it = resources.entries.erase(it);
            else
                ++it;
        }
        resources.entries.emplace_back(
            "Font", foundation::objects::Value{std::move(fonts)});

        // Merge cell-background image XObjects into /Resources /XObject.
        if (!page_xobjs.empty()) {
            foundation::objects::Dict xobjects;
            for (const auto& kv : resources.entries)
                if (kv.first == "XObject")
                    xobjects = ResolveDictCopy(kv.second, dump);
            for (const auto& [name, id] : page_xobjs)
                xobjects.entries.emplace_back(
                    name, foundation::objects::Value{
                              foundation::objects::Ref{id, 0}});
            for (auto it = resources.entries.begin();
                 it != resources.entries.end();) {
                if (it->first == "XObject")
                    it = resources.entries.erase(it);
                else
                    ++it;
            }
            resources.entries.emplace_back(
                "XObject", foundation::objects::Value{std::move(xobjects)});
        }

        for (auto it = updated.entries.begin(); it != updated.entries.end();) {
            if (it->first == "Resources")
                it = updated.entries.erase(it);
            else
                ++it;
        }
        updated.entries.emplace_back(
            "Resources", foundation::objects::Value{std::move(resources)});

        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = page_id;
        d.generation = page_obj->generation;
        d.value.v = std::move(updated);
        dirty.push_back(std::move(d));
    }

    // Link the broken-table continuation pages into the /Pages root.
    if (!continuation_kids.empty() && paras_root_id != 0) {
        const auto* root_obj = FindPageObj(dump, paras_root_id);
        const auto* root_dict =
            root_obj
                ? std::get_if<foundation::objects::Dict>(&root_obj->value.v)
                : nullptr;
        if (root_dict != nullptr) {
            foundation::objects::Array kids;
            std::int64_t count = 0;
            for (const auto& kv : root_dict->entries) {
                if (kv.first == "Kids") {
                    if (auto* a =
                            std::get_if<foundation::objects::Array>(&kv.second.v))
                        kids = *a;
                } else if (kv.first == "Count") {
                    if (auto* n = std::get_if<std::int64_t>(&kv.second.v))
                        count = *n;
                }
            }
            for (auto& ref : continuation_kids)
                kids.items.push_back(std::move(ref));
            count += static_cast<std::int64_t>(continuation_kids.size());

            foundation::objects::Dict updated_root = *root_dict;
            for (auto it = updated_root.entries.begin();
                 it != updated_root.entries.end();) {
                if (it->first == "Kids" || it->first == "Count")
                    it = updated_root.entries.erase(it);
                else
                    ++it;
            }
            updated_root.entries.emplace_back(
                "Kids", foundation::objects::Value{std::move(kids)});
            {
                foundation::objects::Value cv;
                cv.v = count;
                updated_root.entries.emplace_back("Count", std::move(cv));
            }
            foundation::pdf_writer_incremental::DirtyObject d;
            d.id = paras_root_id;
            d.generation = root_obj->generation;
            d.value.v = std::move(updated_root);
            dirty.push_back(std::move(d));
        }
    }

    if (dirty.size() <= 1) return working;  // only the font — no tables drawn
    return foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
}

// ===== Content-stream text replace ===========================================

namespace {

bool StreamIsFlate(const foundation::objects::Dict& header) {
    for (const auto& kv : header.entries) {
        if (kv.first != "Filter") continue;
        if (auto* n = std::get_if<std::string>(&kv.second.v))
            return *n == "FlateDecode";
        if (auto* a = std::get_if<foundation::objects::Array>(&kv.second.v)) {
            for (const auto& it : a->items)
                if (auto* nn = std::get_if<std::string>(&it.v))
                    if (*nn == "FlateDecode") return true;
        }
    }
    return false;
}

// Replace every occurrence of `needle` with `repl` in `data`; returns
// the count.
int ReplaceAllBytes(std::vector<std::byte>& data, const std::string& needle,
                    const std::string& repl) {
    if (needle.empty()) return 0;
    std::string s(reinterpret_cast<const char*>(data.data()), data.size());
    int count = 0;
    std::string::size_type pos = 0;
    while ((pos = s.find(needle, pos)) != std::string::npos) {
        s.replace(pos, needle.size(), repl);
        pos += repl.size();
        ++count;
    }
    if (count > 0) {
        data.assign(reinterpret_cast<const std::byte*>(s.data()),
                    reinterpret_cast<const std::byte*>(s.data()) + s.size());
    }
    return count;
}

}  // namespace

int Document::ReplaceTextInContent(const std::string& src,
                                   const std::string& dest,
                                   int pageNumber1BasedOr0) {
    if (!tree_) return 0;
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return 0;
    }

    // Target leaves.
    std::vector<std::size_t> leaves;
    if (pageNumber1BasedOr0 > 0) {
        if (static_cast<std::size_t>(pageNumber1BasedOr0) <=
            tree_->leaves.size())
            leaves.push_back(
                static_cast<std::size_t>(pageNumber1BasedOr0 - 1));
    } else {
        for (std::size_t i = 0; i < tree_->leaves.size(); ++i)
            leaves.push_back(i);
    }

    // Collect content-stream object ids from the target pages.
    std::vector<std::uint32_t> stream_ids;
    for (std::size_t leaf : leaves) {
        const auto* page_obj = FindPageObj(dump, tree_->leaves[leaf].id);
        const auto* page_dict =
            page_obj
                ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                : nullptr;
        if (page_dict == nullptr) continue;
        for (const auto& kv : page_dict->entries) {
            if (kv.first != "Contents") continue;
            if (auto* r = std::get_if<foundation::objects::Ref>(&kv.second.v))
                stream_ids.push_back(r->id);
            else if (auto* a = std::get_if<foundation::objects::Array>(
                         &kv.second.v))
                for (const auto& it : a->items)
                    if (auto* rr =
                            std::get_if<foundation::objects::Ref>(&it.v))
                        stream_ids.push_back(rr->id);
        }
    }

    const std::string needle = "(" + src + ")";
    const std::string repl = "(" + dest + ")";
    int total = 0;
    std::vector<std::vector<std::byte>> bodies;
    bodies.reserve(stream_ids.size());
    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;

    for (std::uint32_t sid : stream_ids) {
        const auto* obj = FindPageObj(dump, sid);
        const auto* stream =
            obj ? std::get_if<foundation::objects::Stream>(&obj->value.v)
                : nullptr;
        if (stream == nullptr) continue;

        std::vector<std::byte> decoded;
        const bool flate = StreamIsFlate(stream->header);
        if (flate) {
            try {
                decoded = foundation::flate::Decode(stream->body);
            } catch (const std::exception&) {
                continue;
            }
        } else {
            decoded.assign(stream->body.begin(), stream->body.end());
        }

        const int n = ReplaceAllBytes(decoded, needle, repl);
        if (n == 0) continue;
        total += n;

        // Re-emit uncompressed: strip /Filter + /DecodeParms + /Length.
        foundation::objects::Dict header = stream->header;
        for (auto it = header.entries.begin(); it != header.entries.end();) {
            if (it->first == "Filter" || it->first == "DecodeParms" ||
                it->first == "Length")
                it = header.entries.erase(it);
            else
                ++it;
        }
        bodies.push_back(std::move(decoded));
        foundation::objects::Stream sv;
        sv.header = std::move(header);
        sv.body = std::span<const std::byte>(bodies.back().data(),
                                             bodies.back().size());
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = sid;
        d.generation = obj->generation;
        d.value.v = std::move(sv);
        dirty.push_back(std::move(d));
    }

    if (dirty.empty()) return 0;
    bytes_ = foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(
            std::span<const std::byte>(bytes_.data(), bytes_.size())));
    page_annotations_.clear();
    pending_mediabox_.clear();
    pending_cropbox_.clear();
    pending_rotation_.clear();
    page_geom_dirty_ = false;
    return total;
}

namespace {

// 3x2 affine text matrix [a b c d e f].
struct Mat { double a = 1, b = 0, c = 0, d = 1, e = 0, f = 0; };

// PDF matrix product M = A . B (row-vector convention).
Mat MatMul(const Mat& A, const Mat& B) {
    return Mat{A.a * B.a + A.b * B.c,
               A.a * B.b + A.b * B.d,
               A.c * B.a + A.d * B.c,
               A.c * B.b + A.d * B.d,
               A.e * B.a + A.f * B.c + B.e,
               A.e * B.b + A.f * B.d + B.f};
}

double NumOperand(const foundation::objects::Value& v) {
    if (const auto* d = std::get_if<double>(&v.v)) return *d;
    if (const auto* i = std::get_if<std::int64_t>(&v.v))
        return static_cast<double>(*i);
    return 0.0;
}

// Latin-1 decode a String operand's bytes.
std::string LatinDecode(const foundation::objects::String& s) {
    std::string out;
    out.reserve(s.bytes.size());
    for (auto b : s.bytes) out.push_back(static_cast<char>(b));
    return out;
}

}  // namespace

std::vector<Document::TextShow> Document::ScanTextShowsInternal(
        std::size_t leaf, bool allPages) const {
    std::vector<TextShow> out;
    if (!tree_) return out;
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return out;
    }

    std::vector<std::size_t> leaves;
    if (allPages) {
        for (std::size_t i = 0; i < tree_->leaves.size(); ++i)
            leaves.push_back(i);
    } else if (leaf < tree_->leaves.size()) {
        leaves.push_back(leaf);
    }

    for (std::size_t lf : leaves) {
        const auto* page_obj = FindPageObj(dump, tree_->leaves[lf].id);
        const auto* page_dict =
            page_obj
                ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                : nullptr;
        if (page_dict == nullptr) continue;

        std::vector<std::uint32_t> stream_ids;
        for (const auto& kv : page_dict->entries) {
            if (kv.first != "Contents") continue;
            if (auto* r = std::get_if<foundation::objects::Ref>(&kv.second.v))
                stream_ids.push_back(r->id);
            else if (auto* a = std::get_if<foundation::objects::Array>(
                         &kv.second.v))
                for (const auto& it : a->items)
                    if (auto* rr =
                            std::get_if<foundation::objects::Ref>(&it.v))
                        stream_ids.push_back(rr->id);
        }

        for (std::uint32_t sid : stream_ids) {
            const auto* obj = FindPageObj(dump, sid);
            const auto* stream =
                obj ? std::get_if<foundation::objects::Stream>(&obj->value.v)
                    : nullptr;
            if (stream == nullptr) continue;
            std::vector<std::byte> decoded;
            if (StreamIsFlate(stream->header)) {
                try {
                    decoded = foundation::flate::Decode(stream->body);
                } catch (const std::exception&) {
                    continue;
                }
            } else {
                decoded.assign(stream->body.begin(), stream->body.end());
            }

            foundation::content_stream::Stream cs;
            try {
                cs = foundation::content_stream::Parse(
                    std::span<const std::byte>(decoded.data(),
                                               decoded.size()));
            } catch (const std::exception&) {
                continue;
            }

            // Track text matrix (Tm/Tlm), leading, and font size; emit a
            // TextShow per single-string Tj. Count occurrences of identical
            // literals within this stream for write-back targeting.
            Mat tm, tlm;
            double leading = 0.0, font_size = 0.0;
            std::map<std::string, std::size_t> literal_seen;
            for (const auto& op : cs.operations) {
                if (op.op == "BT") {
                    tm = Mat{};
                    tlm = Mat{};
                } else if (op.op == "Tf" && op.operands.size() >= 2) {
                    font_size = NumOperand(op.operands[1]);
                } else if (op.op == "TL" && !op.operands.empty()) {
                    leading = NumOperand(op.operands[0]);
                } else if (op.op == "Td" && op.operands.size() >= 2) {
                    tlm = MatMul(Mat{1, 0, 0, 1, NumOperand(op.operands[0]),
                                     NumOperand(op.operands[1])},
                                 tlm);
                    tm = tlm;
                } else if (op.op == "TD" && op.operands.size() >= 2) {
                    leading = -NumOperand(op.operands[1]);
                    tlm = MatMul(Mat{1, 0, 0, 1, NumOperand(op.operands[0]),
                                     NumOperand(op.operands[1])},
                                 tlm);
                    tm = tlm;
                } else if (op.op == "Tm" && op.operands.size() >= 6) {
                    tlm = Mat{NumOperand(op.operands[0]),
                              NumOperand(op.operands[1]),
                              NumOperand(op.operands[2]),
                              NumOperand(op.operands[3]),
                              NumOperand(op.operands[4]),
                              NumOperand(op.operands[5])};
                    tm = tlm;
                } else if (op.op == "T*") {
                    tlm = MatMul(Mat{1, 0, 0, 1, 0, -leading}, tlm);
                    tm = tlm;
                } else if ((op.op == "Tj" || op.op == "'" || op.op == "\"") &&
                           !op.operands.empty()) {
                    if (op.op != "Tj") {  // ' and " move to next line first
                        tlm = MatMul(Mat{1, 0, 0, 1, 0, -leading}, tlm);
                        tm = tlm;
                    }
                    const auto* s = std::get_if<foundation::objects::String>(
                        op.op == "\"" && op.operands.size() >= 3
                            ? &op.operands[2].v
                            : &op.operands[0].v);
                    if (s == nullptr) continue;
                    TextShow show;
                    show.text = LatinDecode(*s);
                    show.x = tm.e;
                    show.y = tm.f;
                    show.font_size = font_size;
                    show.leaf = lf;
                    show.stream_id = sid;
                    show.occurrence = literal_seen[show.text]++;
                    out.push_back(std::move(show));
                }
            }
        }
    }
    return out;
}

void Document::RegisterTextEditInternal(const TextEdit& edit) {
    pending_text_edits_.push_back(edit);
}

void Document::ApplyRedactionInternal(std::size_t leaf,
                                      const Aspose::Pdf::Rectangle& region,
                                      const std::string& overlayText,
                                      double fontSize, int alignment) {
    const double rllx = region.LLX();
    const double rlly = region.LLY();
    const double rurx = region.URX();
    const double rury = region.URY();

    // Remove every text show whose box intersects the region so the text is
    // gone from the saved content stream — not merely painted over.
    const auto shows = ScanTextShowsInternal(leaf, false);
    for (const auto& s : shows) {
        if (s.text.empty()) continue;
        const double w = static_cast<double>(s.text.size()) * 0.5 * s.font_size;
        const double sx0 = s.x, sy0 = s.y;
        const double sx1 = s.x + w, sy1 = s.y + s.font_size;
        if (sx1 <= rllx || sx0 >= rurx || sy1 <= rlly || sy0 >= rury)
            continue;  // no overlap with the redaction region
        TextEdit e;
        e.stream_id = s.stream_id;
        e.occurrence = s.occurrence;
        e.old_literal = s.text;
        e.new_literal = "";
        RegisterTextEditInternal(e);
    }

    PendingRedaction pr;
    pr.leaf = leaf;
    pr.llx = rllx;
    pr.lly = rlly;
    pr.urx = rurx;
    pr.ury = rury;
    pr.overlay_text = overlayText;
    pr.font_size = fontSize;
    pr.alignment = alignment;
    pending_redactions_.push_back(std::move(pr));
}

std::vector<std::byte> Document::AppendRedactionsUpdate(
        const std::vector<std::byte>& working) const {
    if (pending_redactions_.empty() || !tree_) return working;

    auto span = std::span<const std::byte>(working.data(), working.size());
    foundation::trailer::Trailer tb;
    foundation::objects::Dump dump;
    try {
        tb = foundation::trailer::Parse(span);
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return working;
    }
    std::uint32_t maxid = 0;
    for (const auto& o : dump.objects) maxid = std::max(maxid, o.id);
    std::uint32_t next = std::max(tb.size, maxid + 1);

    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;
    std::vector<std::vector<std::byte>> bodies;
    bool uses_font = false;

    // Shared Helvetica for overlay text (/AspHelv).
    const std::uint32_t font_id = next++;
    {
        foundation::objects::Dict font;
        font.entries.emplace_back("Type", PageNameValue("Font"));
        font.entries.emplace_back("Subtype", PageNameValue("Type1"));
        font.entries.emplace_back("BaseFont", PageNameValue("Helvetica"));
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = font_id;
        d.generation = 0;
        d.value.v = std::move(font);
        dirty.push_back(std::move(d));
    }

    // Group redactions by leaf so each page gets a single content append.
    std::set<std::size_t> leaves;
    for (const auto& pr : pending_redactions_) leaves.insert(pr.leaf);

    for (std::size_t leaf : leaves) {
        if (leaf >= tree_->leaves.size()) continue;
        const std::uint32_t page_id = tree_->leaves[leaf].id;
        const auto* page_obj = FindPageObj(dump, page_id);
        const auto* page_dict =
            page_obj ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                     : nullptr;
        if (page_dict == nullptr) continue;

        std::string body;
        char buf[256];
        for (const auto& pr : pending_redactions_) {
            if (pr.leaf != leaf) continue;
            const double w = pr.urx - pr.llx;
            const double h = pr.ury - pr.lly;
            if (w <= 0.0 || h <= 0.0) continue;
            // Opaque black cover box.
            std::snprintf(buf, sizeof(buf), "q 0 0 0 rg %.2f %.2f %.2f %.2f re f Q\n",
                          pr.llx, pr.lly, w, h);
            body += buf;
            if (!pr.overlay_text.empty()) {
                double fs = pr.font_size > 0.0 ? pr.font_size
                                               : std::min(12.0, h * 0.6);
                if (fs <= 0.0) fs = 8.0;
                const double tw =
                    static_cast<double>(pr.overlay_text.size()) * 0.5 * fs;
                double tx = pr.llx + 2.0;
                if (pr.alignment == 2)  // Center
                    tx = pr.llx + (w - tw) / 2.0;
                else if (pr.alignment == 3)  // Right
                    tx = pr.urx - tw - 2.0;
                const double ty = pr.lly + (h - fs) / 2.0;
                // White text on the black box.
                std::snprintf(buf, sizeof(buf),
                              "q 1 1 1 rg BT /AspHelv %.2f Tf %.2f %.2f Td\n",
                              fs, tx, ty);
                body += buf;
                body += "(" + EscapeContentLiteral(pr.overlay_text) +
                        ") Tj ET Q\n";
                uses_font = true;
            }
        }
        if (body.empty()) continue;

        std::vector<std::byte> bb;
        bb.reserve(body.size());
        for (char c : body) bb.push_back(static_cast<std::byte>(c));
        bodies.push_back(std::move(bb));
        const std::uint32_t stream_id = next++;
        {
            foundation::objects::Stream sv;
            sv.body = std::span<const std::byte>(bodies.back().data(),
                                                 bodies.back().size());
            foundation::pdf_writer_incremental::DirtyObject d;
            d.id = stream_id;
            d.generation = 0;
            d.value.v = std::move(sv);
            dirty.push_back(std::move(d));
        }

        foundation::objects::Dict updated = *page_dict;
        foundation::objects::Array contents;
        const foundation::objects::Value* existing = nullptr;
        for (const auto& kv : page_dict->entries)
            if (kv.first == "Contents") existing = &kv.second;
        if (existing != nullptr) {
            if (auto* arr =
                    std::get_if<foundation::objects::Array>(&existing->v))
                contents = *arr;
            else if (std::get_if<foundation::objects::Ref>(&existing->v))
                contents.items.push_back(*existing);
        }
        contents.items.push_back(foundation::objects::Value{
            foundation::objects::Ref{stream_id, 0}});  // cover drawn last (top)
        for (auto it = updated.entries.begin(); it != updated.entries.end();) {
            if (it->first == "Contents")
                it = updated.entries.erase(it);
            else
                ++it;
        }
        updated.entries.emplace_back(
            "Contents", foundation::objects::Value{std::move(contents)});

        if (uses_font) {
            foundation::objects::Dict resources;
            for (const auto& kv : page_dict->entries)
                if (kv.first == "Resources")
                    resources = ResolveDictCopy(kv.second, dump);
            foundation::objects::Dict fonts;
            for (const auto& kv : resources.entries)
                if (kv.first == "Font")
                    fonts = ResolveDictCopy(kv.second, dump);
            for (auto it = fonts.entries.begin(); it != fonts.entries.end();) {
                if (it->first == "AspHelv")
                    it = fonts.entries.erase(it);
                else
                    ++it;
            }
            fonts.entries.emplace_back(
                "AspHelv", foundation::objects::Value{
                               foundation::objects::Ref{font_id, 0}});
            for (auto it = resources.entries.begin();
                 it != resources.entries.end();) {
                if (it->first == "Font")
                    it = resources.entries.erase(it);
                else
                    ++it;
            }
            resources.entries.emplace_back(
                "Font", foundation::objects::Value{std::move(fonts)});
            for (auto it = updated.entries.begin();
                 it != updated.entries.end();) {
                if (it->first == "Resources")
                    it = updated.entries.erase(it);
                else
                    ++it;
            }
            updated.entries.emplace_back(
                "Resources", foundation::objects::Value{std::move(resources)});
        }

        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = page_id;
        d.generation = page_obj->generation;
        d.value.v = std::move(updated);
        dirty.push_back(std::move(d));
    }

    if (dirty.size() <= 1) return working;  // only the font
    return foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
}

std::vector<std::byte> Document::AppendTextEditsUpdate(
        const std::vector<std::byte>& working) const {
    if (pending_text_edits_.empty()) return working;
    auto span = std::span<const std::byte>(working.data(), working.size());
    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return working;
    }

    // Group edits by stream.
    std::map<std::uint32_t, std::vector<const TextEdit*>> by_stream;
    for (const auto& e : pending_text_edits_) by_stream[e.stream_id].push_back(&e);

    std::vector<std::vector<std::byte>> bodies;
    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;

    for (auto& [sid, edits] : by_stream) {
        const auto* obj = FindPageObj(dump, sid);
        const auto* stream =
            obj ? std::get_if<foundation::objects::Stream>(&obj->value.v)
                : nullptr;
        if (stream == nullptr) continue;
        std::vector<std::byte> decoded;
        if (StreamIsFlate(stream->header)) {
            try {
                decoded = foundation::flate::Decode(stream->body);
            } catch (const std::exception&) {
                continue;
            }
        } else {
            decoded.assign(stream->body.begin(), stream->body.end());
        }

        // Apply each edit: replace the occurrence-th `(old_literal)` with
        // `(new_literal)`.
        bool changed = false;
        for (const TextEdit* e : edits) {
            const std::string needle = "(" + e->old_literal + ")";
            const std::string repl = "(" + e->new_literal + ")";
            std::string text(reinterpret_cast<const char*>(decoded.data()),
                             decoded.size());
            std::size_t pos = 0;
            std::size_t found = std::string::npos;
            for (std::size_t k = 0; k <= e->occurrence; ++k) {
                found = text.find(needle, pos);
                if (found == std::string::npos) break;
                pos = found + needle.size();
            }
            if (found == std::string::npos) continue;
            text.replace(found, needle.size(), repl);
            decoded.assign(reinterpret_cast<const std::byte*>(text.data()),
                           reinterpret_cast<const std::byte*>(text.data()) +
                               text.size());
            changed = true;
        }
        if (!changed) continue;

        foundation::objects::Dict header = stream->header;
        for (auto it = header.entries.begin(); it != header.entries.end();) {
            if (it->first == "Filter" || it->first == "DecodeParms" ||
                it->first == "Length")
                it = header.entries.erase(it);
            else
                ++it;
        }
        bodies.push_back(std::move(decoded));
        foundation::objects::Stream sv;
        sv.header = std::move(header);
        sv.body = std::span<const std::byte>(bodies.back().data(),
                                             bodies.back().size());
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = sid;
        d.generation = obj->generation;
        d.value.v = std::move(sv);
        dirty.push_back(std::move(d));
    }

    if (dirty.empty()) return working;
    return foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
}

// ===== Images (XObject embed / extract) ======================================

namespace {

// Parse a JPEG's SOF marker → (width, height, components). Returns
// false if not a JPEG / no SOF found.
bool JpegInfo(const std::vector<std::byte>& b, int& w, int& h, int& comps) {
    if (b.size() < 4) return false;
    auto u8 = [&](std::size_t i) { return static_cast<unsigned>(b[i]); };
    if (u8(0) != 0xFF || u8(1) != 0xD8) return false;
    std::size_t i = 2;
    while (i + 4 <= b.size()) {
        if (u8(i) != 0xFF) { ++i; continue; }
        unsigned marker = u8(i + 1);
        if (marker == 0xD9 || marker == 0x01 ||
            (marker >= 0xD0 && marker <= 0xD7)) {
            i += 2;
            continue;
        }
        const std::size_t len = (u8(i + 2) << 8) | u8(i + 3);
        const bool sof =
            (marker >= 0xC0 && marker <= 0xCF) && marker != 0xC4 &&
            marker != 0xC8 && marker != 0xCC;
        if (sof && i + 9 < b.size()) {
            h = static_cast<int>((u8(i + 5) << 8) | u8(i + 6));
            w = static_cast<int>((u8(i + 7) << 8) | u8(i + 8));
            comps = static_cast<int>(u8(i + 9));
            return w > 0 && h > 0;
        }
        i += 2 + len;
    }
    return false;
}

// Collect (xobjectName, streamId) image XObjects on a page.
std::vector<std::pair<std::string, std::uint32_t>> PageImageXObjects(
        const foundation::objects::Dump& dump,
        const foundation::objects::Dict& page_dict) {
    std::vector<std::pair<std::string, std::uint32_t>> out;
    for (const auto& kv : page_dict.entries) {
        if (kv.first != "Resources") continue;
        foundation::objects::Dict res = ResolveDictCopy(kv.second, dump);
        for (const auto& rkv : res.entries) {
            if (rkv.first != "XObject") continue;
            foundation::objects::Dict xo = ResolveDictCopy(rkv.second, dump);
            for (const auto& xkv : xo.entries) {
                const auto* r =
                    std::get_if<foundation::objects::Ref>(&xkv.second.v);
                if (r == nullptr) continue;
                const auto* obj = FindPageObj(dump, r->id);
                const auto* st =
                    obj ? std::get_if<foundation::objects::Stream>(
                              &obj->value.v)
                        : nullptr;
                if (st == nullptr) continue;
                for (const auto& h : st->header.entries) {
                    if (h.first == "Subtype") {
                        if (auto* n =
                                std::get_if<std::string>(&h.second.v))
                            if (*n == "Image")
                                out.emplace_back(xkv.first, r->id);
                    }
                }
            }
        }
    }
    return out;
}

// Decode raw PNG / JPEG image-file bytes into an image-XObject stream body,
// pixel dimensions, filter name and colour space. JPEG passes through as
// DCTDecode; PNG is composited over white to RGB and FlateDecode-encoded.
// Returns false on unsupported / malformed input.
bool DecodeImageForXObject(const std::vector<std::byte>& file_bytes, int& w,
                           int& h, std::vector<std::byte>& body,
                           const char*& filter, const char*& colorspace) {
    if (file_bytes.size() < 8) return false;
    auto u8 = [&](std::size_t i) {
        return static_cast<unsigned>(file_bytes[i]);
    };
    int comps = 3;
    if (u8(0) == 0xFF && u8(1) == 0xD8) {
        if (!JpegInfo(file_bytes, w, h, comps)) return false;
        body = file_bytes;  // DCTDecode passthrough
        filter = "DCTDecode";
        colorspace = comps == 1 ? "DeviceGray"
                                : (comps == 4 ? "DeviceCMYK" : "DeviceRGB");
        return true;
    }
    if (u8(0) == 0x89 && u8(1) == 0x50 && u8(2) == 0x4E && u8(3) == 0x47) {
        foundation::png_decoder::DecodedImage img;
        try {
            img = foundation::png_decoder::Decode(
                std::span<const std::byte>(file_bytes.data(),
                                           file_bytes.size()));
        } catch (const std::exception&) {
            return false;
        }
        w = static_cast<int>(img.width);
        h = static_cast<int>(img.height);
        // Composite RGBA over white → RGB.
        std::vector<std::byte> rgb;
        rgb.reserve(static_cast<std::size_t>(w) * h * 3);
        for (std::size_t p = 0; p + 3 < img.pixels.size(); p += 4) {
            const double a = static_cast<unsigned>(img.pixels[p + 3]) / 255.0;
            for (int c = 0; c < 3; ++c) {
                const double v = static_cast<unsigned>(img.pixels[p + c]);
                const double out = v * a + 255.0 * (1.0 - a);
                rgb.push_back(static_cast<std::byte>(
                    static_cast<unsigned char>(out + 0.5)));
            }
        }
        body = foundation::flate::Encode(
            std::span<const std::byte>(rgb.data(), rgb.size()));
        filter = "FlateDecode";
        colorspace = "DeviceRGB";
        return true;
    }
    return false;
}

}  // namespace

bool Document::AddImageToPage(std::size_t leaf, const std::string& imagePath,
                              double llx, double lly, double urx, double ury) {
    std::ifstream f(imagePath, std::ios::binary);
    if (!f) return false;
    const std::string raw((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
    std::vector<std::byte> file_bytes(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i)
        file_bytes[i] = static_cast<std::byte>(
            static_cast<unsigned char>(raw[i]));
    return AddImageBytesToPage(leaf, file_bytes, llx, lly, urx, ury);
}

bool Document::AddImageBytesToPage(std::size_t leaf,
                                   const std::vector<std::byte>& file_bytes,
                                   double llx, double lly, double urx,
                                   double ury, bool autoAdjust,
                                   const Aspose::Pdf::Rectangle* clipBbox) {
    if (!tree_ || leaf >= tree_->leaves.size()) return false;

    int w = 0, h = 0;
    std::vector<std::byte> body;
    const char* filter = nullptr;
    const char* colorspace = "DeviceRGB";
    if (!DecodeImageForXObject(file_bytes, w, h, body, filter, colorspace))
        return false;

    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    auto trailer_bundle = foundation::trailer::Parse(span);
    auto dump = foundation::objects::Parse(span);
    std::uint32_t maxid = 0;
    for (const auto& o : dump.objects) maxid = std::max(maxid, o.id);
    std::uint32_t next = std::max(trailer_bundle.size, maxid + 1);

    const std::uint32_t img_id = next++;
    const std::string img_name = "ImgEmb" + std::to_string(img_id);

    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;
    // Image XObject stream.
    {
        foundation::objects::Stream sv;
        sv.header.entries.emplace_back("Type", PageNameValue("XObject"));
        sv.header.entries.emplace_back("Subtype", PageNameValue("Image"));
        auto numEntry = [&](const char* k, int v) {
            foundation::objects::Value vv;
            vv.v = static_cast<std::int64_t>(v);
            sv.header.entries.emplace_back(k, std::move(vv));
        };
        numEntry("Width", w);
        numEntry("Height", h);
        sv.header.entries.emplace_back("ColorSpace",
                                       PageNameValue(colorspace));
        numEntry("BitsPerComponent", 8);
        sv.header.entries.emplace_back("Filter", PageNameValue(filter));
        sv.body = std::span<const std::byte>(body.data(), body.size());
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = img_id;
        d.generation = 0;
        d.value.v = std::move(sv);
        dirty.push_back(std::move(d));
    }

    // Draw rectangle: by default the full (llx,lly)-(urx,ury) box; when
    // autoAdjust is set, shrink to the image aspect ratio and centre.
    double dx = llx, dy = lly, dw = urx - llx, dh = ury - lly;
    if (autoAdjust && w > 0 && h > 0 && dw > 0.0 && dh > 0.0) {
        const double scale =
            std::min(dw / static_cast<double>(w), dh / static_cast<double>(h));
        const double sw = static_cast<double>(w) * scale;
        const double sh = static_cast<double>(h) * scale;
        dx = llx + (dw - sw) / 2.0;
        dy = lly + (dh - sh) / 2.0;
        dw = sw;
        dh = sh;
    }

    // Content stream: [bbox clip] q [W X Y re W n] W 0 0 H X Y cm /Img Do Q.
    const std::uint32_t content_id = next++;
    char op[320];
    if (clipBbox != nullptr && !clipBbox->IsEmpty()) {
        std::snprintf(
            op, sizeof(op),
            "q %.2f %.2f %.2f %.2f re W n %.2f 0 0 %.2f %.2f %.2f cm /%s Do Q\n",
            clipBbox->LLX(), clipBbox->LLY(), clipBbox->Width(),
            clipBbox->Height(), dw, dh, dx, dy, img_name.c_str());
    } else {
        std::snprintf(op, sizeof(op),
                      "q %.2f 0 0 %.2f %.2f %.2f cm /%s Do Q\n", dw, dh, dx,
                      dy, img_name.c_str());
    }
    std::vector<std::byte> content_bytes;
    for (const char* p = op; *p; ++p)
        content_bytes.push_back(static_cast<std::byte>(*p));
    {
        foundation::objects::Stream sv;
        sv.body = std::span<const std::byte>(content_bytes.data(),
                                             content_bytes.size());
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = content_id;
        d.generation = 0;
        d.value.v = std::move(sv);
        dirty.push_back(std::move(d));
    }

    // Update the page: /Contents append + /Resources /XObject merge.
    const std::uint32_t page_id = tree_->leaves[leaf].id;
    const auto* page_obj = FindPageObj(dump, page_id);
    const auto* page_dict =
        page_obj ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                 : nullptr;
    if (page_dict == nullptr) return false;
    foundation::objects::Dict updated = *page_dict;

    foundation::objects::Array contents;
    for (const auto& kv : page_dict->entries) {
        if (kv.first != "Contents") continue;
        if (auto* arr =
                std::get_if<foundation::objects::Array>(&kv.second.v))
            contents = *arr;
        else if (std::get_if<foundation::objects::Ref>(&kv.second.v))
            contents.items.push_back(kv.second);
    }
    {
        foundation::objects::Value cref;
        cref.v = foundation::objects::Ref{content_id, 0};
        contents.items.push_back(std::move(cref));
    }
    for (auto it = updated.entries.begin(); it != updated.entries.end();) {
        if (it->first == "Contents")
            it = updated.entries.erase(it);
        else
            ++it;
    }
    {
        foundation::objects::Value cv;
        cv.v = std::move(contents);
        updated.entries.emplace_back("Contents", std::move(cv));
    }

    foundation::objects::Dict resources;
    for (const auto& kv : page_dict->entries)
        if (kv.first == "Resources")
            resources = ResolveDictCopy(kv.second, dump);
    foundation::objects::Dict xobjects;
    for (const auto& kv : resources.entries)
        if (kv.first == "XObject")
            xobjects = ResolveDictCopy(kv.second, dump);
    {
        foundation::objects::Value iref;
        iref.v = foundation::objects::Ref{img_id, 0};
        xobjects.entries.emplace_back(img_name, std::move(iref));
    }
    for (auto it = resources.entries.begin();
         it != resources.entries.end();) {
        if (it->first == "XObject")
            it = resources.entries.erase(it);
        else
            ++it;
    }
    {
        foundation::objects::Value xv;
        xv.v = std::move(xobjects);
        resources.entries.emplace_back("XObject", std::move(xv));
    }
    for (auto it = updated.entries.begin(); it != updated.entries.end();) {
        if (it->first == "Resources")
            it = updated.entries.erase(it);
        else
            ++it;
    }
    {
        foundation::objects::Value rv;
        rv.v = std::move(resources);
        updated.entries.emplace_back("Resources", std::move(rv));
    }
    {
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = page_id;
        d.generation = page_obj->generation;
        d.value.v = std::move(updated);
        dirty.push_back(std::move(d));
    }

    bytes_ = foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(
            std::span<const std::byte>(bytes_.data(), bytes_.size())));
    // A content edit (text / image XObject) keeps the page-tree structure
    // unchanged — same leaf count, ids and order — so the staged, per-leaf
    // write state stays correctly indexed. Do NOT clear it here: clearing
    // would silently discard annotations or a SetPageSize staged on this or
    // any other page before this edit (e.g. drawing a footer after an
    // annotation was added drops the annotation).
    return true;
}

bool Document::AddTextToPage(std::size_t leaf, const std::string& text,
                             double x, double y, const std::string& fontName,
                             double fontSize,
                             const Aspose::Pdf::Color* color) {
    if (!tree_ || leaf >= tree_->leaves.size()) return false;

    const std::string base_font = fontName.empty() ? "Helvetica" : fontName;
    const double size = fontSize > 0.0 ? fontSize : 12.0;

    // Escape the show-string literal per PDF §7.3.4.2.
    std::string lit;
    lit.reserve(text.size() + 2);
    for (char c : text) {
        if (c == '(' || c == ')' || c == '\\') lit.push_back('\\');
        lit.push_back(c);
    }

    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    auto trailer_bundle = foundation::trailer::Parse(span);
    auto dump = foundation::objects::Parse(span);
    std::uint32_t maxid = 0;
    for (const auto& o : dump.objects) maxid = std::max(maxid, o.id);
    std::uint32_t next = std::max(trailer_bundle.size, maxid + 1);

    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;

    // Standard-14 Type1 font object.
    const std::uint32_t font_id = next++;
    const std::string font_name = "FEmb" + std::to_string(font_id);
    {
        foundation::objects::Dict fd;
        fd.entries.emplace_back("Type", PageNameValue("Font"));
        fd.entries.emplace_back("Subtype", PageNameValue("Type1"));
        fd.entries.emplace_back("BaseFont", PageNameValue(base_font.c_str()));
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = font_id;
        d.generation = 0;
        d.value.v = std::move(fd);
        dirty.push_back(std::move(d));
    }

    // Content stream: BT [r g b rg] /F size Tf x y Td (text) Tj ET. An opaque
    // authored colour (alpha > 0) is emitted as a non-stroking RGB fill.
    const std::uint32_t content_id = next++;
    std::string fill;
    if (color != nullptr && color->A() > 0.0) {
        char cop[64];
        std::snprintf(cop, sizeof(cop), "%.4f %.4f %.4f rg ", color->r_,
                      color->g_, color->b_);
        fill = cop;
    }
    char op[256];
    std::snprintf(op, sizeof(op), "\nBT %s/%s %.2f Tf %.2f %.2f Td (",
                  fill.c_str(), font_name.c_str(), size, x, y);
    std::string content(op);
    content += lit;
    content += ") Tj ET\n";
    std::vector<std::byte> content_bytes(content.size());
    for (std::size_t i = 0; i < content.size(); ++i)
        content_bytes[i] =
            static_cast<std::byte>(static_cast<unsigned char>(content[i]));
    {
        foundation::objects::Stream sv;
        sv.body = std::span<const std::byte>(content_bytes.data(),
                                             content_bytes.size());
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = content_id;
        d.generation = 0;
        d.value.v = std::move(sv);
        dirty.push_back(std::move(d));
    }

    // Update the page: /Contents append + /Resources /Font merge.
    const std::uint32_t page_id = tree_->leaves[leaf].id;
    const auto* page_obj = FindPageObj(dump, page_id);
    const auto* page_dict =
        page_obj ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                 : nullptr;
    if (page_dict == nullptr) return false;
    foundation::objects::Dict updated = *page_dict;

    foundation::objects::Array contents;
    for (const auto& kv : page_dict->entries) {
        if (kv.first != "Contents") continue;
        if (auto* arr = std::get_if<foundation::objects::Array>(&kv.second.v))
            contents = *arr;
        else if (std::get_if<foundation::objects::Ref>(&kv.second.v))
            contents.items.push_back(kv.second);
    }
    {
        foundation::objects::Value cref;
        cref.v = foundation::objects::Ref{content_id, 0};
        contents.items.push_back(std::move(cref));
    }
    for (auto it = updated.entries.begin(); it != updated.entries.end();) {
        if (it->first == "Contents")
            it = updated.entries.erase(it);
        else
            ++it;
    }
    {
        foundation::objects::Value cv;
        cv.v = std::move(contents);
        updated.entries.emplace_back("Contents", std::move(cv));
    }

    foundation::objects::Dict resources;
    for (const auto& kv : page_dict->entries)
        if (kv.first == "Resources")
            resources = ResolveDictCopy(kv.second, dump);
    foundation::objects::Dict fonts;
    for (const auto& kv : resources.entries)
        if (kv.first == "Font") fonts = ResolveDictCopy(kv.second, dump);
    {
        foundation::objects::Value fref;
        fref.v = foundation::objects::Ref{font_id, 0};
        fonts.entries.emplace_back(font_name, std::move(fref));
    }
    for (auto it = resources.entries.begin();
         it != resources.entries.end();) {
        if (it->first == "Font")
            it = resources.entries.erase(it);
        else
            ++it;
    }
    {
        foundation::objects::Value fv;
        fv.v = std::move(fonts);
        resources.entries.emplace_back("Font", std::move(fv));
    }
    for (auto it = updated.entries.begin(); it != updated.entries.end();) {
        if (it->first == "Resources")
            it = updated.entries.erase(it);
        else
            ++it;
    }
    {
        foundation::objects::Value rv;
        rv.v = std::move(resources);
        updated.entries.emplace_back("Resources", std::move(rv));
    }
    {
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = page_id;
        d.generation = page_obj->generation;
        d.value.v = std::move(updated);
        dirty.push_back(std::move(d));
    }

    bytes_ = foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(
            std::span<const std::byte>(bytes_.data(), bytes_.size())));
    // A content edit (text / image XObject) keeps the page-tree structure
    // unchanged — same leaf count, ids and order — so the staged, per-leaf
    // write state stays correctly indexed. Do NOT clear it here: clearing
    // would silently discard annotations or a SetPageSize staged on this or
    // any other page before this edit (e.g. drawing a footer after an
    // annotation was added drops the annotation).
    return true;
}

namespace {

// Subset a TrueType font program to the glyphs needed for `usedChars`,
// rewriting only the glyf/loca tables (used glyph outlines kept, all others
// emptied) and preserving every other table verbatim. Returns the original
// program unchanged if the font cannot be safely subset (graceful fallback —
// a full but valid embed beats a broken subset).
std::vector<std::byte> SubsetTrueTypeProgram(
        const std::vector<std::byte>& program,
        const std::set<std::uint32_t>& usedChars) {
    const auto* p = reinterpret_cast<const unsigned char*>(program.data());
    const std::size_t n = program.size();
    auto u16 = [&](std::size_t o) -> std::uint32_t {
        return o + 1 < n ? (static_cast<std::uint32_t>(p[o]) << 8) | p[o + 1]
                         : 0;
    };
    auto u32 = [&](std::size_t o) -> std::uint32_t {
        return o + 3 < n ? (static_cast<std::uint32_t>(p[o]) << 24) |
                               (static_cast<std::uint32_t>(p[o + 1]) << 16) |
                               (static_cast<std::uint32_t>(p[o + 2]) << 8) |
                               p[o + 3]
                         : 0;
    };
    if (n < 12) return program;
    const std::uint32_t num_tables = u16(4);
    struct Tab {
        std::uint32_t offset, length;
    };
    std::map<std::string, Tab> tabs;
    std::vector<std::string> order;
    for (std::uint32_t i = 0; i < num_tables; ++i) {
        const std::size_t rec = 12 + i * 16;
        if (rec + 16 > n) return program;
        std::string tag(reinterpret_cast<const char*>(p + rec), 4);
        tabs[tag] = {u32(rec + 8), u32(rec + 12)};
        order.push_back(tag);
    }
    if (!tabs.count("glyf") || !tabs.count("loca") || !tabs.count("head") ||
        !tabs.count("maxp"))
        return program;  // not a glyf-based TrueType (e.g. CFF/OTF)

    const std::uint32_t num_glyphs = u16(tabs["maxp"].offset + 4);
    const int loc_fmt = static_cast<std::int16_t>(u16(tabs["head"].offset + 50));
    const std::uint32_t loca_off = tabs["loca"].offset;
    const std::uint32_t glyf_off = tabs["glyf"].offset;
    if (num_glyphs == 0) return program;

    // Original loca → per-glyph [start,end) byte ranges in glyf.
    std::vector<std::uint32_t> loca(num_glyphs + 1);
    for (std::uint32_t i = 0; i <= num_glyphs; ++i)
        loca[i] = loc_fmt == 0 ? u16(loca_off + i * 2) * 2
                               : u32(loca_off + i * 4);

    // Used glyph ids: .notdef + the glyphs the used chars map to (via the
    // parsed cmap), expanded to include composite components.
    foundation::truetype::TrueType tt;
    try {
        tt = foundation::truetype::Parse(
            std::span<const std::byte>(program.data(), program.size()));
    } catch (const std::exception&) {
        return program;
    }
    std::set<std::uint32_t> used{0};
    for (const auto& e : tt.cmap)
        if (usedChars.count(e.char_code)) used.insert(e.glyph_id);

    auto glyphIsComposite = [&](std::uint32_t gid) {
        if (gid + 1 >= loca.size() || loca[gid + 1] <= loca[gid]) return false;
        return static_cast<std::int16_t>(u16(glyf_off + loca[gid])) < 0;
    };
    // Pull in composite component glyphs (one bounded pass is enough for the
    // fonts we embed; nested composites are rare).
    for (int pass = 0; pass < 4; ++pass) {
        std::set<std::uint32_t> add;
        for (std::uint32_t gid : used) {
            if (gid + 1 >= loca.size() || !glyphIsComposite(gid)) continue;
            std::size_t o = glyf_off + loca[gid] + 10;  // skip hdr + bbox
            for (int guard = 0; guard < 64; ++guard) {
                if (o + 4 > n) break;
                const std::uint32_t flags = u16(o);
                const std::uint32_t comp = u16(o + 2);
                if (comp < num_glyphs) add.insert(comp);
                o += 4;
                o += (flags & 0x0001) ? 4 : 2;        // ARG_1_AND_2_ARE_WORDS
                if (flags & 0x0008) o += 2;            // WE_HAVE_A_SCALE
                else if (flags & 0x0040) o += 4;       // X_AND_Y_SCALE
                else if (flags & 0x0080) o += 8;       // TWO_BY_TWO
                if (!(flags & 0x0020)) break;          // MORE_COMPONENTS
            }
        }
        const std::size_t before = used.size();
        used.insert(add.begin(), add.end());
        if (used.size() == before) break;
    }

    // New glyf (used outlines kept, unused emptied) + matching loca.
    std::vector<unsigned char> new_glyf;
    std::vector<std::uint32_t> new_loca(num_glyphs + 1);
    for (std::uint32_t gid = 0; gid < num_glyphs; ++gid) {
        new_loca[gid] = static_cast<std::uint32_t>(new_glyf.size());
        if (used.count(gid) && gid + 1 < loca.size() &&
            loca[gid + 1] > loca[gid] && glyf_off + loca[gid + 1] <= n) {
            new_glyf.insert(new_glyf.end(), p + glyf_off + loca[gid],
                            p + glyf_off + loca[gid + 1]);
            if (new_glyf.size() & 1) new_glyf.push_back(0);  // 2-byte align
        }
    }
    new_loca[num_glyphs] = static_cast<std::uint32_t>(new_glyf.size());

    // Encode the new loca per the original indexToLocFormat.
    std::vector<unsigned char> new_loca_bytes;
    auto pushU16 = [&](std::vector<unsigned char>& v, std::uint32_t x) {
        v.push_back(static_cast<unsigned char>(x >> 8));
        v.push_back(static_cast<unsigned char>(x));
    };
    auto pushU32 = [&](std::vector<unsigned char>& v, std::uint32_t x) {
        v.push_back(static_cast<unsigned char>(x >> 24));
        v.push_back(static_cast<unsigned char>(x >> 16));
        v.push_back(static_cast<unsigned char>(x >> 8));
        v.push_back(static_cast<unsigned char>(x));
    };
    for (std::uint32_t i = 0; i <= num_glyphs; ++i) {
        if (loc_fmt == 0)
            pushU16(new_loca_bytes, new_loca[i] / 2);
        else
            pushU32(new_loca_bytes, new_loca[i]);
    }

    // Re-lay-out all tables (original order), substituting glyf/loca.
    auto tableBytes = [&](const std::string& tag) -> std::vector<unsigned char> {
        if (tag == "glyf") return new_glyf;
        if (tag == "loca") return new_loca_bytes;
        const Tab& t = tabs[tag];
        if (t.offset + t.length > n) return {};
        return std::vector<unsigned char>(p + t.offset, p + t.offset + t.length);
    };
    auto checksum = [](const std::vector<unsigned char>& b) -> std::uint32_t {
        std::uint32_t sum = 0;
        for (std::size_t i = 0; i < b.size(); i += 4) {
            std::uint32_t w = 0;
            for (int k = 0; k < 4; ++k)
                w = (w << 8) | (i + k < b.size() ? b[i + k] : 0);
            sum += w;
        }
        return sum;
    };

    const std::size_t dir_size = 12 + num_tables * 16;
    std::vector<std::vector<unsigned char>> bodies;
    std::vector<std::uint32_t> offs(num_tables), lens(num_tables),
        sums(num_tables);
    std::size_t running = dir_size;
    std::size_t head_idx = 0;
    for (std::uint32_t i = 0; i < num_tables; ++i) {
        std::vector<unsigned char> b = tableBytes(order[i]);
        lens[i] = static_cast<std::uint32_t>(b.size());
        sums[i] = checksum(b);
        offs[i] = static_cast<std::uint32_t>(running);
        while (b.size() & 3) b.push_back(0);  // 4-byte align
        running += b.size();
        if (order[i] == "head") head_idx = i;
        bodies.push_back(std::move(b));
    }

    std::vector<unsigned char> out;
    out.reserve(running);
    // Header (copy sfnt version + table-count search fields from original).
    out.insert(out.end(), p, p + 12);
    for (std::uint32_t i = 0; i < num_tables; ++i) {
        const std::size_t rec = 12 + i * 16;
        out.insert(out.end(), p + rec, p + rec + 4);  // tag
        std::vector<unsigned char> tmp;
        pushU32(tmp, sums[i]);
        pushU32(tmp, offs[i]);
        pushU32(tmp, lens[i]);
        out.insert(out.end(), tmp.begin(), tmp.end());
    }
    for (auto& b : bodies) out.insert(out.end(), b.begin(), b.end());

    // head.checkSumAdjustment = 0xB1B0AFBA - checksum(whole file).
    const std::size_t cka = offs[head_idx] + 8;
    if (cka + 4 <= out.size()) {
        out[cka] = out[cka + 1] = out[cka + 2] = out[cka + 3] = 0;
        std::uint32_t file_sum = 0;
        for (std::size_t i = 0; i < out.size(); i += 4) {
            std::uint32_t w = 0;
            for (int k = 0; k < 4; ++k)
                w = (w << 8) | (i + k < out.size() ? out[i + k] : 0);
            file_sum += w;
        }
        const std::uint32_t adj = 0xB1B0AFBAu - file_sum;
        out[cka] = static_cast<unsigned char>(adj >> 24);
        out[cka + 1] = static_cast<unsigned char>(adj >> 16);
        out[cka + 2] = static_cast<unsigned char>(adj >> 8);
        out[cka + 3] = static_cast<unsigned char>(adj);
    }

    // Safety net: only return the subset if it still parses + actually shrank.
    if (out.size() >= program.size()) return program;
    std::vector<std::byte> result(out.size());
    for (std::size_t i = 0; i < out.size(); ++i)
        result[i] = static_cast<std::byte>(out[i]);
    try {
        foundation::truetype::Parse(
            std::span<const std::byte>(result.data(), result.size()));
    } catch (const std::exception&) {
        return program;  // subset somehow invalid — fall back to full embed
    }
    return result;
}

}  // namespace

bool Document::AddEmbeddedTextToPage(std::size_t leaf, const std::string& text,
                                     double x, double y,
                                     const std::vector<std::byte>& fontProgram,
                                     const std::string& baseFont,
                                     double fontSize, bool subset,
                                     const Aspose::Pdf::Color* color) {
    if (!tree_ || leaf >= tree_->leaves.size()) return false;
    if (fontProgram.empty())
        return AddTextToPage(leaf, text, x, y, baseFont, fontSize, color);

    // Subset the program to the glyphs this text needs (Font.IsSubset).
    std::vector<std::byte> program = fontProgram;
    std::string base = baseFont.empty() ? "EmbeddedFont" : baseFont;
    if (subset) {
        std::set<std::uint32_t> used;
        for (unsigned char c : text) used.insert(c);
        const std::vector<std::byte> sub = SubsetTrueTypeProgram(program, used);
        if (sub.size() < program.size()) {
            program = sub;
            // Canonical subset BaseFont tag: 6 uppercase letters + '+'.
            std::string tag;
            std::uint32_t h = 0x811c9dc5u;
            for (unsigned char c : text) h = (h ^ c) * 16777619u;
            for (int i = 0; i < 6; ++i) {
                tag.push_back(static_cast<char>('A' + (h % 26)));
                h /= 26;
            }
            base = tag + "+" + base;
        }
    }

    foundation::truetype::TrueType tt;
    try {
        tt = foundation::truetype::Parse(
            std::span<const std::byte>(program.data(), program.size()));
    } catch (const std::exception&) {
        return AddTextToPage(leaf, text, x, y, baseFont, fontSize, color);
    }
    const double size = fontSize > 0.0 ? fontSize : 12.0;
    const double upm = tt.units_per_em > 0 ? tt.units_per_em : 1000.0;
    const double scale = 1000.0 / upm;

    const int first_char = 32, last_char = 126;
    auto widthOf = [&](int code) -> int {
        for (const auto& e : tt.cmap) {
            if (static_cast<int>(e.char_code) == code &&
                e.glyph_id < tt.glyph_widths.size())
                return static_cast<int>(tt.glyph_widths[e.glyph_id] * scale +
                                        0.5);
        }
        return 500;
    };

    std::string lit;
    lit.reserve(text.size() + 2);
    for (char c : text) {
        if (c == '(' || c == ')' || c == '\\') lit.push_back('\\');
        lit.push_back(c);
    }

    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    auto trailer_bundle = foundation::trailer::Parse(span);
    auto dump = foundation::objects::Parse(span);
    std::uint32_t maxid = 0;
    for (const auto& o : dump.objects) maxid = std::max(maxid, o.id);
    std::uint32_t next = std::max(trailer_bundle.size, maxid + 1);

    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;
    std::vector<std::vector<std::byte>> bodies;

    auto numEntry = [](foundation::objects::Dict& d, const char* k, double v) {
        foundation::objects::Value vv;
        vv.v = static_cast<std::int64_t>(v + (v < 0 ? -0.5 : 0.5));
        d.entries.emplace_back(k, std::move(vv));
    };

    // FontFile2 — the raw TrueType program, FlateDecode-compressed.
    const std::uint32_t fontfile_id = next++;
    {
        bodies.push_back(foundation::flate::Encode(std::span<const std::byte>(
            program.data(), program.size())));
        foundation::objects::Stream sv;
        numEntry(sv.header, "Length1", static_cast<double>(program.size()));
        sv.header.entries.emplace_back("Filter", PageNameValue("FlateDecode"));
        sv.body = std::span<const std::byte>(bodies.back().data(),
                                             bodies.back().size());
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = fontfile_id;
        d.generation = 0;
        d.value.v = std::move(sv);
        dirty.push_back(std::move(d));
    }

    // FontDescriptor.
    const std::uint32_t desc_id = next++;
    {
        foundation::objects::Dict fd;
        fd.entries.emplace_back("Type", PageNameValue("FontDescriptor"));
        fd.entries.emplace_back("FontName", PageNameValue(base.c_str()));
        numEntry(fd, "Flags", 32);  // nonsymbolic
        {
            foundation::objects::Array bbox;
            for (double v : {0.0, tt.descent * scale, 1000.0,
                             tt.ascent * scale}) {
                foundation::objects::Value vv;
                vv.v = static_cast<std::int64_t>(v + (v < 0 ? -0.5 : 0.5));
                bbox.items.push_back(std::move(vv));
            }
            foundation::objects::Value bv;
            bv.v = std::move(bbox);
            fd.entries.emplace_back("FontBBox", std::move(bv));
        }
        numEntry(fd, "ItalicAngle", 0);
        numEntry(fd, "Ascent", tt.ascent * scale);
        numEntry(fd, "Descent", tt.descent * scale);
        numEntry(fd, "CapHeight", tt.ascent * scale);
        numEntry(fd, "StemV", 80);
        {
            foundation::objects::Value ff;
            ff.v = foundation::objects::Ref{fontfile_id, 0};
            fd.entries.emplace_back("FontFile2", std::move(ff));
        }
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = desc_id;
        d.generation = 0;
        d.value.v = std::move(fd);
        dirty.push_back(std::move(d));
    }

    // TrueType font dict.
    const std::uint32_t font_id = next++;
    const std::string font_name = "FEmb" + std::to_string(font_id);
    {
        foundation::objects::Dict fdict;
        fdict.entries.emplace_back("Type", PageNameValue("Font"));
        fdict.entries.emplace_back("Subtype", PageNameValue("TrueType"));
        fdict.entries.emplace_back("BaseFont", PageNameValue(base.c_str()));
        fdict.entries.emplace_back("Encoding",
                                   PageNameValue("WinAnsiEncoding"));
        numEntry(fdict, "FirstChar", first_char);
        numEntry(fdict, "LastChar", last_char);
        {
            foundation::objects::Array widths;
            for (int c = first_char; c <= last_char; ++c) {
                foundation::objects::Value vv;
                vv.v = static_cast<std::int64_t>(widthOf(c));
                widths.items.push_back(std::move(vv));
            }
            foundation::objects::Value wv;
            wv.v = std::move(widths);
            fdict.entries.emplace_back("Widths", std::move(wv));
        }
        {
            foundation::objects::Value dr;
            dr.v = foundation::objects::Ref{desc_id, 0};
            fdict.entries.emplace_back("FontDescriptor", std::move(dr));
        }
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = font_id;
        d.generation = 0;
        d.value.v = std::move(fdict);
        dirty.push_back(std::move(d));
    }

    // Content stream (with an optional non-stroking RGB fill for the text).
    const std::uint32_t content_id = next++;
    std::string fill;
    if (color != nullptr && color->A() > 0.0) {
        char cop[64];
        std::snprintf(cop, sizeof(cop), "%.4f %.4f %.4f rg ", color->r_,
                      color->g_, color->b_);
        fill = cop;
    }
    char op[256];
    std::snprintf(op, sizeof(op), "\nBT %s/%s %.2f Tf %.2f %.2f Td (",
                  fill.c_str(), font_name.c_str(), size, x, y);
    std::string content(op);
    content += lit;
    content += ") Tj ET\n";
    {
        bodies.emplace_back(content.size());
        for (std::size_t i = 0; i < content.size(); ++i)
            bodies.back()[i] =
                static_cast<std::byte>(static_cast<unsigned char>(content[i]));
        foundation::objects::Stream sv;
        sv.body = std::span<const std::byte>(bodies.back().data(),
                                             bodies.back().size());
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = content_id;
        d.generation = 0;
        d.value.v = std::move(sv);
        dirty.push_back(std::move(d));
    }

    const std::uint32_t page_id = tree_->leaves[leaf].id;
    const auto* page_obj = FindPageObj(dump, page_id);
    const auto* page_dict =
        page_obj ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                 : nullptr;
    if (page_dict == nullptr) return false;
    foundation::objects::Dict updated = *page_dict;

    foundation::objects::Array contents;
    for (const auto& kv : page_dict->entries) {
        if (kv.first != "Contents") continue;
        if (auto* arr = std::get_if<foundation::objects::Array>(&kv.second.v))
            contents = *arr;
        else if (std::get_if<foundation::objects::Ref>(&kv.second.v))
            contents.items.push_back(kv.second);
    }
    contents.items.push_back(
        foundation::objects::Value{foundation::objects::Ref{content_id, 0}});
    for (auto it = updated.entries.begin(); it != updated.entries.end();) {
        if (it->first == "Contents")
            it = updated.entries.erase(it);
        else
            ++it;
    }
    updated.entries.emplace_back(
        "Contents", foundation::objects::Value{std::move(contents)});

    foundation::objects::Dict resources;
    for (const auto& kv : page_dict->entries)
        if (kv.first == "Resources")
            resources = ResolveDictCopy(kv.second, dump);
    foundation::objects::Dict fonts;
    for (const auto& kv : resources.entries)
        if (kv.first == "Font") fonts = ResolveDictCopy(kv.second, dump);
    fonts.entries.emplace_back(
        font_name,
        foundation::objects::Value{foundation::objects::Ref{font_id, 0}});
    for (auto it = resources.entries.begin(); it != resources.entries.end();) {
        if (it->first == "Font")
            it = resources.entries.erase(it);
        else
            ++it;
    }
    resources.entries.emplace_back(
        "Font", foundation::objects::Value{std::move(fonts)});
    for (auto it = updated.entries.begin(); it != updated.entries.end();) {
        if (it->first == "Resources")
            it = updated.entries.erase(it);
        else
            ++it;
    }
    updated.entries.emplace_back(
        "Resources", foundation::objects::Value{std::move(resources)});
    {
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = page_id;
        d.generation = page_obj->generation;
        d.value.v = std::move(updated);
        dirty.push_back(std::move(d));
    }

    bytes_ = foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(
            std::span<const std::byte>(bytes_.data(), bytes_.size())));
    // A content edit (text / image XObject) keeps the page-tree structure
    // unchanged — same leaf count, ids and order — so the staged, per-leaf
    // write state stays correctly indexed. Do NOT clear it here: clearing
    // would silently discard annotations or a SetPageSize staged on this or
    // any other page before this edit (e.g. drawing a footer after an
    // annotation was added drops the annotation).
    return true;
}

int Document::CountImages(int pageNumber1BasedOr0) const {
    if (!tree_) return 0;
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return 0;
    }
    std::vector<std::size_t> leaves;
    if (pageNumber1BasedOr0 > 0) {
        if (static_cast<std::size_t>(pageNumber1BasedOr0) <=
            tree_->leaves.size())
            leaves.push_back(
                static_cast<std::size_t>(pageNumber1BasedOr0 - 1));
    } else {
        for (std::size_t i = 0; i < tree_->leaves.size(); ++i)
            leaves.push_back(i);
    }
    int n = 0;
    for (std::size_t leaf : leaves) {
        const auto* po = FindPageObj(dump, tree_->leaves[leaf].id);
        const auto* pd =
            po ? std::get_if<foundation::objects::Dict>(&po->value.v)
               : nullptr;
        if (pd) n += static_cast<int>(PageImageXObjects(dump, *pd).size());
    }
    return n;
}

void Document::LoadPageImages(std::size_t leaf, XImageCollection& coll) const {
    if (!tree_ || leaf >= tree_->leaves.size()) return;
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return;
    }
    const std::uint32_t page_id = tree_->leaves[leaf].id;
    const auto* page_obj = FindPageObj(dump, page_id);
    const auto* page_dict =
        page_obj ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                 : nullptr;
    if (page_dict == nullptr) return;

    for (const auto& [name, sid] : PageImageXObjects(dump, *page_dict)) {
        const auto* obj = FindPageObj(dump, sid);
        const auto* st =
            obj ? std::get_if<foundation::objects::Stream>(&obj->value.v)
                : nullptr;
        if (st == nullptr) continue;
        auto img = std::make_unique<XImage>();
        img->name_ = name;
        for (const auto& kv : st->header.entries) {
            if (kv.first == "Width") {
                if (const auto* v = std::get_if<std::int64_t>(&kv.second.v))
                    img->width_ = static_cast<int>(*v);
            } else if (kv.first == "Height") {
                if (const auto* v = std::get_if<std::int64_t>(&kv.second.v))
                    img->height_ = static_cast<int>(*v);
            } else if (kv.first == "ImageMask") {
                if (const auto* b = std::get_if<bool>(&kv.second.v))
                    img->image_mask_ = *b;
            } else if (kv.first == "SMask") {
                img->contains_transparency_ = true;
            }
        }
        img->data_.assign(st->body.begin(), st->body.end());
        coll.images_.push_back(std::move(img));
    }
    coll.loaded_names_.clear();
    coll.loaded_names_.reserve(coll.images_.size());
    for (const auto& im : coll.images_) coll.loaded_names_.push_back(im->Name());
}

int Document::DeleteImagesFromPage(
        int pageNumber1BasedOr0,
        const std::vector<int>& imageNumbers1Based) {
    if (!tree_) return 0;
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return 0;
    }

    std::vector<std::size_t> leaves;
    if (pageNumber1BasedOr0 > 0) {
        if (static_cast<std::size_t>(pageNumber1BasedOr0) <=
            tree_->leaves.size())
            leaves.push_back(
                static_cast<std::size_t>(pageNumber1BasedOr0 - 1));
    } else {
        for (std::size_t i = 0; i < tree_->leaves.size(); ++i)
            leaves.push_back(i);
    }

    int removed = 0;
    std::vector<std::vector<std::byte>> bodies;  // keep stream bodies alive
    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;

    for (std::size_t leaf : leaves) {
        const std::uint32_t page_id = tree_->leaves[leaf].id;
        const auto* page_obj = FindPageObj(dump, page_id);
        const auto* page_dict =
            page_obj
                ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                : nullptr;
        if (page_dict == nullptr) continue;

        const auto imgs = PageImageXObjects(dump, *page_dict);
        if (imgs.empty()) continue;

        // Resolve the per-page 1-based selection to XObject names.
        std::set<std::string> names;
        if (imageNumbers1Based.empty()) {
            for (const auto& [name, sid] : imgs) {
                (void)sid;
                names.insert(name);
            }
        } else {
            for (int idx : imageNumbers1Based) {
                if (idx >= 1 && static_cast<std::size_t>(idx) <= imgs.size())
                    names.insert(imgs[static_cast<std::size_t>(idx - 1)].first);
            }
        }
        if (names.empty()) continue;

        foundation::objects::Dict updated = *page_dict;

        // Remove the named entries from /Resources /XObject (re-emit inline).
        foundation::objects::Dict resources;
        for (const auto& kv : page_dict->entries)
            if (kv.first == "Resources")
                resources = ResolveDictCopy(kv.second, dump);
        foundation::objects::Dict xobjects;
        for (const auto& kv : resources.entries)
            if (kv.first == "XObject")
                xobjects = ResolveDictCopy(kv.second, dump);
        for (auto it = xobjects.entries.begin();
             it != xobjects.entries.end();) {
            if (names.count(it->first)) {
                it = xobjects.entries.erase(it);
                ++removed;
            } else {
                ++it;
            }
        }
        for (auto it = resources.entries.begin();
             it != resources.entries.end();) {
            if (it->first == "XObject")
                it = resources.entries.erase(it);
            else
                ++it;
        }
        {
            foundation::objects::Value xv;
            xv.v = std::move(xobjects);
            resources.entries.emplace_back("XObject", std::move(xv));
        }
        for (auto it = updated.entries.begin(); it != updated.entries.end();) {
            if (it->first == "Resources")
                it = updated.entries.erase(it);
            else
                ++it;
        }
        {
            foundation::objects::Value rv;
            rv.v = std::move(resources);
            updated.entries.emplace_back("Resources", std::move(rv));
        }

        // Strip each `/<name> Do` draw operator from the page content
        // streams so nothing references the removed XObject.
        std::vector<std::uint32_t> stream_ids;
        for (const auto& kv : page_dict->entries) {
            if (kv.first != "Contents") continue;
            if (auto* r = std::get_if<foundation::objects::Ref>(&kv.second.v))
                stream_ids.push_back(r->id);
            else if (auto* a = std::get_if<foundation::objects::Array>(
                         &kv.second.v))
                for (const auto& it : a->items)
                    if (auto* rr =
                            std::get_if<foundation::objects::Ref>(&it.v))
                        stream_ids.push_back(rr->id);
        }
        for (std::uint32_t sid : stream_ids) {
            const auto* obj = FindPageObj(dump, sid);
            const auto* stream =
                obj ? std::get_if<foundation::objects::Stream>(&obj->value.v)
                    : nullptr;
            if (stream == nullptr) continue;
            std::vector<std::byte> decoded;
            const bool flate = StreamIsFlate(stream->header);
            if (flate) {
                try {
                    decoded = foundation::flate::Decode(stream->body);
                } catch (const std::exception&) {
                    continue;
                }
            } else {
                decoded.assign(stream->body.begin(), stream->body.end());
            }
            int edits = 0;
            for (const auto& name : names)
                edits += ReplaceAllBytes(decoded, "/" + name + " Do", "");
            if (edits == 0) continue;
            foundation::objects::Dict header = stream->header;
            for (auto it = header.entries.begin();
                 it != header.entries.end();) {
                if (it->first == "Filter" || it->first == "DecodeParms" ||
                    it->first == "Length")
                    it = header.entries.erase(it);
                else
                    ++it;
            }
            bodies.push_back(std::move(decoded));
            foundation::objects::Stream sv;
            sv.header = std::move(header);
            sv.body = std::span<const std::byte>(bodies.back().data(),
                                                 bodies.back().size());
            foundation::pdf_writer_incremental::DirtyObject d;
            d.id = sid;
            d.generation = obj->generation;
            d.value.v = std::move(sv);
            dirty.push_back(std::move(d));
        }

        foundation::pdf_writer_incremental::DirtyObject pd_dirty;
        pd_dirty.id = page_id;
        pd_dirty.generation = page_obj->generation;
        pd_dirty.value.v = std::move(updated);
        dirty.push_back(std::move(pd_dirty));
    }

    if (dirty.empty()) return 0;
    bytes_ = foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(
            std::span<const std::byte>(bytes_.data(), bytes_.size())));
    page_annotations_.clear();
    pending_mediabox_.clear();
    pending_cropbox_.clear();
    pending_rotation_.clear();
    page_geom_dirty_ = false;
    return removed;
}

std::vector<std::byte> Document::AppendImagesUpdate(
        const std::vector<std::byte>& working) const {
    if (!tree_) return working;

    auto span = std::span<const std::byte>(working.data(), working.size());
    foundation::trailer::Trailer tb;
    foundation::objects::Dump dump;
    try {
        tb = foundation::trailer::Parse(span);
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return working;
    }
    std::uint32_t maxid = 0;
    for (const auto& o : dump.objects) maxid = std::max(maxid, o.id);
    std::uint32_t next = std::max(tb.size, maxid + 1);

    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;
    std::vector<std::vector<std::byte>> bodies;  // keep stream bodies alive

    for (std::size_t leaf = 0; leaf < page_resources_.size(); ++leaf) {
        const auto& slot = page_resources_[leaf];
        if (!slot || leaf >= tree_->leaves.size()) continue;
        const XImageCollection& coll = slot->images_;

        // Partition the live collection into surviving loaded images, newly
        // staged ones and replaced ones, then derive deletions from the load
        // set.
        std::set<std::string> surviving;
        std::vector<const XImage*> new_images;
        std::vector<const XImage*> replaced_images;
        for (const auto& im : coll.images_) {
            if (im->is_new_) {
                new_images.push_back(im.get());
            } else {
                surviving.insert(im->Name());
                if (im->is_replaced_) replaced_images.push_back(im.get());
            }
        }
        std::set<std::string> deleted;
        for (const auto& nm : coll.loaded_names_)
            if (!surviving.count(nm)) deleted.insert(nm);
        if (deleted.empty() && new_images.empty() && replaced_images.empty())
            continue;

        const std::uint32_t page_id = tree_->leaves[leaf].id;
        const auto* page_obj = FindPageObj(dump, page_id);
        const auto* page_dict =
            page_obj
                ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                : nullptr;
        if (page_dict == nullptr) continue;
        foundation::objects::Dict updated = *page_dict;

        // Build the page's /Resources /XObject dict: start from the existing
        // entries, drop the deleted names, append the new image XObjects.
        foundation::objects::Dict resources;
        for (const auto& kv : page_dict->entries)
            if (kv.first == "Resources")
                resources = ResolveDictCopy(kv.second, dump);
        foundation::objects::Dict xobjects;
        for (const auto& kv : resources.entries)
            if (kv.first == "XObject")
                xobjects = ResolveDictCopy(kv.second, dump);

        for (auto it = xobjects.entries.begin();
             it != xobjects.entries.end();) {
            if (deleted.count(it->first))
                it = xobjects.entries.erase(it);
            else
                ++it;
        }

        // Embed `bytes` as an image XObject; returns its object id, or 0 on
        // an undecodable image.
        auto embedImage = [&](const std::vector<std::byte>& bytes)
            -> std::uint32_t {
            int w = 0, h = 0;
            std::vector<std::byte> body;
            const char* filter = nullptr;
            const char* colorspace = "DeviceRGB";
            if (!DecodeImageForXObject(bytes, w, h, body, filter, colorspace))
                return 0;
            const std::uint32_t img_id = next++;
            bodies.push_back(std::move(body));
            foundation::objects::Stream sv;
            sv.header.entries.emplace_back("Type", PageNameValue("XObject"));
            sv.header.entries.emplace_back("Subtype", PageNameValue("Image"));
            auto numEntry = [&](const char* k, int v) {
                foundation::objects::Value vv;
                vv.v = static_cast<std::int64_t>(v);
                sv.header.entries.emplace_back(k, std::move(vv));
            };
            numEntry("Width", w);
            numEntry("Height", h);
            sv.header.entries.emplace_back("ColorSpace",
                                           PageNameValue(colorspace));
            numEntry("BitsPerComponent", 8);
            sv.header.entries.emplace_back("Filter", PageNameValue(filter));
            sv.body = std::span<const std::byte>(bodies.back().data(),
                                                 bodies.back().size());
            foundation::pdf_writer_incremental::DirtyObject d;
            d.id = img_id;
            d.generation = 0;
            d.value.v = std::move(sv);
            dirty.push_back(std::move(d));
            return img_id;
        };

        for (const XImage* im : new_images) {
            const std::uint32_t img_id = embedImage(im->data_);
            if (img_id == 0) continue;
            foundation::objects::Value iref;
            iref.v = foundation::objects::Ref{img_id, 0};
            xobjects.entries.emplace_back(im->Name(), std::move(iref));
        }

        // Replaced images: re-embed under the same resource name so existing
        // `/<name> Do` references render the new content.
        for (const XImage* im : replaced_images) {
            const std::uint32_t img_id = embedImage(im->data_);
            if (img_id == 0) continue;
            for (auto it = xobjects.entries.begin();
                 it != xobjects.entries.end();) {
                if (it->first == im->Name())
                    it = xobjects.entries.erase(it);
                else
                    ++it;
            }
            foundation::objects::Value iref;
            iref.v = foundation::objects::Ref{img_id, 0};
            xobjects.entries.emplace_back(im->Name(), std::move(iref));
        }

        for (auto it = resources.entries.begin();
             it != resources.entries.end();) {
            if (it->first == "XObject")
                it = resources.entries.erase(it);
            else
                ++it;
        }
        {
            foundation::objects::Value xv;
            xv.v = std::move(xobjects);
            resources.entries.emplace_back("XObject", std::move(xv));
        }
        for (auto it = updated.entries.begin(); it != updated.entries.end();) {
            if (it->first == "Resources")
                it = updated.entries.erase(it);
            else
                ++it;
        }
        {
            foundation::objects::Value rv;
            rv.v = std::move(resources);
            updated.entries.emplace_back("Resources", std::move(rv));
        }

        // For deletions, strip each `/<name> Do` draw operator from the page
        // content streams so nothing references the removed XObject.
        if (!deleted.empty()) {
            std::vector<std::uint32_t> stream_ids;
            for (const auto& kv : page_dict->entries) {
                if (kv.first != "Contents") continue;
                if (auto* r =
                        std::get_if<foundation::objects::Ref>(&kv.second.v))
                    stream_ids.push_back(r->id);
                else if (auto* a = std::get_if<foundation::objects::Array>(
                             &kv.second.v))
                    for (const auto& it : a->items)
                        if (auto* rr =
                                std::get_if<foundation::objects::Ref>(&it.v))
                            stream_ids.push_back(rr->id);
            }
            for (std::uint32_t sid : stream_ids) {
                const auto* obj = FindPageObj(dump, sid);
                const auto* stream =
                    obj ? std::get_if<foundation::objects::Stream>(
                              &obj->value.v)
                        : nullptr;
                if (stream == nullptr) continue;
                std::vector<std::byte> decoded;
                if (StreamIsFlate(stream->header)) {
                    try {
                        decoded = foundation::flate::Decode(stream->body);
                    } catch (const std::exception&) {
                        continue;
                    }
                } else {
                    decoded.assign(stream->body.begin(), stream->body.end());
                }
                int edits = 0;
                for (const auto& name : deleted)
                    edits += ReplaceAllBytes(decoded, "/" + name + " Do", "");
                if (edits == 0) continue;
                foundation::objects::Dict header = stream->header;
                for (auto it = header.entries.begin();
                     it != header.entries.end();) {
                    if (it->first == "Filter" || it->first == "DecodeParms" ||
                        it->first == "Length")
                        it = header.entries.erase(it);
                    else
                        ++it;
                }
                bodies.push_back(std::move(decoded));
                foundation::objects::Stream sv;
                sv.header = std::move(header);
                sv.body = std::span<const std::byte>(bodies.back().data(),
                                                     bodies.back().size());
                foundation::pdf_writer_incremental::DirtyObject d;
                d.id = sid;
                d.generation = obj->generation;
                d.value.v = std::move(sv);
                dirty.push_back(std::move(d));
            }
        }

        foundation::pdf_writer_incremental::DirtyObject pd_dirty;
        pd_dirty.id = page_id;
        pd_dirty.generation = page_obj->generation;
        pd_dirty.value.v = std::move(updated);
        dirty.push_back(std::move(pd_dirty));
    }

    if (dirty.empty()) return working;
    return foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
}

namespace {

// Escape a PDF literal-string body per §7.3.4.2.
std::string EscapePdfLiteral(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        if (c == '(' || c == ')' || c == '\\') out.push_back('\\');
        out.push_back(c);
    }
    return out;
}

}  // namespace

std::vector<std::byte> Document::AppendArtifactsUpdate(
        const std::vector<std::byte>& working) const {
    bool any = false;
    for (const auto& slot : page_artifacts_)
        if (slot && slot->Count() > 0) any = true;
    if (!any || !tree_) return working;

    auto span = std::span<const std::byte>(working.data(), working.size());
    foundation::trailer::Trailer tb;
    foundation::objects::Dump dump;
    try {
        tb = foundation::trailer::Parse(span);
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return working;
    }
    std::uint32_t maxid = 0;
    for (const auto& o : dump.objects) maxid = std::max(maxid, o.id);
    std::uint32_t next = std::max(tb.size, maxid + 1);

    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;
    std::vector<std::vector<std::byte>> bodies;  // keep stream bodies alive

    // Shared Helvetica font for text watermarks (/AspHelv).
    const std::uint32_t font_id = next++;
    {
        foundation::objects::Dict font;
        font.entries.emplace_back("Type", PageNameValue("Font"));
        font.entries.emplace_back("Subtype", PageNameValue("Type1"));
        font.entries.emplace_back("BaseFont", PageNameValue("Helvetica"));
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = font_id;
        d.generation = 0;
        d.value.v = std::move(font);
        dirty.push_back(std::move(d));
    }

    for (std::size_t leaf = 0; leaf < page_artifacts_.size(); ++leaf) {
        const auto& slot = page_artifacts_[leaf];
        if (!slot || slot->Count() == 0) continue;
        if (leaf >= tree_->leaves.size()) continue;
        const std::uint32_t page_id = tree_->leaves[leaf].id;
        const auto* page_obj = FindPageObj(dump, page_id);
        const auto* page_dict =
            page_obj ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                     : nullptr;
        if (page_dict == nullptr) continue;

        const Rectangle page_rect = GetPageRectInternal(leaf);
        const double pw = page_rect.Width();
        const double ph = page_rect.Height();

        std::string fg, bg;  // foreground / background content operators
        std::vector<std::pair<std::string, std::uint32_t>> gs_refs;
        std::vector<std::pair<std::string, std::uint32_t>> xobj_refs;
        bool uses_font = false;
        int counter = 0;

        for (int i = 1; i <= slot->Count(); ++i) {
            const Artifact& a = (*slot)[i];

            // Decode an image artifact up front (skip on failure).
            int iw = 0, ih = 0;
            std::vector<std::byte> ibody;
            const char* ifilter = nullptr;
            const char* icolor = "DeviceRGB";
            std::string img_name;
            if (a.has_image_) {
                if (!DecodeImageForXObject(a.image_, iw, ih, ibody, ifilter,
                                           icolor))
                    continue;
            } else if (a.text_.empty()) {
                continue;  // nothing to draw
            }

            std::string& target = a.is_background_ ? bg : fg;

            std::string gs_name;
            if (a.opacity_ < 1.0) {
                gs_name = "GSwm" + std::to_string(counter);
                const std::uint32_t gs_id = next++;
                foundation::objects::Dict gs;
                gs.entries.emplace_back("Type", PageNameValue("ExtGState"));
                auto alpha = [&](const char* k) {
                    foundation::objects::Value vv;
                    vv.v = a.opacity_;
                    gs.entries.emplace_back(k, std::move(vv));
                };
                alpha("ca");
                alpha("CA");
                foundation::pdf_writer_incremental::DirtyObject d;
                d.id = gs_id;
                d.generation = 0;
                d.value.v = std::move(gs);
                dirty.push_back(std::move(d));
                gs_refs.emplace_back(gs_name, gs_id);
            }

            // Placement: honour Position, defaulting an unset (0,0) to centre.
            double px = a.position_.X();
            double py = a.position_.Y();
            if (px == 0.0 && py == 0.0) {
                px = pw * 0.5;
                py = ph * 0.5;
            }
            const double th = a.rotation_ * 3.14159265358979323846 / 180.0;
            const double c = std::cos(th);
            const double s = std::sin(th);

            char buf[256];
            target += "q\n";
            if (!gs_name.empty()) target += "/" + gs_name + " gs\n";
            std::snprintf(buf, sizeof(buf), "%.4f %.4f %.4f %.4f %.2f %.2f cm\n",
                          c, s, -s, c, px, py);
            target += buf;

            if (a.has_image_) {
                img_name = "XImgWm" + std::to_string(counter);
                const std::uint32_t img_id = next++;
                foundation::objects::Stream sv;
                sv.header.entries.emplace_back("Type",
                                               PageNameValue("XObject"));
                sv.header.entries.emplace_back("Subtype",
                                               PageNameValue("Image"));
                auto numEntry = [&](const char* k, int v) {
                    foundation::objects::Value vv;
                    vv.v = static_cast<std::int64_t>(v);
                    sv.header.entries.emplace_back(k, std::move(vv));
                };
                numEntry("Width", iw);
                numEntry("Height", ih);
                sv.header.entries.emplace_back("ColorSpace",
                                               PageNameValue(icolor));
                numEntry("BitsPerComponent", 8);
                sv.header.entries.emplace_back("Filter", PageNameValue(ifilter));
                bodies.push_back(std::move(ibody));
                sv.body = std::span<const std::byte>(bodies.back().data(),
                                                     bodies.back().size());
                foundation::pdf_writer_incremental::DirtyObject d;
                d.id = img_id;
                d.generation = 0;
                d.value.v = std::move(sv);
                dirty.push_back(std::move(d));
                xobj_refs.emplace_back(img_name, img_id);
                std::snprintf(buf, sizeof(buf), "%d 0 0 %d 0 0 cm\n", iw, ih);
                target += buf;
                target += "/" + img_name + " Do\n";
            } else {
                double size = a.text_state_.FontSize();
                if (size <= 0.0) size = 24.0;
                std::snprintf(buf, sizeof(buf), "BT /AspHelv %.2f Tf 0 0 Td\n",
                              size);
                target += buf;
                target += "(" + EscapePdfLiteral(a.text_) + ") Tj ET\n";
                uses_font = true;
            }
            target += "Q\n";
            ++counter;
        }

        if (fg.empty() && bg.empty()) continue;

        // Emit the background and foreground content streams.
        std::uint32_t bg_id = 0, fg_id = 0;
        auto emit_stream = [&](const std::string& ops) -> std::uint32_t {
            std::vector<std::byte> b;
            b.reserve(ops.size());
            for (char ch : ops) b.push_back(static_cast<std::byte>(ch));
            bodies.push_back(std::move(b));
            const std::uint32_t id = next++;
            foundation::objects::Stream sv;
            sv.body = std::span<const std::byte>(bodies.back().data(),
                                                 bodies.back().size());
            foundation::pdf_writer_incremental::DirtyObject d;
            d.id = id;
            d.generation = 0;
            d.value.v = std::move(sv);
            dirty.push_back(std::move(d));
            return id;
        };
        if (!bg.empty()) bg_id = emit_stream(bg);
        if (!fg.empty()) fg_id = emit_stream(fg);

        // Rebuild /Contents: background stream first, existing content, then
        // foreground stream.
        foundation::objects::Dict updated = *page_dict;
        foundation::objects::Array contents;
        if (bg_id != 0)
            contents.items.push_back(foundation::objects::Value{
                foundation::objects::Ref{bg_id, 0}});
        const foundation::objects::Value* existing = nullptr;
        for (const auto& kv : page_dict->entries)
            if (kv.first == "Contents") existing = &kv.second;
        if (existing != nullptr) {
            if (auto* arr =
                    std::get_if<foundation::objects::Array>(&existing->v))
                for (const auto& it : arr->items) contents.items.push_back(it);
            else if (std::get_if<foundation::objects::Ref>(&existing->v))
                contents.items.push_back(*existing);
        }
        if (fg_id != 0)
            contents.items.push_back(foundation::objects::Value{
                foundation::objects::Ref{fg_id, 0}});
        for (auto it = updated.entries.begin(); it != updated.entries.end();) {
            if (it->first == "Contents")
                it = updated.entries.erase(it);
            else
                ++it;
        }
        updated.entries.emplace_back(
            "Contents", foundation::objects::Value{std::move(contents)});

        // Merge /Font, /ExtGState and /XObject into the page resources.
        foundation::objects::Dict resources;
        for (const auto& kv : page_dict->entries)
            if (kv.first == "Resources")
                resources = ResolveDictCopy(kv.second, dump);

        auto merge_sub = [&](const char* key,
                             const std::vector<std::pair<std::string,
                                                         std::uint32_t>>& refs) {
            if (refs.empty()) return;
            foundation::objects::Dict sub;
            for (const auto& kv : resources.entries)
                if (kv.first == key) sub = ResolveDictCopy(kv.second, dump);
            for (const auto& [name, id] : refs) {
                for (auto it = sub.entries.begin(); it != sub.entries.end();) {
                    if (it->first == name)
                        it = sub.entries.erase(it);
                    else
                        ++it;
                }
                sub.entries.emplace_back(
                    name, foundation::objects::Value{
                              foundation::objects::Ref{id, 0}});
            }
            for (auto it = resources.entries.begin();
                 it != resources.entries.end();) {
                if (it->first == key)
                    it = resources.entries.erase(it);
                else
                    ++it;
            }
            resources.entries.emplace_back(
                key, foundation::objects::Value{std::move(sub)});
        };

        if (uses_font)
            merge_sub("Font", {{"AspHelv", font_id}});
        merge_sub("ExtGState", gs_refs);
        merge_sub("XObject", xobj_refs);

        for (auto it = updated.entries.begin(); it != updated.entries.end();) {
            if (it->first == "Resources")
                it = updated.entries.erase(it);
            else
                ++it;
        }
        updated.entries.emplace_back(
            "Resources", foundation::objects::Value{std::move(resources)});

        foundation::pdf_writer_incremental::DirtyObject pd_dirty;
        pd_dirty.id = page_id;
        pd_dirty.generation = page_obj->generation;
        pd_dirty.value.v = std::move(updated);
        dirty.push_back(std::move(pd_dirty));
    }

    if (dirty.size() <= 1) return working;  // only the shared font → no-op
    return foundation::pdf_writer_incremental::AppendIncremental(
        span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
}

bool Document::ExtractImageToFile(int globalIndex,
                                  const std::string& path) const {
    if (!tree_ || globalIndex < 0) return false;
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return false;
    }
    int idx = 0;
    for (std::size_t leaf = 0; leaf < tree_->leaves.size(); ++leaf) {
        const auto* po = FindPageObj(dump, tree_->leaves[leaf].id);
        const auto* pd =
            po ? std::get_if<foundation::objects::Dict>(&po->value.v)
               : nullptr;
        if (pd == nullptr) continue;
        for (const auto& [name, sid] : PageImageXObjects(dump, *pd)) {
            (void)name;
            if (idx == globalIndex) {
                const auto* obj = FindPageObj(dump, sid);
                const auto* st =
                    obj ? std::get_if<foundation::objects::Stream>(
                              &obj->value.v)
                        : nullptr;
                if (st == nullptr) return false;
                std::ofstream of(path, std::ios::binary | std::ios::trunc);
                of.write(reinterpret_cast<const char*>(st->body.data()),
                         static_cast<std::streamsize>(st->body.size()));
                return true;
            }
            ++idx;
        }
    }
    return false;
}

int Document::ExtractImagesToFiles(int pageNumber1BasedOr0,
                                   const std::string& basePath) const {
    if (!tree_) return 0;
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return 0;
    }
    std::vector<std::size_t> leaves;
    if (pageNumber1BasedOr0 > 0) {
        if (static_cast<std::size_t>(pageNumber1BasedOr0) <=
            tree_->leaves.size())
            leaves.push_back(
                static_cast<std::size_t>(pageNumber1BasedOr0 - 1));
    } else {
        for (std::size_t i = 0; i < tree_->leaves.size(); ++i)
            leaves.push_back(i);
    }
    std::string stem = basePath, ext;
    if (auto dot = basePath.find_last_of('.');
        dot != std::string::npos && dot > basePath.find_last_of("/\\") + 0) {
        stem = basePath.substr(0, dot);
        ext = basePath.substr(dot);
    }
    int written = 0;
    for (std::size_t leaf : leaves) {
        const auto* po = FindPageObj(dump, tree_->leaves[leaf].id);
        const auto* pd =
            po ? std::get_if<foundation::objects::Dict>(&po->value.v)
               : nullptr;
        if (pd == nullptr) continue;
        for (const auto& [name, sid] : PageImageXObjects(dump, *pd)) {
            (void)name;
            const auto* obj = FindPageObj(dump, sid);
            const auto* st =
                obj ? std::get_if<foundation::objects::Stream>(&obj->value.v)
                    : nullptr;
            if (st == nullptr) continue;
            const std::string path =
                written == 0 ? basePath
                             : stem + "_" + std::to_string(written) + ext;
            std::ofstream of(path, std::ios::binary | std::ios::trunc);
            of.write(reinterpret_cast<const char*>(st->body.data()),
                     static_cast<std::streamsize>(st->body.size()));
            ++written;
        }
    }
    return written;
}

namespace {

// PDF string literal escaping for use in a name-tree Names array
// key. Returns the literal bytes (no surrounding parens — the
// String value will be wrapped by the writer). For v1, embedded
// file keys are restricted to ASCII paths.
std::vector<std::byte> EncodeNameTreeKey(const std::string& s) {
    std::vector<std::byte> out;
    out.reserve(s.size());
    for (char c : s) out.push_back(static_cast<std::byte>(c));
    return out;
}

std::vector<std::byte> ReadFileBytes(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const auto end = f.tellg();
    if (end < 0) return {};
    std::vector<std::byte> out(static_cast<std::size_t>(end));
    f.seekg(0, std::ios::beg);
    f.read(reinterpret_cast<char*>(out.data()),
           static_cast<std::streamsize>(out.size()));
    return out;
}

}  // namespace

std::vector<std::byte> Document::AppendEmbeddedFilesUpdate(
        const std::vector<std::byte>& working) const {
    // Proceed when there are entries to (re)write, OR when loaded
    // attachments were all removed (Count 0 while loaded_count_ > 0 — the
    // name tree must be cleared).
    if (!embedded_files_ ||
        (embedded_files_->Count() == 0 &&
         embedded_files_->loaded_count_ == 0)) {
        return working;
    }

    auto working_span = std::span<const std::byte>(
        working.data(), working.size());

    foundation::trailer::Trailer trailer_bundle;
    try {
        trailer_bundle = foundation::trailer::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }
    if (trailer_bundle.root_id == 0 || trailer_bundle.size == 0) {
        return working;
    }

    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }

    // Find the Catalog object.
    const foundation::objects::IndirectObject* catalog = nullptr;
    for (const auto& obj : dump.objects) {
        if (obj.id == trailer_bundle.root_id) {
            catalog = &obj;
            break;
        }
    }
    if (catalog == nullptr) return working;
    const auto* catalog_dict =
        std::get_if<foundation::objects::Dict>(&catalog->value.v);
    if (catalog_dict == nullptr) return working;

    std::uint32_t next_id = trailer_bundle.size;
    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;

    // For each FileSpec entry: read bytes from disk, emit
    // /EmbeddedFile stream + /Filespec dict.
    // Track (key → Filespec ref) for the name-tree leaf.
    struct NameEntry {
        std::string key;
        foundation::objects::Ref filespec_ref;
    };
    std::vector<NameEntry> name_entries;
    // Keep the loaded file bodies alive for the duration of
    // AppendIncremental — the Stream value holds a span pointing
    // into these buffers.
    std::vector<std::vector<std::byte>> file_buffers;
    file_buffers.reserve(
        static_cast<std::size_t>(embedded_files_->Count()));

    const auto keys = embedded_files_->Keys();
    for (std::size_t i = 0; i < keys.size(); ++i) {
        const auto& key = keys[i];
        const auto& fs = (*embedded_files_)[static_cast<int>(i)];
        if (!fs.IncludeContents()) continue;

        // Bytes come from the in-memory carrier (loaded attachments /
        // future in-memory attach) when present; otherwise read Name() as a
        // filesystem path. Empty / unreadable files are skipped.
        const auto bytes =
            !fs.content_.empty() ? fs.content_ : ReadFileBytes(fs.Name());
        if (bytes.empty()) continue;
        file_buffers.push_back(bytes);
        const auto& stable = file_buffers.back();

        // Emit /EmbeddedFile stream.
        const std::uint32_t stream_id = next_id++;
        {
            foundation::objects::Stream stream_value;
            foundation::objects::Value type_val;
            type_val.v = std::string("EmbeddedFile");
            stream_value.header.entries.emplace_back("Type",
                std::move(type_val));
            foundation::objects::Value length_val;
            length_val.v = static_cast<std::int64_t>(stable.size());
            stream_value.header.entries.emplace_back("Length",
                std::move(length_val));
            stream_value.body = std::span<const std::byte>(
                stable.data(), stable.size());

            foundation::pdf_writer_incremental::DirtyObject d;
            d.id = stream_id;
            d.generation = 0;
            d.value.v = std::move(stream_value);
            dirty.push_back(std::move(d));
        }

        // Emit /Filespec dict.
        const std::uint32_t filespec_id = next_id++;
        {
            foundation::objects::Dict d;
            {
                foundation::objects::Value v;
                v.v = std::string("Filespec");
                d.entries.emplace_back("Type", std::move(v));
            }
            {
                foundation::objects::String pdf_str;
                pdf_str.bytes = EncodePdfString(fs.Name());
                pdf_str.kind = foundation::objects::StringKind::Literal;
                foundation::objects::Value v;
                v.v = std::move(pdf_str);
                d.entries.emplace_back("F", std::move(v));
            }
            if (!fs.UnicodeName().empty()) {
                foundation::objects::String pdf_str;
                pdf_str.bytes = EncodePdfString(fs.UnicodeName());
                pdf_str.kind = foundation::objects::StringKind::Literal;
                foundation::objects::Value v;
                v.v = std::move(pdf_str);
                d.entries.emplace_back("UF", std::move(v));
            }
            if (!fs.Description().empty()) {
                foundation::objects::String pdf_str;
                pdf_str.bytes = EncodePdfString(fs.Description());
                pdf_str.kind = foundation::objects::StringKind::Literal;
                foundation::objects::Value v;
                v.v = std::move(pdf_str);
                d.entries.emplace_back("Desc", std::move(v));
            }
            // /EF << /F <stream-ref> >>
            {
                foundation::objects::Dict ef_dict;
                foundation::objects::Value ef_ref_val;
                ef_ref_val.v = foundation::objects::Ref{stream_id, 0};
                ef_dict.entries.emplace_back("F",
                    std::move(ef_ref_val));
                foundation::objects::Value ef_val;
                ef_val.v = std::move(ef_dict);
                d.entries.emplace_back("EF", std::move(ef_val));
            }

            foundation::pdf_writer_incremental::DirtyObject obj;
            obj.id = filespec_id;
            obj.generation = 0;
            obj.value.v = std::move(d);
            dirty.push_back(std::move(obj));
        }

        name_entries.push_back({
            key,
            foundation::objects::Ref{filespec_id, 0}});
    }

    // No entries AND nothing was loaded — genuinely nothing to write. When
    // entries were loaded but all removed (delete-all), fall through to
    // write an empty /EmbeddedFiles name tree (clears it on disk).
    if (name_entries.empty() && embedded_files_->loaded_count_ == 0) {
        return working;
    }

    // Build the /EmbeddedFiles flat-leaf name tree:
    //   << /Names [(key1) <ref1> (key2) <ref2> ...] >>
    foundation::objects::Dict embedded_files_dict;
    {
        foundation::objects::Array names_arr;
        names_arr.items.reserve(name_entries.size() * 2);
        for (const auto& ne : name_entries) {
            foundation::objects::String key_str;
            key_str.bytes = EncodeNameTreeKey(ne.key);
            key_str.kind = foundation::objects::StringKind::Literal;
            foundation::objects::Value key_val;
            key_val.v = std::move(key_str);
            names_arr.items.push_back(std::move(key_val));

            foundation::objects::Value ref_val;
            ref_val.v = ne.filespec_ref;
            names_arr.items.push_back(std::move(ref_val));
        }
        foundation::objects::Value names_val;
        names_val.v = std::move(names_arr);
        embedded_files_dict.entries.emplace_back("Names",
            std::move(names_val));
    }

    // Look at Catalog's existing /Names entry. Two layouts:
    //   (a) inline Dict — we update it in place.
    //   (b) Ref to a Names dict object — we update that object
    //       (load + rewrite).
    //   (c) absent — we create a new Names dict object and add
    //       a /Names Ref to the Catalog.
    foundation::objects::Dict updated_catalog = *catalog_dict;
    bool catalog_dirty = false;

    // Locate existing /Names entry index, if any.
    std::ptrdiff_t names_slot = -1;
    for (std::size_t i = 0; i < updated_catalog.entries.size(); ++i) {
        if (updated_catalog.entries[i].first == "Names") {
            names_slot = static_cast<std::ptrdiff_t>(i);
            break;
        }
    }

    if (names_slot == -1) {
        // No /Names entry — create a fresh inline Names dict on
        // the Catalog, embedding our /EmbeddedFiles directly.
        foundation::objects::Dict names_dict;
        foundation::objects::Value embedded_val;
        embedded_val.v = std::move(embedded_files_dict);
        names_dict.entries.emplace_back("EmbeddedFiles",
            std::move(embedded_val));
        foundation::objects::Value names_val;
        names_val.v = std::move(names_dict);
        updated_catalog.entries.emplace_back("Names",
            std::move(names_val));
        catalog_dirty = true;
    } else {
        // /Names already present. Two sub-cases: inline or Ref.
        auto& names_value = updated_catalog.entries[
            static_cast<std::size_t>(names_slot)].second;
        if (auto* names_dict =
                std::get_if<foundation::objects::Dict>(&names_value.v)) {
            // Replace any existing /EmbeddedFiles entry on this
            // inline dict. v1 doesn't merge — full replace.
            for (auto it = names_dict->entries.begin();
                 it != names_dict->entries.end(); ) {
                if (it->first == "EmbeddedFiles") {
                    it = names_dict->entries.erase(it);
                } else {
                    ++it;
                }
            }
            foundation::objects::Value embedded_val;
            embedded_val.v = std::move(embedded_files_dict);
            names_dict->entries.emplace_back("EmbeddedFiles",
                std::move(embedded_val));
            catalog_dirty = true;
        } else if (auto* names_ref =
                       std::get_if<foundation::objects::Ref>(
                           &names_value.v)) {
            // /Names points to a separate object — rewrite that
            // object with the new /EmbeddedFiles entry.
            const foundation::objects::IndirectObject* names_obj
                = nullptr;
            for (const auto& obj : dump.objects) {
                if (obj.id == names_ref->id) {
                    names_obj = &obj;
                    break;
                }
            }
            if (names_obj == nullptr) {
                // Referenced /Names object missing — fall back to
                // overwriting the Catalog slot with an inline
                // dict containing only /EmbeddedFiles.
                foundation::objects::Dict fresh;
                foundation::objects::Value embedded_val;
                embedded_val.v = std::move(embedded_files_dict);
                fresh.entries.emplace_back("EmbeddedFiles",
                    std::move(embedded_val));
                foundation::objects::Value v;
                v.v = std::move(fresh);
                names_value = std::move(v);
                catalog_dirty = true;
            } else if (const auto* names_obj_dict =
                           std::get_if<foundation::objects::Dict>(
                               &names_obj->value.v)) {
                foundation::objects::Dict updated_names
                    = *names_obj_dict;
                for (auto it = updated_names.entries.begin();
                     it != updated_names.entries.end(); ) {
                    if (it->first == "EmbeddedFiles") {
                        it = updated_names.entries.erase(it);
                    } else {
                        ++it;
                    }
                }
                foundation::objects::Value embedded_val;
                embedded_val.v = std::move(embedded_files_dict);
                updated_names.entries.emplace_back("EmbeddedFiles",
                    std::move(embedded_val));

                foundation::pdf_writer_incremental::DirtyObject d;
                d.id = names_obj->id;
                d.generation = names_obj->generation;
                d.value.v = std::move(updated_names);
                dirty.push_back(std::move(d));
            }
        }
    }

    if (catalog_dirty) {
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = catalog->id;
        d.generation = catalog->generation;
        d.value.v = std::move(updated_catalog);
        dirty.push_back(std::move(d));
    }

    if (dirty.empty()) return working;

    return foundation::pdf_writer_incremental::AppendIncremental(
        working_span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
}

namespace {

// Decompose an explicit destination into its PDF /D-array pieces: the 1-based
// target page number, the fit verb (the /XYZ /Fit /FitH … name), and its
// numeric operands. The verb + operands come from the destination's textual
// form ("<page> <VERB> <ops…>", produced by the *ExplicitDestination ToString
// overrides); the page comes from PageNumber(). Returns false for IAppointment
// targets that are not explicit destinations (those are not persisted as /D
// arrays in v1).
bool DecomposeExplicitDest(const Aspose::Pdf::Annotations::IAppointment* app,
                           int& page_number, std::string& verb,
                           std::vector<double>& operands) {
    const auto* ed =
        dynamic_cast<const Aspose::Pdf::Annotations::ExplicitDestination*>(app);
    if (ed == nullptr) return false;
    page_number = ed->PageNumber();
    std::istringstream iss(ed->ToString());
    std::string page_tok;
    iss >> page_tok >> verb;
    operands.clear();
    double d;
    while (iss >> d) operands.push_back(d);
    if (verb.empty()) verb = "Fit";
    return true;
}

}  // namespace

std::vector<std::byte> Document::AppendNamedDestinationsUpdate(
        const std::vector<std::byte>& working) const {
    if (!named_destinations_ ||
        (named_destinations_->Count() == 0 &&
         named_destinations_->loaded_count_ == 0)) {
        return working;
    }

    auto working_span =
        std::span<const std::byte>(working.data(), working.size());

    foundation::trailer::Trailer trailer_bundle;
    try {
        trailer_bundle = foundation::trailer::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }
    if (trailer_bundle.root_id == 0 || trailer_bundle.size == 0) {
        return working;
    }

    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }

    const auto* catalog = FindObject(dump, trailer_bundle.root_id);
    const auto* catalog_dict =
        catalog ? std::get_if<foundation::objects::Dict>(&catalog->value.v)
                : nullptr;
    if (catalog_dict == nullptr) return working;

    // Build the /Dests flat-leaf name tree:
    //   << /Names [(name1) [<page-ref> /verb ops…] (name2) [...] …] >>
    foundation::objects::Array names_arr;
    for (const auto& name : named_destinations_->Names()) {
        const auto* app = (*named_destinations_)[name];
        int page_number = 0;
        std::string verb;
        std::vector<double> operands;
        if (!DecomposeExplicitDest(app, page_number, verb, operands)) continue;
        if (page_number < 1 ||
            static_cast<std::size_t>(page_number) > tree_->leaves.size()) {
            continue;  // unresolvable target page — skip
        }
        const std::uint32_t page_id =
            tree_->leaves[static_cast<std::size_t>(page_number) - 1].id;

        foundation::objects::Array dest;
        dest.items.push_back(foundation::objects::Value{
            foundation::objects::Ref{page_id, 0}});
        dest.items.push_back(foundation::objects::Value{verb});  // /Name
        for (double op : operands) {
            dest.items.push_back(foundation::objects::Value{op});  // Real
        }

        foundation::objects::String key_str;
        key_str.bytes = EncodeNameTreeKey(name);
        key_str.kind = foundation::objects::StringKind::Literal;
        names_arr.items.push_back(foundation::objects::Value{std::move(key_str)});
        names_arr.items.push_back(
            foundation::objects::Value{std::move(dest)});
    }

    // Nothing resolvable AND nothing was loaded — leave the file untouched.
    if (names_arr.items.empty() && named_destinations_->loaded_count_ == 0) {
        return working;
    }

    foundation::objects::Dict dests_dict;
    dests_dict.entries.emplace_back(
        "Names", foundation::objects::Value{std::move(names_arr)});

    // Insert /Dests under the Catalog's /Names (mirrors the /EmbeddedFiles
    // slot logic: absent → create; inline dict → replace /Dests; Ref →
    // rewrite the referenced Names object).
    foundation::objects::Dict updated_catalog = *catalog_dict;
    bool catalog_dirty = false;
    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;

    std::ptrdiff_t names_slot = -1;
    for (std::size_t i = 0; i < updated_catalog.entries.size(); ++i) {
        if (updated_catalog.entries[i].first == "Names") {
            names_slot = static_cast<std::ptrdiff_t>(i);
            break;
        }
    }

    auto set_dests = [&](foundation::objects::Dict& names_dict) {
        for (auto it = names_dict.entries.begin();
             it != names_dict.entries.end();) {
            if (it->first == "Dests")
                it = names_dict.entries.erase(it);
            else
                ++it;
        }
        names_dict.entries.emplace_back(
            "Dests", foundation::objects::Value{std::move(dests_dict)});
    };

    if (names_slot == -1) {
        foundation::objects::Dict names_dict;
        set_dests(names_dict);
        updated_catalog.entries.emplace_back(
            "Names", foundation::objects::Value{std::move(names_dict)});
        catalog_dirty = true;
    } else {
        auto& names_value =
            updated_catalog.entries[static_cast<std::size_t>(names_slot)]
                .second;
        if (auto* names_dict =
                std::get_if<foundation::objects::Dict>(&names_value.v)) {
            set_dests(*names_dict);
            catalog_dirty = true;
        } else if (auto* names_ref =
                       std::get_if<foundation::objects::Ref>(&names_value.v)) {
            const auto* names_obj = FindObject(dump, names_ref->id);
            const auto* names_obj_dict =
                names_obj ? std::get_if<foundation::objects::Dict>(
                                &names_obj->value.v)
                          : nullptr;
            if (names_obj_dict == nullptr) {
                foundation::objects::Dict fresh;
                set_dests(fresh);
                names_value = foundation::objects::Value{std::move(fresh)};
                catalog_dirty = true;
            } else {
                foundation::objects::Dict updated_names = *names_obj_dict;
                set_dests(updated_names);
                foundation::pdf_writer_incremental::DirtyObject d;
                d.id = names_obj->id;
                d.generation = names_obj->generation;
                d.value.v = std::move(updated_names);
                dirty.push_back(std::move(d));
            }
        }
    }

    if (catalog_dirty) {
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = catalog->id;
        d.generation = catalog->generation;
        d.value.v = std::move(updated_catalog);
        dirty.push_back(std::move(d));
    }

    if (dirty.empty()) return working;

    return foundation::pdf_writer_incremental::AppendIncremental(
        working_span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
}

namespace {

// Field-subtype dispatch. Returns the PDF /FT name + an /Ff
// flag-bitmask contribution for the field-subtype's specific
// canonical flags (PDF §12.7.4 — variable text / button / choice
// flags). The base /Ff bits (ReadOnly / Required / NoExport) are
// added by the caller.
struct FieldKind {
    std::string ft;
    std::int64_t ff = 0;
};

FieldKind FieldKindOf(const Aspose::Pdf::Forms::Field& field) {
    namespace F = Aspose::Pdf::Forms;
    if (const auto* sig = dynamic_cast<const F::SignatureField*>(&field)) {
        (void)sig; return {"Sig", 0};
    }
    if (const auto* btn = dynamic_cast<const F::ButtonField*>(&field)) {
        (void)btn; return {"Btn", 1LL << 16};  // /Pushbutton
    }
    if (const auto* radio = dynamic_cast<const F::RadioButtonField*>(&field)) {
        (void)radio; return {"Btn", 1LL << 15};  // /Radio
    }
    if (const auto* cb = dynamic_cast<const F::CheckboxField*>(&field)) {
        (void)cb; return {"Btn", 0};
    }
    if (const auto* combo = dynamic_cast<const F::ComboBoxField*>(&field)) {
        (void)combo; return {"Ch", 1LL << 17};  // /Combo
    }
    if (const auto* lb = dynamic_cast<const F::ListBoxField*>(&field)) {
        (void)lb; return {"Ch", 0};
    }
    if (const auto* rich = dynamic_cast<const F::RichTextBoxField*>(&field)) {
        (void)rich; return {"Tx", 1LL << 25};  // /RichText
    }
    if (const auto* file = dynamic_cast<const F::FileSelectBoxField*>(&field)) {
        (void)file; return {"Tx", 1LL << 20};  // /FileSelect
    }
    if (const auto* pwd = dynamic_cast<const F::PasswordBoxField*>(&field)) {
        (void)pwd; return {"Tx", 1LL << 13};  // /Password
    }
    if (const auto* tx = dynamic_cast<const F::TextBoxField*>(&field)) {
        std::int64_t ff = 0;
        if (tx->Multiline()) ff |= 1LL << 12;  // /Multiline
        return {"Tx", ff};
    }
    // ChoiceField / ChoiceField subtree fallback / unknown Field
    // subclass — default to /Tx so the entry is still parseable.
    return {"Tx", 0};
}

// Resolve an /AcroForm Value (Ref or inline Dict) to its dict, recording the
// indirect id (0 when inline / absent).
const foundation::objects::Dict* ResolveAcroForm(
        const foundation::objects::Dump& dump,
        const foundation::objects::Value* entry, std::uint32_t& acroform_id) {
    acroform_id = 0;
    if (entry == nullptr) return nullptr;
    if (const auto* ref = std::get_if<foundation::objects::Ref>(&entry->v)) {
        acroform_id = ref->id;
        const auto* obj = FindObject(dump, ref->id);
        return obj ? std::get_if<foundation::objects::Dict>(&obj->value.v)
                   : nullptr;
    }
    return std::get_if<foundation::objects::Dict>(&entry->v);
}

// A form field's effective value as the string that gets written to /V:
// checkboxes report an on/off name, choices their shadowed value, everything
// else the base Field value. Used to snapshot loaded fields at load and to
// detect edits at Save.
std::string FieldEffectiveValue(const Aspose::Pdf::Forms::Field* fp) {
    namespace F = Aspose::Pdf::Forms;
    if (const auto* cb = dynamic_cast<const F::CheckboxField*>(fp))
        return cb->Checked() ? "Yes" : "Off";
    if (const auto* ch = dynamic_cast<const F::ChoiceField*>(fp))
        return ch->Value();
    return fp->Value();
}

// Strip any existing /V + /AS on a field dict and write the current value:
// checkboxes write an on/off NAME (+ matching /AS); everything else a literal
// string (omitted when empty).
void ApplyFieldValue(foundation::objects::Dict& d,
                     Aspose::Pdf::Forms::Field* fp) {
    for (auto it = d.entries.begin(); it != d.entries.end();) {
        if (it->first == "V" || it->first == "AS")
            it = d.entries.erase(it);
        else
            ++it;
    }
    if (auto* cb = dynamic_cast<Aspose::Pdf::Forms::CheckboxField*>(fp)) {
        const char* on = cb->Checked() ? "Yes" : "Off";
        d.entries.emplace_back("V", PageNameValue(on));
        d.entries.emplace_back("AS", PageNameValue(on));
        return;
    }
    std::string val = fp->Value();
    if (auto* ch = dynamic_cast<Aspose::Pdf::Forms::ChoiceField*>(fp))
        val = ch->Value();
    if (!val.empty()) {
        foundation::objects::String s;
        s.bytes = EncodePdfString(val);
        s.kind = foundation::objects::StringKind::Literal;
        foundation::objects::Value v;
        v.v = std::move(s);
        d.entries.emplace_back("V", std::move(v));
    }
}

// Read a PDF string/name Value into a std::string (literal/hex bytes, or a
// name's text). Returns "" for anything else.
std::string ReadStrOrName(const foundation::objects::Value* v) {
    if (v == nullptr) return {};
    if (const auto* s = std::get_if<foundation::objects::String>(&v->v)) {
        std::string out;
        out.reserve(s->bytes.size());
        for (auto b : s->bytes) out.push_back(static_cast<char>(b));
        return out;
    }
    if (const auto* n = std::get_if<std::string>(&v->v)) return *n;
    return {};
}

std::int64_t ReadInt(const foundation::objects::Value* v,
                     std::int64_t fallback) {
    if (v == nullptr) return fallback;
    if (const auto* i = std::get_if<std::int64_t>(&v->v)) return *i;
    return fallback;
}

}  // namespace

void Document::LoadAcroFormFields(Aspose::Pdf::Forms::Form& form) {
    namespace F = Aspose::Pdf::Forms;
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());

    foundation::trailer::Trailer trailer_bundle;
    foundation::objects::Dump dump;
    try {
        trailer_bundle = foundation::trailer::Parse(span);
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return;
    }
    if (trailer_bundle.root_id == 0) return;
    const auto* catalog = FindObject(dump, trailer_bundle.root_id);
    const auto* catalog_dict =
        catalog ? std::get_if<foundation::objects::Dict>(&catalog->value.v)
                : nullptr;
    if (catalog_dict == nullptr) return;

    // Resolve /AcroForm (ref or inline) → its dict.
    std::uint32_t acroform_id = 0;
    const auto* acro =
        ResolveAcroForm(dump, DictGet(*catalog_dict, "AcroForm"), acroform_id);
    if (acro == nullptr) return;

    // /SigFlags → SignaturesExist / AppendOnly.
    if (const std::int64_t sf = ReadInt(DictGet(*acro, "SigFlags"), 0)) {
        if (sf & 1) form.SignaturesExist(true);
        if (sf & 2) form.SignaturesAppendOnly(true);
    }

    const auto* fields = DictGet(*acro, "Fields");
    const auto* fields_arr =
        fields ? std::get_if<foundation::objects::Array>(&fields->v) : nullptr;
    if (fields_arr == nullptr) return;

    for (const auto& item : fields_arr->items) {
        const auto* ref = std::get_if<foundation::objects::Ref>(&item.v);
        if (ref == nullptr) continue;
        const auto* fobj = FindObject(dump, ref->id);
        const auto* fdict =
            fobj ? std::get_if<foundation::objects::Dict>(&fobj->value.v)
                 : nullptr;
        if (fdict == nullptr) continue;

        const std::string ft = ReadStrOrName(DictGet(*fdict, "FT"));
        const std::int64_t ff = ReadInt(DictGet(*fdict, "Ff"), 0);
        const std::string t = ReadStrOrName(DictGet(*fdict, "T"));
        const std::string vstr = ReadStrOrName(DictGet(*fdict, "V"));

        // Field has no /FT and no /T — not a terminal field we model.
        if (ft.empty() && t.empty()) continue;

        // Concrete subclass per /FT + /Ff (inverse of FieldKindOf).
        const Aspose::Pdf::Rectangle rect0(0.0, 0.0, 0.0, 0.0, false);
        std::unique_ptr<F::Field> field;
        if (ft == "Sig") {
            field = std::make_unique<F::SignatureField>(*this, rect0);
        } else if (ft == "Btn") {
            if (ff & (1LL << 16))
                field = std::make_unique<F::ButtonField>(*this, rect0);
            else if (ff & (1LL << 15))
                field = std::make_unique<F::RadioButtonField>(*this);
            else
                field = std::make_unique<F::CheckboxField>(*this);
        } else if (ft == "Ch") {
            if (ff & (1LL << 17))
                field = std::make_unique<F::ComboBoxField>(*this);
            else
                field = std::make_unique<F::ListBoxField>(*this, rect0);
        } else if (ff & (1LL << 25)) {
            field = std::make_unique<F::RichTextBoxField>();
        } else if (ff & (1LL << 20)) {
            field = std::make_unique<F::FileSelectBoxField>();
        } else if (ff & (1LL << 13)) {
            field = std::make_unique<F::PasswordBoxField>();
        } else {
            field = std::make_unique<F::TextBoxField>(*this);
        }

        F::Field* raw = field.get();
        raw->PartialName(t);
        raw->ReadOnly((ff & (1LL << 0)) != 0);
        raw->Required((ff & (1LL << 1)) != 0);
        raw->Exportable((ff & (1LL << 2)) == 0);

        // /Rect → Rectangle.
        if (const auto* r = DictGet(*fdict, "Rect")) {
            if (const auto* arr =
                    std::get_if<foundation::objects::Array>(&r->v)) {
                if (arr->items.size() == 4) {
                    auto num = [](const foundation::objects::Value& v) {
                        if (const auto* d = std::get_if<double>(&v.v))
                            return *d;
                        if (const auto* i = std::get_if<std::int64_t>(&v.v))
                            return static_cast<double>(*i);
                        return 0.0;
                    };
                    raw->Rect(Aspose::Pdf::Rectangle(
                        num(arr->items[0]), num(arr->items[1]),
                        num(arr->items[2]), num(arr->items[3]), false));
                }
            }
        }

        // Type-specific state.
        if (auto* tx = dynamic_cast<F::TextBoxField*>(raw)) {
            tx->Multiline((ff & (1LL << 12)) != 0);
            tx->Value(vstr);
        } else if (auto* cb = dynamic_cast<F::CheckboxField*>(raw)) {
            cb->Checked(!vstr.empty() && vstr != "Off");
            cb->Value(vstr);
        } else if (auto* ch = dynamic_cast<F::ChoiceField*>(raw)) {
            if (const auto* opt = DictGet(*fdict, "Opt")) {
                if (const auto* oarr =
                        std::get_if<foundation::objects::Array>(&opt->v)) {
                    int idx = 0, sel = -1;
                    for (const auto& oi : oarr->items) {
                        std::string name;
                        if (const auto* pair =
                                std::get_if<foundation::objects::Array>(
                                    &oi.v)) {
                            // [export display] — use the display name.
                            if (pair->items.size() == 2)
                                name = ReadStrOrName(&pair->items[1]);
                            else if (!pair->items.empty())
                                name = ReadStrOrName(&pair->items[0]);
                        } else {
                            name = ReadStrOrName(&oi);
                        }
                        ch->AddOption(name);
                        if (!vstr.empty() && name == vstr) sel = idx;
                        ++idx;
                    }
                    if (sel >= 0) ch->Selected(sel);
                }
            }
            ch->Value(vstr);
        } else {
            raw->Value(vstr);
        }

        form.loaded_ids_[raw] = ref->id;
        form.loaded_values_[raw] = FieldEffectiveValue(raw);
        form.fields_.push_back(raw);
        form.owned_fields_.push_back(std::move(field));
    }
    form.loaded_count_ = form.owned_fields_.size();
}

void Document::LoadPageAnnotations(
        std::size_t leaf,
        Aspose::Pdf::Annotations::AnnotationCollection& coll,
        Aspose::Pdf::Page& page) {
    namespace A = Aspose::Pdf::Annotations;
    if (tree_ == nullptr || leaf >= tree_->leaves.size()) return;

    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return;
    }

    const std::uint32_t page_id = tree_->leaves[leaf].id;
    const auto* page_obj = FindObject(dump, page_id);
    const auto* page_dict =
        page_obj ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                 : nullptr;
    if (page_dict == nullptr) return;
    const auto* annots = DictGet(*page_dict, "Annots");
    const auto* annots_arr =
        annots ? std::get_if<foundation::objects::Array>(&annots->v) : nullptr;
    if (annots_arr == nullptr) return;

    for (const auto& item : annots_arr->items) {
        const auto* ref = std::get_if<foundation::objects::Ref>(&item.v);
        if (ref == nullptr) continue;
        const auto* aobj = FindObject(dump, ref->id);
        const auto* adict =
            aobj ? std::get_if<foundation::objects::Dict>(&aobj->value.v)
                 : nullptr;
        if (adict == nullptr) continue;

        const std::string subtype = ReadStrOrName(DictGet(*adict, "Subtype"));
        const std::string contents = ReadStrOrName(DictGet(*adict, "Contents"));

        Aspose::Pdf::Rectangle rect(0.0, 0.0, 0.0, 0.0, false);
        if (const auto* r = DictGet(*adict, "Rect")) {
            if (const auto* arr =
                    std::get_if<foundation::objects::Array>(&r->v)) {
                if (arr->items.size() == 4) {
                    auto num = [](const foundation::objects::Value& v) {
                        if (const auto* d = std::get_if<double>(&v.v))
                            return *d;
                        if (const auto* i = std::get_if<std::int64_t>(&v.v))
                            return static_cast<double>(*i);
                        return 0.0;
                    };
                    rect = Aspose::Pdf::Rectangle(
                        num(arr->items[0]), num(arr->items[1]),
                        num(arr->items[2]), num(arr->items[3]), false);
                }
            }
        }

        // Concrete subclass per /Subtype (clean (Page&, Rect) ctors only;
        // subtypes needing extra construction args — FreeText, Line, Ink,
        // Polygon, FileAttachment — are left in place, preserved losslessly).
        std::unique_ptr<A::Annotation> annot;
        if (subtype == "Text")
            annot = std::make_unique<A::TextAnnotation>(page, rect);
        else if (subtype == "Link")
            annot = std::make_unique<A::LinkAnnotation>(page, rect);
        else if (subtype == "Square")
            annot = std::make_unique<A::SquareAnnotation>(page, rect);
        else if (subtype == "Circle")
            annot = std::make_unique<A::CircleAnnotation>(page, rect);
        else if (subtype == "Highlight")
            annot = std::make_unique<A::HighlightAnnotation>(page, rect);
        else if (subtype == "Underline")
            annot = std::make_unique<A::UnderlineAnnotation>(page, rect);
        else if (subtype == "Squiggly")
            annot = std::make_unique<A::SquigglyAnnotation>(page, rect);
        else if (subtype == "StrikeOut")
            annot = std::make_unique<A::StrikeOutAnnotation>(page, rect);
        else if (subtype == "Stamp")
            annot = std::make_unique<A::StampAnnotation>(page, rect);
        else if (subtype == "Caret")
            annot = std::make_unique<A::CaretAnnotation>(page, rect);
        else if (subtype == "Watermark")
            annot = std::make_unique<A::WatermarkAnnotation>(page, rect);
        else if (subtype == "Redact")
            annot = std::make_unique<A::RedactionAnnotation>(page, rect);
        else
            continue;  // out-of-v1-scope subtype — preserved on disk

        A::Annotation* raw = annot.get();
        if (!contents.empty()) raw->Contents(contents);
        coll.loaded_ids_[raw] = ref->id;
        coll.owned_.push_back(std::move(annot));
        coll.Add(*raw);
    }
    coll.loaded_count_ = coll.owned_.size();
}

namespace {

// Decode an /EmbeddedFile stream body per its /Filter (FlateDecode or none;
// other filters fall through as raw bytes).
std::vector<std::byte> DecodeStreamBody(const foundation::objects::Stream& s) {
    bool flate = false;
    if (const auto* f = DictGet(s.header, "Filter")) {
        if (const auto* n = std::get_if<std::string>(&f->v))
            flate = (*n == "FlateDecode");
        else if (const auto* arr =
                     std::get_if<foundation::objects::Array>(&f->v)) {
            for (const auto& it : arr->items)
                if (const auto* n = std::get_if<std::string>(&it.v))
                    if (*n == "FlateDecode") flate = true;
        }
    }
    std::vector<std::byte> body(s.body.begin(), s.body.end());
    if (flate) {
        try {
            return foundation::flate::Decode(
                std::span<const std::byte>(body.data(), body.size()));
        } catch (const std::exception&) {
            return {};
        }
    }
    return body;
}

// Resolve a Value (Ref or inline) to the indirect object dict it denotes.
const foundation::objects::Dict* DerefDict(
        const foundation::objects::Dump& dump,
        const foundation::objects::Value* v) {
    if (v == nullptr) return nullptr;
    if (const auto* d = std::get_if<foundation::objects::Dict>(&v->v))
        return d;
    if (const auto* ref = std::get_if<foundation::objects::Ref>(&v->v)) {
        const auto* obj = FindObject(dump, ref->id);
        return obj ? std::get_if<foundation::objects::Dict>(&obj->value.v)
                   : nullptr;
    }
    return nullptr;
}

// Walk an /EmbeddedFiles name-tree node (flat /Names array or /Kids
// subtrees), appending (key, filespec-dict) pairs.
void CollectNameTree(
        const foundation::objects::Dump& dump,
        const foundation::objects::Dict* node,
        std::vector<std::pair<std::string,
                              const foundation::objects::Dict*>>& out,
        int depth) {
    if (node == nullptr || depth > 50) return;
    if (const auto* names = DictGet(*node, "Names")) {
        if (const auto* arr =
                std::get_if<foundation::objects::Array>(&names->v)) {
            for (std::size_t i = 0; i + 1 < arr->items.size(); i += 2) {
                std::string key;
                if (const auto* s = std::get_if<foundation::objects::String>(
                        &arr->items[i].v)) {
                    for (auto b : s->bytes)
                        key.push_back(static_cast<char>(b));
                }
                const auto* fsd = DerefDict(dump, &arr->items[i + 1]);
                if (fsd) out.emplace_back(std::move(key), fsd);
            }
        }
    }
    if (const auto* kids = DictGet(*node, "Kids")) {
        if (const auto* arr =
                std::get_if<foundation::objects::Array>(&kids->v)) {
            for (const auto& it : arr->items)
                CollectNameTree(dump, DerefDict(dump, &it), out, depth + 1);
        }
    }
}

}  // namespace

void Document::LoadEmbeddedFiles(EmbeddedFileCollection& coll) {
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::trailer::Trailer trailer_bundle;
    foundation::objects::Dump dump;
    try {
        trailer_bundle = foundation::trailer::Parse(span);
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return;
    }
    if (trailer_bundle.root_id == 0) return;
    const auto* catalog = FindObject(dump, trailer_bundle.root_id);
    const auto* catalog_dict =
        catalog ? std::get_if<foundation::objects::Dict>(&catalog->value.v)
                : nullptr;
    if (catalog_dict == nullptr) return;

    const auto* names = DerefDict(dump, DictGet(*catalog_dict, "Names"));
    if (names == nullptr) return;
    const auto* ef = DerefDict(dump, DictGet(*names, "EmbeddedFiles"));
    if (ef == nullptr) return;

    std::vector<std::pair<std::string, const foundation::objects::Dict*>>
        leaves;
    CollectNameTree(dump, ef, leaves, 0);

    for (const auto& [key, fsd] : leaves) {
        FileSpecification fs;
        const std::string name = ReadStrOrName(DictGet(*fsd, "UF")).empty()
                                     ? ReadStrOrName(DictGet(*fsd, "F"))
                                     : ReadStrOrName(DictGet(*fsd, "UF"));
        fs.Name(name);
        const std::string desc = ReadStrOrName(DictGet(*fsd, "Desc"));
        if (!desc.empty()) fs.Description(desc);

        // /EF << /F <stream-ref> >> → the embedded-file stream bytes.
        if (const auto* efd = DerefDict(dump, DictGet(*fsd, "EF"))) {
            const foundation::objects::Value* sref = DictGet(*efd, "F");
            if (sref == nullptr) sref = DictGet(*efd, "UF");
            if (const auto* r =
                    sref ? std::get_if<foundation::objects::Ref>(&sref->v)
                         : nullptr) {
                const auto* sobj = FindObject(dump, r->id);
                if (sobj) {
                    if (const auto* st = std::get_if<foundation::objects::Stream>(
                            &sobj->value.v)) {
                        fs.content_ = DecodeStreamBody(*st);
                    }
                }
            }
        }
        coll.Add(key.empty() ? name : key, std::move(fs));
    }
    coll.loaded_count_ = static_cast<std::size_t>(coll.Count());
}

namespace {

// Walk a /Dests name-tree node (flat /Names array or /Kids subtrees),
// collecting (key, destination-value) pairs. Unlike CollectNameTree this keeps
// the raw Value so array-form and dict-form (/D) destinations both survive.
void CollectDestLeaves(
        const foundation::objects::Dump& dump,
        const foundation::objects::Dict* node,
        std::vector<std::pair<std::string,
                              const foundation::objects::Value*>>& out,
        int depth) {
    if (node == nullptr || depth > 50) return;
    if (const auto* names = DictGet(*node, "Names")) {
        if (const auto* arr =
                std::get_if<foundation::objects::Array>(&names->v)) {
            for (std::size_t i = 0; i + 1 < arr->items.size(); i += 2) {
                std::string key;
                if (const auto* s = std::get_if<foundation::objects::String>(
                        &arr->items[i].v)) {
                    for (auto b : s->bytes)
                        key.push_back(static_cast<char>(b));
                }
                out.emplace_back(std::move(key), &arr->items[i + 1]);
            }
        }
    }
    if (const auto* kids = DictGet(*node, "Kids")) {
        if (const auto* arr =
                std::get_if<foundation::objects::Array>(&kids->v)) {
            for (const auto& it : arr->items)
                CollectDestLeaves(dump, DerefDict(dump, &it), out, depth + 1);
        }
    }
}

// Resolve a /Dests leaf value to its destination array — direct array, a
// dict with a /D array, or an indirect reference to either.
const foundation::objects::Array* ResolveDestArray(
        const foundation::objects::Dump& dump,
        const foundation::objects::Value* v) {
    if (v == nullptr) return nullptr;
    if (const auto* a = std::get_if<foundation::objects::Array>(&v->v))
        return a;
    if (const auto* d = std::get_if<foundation::objects::Dict>(&v->v)) {
        if (const auto* dd = DictGet(*d, "D"))
            return std::get_if<foundation::objects::Array>(&dd->v);
    }
    if (const auto* r = std::get_if<foundation::objects::Ref>(&v->v)) {
        const auto* o = FindObject(dump, r->id);
        if (o == nullptr) return nullptr;
        if (const auto* a =
                std::get_if<foundation::objects::Array>(&o->value.v))
            return a;
        if (const auto* d =
                std::get_if<foundation::objects::Dict>(&o->value.v)) {
            if (const auto* dd = DictGet(*d, "D"))
                return std::get_if<foundation::objects::Array>(&dd->v);
        }
    }
    return nullptr;
}

double DestOperand(const foundation::objects::Array& arr, std::size_t i) {
    if (i >= arr.items.size()) return 0.0;
    const auto& v = arr.items[i].v;
    if (const auto* d = std::get_if<double>(&v)) return *d;
    if (const auto* n = std::get_if<std::int64_t>(&v))
        return static_cast<double>(*n);
    return 0.0;  // null ("unchanged") or non-numeric → 0
}

}  // namespace

void Document::LoadNamedDestinations(NamedDestinationCollection& coll) {
    if (tree_ == nullptr) return;
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::trailer::Trailer trailer_bundle;
    foundation::objects::Dump dump;
    try {
        trailer_bundle = foundation::trailer::Parse(span);
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return;
    }
    if (trailer_bundle.root_id == 0) return;
    const auto* catalog = FindObject(dump, trailer_bundle.root_id);
    const auto* catalog_dict =
        catalog ? std::get_if<foundation::objects::Dict>(&catalog->value.v)
                : nullptr;
    if (catalog_dict == nullptr) return;

    const auto* names = DerefDict(dump, DictGet(*catalog_dict, "Names"));
    if (names == nullptr) return;
    const auto* dests = DerefDict(dump, DictGet(*names, "Dests"));
    if (dests == nullptr) return;

    std::vector<std::pair<std::string, const foundation::objects::Value*>>
        leaves;
    CollectDestLeaves(dump, dests, leaves, 0);

    for (const auto& [name, value] : leaves) {
        const auto* arr = ResolveDestArray(dump, value);
        if (arr == nullptr || arr->items.empty()) continue;

        // First element: the target page (indirect ref → page index, or a
        // 0-based integer page index for remote-style destinations).
        int page_number = 0;
        const auto& page_v = arr->items[0].v;
        if (const auto* r = std::get_if<foundation::objects::Ref>(&page_v)) {
            for (std::size_t i = 0; i < tree_->leaves.size(); ++i) {
                if (tree_->leaves[i].id == r->id) {
                    page_number = static_cast<int>(i) + 1;
                    break;
                }
            }
        } else if (const auto* n = std::get_if<std::int64_t>(&page_v)) {
            page_number = static_cast<int>(*n) + 1;
        }
        if (page_number < 1) continue;

        std::string verb;
        if (arr->items.size() >= 2) {
            if (const auto* s =
                    std::get_if<std::string>(&arr->items[1].v)) {
                verb = *s;
            }
        }

        // Operands start at array index 2.
        const double o0 = DestOperand(*arr, 2);
        const double o1 = DestOperand(*arr, 3);
        const double o2 = DestOperand(*arr, 4);
        const double o3 = DestOperand(*arr, 5);
        using namespace Aspose::Pdf::Annotations;
        if (verb == "XYZ") {
            coll.Add(name, XYZExplicitDestination{page_number, o0, o1, o2});
        } else if (verb == "FitH") {
            coll.Add(name, FitHExplicitDestination{page_number, o0});
        } else if (verb == "FitV") {
            coll.Add(name, FitVExplicitDestination{page_number, o0});
        } else if (verb == "FitR") {
            coll.Add(name,
                     FitRExplicitDestination{page_number, o0, o1, o2, o3});
        } else if (verb == "FitB") {
            coll.Add(name, FitBExplicitDestination{page_number});
        } else if (verb == "FitBH") {
            coll.Add(name, FitBHExplicitDestination{page_number, o0});
        } else if (verb == "FitBV") {
            coll.Add(name, FitBVExplicitDestination{page_number, o0});
        } else {  // "Fit" and any unrecognised verb → whole-page fit
            coll.Add(name, FitExplicitDestination{page_number});
        }
    }
    coll.loaded_count_ = static_cast<std::size_t>(coll.Count());
}

namespace {

// Resolve the leading element of a /D-style destination array to a 1-based
// page number (indirect page ref → tree index, or integer page index + 1).
int DestPageNumber(const foundation::pages_tree::Tree& tree,
                   const foundation::objects::Value& page_v) {
    if (const auto* r = std::get_if<foundation::objects::Ref>(&page_v.v)) {
        for (std::size_t i = 0; i < tree.leaves.size(); ++i) {
            if (tree.leaves[i].id == r->id) return static_cast<int>(i) + 1;
        }
    } else if (const auto* n = std::get_if<std::int64_t>(&page_v.v)) {
        return static_cast<int>(*n) + 1;
    }
    return 0;
}

// Build the typed ExplicitDestination for a destination array
// ([page /verb ops…]). Returns null when the page is unresolvable.
std::unique_ptr<Aspose::Pdf::Annotations::IAppointment> BuildExplicitDest(
        const foundation::pages_tree::Tree& tree,
        const foundation::objects::Array& arr) {
    using namespace Aspose::Pdf::Annotations;
    if (arr.items.empty()) return nullptr;
    const int page = DestPageNumber(tree, arr.items[0]);
    if (page < 1) return nullptr;
    std::string verb;
    if (arr.items.size() >= 2) {
        if (const auto* s = std::get_if<std::string>(&arr.items[1].v))
            verb = *s;
    }
    const double o0 = DestOperand(arr, 2);
    const double o1 = DestOperand(arr, 3);
    const double o2 = DestOperand(arr, 4);
    const double o3 = DestOperand(arr, 5);
    if (verb == "XYZ")
        return std::make_unique<XYZExplicitDestination>(page, o0, o1, o2);
    if (verb == "FitH")
        return std::make_unique<FitHExplicitDestination>(page, o0);
    if (verb == "FitV")
        return std::make_unique<FitVExplicitDestination>(page, o0);
    if (verb == "FitR")
        return std::make_unique<FitRExplicitDestination>(page, o0, o1, o2, o3);
    if (verb == "FitB")
        return std::make_unique<FitBExplicitDestination>(page);
    if (verb == "FitBH")
        return std::make_unique<FitBHExplicitDestination>(page, o0);
    if (verb == "FitBV")
        return std::make_unique<FitBVExplicitDestination>(page, o0);
    return std::make_unique<FitExplicitDestination>(page);
}

// A parsed outline node (no foundation types) — converted into the public
// OutlineItemCollection tree via the public API afterwards.
struct RawOutline {
    std::string title;
    bool bold = false;
    bool italic = false;
    bool open = true;
    std::unique_ptr<Aspose::Pdf::Annotations::IAppointment> destination;
    std::unique_ptr<Aspose::Pdf::Annotations::PdfAction> action;
    std::vector<RawOutline> children;
};

// Parse one /Outlines item dict into a RawOutline, recursing /First / /Next.
RawOutline ParseOutlineItem(Document& doc,
                            const foundation::pages_tree::Tree& tree,
                            const foundation::objects::Dump& dump,
                            const foundation::objects::Dict& item) {
    RawOutline r;
    r.title = ReadStrOrName(DictGet(item, "Title"));

    if (const auto* f = DictGet(item, "F")) {
        std::int64_t flags = 0;
        if (const auto* n = std::get_if<std::int64_t>(&f->v)) flags = *n;
        r.italic = (flags & 1) != 0;  // PDF §12.3.3: bit 1 Italic, bit 2 Bold
        r.bold = (flags & 2) != 0;
    }
    if (const auto* c = DictGet(item, "Count")) {
        if (const auto* n = std::get_if<std::int64_t>(&c->v)) r.open = *n >= 0;
    }

    // Navigation target: /Dest (array → explicit, name/string → named) or
    // /A /URI → GoToURIAction.
    if (const auto* dst = DictGet(item, "Dest")) {
        if (const auto* arr =
                std::get_if<foundation::objects::Array>(&dst->v)) {
            r.destination = BuildExplicitDest(tree, *arr);
        } else if (const auto* nm = std::get_if<std::string>(&dst->v)) {
            r.destination =
                std::make_unique<Aspose::Pdf::Annotations::NamedDestination>(
                    doc, *nm);
        } else if (const auto* s =
                       std::get_if<foundation::objects::String>(&dst->v)) {
            std::string name;
            for (auto b : s->bytes) name.push_back(static_cast<char>(b));
            r.destination =
                std::make_unique<Aspose::Pdf::Annotations::NamedDestination>(
                    doc, name);
        }
    }
    if (!r.destination) {
        if (const auto* ad = DerefDict(dump, DictGet(item, "A"))) {
            const std::string s = ReadStrOrName(DictGet(*ad, "S"));
            if (s == "URI") {
                r.action =
                    std::make_unique<Aspose::Pdf::Annotations::GoToURIAction>(
                        ReadStrOrName(DictGet(*ad, "URI")));
            }
        }
    }

    const auto* child = DerefDict(dump, DictGet(item, "First"));
    int guard = 0;
    while (child != nullptr && guard++ < 100000) {
        r.children.push_back(ParseOutlineItem(doc, tree, dump, *child));
        child = DerefDict(dump, DictGet(*child, "Next"));
    }
    return r;
}

// Convert a RawOutline into a node added under `parent`, using only the
// public OutlineItemCollection API (Add deep-copies into the tree).
void AttachOutline(const RawOutline& r, Aspose::Pdf::Outlines& parent,
                   Aspose::Pdf::OutlineCollection& root) {
    Aspose::Pdf::OutlineItemCollection node{root};
    node.Title(r.title);
    node.Bold(r.bold);
    node.Italic(r.italic);
    node.Open(r.open);
    if (r.destination)
        node.Destination(*r.destination);
    else if (r.action)
        node.Action(*r.action);
    parent.Add(node);
    Aspose::Pdf::OutlineItemCollection& added = parent[parent.Count()];
    for (const auto& c : r.children) AttachOutline(c, added, root);
}

}  // namespace

void Document::LoadOutlines(OutlineCollection& root) {
    if (tree_ == nullptr) return;
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::trailer::Trailer trailer_bundle;
    foundation::objects::Dump dump;
    try {
        trailer_bundle = foundation::trailer::Parse(span);
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return;
    }
    if (trailer_bundle.root_id == 0) return;
    const auto* catalog = FindObject(dump, trailer_bundle.root_id);
    const auto* catalog_dict =
        catalog ? std::get_if<foundation::objects::Dict>(&catalog->value.v)
                : nullptr;
    if (catalog_dict == nullptr) return;

    const auto* outlines = DerefDict(dump, DictGet(*catalog_dict, "Outlines"));
    if (outlines == nullptr) return;

    const auto* item = DerefDict(dump, DictGet(*outlines, "First"));
    int guard = 0;
    while (item != nullptr && guard++ < 100000) {
        RawOutline r = ParseOutlineItem(*this, *tree_, dump, *item);
        AttachOutline(r, root, root);
        item = DerefDict(dump, DictGet(*item, "Next"));
    }
}

Document::OutlineNode Document::OutlineNodeFromItem(
        const OutlineItemCollection& item) const {
    OutlineNode n;
    n.title = item.Title();
    n.bold = item.Bold();
    n.italic = item.Italic();
    n.open = item.Open();

    if (const auto* dest = item.Destination()) {
        if (const auto* ed = dynamic_cast<
                const Aspose::Pdf::Annotations::ExplicitDestination*>(dest)) {
            n.page = ed->PageNumber();
            std::istringstream iss(ed->ToString());  // "<page> <verb> <ops…>"
            std::string page_tok;
            iss >> page_tok >> n.dest_verb;
            double op;
            while (iss >> op) n.dest_ops.push_back(op);
        } else if (const auto* nd = dynamic_cast<
                       const Aspose::Pdf::Annotations::NamedDestination*>(
                       dest)) {
            n.named_dest = nd->Name();
        }
    } else if (const auto* act = item.Action()) {
        if (const auto* uri = dynamic_cast<
                const Aspose::Pdf::Annotations::GoToURIAction*>(act)) {
            n.uri = uri->URI();
        }
    }

    for (int i = 1; i <= item.Count(); ++i)
        n.children.push_back(OutlineNodeFromItem(item[i]));
    return n;
}

namespace {

// NumberingStyle ↔ PDF /S name (PDF §12.4.2). NumberingStyle::None has no /S.
const char* NumberingStyleToName(Aspose::Pdf::NumberingStyle s) {
    switch (s) {
        case Aspose::Pdf::NumberingStyle::NumeralsArabic: return "D";
        case Aspose::Pdf::NumberingStyle::NumeralsRomanUppercase: return "R";
        case Aspose::Pdf::NumberingStyle::NumeralsRomanLowercase: return "r";
        case Aspose::Pdf::NumberingStyle::LettersUppercase: return "A";
        case Aspose::Pdf::NumberingStyle::LettersLowercase: return "a";
        case Aspose::Pdf::NumberingStyle::None: return "";
    }
    return "";
}

Aspose::Pdf::NumberingStyle NumberingStyleFromName(const std::string& s) {
    if (s == "D") return Aspose::Pdf::NumberingStyle::NumeralsArabic;
    if (s == "R") return Aspose::Pdf::NumberingStyle::NumeralsRomanUppercase;
    if (s == "r") return Aspose::Pdf::NumberingStyle::NumeralsRomanLowercase;
    if (s == "A") return Aspose::Pdf::NumberingStyle::LettersUppercase;
    if (s == "a") return Aspose::Pdf::NumberingStyle::LettersLowercase;
    return Aspose::Pdf::NumberingStyle::None;
}

// Walk a /PageLabels number-tree node (flat /Nums array or /Kids subtrees),
// collecting (page-index, label-dict) pairs.
void CollectNumberTree(
        const foundation::objects::Dump& dump,
        const foundation::objects::Dict* node,
        std::vector<std::pair<int, const foundation::objects::Dict*>>& out,
        int depth) {
    if (node == nullptr || depth > 50) return;
    if (const auto* nums = DictGet(*node, "Nums")) {
        if (const auto* arr =
                std::get_if<foundation::objects::Array>(&nums->v)) {
            for (std::size_t i = 0; i + 1 < arr->items.size(); i += 2) {
                const auto* k =
                    std::get_if<std::int64_t>(&arr->items[i].v);
                const auto* d = DerefDict(dump, &arr->items[i + 1]);
                if (k != nullptr && d != nullptr)
                    out.emplace_back(static_cast<int>(*k), d);
            }
        }
    }
    if (const auto* kids = DictGet(*node, "Kids")) {
        if (const auto* arr =
                std::get_if<foundation::objects::Array>(&kids->v)) {
            for (const auto& it : arr->items)
                CollectNumberTree(dump, DerefDict(dump, &it), out, depth + 1);
        }
    }
}

}  // namespace

std::vector<std::byte> Document::AppendPageLabelsUpdate(
        const std::vector<std::byte>& working) const {
    if (!page_labels_ || (page_labels_->ranges_.empty() &&
                          page_labels_->loaded_count_ == 0)) {
        return working;
    }

    auto working_span =
        std::span<const std::byte>(working.data(), working.size());

    foundation::trailer::Trailer trailer_bundle;
    try {
        trailer_bundle = foundation::trailer::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }
    if (trailer_bundle.root_id == 0 || trailer_bundle.size == 0) {
        return working;
    }

    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }

    const auto* catalog = FindObject(dump, trailer_bundle.root_id);
    const auto* catalog_dict =
        catalog ? std::get_if<foundation::objects::Dict>(&catalog->value.v)
                : nullptr;
    if (catalog_dict == nullptr) return working;

    // Build /PageLabels << /Nums [idx0 <<dict0>> idx1 <<dict1>> …] >>.
    foundation::objects::Array nums;
    for (const auto& [start, label] : page_labels_->ranges_) {
        nums.items.push_back(
            foundation::objects::Value{static_cast<std::int64_t>(start)});

        foundation::objects::Dict d;
        const char* style = NumberingStyleToName(label.NumberingStyle());
        if (style[0] != '\0') {
            d.entries.emplace_back(
                "S", foundation::objects::Value{std::string(style)});
        }
        if (label.StartingValue() != 1) {
            d.entries.emplace_back(
                "St", foundation::objects::Value{
                          static_cast<std::int64_t>(label.StartingValue())});
        }
        if (!label.Prefix().empty()) {
            foundation::objects::String p;
            p.bytes = EncodePdfString(label.Prefix());
            p.kind = foundation::objects::StringKind::Literal;
            d.entries.emplace_back("P", foundation::objects::Value{std::move(p)});
        }
        nums.items.push_back(foundation::objects::Value{std::move(d)});
    }

    foundation::objects::Dict page_labels_dict;
    page_labels_dict.entries.emplace_back(
        "Nums", foundation::objects::Value{std::move(nums)});

    foundation::objects::Dict updated_catalog = *catalog_dict;
    for (auto it = updated_catalog.entries.begin();
         it != updated_catalog.entries.end();) {
        if (it->first == "PageLabels")
            it = updated_catalog.entries.erase(it);
        else
            ++it;
    }
    updated_catalog.entries.emplace_back(
        "PageLabels", foundation::objects::Value{std::move(page_labels_dict)});

    foundation::pdf_writer_incremental::DirtyObject d;
    d.id = catalog->id;
    d.generation = catalog->generation;
    d.value.v = std::move(updated_catalog);

    return foundation::pdf_writer_incremental::AppendIncremental(
        working_span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(&d, 1));
}

void Document::LoadPageLabels(PageLabelCollection& coll) {
    auto span = std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::trailer::Trailer trailer_bundle;
    foundation::objects::Dump dump;
    try {
        trailer_bundle = foundation::trailer::Parse(span);
        dump = foundation::objects::Parse(span);
    } catch (const std::exception&) {
        return;
    }
    if (trailer_bundle.root_id == 0) return;
    const auto* catalog = FindObject(dump, trailer_bundle.root_id);
    const auto* catalog_dict =
        catalog ? std::get_if<foundation::objects::Dict>(&catalog->value.v)
                : nullptr;
    if (catalog_dict == nullptr) return;

    const auto* tree = DerefDict(dump, DictGet(*catalog_dict, "PageLabels"));
    if (tree == nullptr) return;

    std::vector<std::pair<int, const foundation::objects::Dict*>> entries;
    CollectNumberTree(dump, tree, entries, 0);

    for (const auto& [start, d] : entries) {
        PageLabel label;
        const std::string s = ReadStrOrName(DictGet(*d, "S"));
        if (!s.empty()) label.NumberingStyle(NumberingStyleFromName(s));
        if (const auto* st = DictGet(*d, "St")) {
            if (const auto* n = std::get_if<std::int64_t>(&st->v))
                label.StartingValue(static_cast<int>(*n));
        }
        const std::string prefix = ReadStrOrName(DictGet(*d, "P"));
        if (!prefix.empty()) label.Prefix(prefix);
        coll.ranges_[start] = label;
    }
    coll.loaded_count_ = coll.ranges_.size();
}

std::vector<std::string> Document::AttachmentNamesInternal() {
    std::vector<std::string> out;
    auto& coll = EmbeddedFiles();
    for (int i = 0; i < coll.Count(); ++i) out.push_back(coll[i].Name());
    return out;
}

bool Document::ExtractAttachmentInternal(const std::string& name,
                                         const std::string& outPath) {
    auto& coll = EmbeddedFiles();
    if (coll.Count() == 0) return false;
    const FileSpecification* fs = nullptr;
    if (name.empty()) {
        fs = &coll[0];
    } else {
        for (int i = 0; i < coll.Count(); ++i) {
            if (coll[i].Name() == name) {
                fs = &coll[i];
                break;
            }
        }
    }
    if (fs == nullptr) return false;
    const std::vector<std::byte>& bytes = fs->content_;
    std::ofstream os(outPath, std::ios::binary);
    if (!os) return false;
    os.write(reinterpret_cast<const char*>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(os);
}

std::vector<std::byte> Document::AppendAcroFormUpdate(
        const std::vector<std::byte>& working) const {
    if (!form_) return working;
    const bool flatten = form_->flatten_requested_;
    if (form_->Count() == 0 && form_->loaded_count_ == 0 && !flatten) {
        return working;
    }

    auto working_span = std::span<const std::byte>(
        working.data(), working.size());

    foundation::trailer::Trailer trailer_bundle;
    try {
        trailer_bundle = foundation::trailer::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }
    if (trailer_bundle.root_id == 0 || trailer_bundle.size == 0) {
        return working;
    }

    foundation::objects::Dump dump;
    try {
        dump = foundation::objects::Parse(working_span);
    } catch (const std::exception&) {
        return working;
    }

    const foundation::objects::IndirectObject* catalog = nullptr;
    for (const auto& obj : dump.objects) {
        if (obj.id == trailer_bundle.root_id) {
            catalog = &obj;
            break;
        }
    }
    if (catalog == nullptr) return working;
    const auto* catalog_dict =
        std::get_if<foundation::objects::Dict>(&catalog->value.v);
    if (catalog_dict == nullptr) return working;

    // Flatten: strip /AcroForm from the catalog and stop. (Widget /Annots
    // are left in place — v1 has no appearance generator to burn values in.)
    if (flatten) {
        if (DictGet(*catalog_dict, "AcroForm") == nullptr) return working;
        foundation::objects::Dict updated_catalog = *catalog_dict;
        for (auto it = updated_catalog.entries.begin();
             it != updated_catalog.entries.end();) {
            if (it->first == "AcroForm")
                it = updated_catalog.entries.erase(it);
            else
                ++it;
        }
        foundation::pdf_writer_incremental::DirtyObject cat_dirty;
        cat_dirty.id = catalog->id;
        cat_dirty.generation = catalog->generation;
        cat_dirty.value.v = std::move(updated_catalog);
        return foundation::pdf_writer_incremental::AppendIncremental(
            working_span,
            std::span<const foundation::pdf_writer_incremental::DirtyObject>(
                &cat_dirty, 1));
    }

    // Resolve any existing /AcroForm (ref or inline) + its current
    // /Fields refs. Loaded fields are preserved by reusing those refs;
    // only newly-added fields are emitted, removed loaded fields are
    // dropped from /Fields, and value-edited loaded fields are re-emitted
    // in place (original object id, patched /V).
    std::uint32_t acroform_id = 0;
    const auto* existing_acro =
        ResolveAcroForm(dump, DictGet(*catalog_dict, "AcroForm"),
                        acroform_id);

    // The set of loaded-field object ids still present in form_->fields_.
    std::set<std::uint32_t> present_loaded;
    for (const auto* fp : form_->fields_) {
        auto it = form_->loaded_ids_.find(fp);
        if (it != form_->loaded_ids_.end())
            present_loaded.insert(it->second);
    }
    const bool removed_loaded =
        present_loaded.size() < form_->loaded_count_;

    // Newly-added (non-loaded) fields, and value-edited loaded fields.
    std::vector<Aspose::Pdf::Forms::Field*> new_fields;
    std::vector<Aspose::Pdf::Forms::Field*> modified_loaded;
    for (auto* fp : form_->fields_) {
        if (fp == nullptr) continue;
        auto lit = form_->loaded_ids_.find(fp);
        if (lit == form_->loaded_ids_.end()) {
            new_fields.push_back(fp);
            continue;
        }
        auto vit = form_->loaded_values_.find(fp);
        if (vit == form_->loaded_values_.end() ||
            vit->second != FieldEffectiveValue(fp)) {
            modified_loaded.push_back(fp);
        }
    }

    const bool fields_changed = !new_fields.empty() || removed_loaded;

    // Nothing changed — leave the existing /AcroForm untouched (lossless).
    if (!fields_changed && modified_loaded.empty()) return working;

    std::uint32_t next_id = trailer_bundle.size;
    for (const auto& o : dump.objects) next_id = std::max(next_id, o.id + 1);

    std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;

    // Re-emit each value-edited loaded field in place: copy its original
    // object dict, patch /V (+ /AS), keep everything else (lossless).
    for (auto* fp : modified_loaded) {
        const std::uint32_t id = form_->loaded_ids_.at(fp);
        const auto* obj = FindObject(dump, id);
        const auto* od =
            obj ? std::get_if<foundation::objects::Dict>(&obj->value.v)
                : nullptr;
        if (od == nullptr) continue;
        foundation::objects::Dict d = *od;
        ApplyFieldValue(d, fp);
        foundation::pdf_writer_incremental::DirtyObject dirty_obj;
        dirty_obj.id = id;
        dirty_obj.generation = obj->generation;
        dirty_obj.value.v = std::move(d);
        dirty.push_back(std::move(dirty_obj));
    }

    // When /Fields itself didn't change (only values edited), the AcroForm
    // dict stays untouched — the patched field objects above suffice.
    if (!fields_changed) {
        return foundation::pdf_writer_incremental::AppendIncremental(
            working_span,
            std::span<const foundation::pdf_writer_incremental::DirtyObject>(
                dirty.data(), dirty.size()));
    }

    // Surviving loaded-field refs, in their original order.
    foundation::objects::Array fields_arr;
    if (existing_acro != nullptr) {
        if (const auto* f = DictGet(*existing_acro, "Fields")) {
            if (const auto* arr =
                    std::get_if<foundation::objects::Array>(&f->v)) {
                for (const auto& it : arr->items) {
                    const auto* ref =
                        std::get_if<foundation::objects::Ref>(&it.v);
                    if (ref && present_loaded.count(ref->id))
                        fields_arr.items.push_back(it);
                }
            }
        }
    }

    // Widgets bound to a page via Add(field, pageNumber): page object id →
    // the field ids to splice into that page's /Annots after the emit loop.
    std::map<std::uint32_t, std::vector<std::uint32_t>> page_widget_refs;

    // ---- Widget appearance-stream support -----------------------------------
    // Acrobat does not regenerate field appearances (NeedAppearances=false), so
    // each widget carries a pre-rendered /AP Form XObject. Bodies are parked in
    // unique_ptrs for stable addresses (Stream.body is a non-owning span).
    std::vector<std::unique_ptr<std::vector<std::byte>>> ap_bodies;
    auto int_v = [](std::int64_t n) {
        foundation::objects::Value v; v.v = n; return v; };
    auto real_v = [](double x) {
        foundation::objects::Value v; v.v = x; return v; };
    auto ref_v = [](std::uint32_t id) {
        foundation::objects::Value v;
        v.v = foundation::objects::Ref{id, 0}; return v; };
    auto dict_v = [](foundation::objects::Dict dd) {
        foundation::objects::Value v; v.v = std::move(dd); return v; };
    auto str_v = [](const std::string& s) {
        foundation::objects::String ps; ps.bytes = EncodePdfString(s);
        ps.kind = foundation::objects::StringKind::Literal;
        foundation::objects::Value v; v.v = std::move(ps); return v; };
    // Emit a /Type /XObject /Subtype /Form appearance stream with a single
    // Type1 font resource; returns its new object id.
    auto emit_ap = [&](double w, double h, const std::string& content,
                       const char* font_key,
                       const char* base_font) -> std::uint32_t {
        foundation::objects::Dict font_inner;
        font_inner.entries.emplace_back("Type", PageNameValue("Font"));
        font_inner.entries.emplace_back("Subtype", PageNameValue("Type1"));
        font_inner.entries.emplace_back("BaseFont", PageNameValue(base_font));
        foundation::objects::Dict fonts;
        fonts.entries.emplace_back(font_key, dict_v(std::move(font_inner)));
        foundation::objects::Dict resources;
        resources.entries.emplace_back("Font", dict_v(std::move(fonts)));
        foundation::objects::Array bbox;
        for (double dv : {0.0, 0.0, w, h}) bbox.items.push_back(real_v(dv));
        foundation::objects::Dict header;
        header.entries.emplace_back("Type", PageNameValue("XObject"));
        header.entries.emplace_back("Subtype", PageNameValue("Form"));
        header.entries.emplace_back("FormType", int_v(1));
        { foundation::objects::Value v; v.v = std::move(bbox);
          header.entries.emplace_back("BBox", std::move(v)); }
        header.entries.emplace_back("Resources", dict_v(std::move(resources)));
        ap_bodies.push_back(std::make_unique<std::vector<std::byte>>(
            reinterpret_cast<const std::byte*>(content.data()),
            reinterpret_cast<const std::byte*>(content.data()) +
                content.size()));
        foundation::objects::Stream st;
        st.header = std::move(header);
        st.body = std::span<const std::byte>(ap_bodies.back()->data(),
                                             ap_bodies.back()->size());
        const std::uint32_t id = next_id++;
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = id; d.generation = 0; d.value.v = std::move(st);
        dirty.push_back(std::move(d));
        return id;
    };
    auto escape_ap = [](const std::string& s) {
        std::string o; o.reserve(s.size());
        for (char c : s) {
            if (c == '(' || c == ')' || c == '\\') o += '\\';
            o += c;
        }
        return o;
    };
    // A 1pt black field border + white background, drawn in widget-local
    // space, so every field is visibly outlined in any viewer (not reliant on
    // the viewer painting /MK chrome).
    auto field_chrome = [](std::ostringstream& cs, double w, double h) {
        cs << "1 1 1 rg 0 0 " << w << ' ' << h << " re f\n"     // white fill
           << "0 G 1 w 0.5 0.5 " << (w - 1.0) << ' ' << (h - 1.0)
           << " re S\n";                                       // black border
    };
    auto text_ap = [&](double w, double h, const std::string& value,
                       const char* font_key, double size) {
        std::ostringstream cs;
        cs << "q\n";
        field_chrome(cs, w, h);
        cs << "0 0 " << w << " " << h << " re W n\n";
        if (!value.empty()) {
            cs << "BT\n/" << font_key << " " << size << " Tf\n0 g\n"
               << "2 " << (h * 0.30) << " Td\n(" << escape_ap(value)
               << ") Tj\nET\n";
        }
        cs << "Q\n";
        return cs.str();
    };
    // Multi-line list-box appearance: border + one option per line, the
    // selected row highlighted.
    auto list_ap = [&](double w, double h,
                       const std::vector<std::string>& options, int selected) {
        std::ostringstream cs;
        cs << "q\n";
        field_chrome(cs, w, h);
        cs << "0 0 " << w << " " << h << " re W n\n";
        const double fs = 11.0, lh = fs * 1.3;
        double y = h - lh + 2.0;
        for (std::size_t i = 0; i < options.size() && y > -lh; ++i) {
            if (static_cast<int>(i) == selected) {
                cs << "0.6 0.75 0.9 rg 1 " << (y - 2.0) << ' ' << (w - 2.0)
                   << ' ' << lh << " re f\n";  // selection highlight
            }
            cs << "BT\n/Helv " << fs << " Tf\n0 g\n2 " << y << " Td\n("
               << escape_ap(options[i]) << ") Tj\nET\n";
            y -= lh;
        }
        cs << "Q\n";
        return cs.str();
    };
    auto check_ap = [&](double w, double h, bool draw) {
        std::ostringstream cs;
        cs << "q\n";
        field_chrome(cs, w, h);  // always show the box outline
        if (draw) {
            // Green vector check mark — drawn with strokes rather than the
            // ZapfDingbats glyph so it renders in any engine.
            cs << "0 0.5 0 RG " << std::max(1.0, std::min(w, h) * 0.13)
               << " w 1 J 1 j " << (w * 0.22) << ' ' << (h * 0.50) << " m "
               << (w * 0.42) << ' ' << (h * 0.27) << " l " << (w * 0.78) << ' '
               << (h * 0.74) << " l S\n";
        }
        cs << "Q\n";
        return cs.str();
    };

    // Resolve a field's 1-based page → page object id (from the page-aware
    // Add overload). Returns 0 when unbound.
    auto field_page_id = [&](Aspose::Pdf::Forms::Field* f) -> std::uint32_t {
        if (!tree_) return 0;
        auto pit = form_->field_pages_.find(f);
        if (pit == form_->field_pages_.end()) return 0;
        const int pn = pit->second;
        if (pn < 1 || static_cast<std::size_t>(pn) > tree_->leaves.size())
            return 0;
        return tree_->leaves[pn - 1].id;
    };
    // A radio dot, drawn in widget-local space: an outer ring always (so the
    // button is visible in any renderer, not just via the widget border), plus
    // a filled inner dot for the selected (on) state. `op` = "S" stroke or "f".
    auto circle_path = [](std::ostringstream& cs, double cx, double cy,
                          double r, const char* op) {
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
    };
    auto radio_ap = [&](double w, double h, bool on) {
        std::ostringstream cs;
        const double cx = w / 2.0, cy = h / 2.0, rad = std::min(w, h) / 2.0;
        cs << "q\n0 g\n0.8 w\n";
        circle_path(cs, cx, cy, rad - 1.0, "S");   // outer ring
        if (on) circle_path(cs, cx, cy, rad * 0.5, "f");  // inner dot
        cs << "Q\n";
        return cs.str();
    };

    // Emit each newly-added field as a combined field+widget object.
    for (auto* fp : new_fields) {
        // Radio group: a parent /Btn field with one Kid widget per option,
        // each carrying on/off appearance states. (A single combined widget
        // cannot represent the per-option buttons.)
        if (auto* radio =
                dynamic_cast<Aspose::Pdf::Forms::RadioButtonField*>(fp);
            radio != nullptr && !radio->option_kids_.empty()) {
            const std::uint32_t page_id = field_page_id(fp);
            // Selected on-state: explicit Value() wins, else Selected() index.
            std::string sel_value = radio->Value();
            if (sel_value.empty()) {
                const int si = radio->Selected();
                if (si >= 0 &&
                    si < static_cast<int>(radio->option_kids_.size()))
                    sel_value = radio->option_kids_[si].OptionName();
            }
            const std::uint32_t parent_id = next_id++;
            foundation::objects::Array kids;
            for (const auto& kid : radio->option_kids_) {
                const std::string state = kid.OptionName();
                const auto& r = kid.Rect();
                const double w = r.URX() - r.LLX(), h = r.URY() - r.LLY();
                const bool is_sel = !sel_value.empty() && state == sel_value;
                foundation::objects::Dict kd;
                kd.entries.emplace_back("Type", PageNameValue("Annot"));
                kd.entries.emplace_back("Subtype", PageNameValue("Widget"));
                kd.entries.emplace_back("Parent", ref_v(parent_id));
                foundation::objects::Array ra;
                for (double dv : {r.LLX(), r.LLY(), r.URX(), r.URY()})
                    ra.items.push_back(real_v(dv));
                { foundation::objects::Value v; v.v = std::move(ra);
                  kd.entries.emplace_back("Rect", std::move(v)); }
                if (page_id != 0)
                    kd.entries.emplace_back("P", ref_v(page_id));
                kd.entries.emplace_back(
                    "AS", PageNameValue((is_sel ? state : std::string("Off"))
                                            .c_str()));
                if (w > 0.0 && h > 0.0) {  // on/off appearance per option
                    const std::uint32_t on_id = emit_ap(
                        w, h, radio_ap(w, h, true), "Helv", "Helvetica");
                    const std::uint32_t off_id = emit_ap(
                        w, h, radio_ap(w, h, false), "Helv", "Helvetica");
                    foundation::objects::Dict n;
                    n.entries.emplace_back(state, ref_v(on_id));
                    n.entries.emplace_back("Off", ref_v(off_id));
                    foundation::objects::Dict ap;
                    ap.entries.emplace_back("N", dict_v(std::move(n)));
                    kd.entries.emplace_back("AP", dict_v(std::move(ap)));
                }
                kd.entries.emplace_back("F", int_v(4));
                foundation::objects::Dict bs;
                bs.entries.emplace_back("W", real_v(1.0));
                bs.entries.emplace_back("S", PageNameValue("S"));
                kd.entries.emplace_back("BS", dict_v(std::move(bs)));
                const std::uint32_t kid_id = next_id++;
                foundation::pdf_writer_incremental::DirtyObject ko;
                ko.id = kid_id; ko.generation = 0; ko.value.v = std::move(kd);
                dirty.push_back(std::move(ko));
                kids.items.push_back(ref_v(kid_id));
                if (page_id != 0) page_widget_refs[page_id].push_back(kid_id);
            }
            foundation::objects::Dict pd;
            pd.entries.emplace_back("FT", PageNameValue("Btn"));
            pd.entries.emplace_back("Ff", int_v(static_cast<std::int64_t>(1)
                                                << 15));  // Radio
            if (!radio->PartialName().empty())
                pd.entries.emplace_back("T", str_v(radio->PartialName()));
            pd.entries.emplace_back(
                "V", PageNameValue(sel_value.empty() ? "Off"
                                                     : sel_value.c_str()));
            pd.entries.emplace_back("DV", PageNameValue("Off"));
            // /Opt — option export values, so load-on-open reconstructs the
            // RadioButtonField's ChoiceField Options collection.
            {
                auto& opts = radio->Options();
                if (opts.Count() > 0) {
                    foundation::objects::Array oarr;
                    for (int oi = 0; oi < opts.Count(); ++oi)
                        oarr.items.push_back(str_v(opts[oi].Value()));
                    foundation::objects::Value v; v.v = std::move(oarr);
                    pd.entries.emplace_back("Opt", std::move(v));
                }
            }
            { foundation::objects::Value v; v.v = std::move(kids);
              pd.entries.emplace_back("Kids", std::move(v)); }
            foundation::pdf_writer_incremental::DirtyObject po;
            po.id = parent_id; po.generation = 0; po.value.v = std::move(pd);
            dirty.push_back(std::move(po));
            fields_arr.items.push_back(ref_v(parent_id));
            continue;
        }

        auto& field = *fp;
        const auto kind = FieldKindOf(field);

        std::int64_t ff = kind.ff;
        if (field.ReadOnly())   ff |= 1LL << 0;
        if (field.Required())   ff |= 1LL << 1;
        if (!field.Exportable()) ff |= 1LL << 2;

        foundation::objects::Dict d;
        d.entries.emplace_back("Type", PageNameValue("Annot"));
        d.entries.emplace_back("Subtype", PageNameValue("Widget"));
        {
            foundation::objects::Value v;
            v.v = kind.ft;
            d.entries.emplace_back("FT", std::move(v));
        }
        if (!field.PartialName().empty()) {
            foundation::objects::String pdf_str;
            pdf_str.bytes = EncodePdfString(field.PartialName());
            pdf_str.kind = foundation::objects::StringKind::Literal;
            foundation::objects::Value v;
            v.v = std::move(pdf_str);
            d.entries.emplace_back("T", std::move(v));
        }

        // /V (+ /AS) — concrete-aware (checkbox name / choice value / text).
        ApplyFieldValue(d, fp);

        // /Opt — choice-field option list.
        if (auto* ch =
                dynamic_cast<Aspose::Pdf::Forms::ChoiceField*>(fp)) {
            auto& opts = ch->Options();
            if (opts.Count() > 0) {
                foundation::objects::Array oarr;
                for (int i = 0; i < opts.Count(); ++i) {
                    foundation::objects::String s;
                    s.bytes = EncodePdfString(opts[i].Value());
                    s.kind = foundation::objects::StringKind::Literal;
                    foundation::objects::Value v;
                    v.v = std::move(s);
                    oarr.items.push_back(std::move(v));
                }
                foundation::objects::Value v;
                v.v = std::move(oarr);
                d.entries.emplace_back("Opt", std::move(v));
            }
        }

        if (ff != 0) {
            foundation::objects::Value v;
            v.v = ff;
            d.entries.emplace_back("Ff", std::move(v));
        }
        {
            const auto& r = field.Rect();
            foundation::objects::Array arr;
            for (double d_val : {r.LLX(), r.LLY(), r.URX(), r.URY()}) {
                foundation::objects::Value v;
                v.v = d_val;
                arr.items.push_back(std::move(v));
            }
            foundation::objects::Value v;
            v.v = std::move(arr);
            d.entries.emplace_back("Rect", std::move(v));
        }

        // /P parent-page ref + page-/Annots linkage, when the field was added
        // with an explicit page via Add(field, pageNumber).
        std::uint32_t widget_page_id = 0;
        if (tree_) {
            if (auto pit = form_->field_pages_.find(fp);
                    pit != form_->field_pages_.end()) {
                const int page_num = pit->second;
                if (page_num >= 1 && static_cast<std::size_t>(page_num) <=
                                         tree_->leaves.size()) {
                    widget_page_id = tree_->leaves[page_num - 1].id;
                    foundation::objects::Value pv;
                    pv.v = foundation::objects::Ref{widget_page_id, 0};
                    d.entries.emplace_back("P", std::move(pv));
                }
            }
        }

        // Visual appearance: /DA, Print flag, border + background, and a
        // pre-rendered /AP so viewers (e.g. Acrobat) draw the field.
        {
            const bool is_checkbox =
                dynamic_cast<Aspose::Pdf::Forms::CheckboxField*>(fp) != nullptr;
            const auto& rr = field.Rect();
            const double w = rr.URX() - rr.LLX();
            const double h = rr.URY() - rr.LLY();
            d.entries.emplace_back(
                "DA", str_v(is_checkbox ? "/ZaDb 0 Tf 0 g" : "/Helv 12 Tf 0 g"));
            d.entries.emplace_back("F", int_v(4));  // Print
            if (w > 0.0 && h > 0.0) {
                foundation::objects::Dict ap;
                if (is_checkbox) {
                    const bool on =
                        dynamic_cast<Aspose::Pdf::Forms::CheckboxField*>(fp)
                            ->Checked();
                    const std::uint32_t yes =
                        emit_ap(w, h, check_ap(w, h, true), "ZaDb",
                                "ZapfDingbats");
                    const std::uint32_t off =
                        emit_ap(w, h, check_ap(w, h, false), "ZaDb",
                                "ZapfDingbats");
                    foundation::objects::Dict n;
                    n.entries.emplace_back("Yes", ref_v(yes));
                    n.entries.emplace_back("Off", ref_v(off));
                    ap.entries.emplace_back("N", dict_v(std::move(n)));
                    (void)on;
                } else if (auto* lb =
                               dynamic_cast<Aspose::Pdf::Forms::ListBoxField*>(
                                   fp)) {
                    // List box: render the options, highlighting the selection.
                    auto& opts = lb->Options();
                    std::vector<std::string> names;
                    for (int oi = 0; oi < opts.Count(); ++oi)
                        names.push_back(opts[oi].Value());
                    const std::uint32_t apn = emit_ap(
                        w, h, list_ap(w, h, names, lb->Selected()), "Helv",
                        "Helvetica");
                    ap.entries.emplace_back("N", ref_v(apn));
                } else if (auto* btn =
                               dynamic_cast<Aspose::Pdf::Forms::ButtonField*>(
                                   fp)) {
                    // Push button: filled box + centred white caption.
                    const std::string cap = btn->NormalCaption().empty()
                                                ? std::string("Button")
                                                : btn->NormalCaption();
                    std::ostringstream bc;
                    bc << "q 0.18 0.20 0.45 rg 0 0 " << w << ' ' << h
                       << " re f 1 1 1 rg BT /Helv 13 Tf "
                       << (w / 2.0 - cap.size() * 3.6) << ' ' << (h / 2.0 - 5.0)
                       << " Td (";
                    for (char c : cap) {
                        if (c == '(' || c == ')' || c == '\\') bc << '\\';
                        bc << c;
                    }
                    bc << ") Tj ET Q\n";
                    const std::uint32_t apn =
                        emit_ap(w, h, bc.str(), "Helv", "Helvetica");
                    ap.entries.emplace_back("N", ref_v(apn));
                } else {
                    std::string val = fp->Value();
                    if (auto* ch =
                            dynamic_cast<Aspose::Pdf::Forms::ChoiceField*>(fp))
                        val = ch->Value();
                    const std::uint32_t apn =
                        emit_ap(w, h, text_ap(w, h, val, "Helv", 12.0), "Helv",
                                "Helvetica");
                    ap.entries.emplace_back("N", ref_v(apn));
                }
                d.entries.emplace_back("AP", dict_v(std::move(ap)));
                foundation::objects::Dict bs;
                bs.entries.emplace_back("W", real_v(1.0));
                bs.entries.emplace_back("S", PageNameValue("S"));
                d.entries.emplace_back("BS", dict_v(std::move(bs)));
                foundation::objects::Dict mk;
                foundation::objects::Array bg;
                for (int i = 0; i < 3; ++i) bg.items.push_back(real_v(1.0));
                { foundation::objects::Value v; v.v = std::move(bg);
                  mk.entries.emplace_back("BG", std::move(v)); }
                d.entries.emplace_back("MK", dict_v(std::move(mk)));
            }
        }

        const std::uint32_t field_id = next_id++;
        foundation::pdf_writer_incremental::DirtyObject obj;
        obj.id = field_id;
        obj.generation = 0;
        obj.value.v = std::move(d);
        dirty.push_back(std::move(obj));
        if (widget_page_id != 0)
            page_widget_refs[widget_page_id].push_back(field_id);

        foundation::objects::Value rv;
        rv.v = foundation::objects::Ref{field_id, 0};
        fields_arr.items.push_back(std::move(rv));
    }

    // Build the updated /AcroForm dict — merge over the existing one,
    // replacing /Fields and refreshing /SigFlags.
    foundation::objects::Dict acroform_dict =
        existing_acro ? *existing_acro : foundation::objects::Dict{};
    for (auto it = acroform_dict.entries.begin();
         it != acroform_dict.entries.end();) {
        if (it->first == "Fields" || it->first == "SigFlags")
            it = acroform_dict.entries.erase(it);
        else
            ++it;
    }
    {
        foundation::objects::Value v;
        v.v = std::move(fields_arr);
        acroform_dict.entries.emplace_back("Fields", std::move(v));
    }
    // Default appearance + resources so a viewer can resolve the widgets'
    // /DA fonts; added once if the AcroForm doesn't already carry them.
    auto acro_has = [&](const char* k) {
        for (const auto& e : acroform_dict.entries)
            if (e.first == k) return true;
        return false;
    };
    if (!acro_has("DR")) {
        auto base_font = [](const char* key, const char* base) {
            foundation::objects::Dict f;
            f.entries.emplace_back("Type", PageNameValue("Font"));
            f.entries.emplace_back("Subtype", PageNameValue("Type1"));
            f.entries.emplace_back("BaseFont", PageNameValue(base));
            return std::make_pair(std::string(key), std::move(f));
        };
        foundation::objects::Dict fonts;
        { auto [k, f] = base_font("Helv", "Helvetica");
          foundation::objects::Value v; v.v = std::move(f);
          fonts.entries.emplace_back(k, std::move(v)); }
        { auto [k, f] = base_font("ZaDb", "ZapfDingbats");
          foundation::objects::Value v; v.v = std::move(f);
          fonts.entries.emplace_back(k, std::move(v)); }
        foundation::objects::Dict dr;
        { foundation::objects::Value v; v.v = std::move(fonts);
          dr.entries.emplace_back("Font", std::move(v)); }
        foundation::objects::Value v; v.v = std::move(dr);
        acroform_dict.entries.emplace_back("DR", std::move(v));
    }
    if (!acro_has("DA")) {
        foundation::objects::String ps;
        ps.bytes = EncodePdfString("/Helv 12 Tf 0 g");
        ps.kind = foundation::objects::StringKind::Literal;
        foundation::objects::Value v; v.v = std::move(ps);
        acroform_dict.entries.emplace_back("DA", std::move(v));
    }
    if (form_->SignaturesExist() || form_->SignaturesAppendOnly()) {
        std::int64_t sf = 0;
        if (form_->SignaturesExist()) sf |= 1;
        if (form_->SignaturesAppendOnly()) sf |= 2;
        foundation::objects::Value v;
        v.v = sf;
        acroform_dict.entries.emplace_back("SigFlags", std::move(v));
    }

    if (acroform_id != 0) {
        // Existing AcroForm is its own object — update it in place.
        const auto* acro_obj = FindObject(dump, acroform_id);
        foundation::pdf_writer_incremental::DirtyObject d;
        d.id = acroform_id;
        d.generation = acro_obj ? acro_obj->generation : 0;
        d.value.v = std::move(acroform_dict);
        dirty.push_back(std::move(d));
    } else {
        // Inline (or new) AcroForm — write it into the catalog.
        foundation::objects::Dict updated_catalog = *catalog_dict;
        for (auto it = updated_catalog.entries.begin();
             it != updated_catalog.entries.end();) {
            if (it->first == "AcroForm")
                it = updated_catalog.entries.erase(it);
            else
                ++it;
        }
        foundation::objects::Value acroform_val;
        acroform_val.v = std::move(acroform_dict);
        updated_catalog.entries.emplace_back("AcroForm",
                                             std::move(acroform_val));
        foundation::pdf_writer_incremental::DirtyObject cat_dirty;
        cat_dirty.id = catalog->id;
        cat_dirty.generation = catalog->generation;
        cat_dirty.value.v = std::move(updated_catalog);
        dirty.push_back(std::move(cat_dirty));
    }

    // Splice page-bound widget refs into each target page's /Annots. A widget
    // referenced only from /AcroForm /Fields is not drawn; it must also be on
    // its page. Existing /Annots (markup annotations added earlier in Save)
    // are preserved and the widgets appended.
    for (const auto& [page_id, refs] : page_widget_refs) {
        const auto* page_obj = FindObject(dump, page_id);
        const auto* page_dict =
            page_obj ? std::get_if<foundation::objects::Dict>(
                           &page_obj->value.v)
                     : nullptr;
        if (page_dict == nullptr) continue;
        foundation::objects::Dict updated = *page_dict;
        foundation::objects::Array annots;
        for (auto it = updated.entries.begin(); it != updated.entries.end();) {
            if (it->first == "Annots") {
                if (auto* a = std::get_if<foundation::objects::Array>(
                        &it->second.v))
                    annots = *a;
                it = updated.entries.erase(it);
            } else {
                ++it;
            }
        }
        for (std::uint32_t rid : refs) {
            foundation::objects::Value v;
            v.v = foundation::objects::Ref{rid, 0};
            annots.items.push_back(std::move(v));
        }
        foundation::objects::Value av;
        av.v = std::move(annots);
        updated.entries.emplace_back("Annots", std::move(av));
        foundation::pdf_writer_incremental::DirtyObject pd;
        pd.id = page_id;
        pd.generation = page_obj->generation;
        pd.value.v = std::move(updated);
        dirty.push_back(std::move(pd));
    }

    return foundation::pdf_writer_incremental::AppendIncremental(
        working_span,
        std::span<const foundation::pdf_writer_incremental::DirtyObject>(
            dirty.data(), dirty.size()));
}

// ===== Digital signatures ====================================================

namespace {

foundation::objects::Value SigNameVal(const char* name) {
    foundation::objects::Value v;
    v.v = std::string(name);
    return v;
}

foundation::objects::Value SigLiteralVal(const std::string& s) {
    foundation::objects::String ps;
    ps.bytes = EncodePdfString(s);
    ps.kind = foundation::objects::StringKind::Literal;
    foundation::objects::Value v;
    v.v = std::move(ps);
    return v;
}

foundation::objects::Value SigIntArray(const std::array<std::int64_t, 4>& a) {
    foundation::objects::Array arr;
    for (std::int64_t n : a) {
        foundation::objects::Value v;
        v.v = n;
        arr.items.push_back(std::move(v));
    }
    foundation::objects::Value v;
    v.v = std::move(arr);
    return v;
}

foundation::objects::Value SigRefVal(std::uint32_t id) {
    foundation::objects::Value v;
    v.v = foundation::objects::Ref{id, 0};
    return v;
}

// Resolve an AcroForm Value (Ref or inline Dict) to its dict, recording the
// indirect id (0 when inline / absent).
std::string DecodePdfStr(const foundation::objects::Value* v) {
    if (v == nullptr) return {};
    if (const auto* s = std::get_if<foundation::objects::String>(&v->v)) {
        std::string out;
        out.reserve(s->bytes.size());
        for (auto b : s->bytes) out.push_back(static_cast<char>(b));
        return out;
    }
    return {};
}

}  // namespace

void Document::SignInternal(const SignatureRequest& req) {
    auto base_span =
        std::span<const std::byte>(bytes_.data(), bytes_.size());
    const auto trailer_bundle = foundation::trailer::Parse(base_span);
    if (trailer_bundle.root_id == 0) {
        throw std::runtime_error(
            "Aspose::Pdf::Document: cannot sign a document without a catalog");
    }
    const auto dump = foundation::objects::Parse(base_span);
    const auto* catalog = FindObject(dump, trailer_bundle.root_id);
    const auto* catalog_dict =
        catalog ? std::get_if<foundation::objects::Dict>(&catalog->value.v)
                : nullptr;
    if (catalog_dict == nullptr) {
        throw std::runtime_error(
            "Aspose::Pdf::Document: catalog is not a dictionary");
    }
    if (tree_->leaves.empty()) {
        throw std::runtime_error(
            "Aspose::Pdf::Document: cannot sign a document with no pages");
    }

    // Resolve any existing AcroForm and its /Fields.
    std::uint32_t acroform_id = 0;
    const auto* acro_entry = DictGet(*catalog_dict, "AcroForm");
    const auto* existing_acro =
        ResolveAcroForm(dump, acro_entry, acroform_id);
    foundation::objects::Array existing_fields;
    if (existing_acro != nullptr) {
        if (const auto* f = DictGet(*existing_acro, "Fields")) {
            if (const auto* arr =
                    std::get_if<foundation::objects::Array>(&f->v)) {
                existing_fields = *arr;
            }
        }
    }

    // Allocate ids for the /Sig value dict and the signature field/widget.
    std::uint32_t max_id = trailer_bundle.size;
    for (const auto& o : dump.objects) max_id = std::max(max_id, o.id + 1);
    const std::uint32_t sig_value_id = max_id++;
    const std::uint32_t field_id = max_id++;
    const std::uint32_t page_id = tree_->leaves[0].id;
    const auto* page_obj = FindObject(dump, page_id);
    const auto* page_dict_in =
        page_obj ? std::get_if<foundation::objects::Dict>(&page_obj->value.v)
                 : nullptr;

    // Builds the full dirty list for a given /ByteRange. Rebuilt each
    // fixed-point iteration because the /Sig dict's /ByteRange widths shift
    // the /Contents offset.
    auto build_dirty =
        [&](const std::array<std::int64_t, 4>& byte_range) {
            std::vector<foundation::pdf_writer_incremental::DirtyObject> dirty;

            // /Sig value dict.
            foundation::objects::Dict sig;
            sig.entries.emplace_back("Type", SigNameVal("Sig"));
            sig.entries.emplace_back("Filter", SigNameVal("Adobe.PPKLite"));
            sig.entries.emplace_back("SubFilter",
                                     SigNameVal("adbe.pkcs7.detached"));
            if (!req.name.empty())
                sig.entries.emplace_back("Name", SigLiteralVal(req.name));
            if (!req.reason.empty())
                sig.entries.emplace_back("Reason", SigLiteralVal(req.reason));
            if (!req.location.empty())
                sig.entries.emplace_back("Location",
                                         SigLiteralVal(req.location));
            if (!req.contact.empty())
                sig.entries.emplace_back("ContactInfo",
                                         SigLiteralVal(req.contact));
            if (!req.date_pdf.empty())
                sig.entries.emplace_back("M", SigLiteralVal(req.date_pdf));
            sig.entries.emplace_back("ByteRange", SigIntArray(byte_range));
            {
                foundation::objects::String contents;
                contents.bytes.assign(req.contents_capacity, std::byte{0});
                contents.kind = foundation::objects::StringKind::Hex;
                foundation::objects::Value v;
                v.v = std::move(contents);
                sig.entries.emplace_back("Contents", std::move(v));
            }
            {
                foundation::pdf_writer_incremental::DirtyObject d;
                d.id = sig_value_id;
                d.generation = 0;
                d.value.v = std::move(sig);
                dirty.push_back(std::move(d));
            }

            // Signature field + widget (combined object).
            foundation::objects::Dict field;
            field.entries.emplace_back("Type", SigNameVal("Annot"));
            field.entries.emplace_back("Subtype", SigNameVal("Widget"));
            field.entries.emplace_back("FT", SigNameVal("Sig"));
            field.entries.emplace_back("T", SigLiteralVal(req.field_name));
            field.entries.emplace_back("V", SigRefVal(sig_value_id));
            field.entries.emplace_back("P", SigRefVal(page_id));
            field.entries.emplace_back("F", [] {
                foundation::objects::Value v;
                v.v = static_cast<std::int64_t>(132);  // Print | Locked
                return v;
            }());
            field.entries.emplace_back("Rect", SigIntArray({0, 0, 0, 0}));
            {
                foundation::pdf_writer_incremental::DirtyObject d;
                d.id = field_id;
                d.generation = 0;
                d.value.v = std::move(field);
                dirty.push_back(std::move(d));
            }

            // Merge the field ref into page 1 /Annots.
            if (page_dict_in != nullptr) {
                foundation::objects::Dict page = *page_dict_in;
                foundation::objects::Array annots;
                if (const auto* a = DictGet(page, "Annots")) {
                    if (const auto* arr =
                            std::get_if<foundation::objects::Array>(&a->v)) {
                        annots = *arr;
                    }
                }
                annots.items.push_back(SigRefVal(field_id));
                for (auto it = page.entries.begin();
                     it != page.entries.end();) {
                    if (it->first == "Annots")
                        it = page.entries.erase(it);
                    else
                        ++it;
                }
                foundation::objects::Value av;
                av.v = std::move(annots);
                page.entries.emplace_back("Annots", std::move(av));
                foundation::pdf_writer_incremental::DirtyObject d;
                d.id = page_id;
                d.generation = page_obj->generation;
                d.value.v = std::move(page);
                dirty.push_back(std::move(d));
            }

            // AcroForm: /Fields (existing + new) + /SigFlags 3.
            foundation::objects::Dict acro =
                existing_acro ? *existing_acro : foundation::objects::Dict{};
            foundation::objects::Array fields = existing_fields;
            fields.items.push_back(SigRefVal(field_id));
            for (auto it = acro.entries.begin(); it != acro.entries.end();) {
                if (it->first == "Fields" || it->first == "SigFlags")
                    it = acro.entries.erase(it);
                else
                    ++it;
            }
            {
                foundation::objects::Value fv;
                fv.v = std::move(fields);
                acro.entries.emplace_back("Fields", std::move(fv));
                foundation::objects::Value sf;
                sf.v = static_cast<std::int64_t>(3);  // SignaturesExist|AppendOnly
                acro.entries.emplace_back("SigFlags", std::move(sf));
            }

            if (acroform_id != 0) {
                // Existing AcroForm is its own indirect object: update it; the
                // catalog already references it.
                const auto* acro_obj = FindObject(dump, acroform_id);
                foundation::pdf_writer_incremental::DirtyObject d;
                d.id = acroform_id;
                d.generation = acro_obj ? acro_obj->generation : 0;
                d.value.v = std::move(acro);
                dirty.push_back(std::move(d));
            } else {
                // Inline (or new) AcroForm: write it into the catalog.
                foundation::objects::Dict cat = *catalog_dict;
                for (auto it = cat.entries.begin(); it != cat.entries.end();) {
                    if (it->first == "AcroForm")
                        it = cat.entries.erase(it);
                    else
                        ++it;
                }
                foundation::objects::Value av;
                av.v = std::move(acro);
                cat.entries.emplace_back("AcroForm", std::move(av));
                foundation::pdf_writer_incremental::DirtyObject d;
                d.id = catalog->id;
                d.generation = catalog->generation;
                d.value.v = std::move(cat);
                dirty.push_back(std::move(d));
            }
            return dirty;
        };

    // Fixed-point: converge /ByteRange against the serialised /Contents gap.
    static constexpr char kNeedle[] = "/Contents <";
    std::array<std::int64_t, 4> br{0, 0, 0, 0};
    std::vector<std::byte> out;
    std::size_t open_pos = 0, close_pos = 0;
    for (int iter = 0; iter < 16; ++iter) {
        auto dirty = build_dirty(br);
        out = foundation::pdf_writer_incremental::AppendIncremental(
            base_span,
            std::span<const foundation::pdf_writer_incremental::DirtyObject>(
                dirty.data(), dirty.size()));

        // Locate "/Contents <" in the appended region.
        std::size_t found = std::string::npos;
        const std::size_t needle_len = std::strlen(kNeedle);
        for (std::size_t i = bytes_.size();
             i + needle_len <= out.size(); ++i) {
            if (std::memcmp(out.data() + i, kNeedle, needle_len) == 0) {
                found = i;
                break;
            }
        }
        if (found == std::string::npos) {
            throw std::runtime_error(
                "Aspose::Pdf::Document: signature /Contents not found");
        }
        open_pos = found + needle_len - 1;  // index of '<'
        close_pos = open_pos;
        while (close_pos < out.size() &&
               static_cast<char>(out[close_pos]) != '>') {
            ++close_pos;
        }
        if (close_pos >= out.size()) {
            throw std::runtime_error(
                "Aspose::Pdf::Document: signature /Contents unterminated");
        }
        std::array<std::int64_t, 4> next{
            0, static_cast<std::int64_t>(open_pos),
            static_cast<std::int64_t>(close_pos + 1),
            static_cast<std::int64_t>(out.size() - (close_pos + 1))};
        if (next == br) break;
        br = next;
    }

    // Phase 2: digest the signed byte ranges, build the PKCS#7, and patch
    // it (hex) into the /Contents gap in place.
    std::vector<std::byte> signed_content;
    signed_content.reserve(open_pos + (out.size() - (close_pos + 1)));
    signed_content.insert(signed_content.end(), out.begin(),
                          out.begin() + static_cast<std::ptrdiff_t>(open_pos));
    signed_content.insert(
        signed_content.end(),
        out.begin() + static_cast<std::ptrdiff_t>(close_pos + 1), out.end());

    const auto pkcs7 = foundation::pkcs7::SignDetached(
        std::span<const std::byte>(signed_content.data(),
                                   signed_content.size()),
        req.signing_time_utc);

    const std::size_t gap = close_pos - (open_pos + 1);  // hex digits available
    if (pkcs7.size() * 2 > gap) {
        throw std::runtime_error(
            "Aspose::Pdf::Document: PKCS#7 blob exceeds /Contents capacity");
    }
    static const char* kHex = "0123456789ABCDEF";
    std::size_t w = open_pos + 1;
    for (auto b : pkcs7) {
        const auto v = static_cast<std::uint8_t>(b);
        out[w++] = static_cast<std::byte>(kHex[v >> 4]);
        out[w++] = static_cast<std::byte>(kHex[v & 0x0F]);
    }
    while (w < close_pos) out[w++] = static_cast<std::byte>('0');

    bytes_ = std::move(out);
    tree_ = std::make_unique<foundation::pages_tree::Tree>(
        foundation::pages_tree::Parse(
            std::span<const std::byte>(bytes_.data(), bytes_.size())));
    page_annotations_.clear();
}

std::vector<Document::SignatureInfo>
Document::ReadSignaturesInternal() const {
    std::vector<SignatureInfo> out;
    auto base_span =
        std::span<const std::byte>(bytes_.data(), bytes_.size());
    foundation::trailer::Trailer trailer_bundle;
    foundation::objects::Dump dump;
    try {
        trailer_bundle = foundation::trailer::Parse(base_span);
        dump = foundation::objects::Parse(base_span);
    } catch (const std::exception&) {
        return out;
    }
    if (trailer_bundle.root_id == 0) return out;
    const auto* catalog = FindObject(dump, trailer_bundle.root_id);
    const auto* catalog_dict =
        catalog ? std::get_if<foundation::objects::Dict>(&catalog->value.v)
                : nullptr;
    if (catalog_dict == nullptr) return out;

    std::uint32_t acroform_id = 0;
    const auto* acro =
        ResolveAcroForm(dump, DictGet(*catalog_dict, "AcroForm"), acroform_id);
    if (acro == nullptr) return out;
    const auto* fields = DictGet(*acro, "Fields");
    const auto* fields_arr =
        fields ? std::get_if<foundation::objects::Array>(&fields->v) : nullptr;
    if (fields_arr == nullptr) return out;

    for (const auto& item : fields_arr->items) {
        const auto* ref = std::get_if<foundation::objects::Ref>(&item.v);
        if (ref == nullptr) continue;
        const auto* fobj = FindObject(dump, ref->id);
        const auto* fdict =
            fobj ? std::get_if<foundation::objects::Dict>(&fobj->value.v)
                 : nullptr;
        if (fdict == nullptr) continue;
        const auto* ft = DictGet(*fdict, "FT");
        const auto* ft_name = ft ? std::get_if<std::string>(&ft->v) : nullptr;
        if (ft_name == nullptr || *ft_name != "Sig") continue;

        SignatureInfo info;
        info.field_name = DecodePdfStr(DictGet(*fdict, "T"));

        const auto* vref =
            DictGet(*fdict, "V")
                ? std::get_if<foundation::objects::Ref>(&DictGet(*fdict, "V")->v)
                : nullptr;
        if (vref != nullptr) {
            const auto* sobj = FindObject(dump, vref->id);
            const auto* sdict =
                sobj ? std::get_if<foundation::objects::Dict>(&sobj->value.v)
                     : nullptr;
            if (sdict != nullptr) {
                info.has_value = true;
                info.name = DecodePdfStr(DictGet(*sdict, "Name"));
                info.reason = DecodePdfStr(DictGet(*sdict, "Reason"));
                info.location = DecodePdfStr(DictGet(*sdict, "Location"));
                info.contact = DecodePdfStr(DictGet(*sdict, "ContactInfo"));
                if (const auto* br = DictGet(*sdict, "ByteRange")) {
                    if (const auto* arr =
                            std::get_if<foundation::objects::Array>(&br->v)) {
                        for (const auto& e : arr->items) {
                            if (const auto* n =
                                    std::get_if<std::int64_t>(&e.v)) {
                                info.byte_range.push_back(
                                    static_cast<int>(*n));
                            }
                        }
                    }
                }
                if (info.byte_range.size() == 4 &&
                    info.byte_range[0] == 0) {
                    const long long end =
                        static_cast<long long>(info.byte_range[2]) +
                        info.byte_range[3];
                    info.covers_whole =
                        end == static_cast<long long>(bytes_.size());
                }
            }
        }
        out.push_back(std::move(info));
    }
    return out;
}

}  // namespace Aspose::Pdf
