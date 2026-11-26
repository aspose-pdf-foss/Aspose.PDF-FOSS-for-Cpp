// Build a multi-page "Feature Showcase" PDF *from scratch* with the
// Aspose::Pdf:: creation surface — the mirror image of the other
// examples here, which all open an existing file. One program that
// exercises every creation capability the library ships, laid out as a
// small brochure.
//
//   usage: 12_create_features <output.pdf>
//
// Pages produced (one capability family each):
//   1  Cover + clickable bookmark outline (document navigation)
//   2  Contents — a table of contents with dot leaders + page numbers
//   3  Text: Standard-14 font grid, word-wrapped paragraph, watermark
//   4  Image embedding (the bundled examples/pdflib.png, read as a stream)
//   5  AcroForm fields: text / checkbox / radio group / combo / list / button
//   6  Annotation gallery: markup, figures, link, ink, stamp, attachment
//   7  Tables: a restaurant bill with a styled header and a spanned total row
//   8  Vector graphics: gradients, translucent fills, dashes, rounded corners
//   9  A bar chart drawn from gradient bars (A4 landscape)
//  10  A pre-filled AcroForm field and a FloatingBox call-out
//
// Design: a pastel "lavender -> mint" palette with an indigo accent and a
// coral highlight. Every page gets a gradient Page::Background, soft
// translucent circles and a white rounded "card" holding the content.
// Those decorations are Graph shapes with a negative ZIndex, so they are
// painted UNDER the page text; shapes with the default ZIndex 0 (the chart
// bars, the vector-graphics demo) paint over it. Fills are flat colours,
// translucent colours (the alpha of Color::FromArgb) or gradients
// (Color::PatternColorSpace with Drawing::GradientAxialShading /
// GradientRadialShading).
//
// Lifetime rule that shapes this file: Paragraphs/Annotations/Form/
// Artifacts/Outlines all store the object *by reference*, so every
// Table, Graph, annotation, field, watermark and outline item must
// outlive Save(). They are parked in an Arena (below) for exactly
// that reason; Pages live in a std::deque for stable addresses.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_info.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/color.hpp>

#include <aspose/pdf/text_builder.hpp>
#include <aspose/pdf/text_fragment.hpp>
#include <aspose/pdf/text_fragment_state.hpp>
#include <aspose/pdf/text_paragraph.hpp>
#include <aspose/pdf/text_state.hpp>
#include <aspose/pdf/font.hpp>
#include <aspose/pdf/font_repository.hpp>
#include <aspose/pdf/position.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/vertical_alignment.hpp>
#include <aspose/pdf/margin_info.hpp>

#include <aspose/pdf/paragraphs.hpp>
#include <aspose/pdf/table.hpp>
#include <aspose/pdf/rows.hpp>
#include <aspose/pdf/row.hpp>
#include <aspose/pdf/cells.hpp>
#include <aspose/pdf/cell.hpp>
#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/border_side.hpp>
#include <aspose/pdf/floating_box.hpp>

#include <aspose/pdf/drawing/graph.hpp>
#include <aspose/pdf/drawing/line.hpp>
#include <aspose/pdf/drawing/rectangle.hpp>
#include <aspose/pdf/drawing/circle.hpp>
#include <aspose/pdf/drawing/ellipse.hpp>
#include <aspose/pdf/drawing/gradient_axial_shading.hpp>
#include <aspose/pdf/drawing/gradient_radial_shading.hpp>
#include <aspose/pdf/graph_info.hpp>

#include <aspose/pdf/watermark_artifact.hpp>
#include <aspose/pdf/artifact_collection.hpp>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/highlight_annotation.hpp>
#include <aspose/pdf/annotations/underline_annotation.hpp>
#include <aspose/pdf/annotations/squiggly_annotation.hpp>
#include <aspose/pdf/annotations/strike_out_annotation.hpp>
#include <aspose/pdf/annotations/square_annotation.hpp>
#include <aspose/pdf/annotations/circle_annotation.hpp>
#include <aspose/pdf/annotations/line_annotation.hpp>
#include <aspose/pdf/annotations/ink_annotation.hpp>
#include <aspose/pdf/annotations/text_annotation.hpp>
#include <aspose/pdf/annotations/free_text_annotation.hpp>
#include <aspose/pdf/annotations/link_annotation.hpp>
#include <aspose/pdf/annotations/stamp_annotation.hpp>
#include <aspose/pdf/annotations/file_attachment_annotation.hpp>
#include <aspose/pdf/annotations/default_appearance.hpp>
#include <aspose/pdf/annotations/go_to_action.hpp>
#include <aspose/pdf/annotations/go_to_uri_action.hpp>
#include <aspose/pdf/annotations/xyz_explicit_destination.hpp>
#include <aspose/pdf/annotations/stamp_icon.hpp>
#include <aspose/pdf/annotations/text_icon.hpp>
#include <aspose/pdf/annotations/file_icon.hpp>

#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/forms/checkbox_field.hpp>
#include <aspose/pdf/forms/radio_button_field.hpp>
#include <aspose/pdf/forms/combo_box_field.hpp>
#include <aspose/pdf/forms/list_box_field.hpp>
#include <aspose/pdf/forms/button_field.hpp>

#include <aspose/pdf/outline_collection.hpp>
#include <aspose/pdf/outline_item_collection.hpp>
#include <aspose/pdf/file_specification.hpp>
#include <aspose/pdf/embedded_file_collection.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace pdf = Aspose::Pdf;
namespace txt = Aspose::Pdf::Text;
namespace draw = Aspose::Pdf::Drawing;
namespace ann = Aspose::Pdf::Annotations;
namespace frm = Aspose::Pdf::Forms;

namespace {

// A4 portrait, in PDF points.
constexpr double kW = 595.0;
constexpr double kH = 842.0;
constexpr int kPages = 10;

// ---- Palette ---------------------------------------------------------------

// 0xRRGGBB with an optional alpha (255 = opaque).
pdf::Color Hex(std::uint32_t rgb, int alpha = 255) {
    return pdf::Color::FromArgb(alpha, int(rgb >> 16) & 0xFF, int(rgb >> 8) & 0xFF,
                                int(rgb) & 0xFF);
}

const std::uint32_t kLavender = 0xEEF0FB;  // page background, top-left
const std::uint32_t kMint = 0xE4F4EC;      // page background, bottom-right
const std::uint32_t kLilac = 0xC9CCF2;     // rules, borders, soft shapes
const std::uint32_t kMist = 0xF5F6FD;      // tiles and zebra rows
const std::uint32_t kIndigo = 0x3B3F8F;    // accent: headings, chips
const std::uint32_t kInk = 0x2B2D42;       // body text
const std::uint32_t kSlate = 0x7A7D99;     // secondary text
const std::uint32_t kCoral = 0xF08A7E;     // highlight
const std::uint32_t kPeach = 0xF9C9A6;
const std::uint32_t kMintDeep = 0x7CC6A8;
const std::uint32_t kSky = 0xA8C8F0;

// A linear gradient "colour". Start / End are measured from the lower-left
// corner of the filled area; leaving both at (0, 0) runs left to right.
pdf::Color Linear(std::uint32_t from, std::uint32_t to, pdf::Point start = pdf::Point(0, 0),
                  pdf::Point end = pdf::Point(0, 0)) {
    auto g = std::make_shared<draw::GradientAxialShading>(Hex(from), Hex(to));
    g->Start(start);
    g->End(end);
    pdf::Color c;
    c.PatternColorSpace(g);
    return c;
}

// A radial gradient from the area's centre (`inner`) to its corners.
pdf::Color Radial(std::uint32_t inner, std::uint32_t outer) {
    pdf::Color c;
    c.PatternColorSpace(std::make_shared<draw::GradientRadialShading>(Hex(inner), Hex(outer)));
    return c;
}

// Rectangle's canonical ctor wants an explicit normalise flag; wrap it.
pdf::Rectangle Rect(double llx, double lly, double urx, double ury) {
    return pdf::Rectangle(llx, lly, urx, ury, true);
}

// Heterogeneous keep-alive store. Every object handed to an Add() that
// stores by reference is parked here so it outlives Save().
struct Arena {
    std::vector<std::shared_ptr<void>> store;
    template <class T, class... A>
    T& make(A&&... args) {
        auto p = std::make_shared<T>(std::forward<A>(args)...);
        store.push_back(p);
        return *p;
    }
};

// ---- Shapes ----------------------------------------------------------------

// Style for a shape: fill (empty Color = none), stroke colour / width
// (width 0 = no outline) and an optional dash pattern.
pdf::GraphInfo Style(const pdf::Color& fill, const pdf::Color& stroke = pdf::Color(),
                     float width = 0.0f, std::vector<int> dash = {}) {
    pdf::GraphInfo gi;
    gi.FillColor(fill);
    gi.Color(stroke);
    gi.LineWidth(width);
    if (!dash.empty()) gi.DashArray(std::move(dash));
    return gi;
}

void AddRect(draw::Graph& g, double x, double y, double w, double h, const pdf::GraphInfo& gi,
             double radius = 0.0) {
    auto r = std::make_unique<draw::Rectangle>(float(x), float(y), float(w), float(h));
    r->GraphInfo(gi);
    r->RoundedCornerRadius(radius);
    g.Shapes().push_back(std::move(r));
}

void AddCircle(draw::Graph& g, double cx, double cy, double radius, const pdf::GraphInfo& gi) {
    auto c = std::make_unique<draw::Circle>(float(cx), float(cy), float(radius));
    c->GraphInfo(gi);
    g.Shapes().push_back(std::move(c));
}

void AddLine(draw::Graph& g, std::vector<float> points, const pdf::GraphInfo& gi) {
    auto l = std::make_unique<draw::Line>(std::move(points));
    l->GraphInfo(gi);
    g.Shapes().push_back(std::move(l));
}

// A page-sized graph placed under the page text (negative ZIndex).
draw::Graph& Under(Arena& arena, pdf::Page& page, double w, double h) {
    auto& g = arena.make<draw::Graph>(w, h);
    g.ZIndex(-1);
    page.Paragraphs().Add(g);
    return g;
}

// ---- Text ------------------------------------------------------------------

// One positioned line of text. TextBuilder::AppendText reads the
// fragment synchronously, so it can be a temporary.
void DrawText(pdf::Page& page, const std::string& s, double x, double y,
              const std::string& font = "Helvetica", float size = 11.0f,
              pdf::Color color = Hex(kInk)) {
    txt::TextFragment frag(s);
    frag.TextState().Font(txt::FontRepository::FindFont(font));
    frag.TextState().FontSize(size);
    frag.TextState().ForegroundColor(color);
    frag.Position(txt::Position(x, y));
    txt::TextBuilder{page}.AppendText(frag);
}

// One line aligned inside [x0, x1] (left / centre / right).
void DrawAligned(pdf::Page& page, const std::string& s, double x0, double x1, double y,
                 pdf::HorizontalAlignment align, const std::string& font, float size,
                 pdf::Color color) {
    txt::TextState ts;
    ts.Font(txt::FontRepository::FindFont(font));
    ts.FontSize(size);
    ts.ForegroundColor(color);
    txt::TextParagraph para;
    para.Rectangle(Rect(x0, y, x1, y + size));
    para.HorizontalAlignment(align);
    para.AppendLine(s, ts);
    txt::TextBuilder{page}.AppendParagraph(para);
}

// Rough Helvetica width, for sizing pills around a label.
double TextWidth(const std::string& s, double size) { return s.size() * size * 0.55; }

// ---- Page frame --------------------------------------------------------------

// Gradient background, soft corner circles, the white content card, the
// footer rule and the page-number pill. Returns the under-text graph so a
// page can add its own decorations.
draw::Graph& Frame(Arena& arena, pdf::Page& page, int number, double w = kW, double h = kH) {
    page.Background(Linear(kLavender, kMint, pdf::Point(0, h), pdf::Point(w, 0)));
    auto& g = Under(arena, page, w, h);
    AddCircle(g, w - 10, h - 20, 130, Style(Hex(kLilac, 90)));
    AddCircle(g, w - 90, h - 10, 60, Style(Hex(kSky, 70)));
    AddCircle(g, 10, 30, 95, Style(Hex(kMintDeep, 60)));
    // The card: translucent white, rounded, with a hairline lilac edge.
    AddRect(g, 36, 56, w - 72, h - 186, Style(Hex(0xFFFFFF, 235), Hex(kLilac), 0.75f), 16);
    // Footer: a rule, the running title and a page-number pill.
    AddLine(g, {36.0f, 40.0f, float(w - 36), 40.0f}, Style(pdf::Color(), Hex(kLilac), 0.75f));
    AddRect(g, w - 36 - 58, 18, 58, 16, Style(Hex(kIndigo)), 8);
    DrawText(page, "Aspose.PDF FOSS for C++  \xB7  Feature Showcase", 36, 23, "Helvetica", 8.0f,
             Hex(kSlate));
    DrawAligned(page, std::to_string(number) + " / " + std::to_string(kPages), w - 94, w - 36,
                22, pdf::HorizontalAlignment::Center, "Helvetica-Bold", 8.0f, Hex(0xFFFFFF));
    return g;
}

// Section heading: a numbered indigo chip, the title, a subtitle and a short
// coral accent bar, above the card.
void Heading(Arena& arena, pdf::Page& page, draw::Graph& under, const std::string& chip,
             const std::string& title, const std::string& subtitle, double h = kH) {
    (void)arena;
    const double cy = h - 74;
    AddCircle(under, 58, cy, 17, Style(Linear(kIndigo, 0x6C70C9, pdf::Point(0, 34),
                                              pdf::Point(34, 0))));
    DrawAligned(page, chip, 41, 75, cy - 6, pdf::HorizontalAlignment::Center, "Helvetica-Bold",
                15.0f, Hex(0xFFFFFF));
    DrawText(page, title, 86, cy - 3, "Helvetica-Bold", 22.0f, Hex(kIndigo));
    DrawText(page, subtitle, 86, cy - 22, "Helvetica", 10.0f, Hex(kSlate));
    AddRect(under, 86, h - 116, 40, 4, Style(Hex(kCoral)), 2);
}

// A rounded label pill (fill + text), e.g. the cover's feature tags.
void Pill(draw::Graph& under, pdf::Page& page, double x, double y, const std::string& label,
          std::uint32_t fill, std::uint32_t text, double size = 10.0) {
    const double w = TextWidth(label, size) + 22;
    AddRect(under, x, y, w, size + 12, Style(Hex(fill), Hex(kLilac), 0.75f), (size + 12) / 2);
    DrawAligned(page, label, x, x + w, y + 6.5, pdf::HorizontalAlignment::Center,
                "Helvetica-Bold", float(size), Hex(text));
}

// ---- Load the bundled pdflib.png image asset -----------------------------
// The PNG ships next to this source (examples/pdflib.png); CMake passes its
// absolute path via CREATE_FEATURES_PNG. Read it into a byte stream and hand
// it to Page::AddImage (which sniffs the 0x89 P N G signature).
#ifndef CREATE_FEATURES_PNG
#define CREATE_FEATURES_PNG "pdflib.png"
#endif

std::vector<std::byte> LoadPng(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    std::vector<std::byte> bytes;
    if (!in) {
        std::cerr << "warning: could not open image asset: " << path << "\n";
        return bytes;
    }
    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    in.seekg(0, std::ios::beg);
    bytes.resize(static_cast<std::size_t>(size));
    in.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: 12_create_features <output.pdf>\n";
        return 2;
    }
    const std::string outPath = argv[1];

    pdf::Document doc;
    Arena arena;
    std::deque<pdf::Page> pages;

    auto addPage = [&]() -> pdf::Page& {
        pdf::Page& p = pages.emplace_back(doc.Pages().Add());
        p.SetPageSize(kW, kH);
        return p;
    };

    // Document metadata: a from-scratch document gets a new /Info object
    // when it is saved.
    doc.Info().Title("Aspose.PDF FOSS for C++ - Feature Showcase");
    doc.Info().Author("Aspose.PDF FOSS examples");

    // ---- Page 1 : cover --------------------------------------------------
    {
        pdf::Page& p = addPage();
        p.Background(Linear(0xE3E5F8, 0xDDF1E8, pdf::Point(0, kH), pdf::Point(kW, 0)));
        auto& g = Under(arena, p, kW, kH);
        // Large soft gradient discs and a few accents.
        AddCircle(g, kW - 70, kH - 110, 210, Style(Radial(0xFFFFFF, kLilac)));
        AddCircle(g, 60, 150, 170, Style(Radial(0xFFFFFF, 0xBFE6D5)));
        AddCircle(g, kW - 150, 250, 30, Style(Hex(kCoral, 210)));
        AddCircle(g, kW - 105, 300, 12, Style(Hex(kPeach)));
        AddCircle(g, 150, kH - 140, 9, Style(Hex(kIndigo, 160)));
        // Title block on a translucent white panel.
        AddRect(g, 50, kH - 470, kW - 100, 200, Style(Hex(0xFFFFFF, 200), Hex(0xFFFFFF), 1.0f), 22);
        AddRect(g, 76, kH - 345, 48, 5, Style(Hex(kCoral)), 2.5);
        DrawText(p, "Aspose.PDF FOSS for C++", 76, kH - 330, "Helvetica-Bold", 34.0f, Hex(kInk));
        DrawText(p, "Feature Showcase", 76, kH - 380, "Helvetica", 26.0f, Hex(kIndigo));
        DrawText(p, "Built entirely from scratch with the public creation API.", 76, kH - 415,
                 "Times-Roman", 13.0f, Hex(kSlate));
        DrawText(p, "Use the bookmark panel or the Contents page to navigate.", 76, kH - 435,
                 "Times-Roman", 13.0f, Hex(kSlate));
        // Feature tags.
        const char* tags[] = {"Text", "Images", "Forms", "Annotations",
                              "Tables", "Graphics", "Charts", "Bookmarks"};
        double tx = 76, ty = kH - 530;
        for (const char* t : tags) {
            const double w = TextWidth(t, 10) + 22;
            if (tx + w > kW - 60) { tx = 76; ty -= 34; }
            Pill(g, p, tx, ty, t, 0xFFFFFF, kIndigo);
            tx += w + 10;
        }
        DrawText(p, "C++20  \xB7  MIT licensed  \xB7  no third-party dependencies", 76, 70,
                 "Helvetica", 9.0f, Hex(kSlate));
    }

    // ---- Page 2 : Contents (table of contents) --------------------------
    struct Toc { const char* title; const char* blurb; int page; };
    static const Toc kToc[] = {
        {"Text Capabilities", "fonts, wrapping, decorations, watermark", 3},
        {"Image Embedding", "PNG and JPEG byte streams", 4},
        {"AcroForm Fields", "text, choice and button fields", 5},
        {"Annotation Gallery", "markup, shapes, links, stamps, files", 6},
        {"Tables", "backgrounds, alignment, spanned cells", 7},
        {"Vector Graphics", "gradients, transparency, dashes", 8},
        {"Bar Chart", "a landscape page of gradient bars", 9},
        {"Pre-filled Field", "field values and a call-out box", 10},
    };
    {
        pdf::Page& p = addPage();
        auto& g = Frame(arena, p, 2);
        Heading(arena, p, g, "\xA7", "Contents", "Eight sections, one feature family each");

        const double x0 = 60, x1 = kW - 60;
        double y = kH - 190;
        int idx = 0;
        for (const Toc& e : kToc) {
            ++idx;
            // A soft row band, a numbered chip, title + blurb, dot leaders and
            // the page number.
            AddRect(g, x0, y - 16, x1 - x0, 46, Style(Hex(idx % 2 ? kMist : 0xFFFFFF, 255)), 12);
            AddCircle(g, x0 + 24, y + 7, 13, Style(Hex(idx % 2 ? kIndigo : kMintDeep)));
            DrawAligned(p, std::to_string(idx), x0 + 11, x0 + 37, y + 2,
                        pdf::HorizontalAlignment::Center, "Helvetica-Bold", 11.0f, Hex(0xFFFFFF));
            DrawText(p, e.title, x0 + 50, y + 8, "Helvetica-Bold", 13.0f, Hex(kInk));
            DrawText(p, e.blurb, x0 + 50, y - 7, "Helvetica", 9.0f, Hex(kSlate));
            std::string leader;
            for (int d = 0; d < 14; ++d) leader += ". ";
            DrawText(p, leader, 330, y + 2, "Helvetica", 11.0f, Hex(kLilac));
            DrawAligned(p, std::to_string(e.page), x1 - 60, x1 - 16, y + 2,
                        pdf::HorizontalAlignment::Right, "Helvetica-Bold", 13.0f, Hex(kIndigo));
            // Clickable link spanning the whole row — jumps to the section page.
            auto& link = arena.make<ann::LinkAnnotation>(p, Rect(x0, y - 16, x1, y + 30));
            link.Action(ann::GoToAction(e.page));
            link.Contents(e.title);
            p.Annotations().Add(link);
            y -= 58.0;
        }
        DrawAligned(p, "Click any row to jump to the section.", 0, kW, 76,
                    pdf::HorizontalAlignment::Center, "Helvetica-Oblique", 10.0f, Hex(kSlate));
    }

    // ---- Page 3 : text capabilities -------------------------------------
    {
        pdf::Page& p = addPage();
        auto& g = Frame(arena, p, 3);
        Heading(arena, p, g, "1", "Text Capabilities",
                "Standard-14 fonts, decorations and wrapped paragraphs");

        // Standard-14 font grid: name on the left, a sample on the right,
        // dashed separators between rows.
        const char* fonts[] = {"Helvetica", "Helvetica-Bold", "Times-Roman",
                               "Times-Italic", "Courier", "Courier-Bold"};
        double y = kH - 170.0;
        for (const char* f : fonts) {
            DrawText(p, f, 64, y, "Helvetica-Bold", 9.0f, Hex(kIndigo));
            DrawText(p, "The quick brown fox jumps over the lazy dog", 170, y, f, 13.0f);
            AddLine(g, {64.0f, float(y - 11), float(kW - 64), float(y - 11)},
                    Style(pdf::Color(), Hex(kLilac), 0.6f, {3, 3}));
            y -= 32.0;
        }

        // Decorated text (underline / strikeout set on the state).
        y -= 8.0;
        {
            txt::TextFragment frag("Underlined + struck-out (decorations set via TextState)");
            frag.TextState().Font(txt::FontRepository::FindFont("Helvetica"));
            frag.TextState().FontSize(13.0f);
            frag.TextState().ForegroundColor(Hex(kCoral));
            frag.TextState().Underline(true);
            frag.TextState().StrikeOut(true);
            frag.Position(txt::Position(64.0, y));
            txt::TextBuilder{p}.AppendText(frag);
            y -= 40.0;
        }

        // Word-wrapped paragraph on a mint call-out with an accent edge.
        AddRect(g, 60, y - 92, kW - 120, 104, Style(Hex(0xE4F4EC)), 12);
        AddRect(g, 60, y - 92, 6, 104, Style(Hex(kMintDeep)), 3);
        {
            txt::TextState ts;
            ts.Font(txt::FontRepository::FindFont("Times-Roman"));
            ts.FontSize(13.0f);
            ts.ForegroundColor(Hex(kInk));
            txt::TextParagraph para;
            para.AppendLine(
                "This paragraph is laid out with TextParagraph: the text is "
                "wrapped to fit a fixed rectangle, so long runs break across "
                "several lines automatically rather than overflowing the page. "
                "It is the building block for flowing body copy.", ts);
            para.Rectangle(Rect(82.0, y - 80, kW - 80.0, y));
            para.Position(txt::Position(82.0, y));
            txt::TextBuilder{p}.AppendParagraph(para);
        }

        // A diagonal watermark behind the content.
        auto& wm = arena.make<pdf::WatermarkArtifact>();
        txt::TextState wts;
        wts.FontSize(96.0f);
        wts.ForegroundColor(Hex(kLilac));
        wm.SetTextAndState("DRAFT", wts);
        wm.Position(pdf::Point(150.0, 120.0));
        wm.Rotation(30.0);
        wm.Opacity(0.45);
        wm.IsBackground(true);
        p.Artifacts().Add(wm);
    }

    // ---- Page 4 : image embedding ---------------------------------------
    {
        pdf::Page& p = addPage();
        auto& g = Frame(arena, p, 4);
        Heading(arena, p, g, "2", "Image Embedding", "Page::AddImage with PNG / JPEG bytes");
        const std::vector<std::byte> png = LoadPng(CREATE_FEATURES_PNG);
        // A soft shadow and a white mat behind the picture.
        const double ix = 80, iy = kH - 470, isz = 260;
        AddRect(g, ix - 14 + 8, iy - 14 - 8, isz + 28, isz + 28, Style(Hex(kIndigo, 28)), 18);
        AddRect(g, ix - 14, iy - 14, isz + 28, isz + 28, Style(Hex(0xFFFFFF), Hex(kLilac), 0.75f), 18);
        p.AddImage(png, Rect(ix, iy, ix + isz, iy + isz));
        DrawAligned(p, "pdflib.png - 1254 x 1254 RGB, scaled to 260 pt", ix - 14, ix + isz + 14,
                    iy - 40, pdf::HorizontalAlignment::Center, "Helvetica-Oblique", 9.0f,
                    Hex(kSlate));
        // Side notes with coloured bullets.
        const struct { std::uint32_t c; const char* head; const char* text; } notes[] = {
            {kIndigo, "Any size", "decoded once, drawn"},
            {kIndigo, "", "in any rectangle"},
            {kMintDeep, "PNG and JPEG", "read from a byte"},
            {kMintDeep, "", "stream or a file"},
            {kCoral, "Aspect ratio", "kept by default,"},
            {kCoral, "", "centred in the box"},
        };
        double ny = kH - 230;
        for (const auto& n : notes) {
            if (*n.head) {
                AddCircle(g, 392, ny + 4, 5, Style(Hex(n.c)));
                DrawText(p, n.head, 404, ny, "Helvetica-Bold", 11.0f, Hex(kInk));
                ny -= 16;
            }
            DrawText(p, n.text, 404, ny, "Helvetica", 10.0f, Hex(kSlate));
            ny -= *n.head ? 14 : 30;
        }
    }

    // ---- Page 5 : AcroForm fields ---------------------------------------
    {
        pdf::Page& p = addPage();
        auto& g = Frame(arena, p, 5);
        Heading(arena, p, g, "3", "AcroForm Fields",
                "Text - checkbox - radio group - combo box - list box - push button");
        auto& form = doc.Form();
        // A label column on a soft band per row.
        auto label = [&](double y, const char* text, double band_h = 34) {
            AddRect(g, 60, y - 12 - (band_h - 34), kW - 120, band_h, Style(Hex(kMist)), 10);
            DrawText(p, text, 76, y, "Helvetica-Bold", 11.0f, Hex(kIndigo));
        };
        double y;

        y = kH - 180.0;
        label(y, "Full name");
        auto& name = arena.make<frm::TextBoxField>(doc, Rect(190.0, y - 6.0, 470.0, y + 14.0));
        name.PartialName("fullName");
        name.Value("Alice Sample");
        form.Add(name, 5);

        y = kH - 230.0;
        label(y, "Subscribe");
        auto& cb = arena.make<frm::CheckboxField>(doc, Rect(190.0, y - 4.0, 208.0, y + 14.0));
        cb.PartialName("subscribe");
        cb.Checked(true);
        form.Add(cb, 5);

        y = kH - 280.0;
        label(y, "Plan");
        auto& radio = arena.make<frm::RadioButtonField>(doc);
        radio.PartialName("plan");
        radio.AddOption("Basic", Rect(190.0, y - 3.0, 208.0, y + 15.0));
        radio.AddOption("Pro", Rect(290.0, y - 3.0, 308.0, y + 15.0));
        radio.AddOption("Enterprise", Rect(390.0, y - 3.0, 408.0, y + 15.0));
        radio.Value("Pro");  // selected option
        DrawText(p, "Basic", 214.0, y, "Helvetica", 11.0f);
        DrawText(p, "Pro", 314.0, y, "Helvetica", 11.0f);
        DrawText(p, "Enterprise", 414.0, y, "Helvetica", 11.0f);
        form.Add(radio, 5);

        y = kH - 330.0;
        label(y, "Country");
        auto& combo = arena.make<frm::ComboBoxField>(doc, Rect(190.0, y - 4.0, 360.0, y + 14.0));
        combo.PartialName("country");
        combo.AddOption("US");
        combo.AddOption("UK");
        combo.AddOption("Germany");
        combo.Value("US");
        form.Add(combo, 5);

        y = kH - 380.0;
        label(y, "Interests", 104);
        auto& list = arena.make<frm::ListBoxField>(doc, Rect(190.0, y - 76.0, 360.0, y + 14.0));
        list.PartialName("interests");
        list.AddOption("PDF Engineering");
        list.AddOption("Cryptography");
        list.AddOption("Typography");
        list.AddOption("Color Science");
        list.Selected(0);  // first item selected
        form.Add(list, 5);

        y = kH - 520.0;
        label(y, "Submit", 52);
        auto& btn = arena.make<frm::ButtonField>(doc, Rect(190.0, y - 22.0, 320.0, y + 18.0));
        btn.PartialName("submit");
        btn.NormalCaption("Submit");
        form.Add(btn, 5);

        DrawText(p, "Every field is a live AcroForm widget: open the file in a viewer and edit it.",
                 76, 90, "Helvetica-Oblique", 9.5f, Hex(kSlate));
    }

    // ---- Page 6 : annotation gallery ------------------------------------
    {
        pdf::Page& p = addPage();
        auto& g = Frame(arena, p, 6);
        Heading(arena, p, g, "4", "Annotation Gallery",
                "Markup, shapes, links, notes, stamps and attachments");
        const double leftX = 70.0, rightX = 318.0, pitch = 88.0;
        const double startY = kH - 160.0;
        // Each sample sits on a soft tile with a coloured spine.
        auto tile = [&](double x, double topY, std::uint32_t accent) {
            AddRect(g, x - 12, topY - 66, 228, 80, Style(Hex(kMist)), 10);
            AddRect(g, x - 12, topY - 66, 4, 80, Style(Hex(accent)), 2);
        };
        auto label = [&](double x, double topY, const char* name, const char* desc,
                         std::uint32_t accent) {
            tile(x, topY, accent);
            DrawText(p, name, x, topY, "Helvetica-Bold", 11.5f, Hex(kIndigo));
            DrawText(p, desc, x, topY - 13.0, "Helvetica", 8.0f, Hex(kSlate));
        };
        auto rgb = [](std::uint32_t c) { return Hex(c); };

        // ---- left column: text markup + link + sticky note + free text ----
        double ly = startY;
        label(leftX, ly, "Highlight", "yellow background over text", 0xF2C94C);
        DrawText(p, "Highlight this phrase", leftX, ly - 45.0, "Helvetica", 13.0f);
        {
            auto& a = arena.make<ann::HighlightAnnotation>(
                p, Rect(leftX, ly - 48.0, leftX + 140.0, ly - 32.0));
            a.Color(rgb(0xFFE16B));
            a.Contents("Highlight");
            p.Annotations().Add(a);
        }
        ly -= pitch;
        label(leftX, ly, "Underline", "single line under the text", kIndigo);
        DrawText(p, "Underline this phrase", leftX, ly - 45.0, "Helvetica", 13.0f);
        {
            auto& a = arena.make<ann::UnderlineAnnotation>(
                p, Rect(leftX, ly - 48.0, leftX + 140.0, ly - 32.0));
            a.Color(rgb(kIndigo));
            a.Contents("Underline");
            p.Annotations().Add(a);
        }
        ly -= pitch;
        label(leftX, ly, "Squiggly", "wavy underline for proofreaders", kCoral);
        DrawText(p, "Squiggle this phrase", leftX, ly - 45.0, "Helvetica", 13.0f);
        {
            auto& a = arena.make<ann::SquigglyAnnotation>(
                p, Rect(leftX, ly - 48.0, leftX + 130.0, ly - 32.0));
            a.Color(rgb(0xE8743B));
            a.Contents("Squiggly");
            p.Annotations().Add(a);
        }
        ly -= pitch;
        label(leftX, ly, "StrikeOut", "line through the text", 0xD64545);
        DrawText(p, "Strike this phrase out", leftX, ly - 45.0, "Helvetica", 13.0f);
        {
            auto& a = arena.make<ann::StrikeOutAnnotation>(
                p, Rect(leftX, ly - 48.0, leftX + 140.0, ly - 32.0));
            a.Color(rgb(0xD64545));
            a.Contents("StrikeOut");
            p.Annotations().Add(a);
        }
        ly -= pitch;
        label(leftX, ly, "Link", "clickable URL with a GoToURI action", kSky);
        DrawText(p, "Open example.com", leftX, ly - 45.0, "Helvetica", 13.0f, rgb(0x2F6FD0));
        {
            auto& a = arena.make<ann::LinkAnnotation>(
                p, Rect(leftX, ly - 48.0, leftX + 120.0, ly - 32.0));
            a.Action(ann::GoToURIAction("https://example.com/"));
            a.Contents("Open example.com");
            p.Annotations().Add(a);
        }
        ly -= pitch;
        label(leftX, ly, "Text - sticky note", "viewer draws a note icon", 0xF2C94C);
        {
            auto& a = arena.make<ann::TextAnnotation>(doc);
            a.Rect(Rect(leftX, ly - 50.0, leftX + 18.0, ly - 32.0));
            a.Icon(ann::TextIcon::Note);
            a.Contents("A sticky note comment.");
            p.Annotations().Add(a);
        }
        ly -= pitch;
        label(leftX, ly, "FreeText", "text drawn directly on the page", kMintDeep);
        {
            ann::DefaultAppearance da;
            da.FontName("Helvetica");
            da.FontSize(11.0);
            auto& a = arena.make<ann::FreeTextAnnotation>(
                p, Rect(leftX, ly - 58.0, leftX + 190.0, ly - 30.0), da);
            a.Contents("FreeText sample");
            p.Annotations().Add(a);
        }

        // ---- right column: figures + line + ink + stamp + attachment ----
        double ry = startY;
        label(rightX, ry, "Square", "rectangle with a border", kCoral);
        {
            auto& a = arena.make<ann::SquareAnnotation>(
                p, Rect(rightX + 30.0, ry - 58.0, rightX + 160.0, ry - 28.0));
            a.Color(rgb(kCoral));
            a.Contents("Square");
            p.Annotations().Add(a);
        }
        ry -= pitch;
        label(rightX, ry, "Circle", "ellipse inside its rectangle", kMintDeep);
        {
            auto& a = arena.make<ann::CircleAnnotation>(
                p, Rect(rightX + 30.0, ry - 58.0, rightX + 160.0, ry - 28.0));
            a.Color(rgb(0x2E9E6E));
            a.Contents("Circle");
            p.Annotations().Add(a);
        }
        ry -= pitch;
        label(rightX, ry, "Line", "line with start / end arrow endings", kIndigo);
        {
            const double ya = ry - 42.0;
            auto& a = arena.make<ann::LineAnnotation>(
                p, Rect(rightX + 30.0, ya - 8.0, rightX + 160.0, ya + 8.0),
                pdf::Point(rightX + 30.0, ya), pdf::Point(rightX + 160.0, ya));
            a.Color(rgb(kIndigo));
            a.Contents("Line");
            p.Annotations().Add(a);
        }
        ry -= pitch;
        label(rightX, ry, "Ink", "free-hand pen strokes", 0x9B59B6);
        {
            const double bx = rightX + 30.0, by = ry - 45.0;
            ann::InkAnnotation::StrokeList strokes{{
                pdf::Point(bx, by), pdf::Point(bx + 35.0, by + 14.0),
                pdf::Point(bx + 70.0, by), pdf::Point(bx + 105.0, by + 14.0),
                pdf::Point(bx + 140.0, by)}};
            auto& a = arena.make<ann::InkAnnotation>(
                p, Rect(bx, by - 6.0, bx + 145.0, by + 20.0), strokes);
            a.Color(rgb(0x9B59B6));
            a.Contents("Ink");
            p.Annotations().Add(a);
        }
        ry -= pitch;
        label(rightX, ry, "Stamp", "predefined or custom text stamp", 0x2E9E6E);
        {
            auto& a = arena.make<ann::StampAnnotation>(doc);
            a.Rect(Rect(rightX + 30.0, ry - 56.0, rightX + 170.0, ry - 28.0));
            a.Icon(ann::StampIcon::Approved);
            a.Color(rgb(0x2E9E6E));
            a.Contents("APPROVED");
            p.Annotations().Add(a);
        }
        ry -= pitch;
        label(rightX, ry, "FileAttachment", "embedded file behind an icon", kSky);
        const std::string attachPath = outPath + ".attachment.txt";
        {
            std::ofstream a(attachPath);
            a << "Attached by 12_create_features.\n";
        }
        pdf::FileSpecification fs(attachPath, "Sample attachment");
        doc.EmbeddedFiles().Add("readme.txt", fs);  // shows in attachment panel
        {
            auto& a = arena.make<ann::FileAttachmentAnnotation>(
                p, Rect(rightX + 30.0, ry - 50.0, rightX + 48.0, ry - 32.0), fs);
            a.Icon(ann::FileIcon::Paperclip);
            a.Contents("Attachment");
            p.Annotations().Add(a);
        }
    }

    // ---- Page 7 : tables -------------------------------------------------
    {
        pdf::Page& p = addPage();
        auto& g = Frame(arena, p, 7);
        Heading(arena, p, g, "5", "Tables", "Row and cell backgrounds, alignment, spanned cells");
        DrawText(p, "A restaurant bill - note the column-spanning total row:", 64.0, kH - 170.0,
                 "Helvetica", 11.0f, Hex(kSlate));

        auto& table = arena.make<pdf::Table>();
        table.ColumnWidths("260 90 117");
        table.Left(64.0f);
        table.Top(float(kH - 190.0));  // y of the table's top edge, from the page bottom
        table.Border(pdf::BorderInfo(pdf::BorderSide::All, 1.0f, Hex(kLilac)));
        table.DefaultCellBorder(pdf::BorderInfo(pdf::BorderSide::Bottom, 0.5f, Hex(kLilac)));
        {
            txt::TextState ts;
            ts.FontSize(11.0f);
            ts.ForegroundColor(Hex(kInk));
            table.DefaultCellTextState(ts);
        }
        pdf::MarginInfo pad;
        pad.Left(12);
        pad.Right(12);
        table.DefaultCellPadding(pad);

        auto row = [&](const char* a, const char* b, const char* c,
                       std::uint32_t bg) -> pdf::Row& {
            pdf::Row& r = table.Rows().Add();
            r.MinRowHeight(30.0);
            r.VerticalAlignment(pdf::VerticalAlignment::Center);
            r.BackgroundColor(Hex(bg));
            r.Cells().Add(a);
            r.Cells().Add(b).Alignment(pdf::HorizontalAlignment::Center);
            r.Cells().Add(c).Alignment(pdf::HorizontalAlignment::Right);
            return r;
        };
        {
            // Header: white bold text on an indigo gradient.
            pdf::Row& h = row("Item", "Qty", "Price", 0);
            h.BackgroundColor(Linear(kIndigo, 0x6C70C9));
            txt::TextState hs;
            hs.Font(txt::FontRepository::FindFont("Helvetica-Bold"));
            hs.FontSize(11.0f);
            hs.ForegroundColor(Hex(0xFFFFFF));
            h.DefaultCellTextState(hs);
        }
        row("Margherita pizza", "2", "$24.00", 0xFFFFFF);
        row("Caesar salad", "1", "$9.50", kMist);
        row("Sparkling water", "3", "$7.50", 0xFFFFFF);
        row("Tiramisu", "1", "$6.50", kMist);

        pdf::Row& total = table.Rows().Add();
        total.MinRowHeight(34.0);
        total.VerticalAlignment(pdf::VerticalAlignment::Center);
        total.BackgroundColor(Hex(0xDDF1E8));
        {
            txt::TextState ts;
            ts.Font(txt::FontRepository::FindFont("Helvetica-Bold"));
            ts.FontSize(12.0f);
            ts.ForegroundColor(Hex(kInk));
            total.DefaultCellTextState(ts);
        }
        pdf::Cell& tc = total.Cells().Add("Total");
        tc.ColSpan(2);
        total.Cells().Add("$47.50").Alignment(pdf::HorizontalAlignment::Right);

        p.Paragraphs().Add(table);

        // A small legend under the table.
        double ly = kH - 420;
        const struct { std::uint32_t c; const char* t; } legend[] = {
            {kIndigo, "Header row: gradient background, bold white text"},
            {kLilac, "Zebra rows: Row::BackgroundColor"},
            {kMintDeep, "Total: a cell spanning two columns (Cell::ColSpan)"},
        };
        for (const auto& l : legend) {
            AddRect(g, 64, ly - 2, 12, 12, Style(Hex(l.c)), 3);
            DrawText(p, l.t, 84, ly, "Helvetica", 10.0f, Hex(kSlate));
            ly -= 20;
        }
    }

    // ---- Page 8 : vector graphics ---------------------------------------
    {
        pdf::Page& p = addPage();
        auto& under = Frame(arena, p, 8);
        Heading(arena, p, under, "6", "Vector Graphics",
                "Graph shapes with gradients, transparency, dashes and rounded corners");
        auto& g = arena.make<draw::Graph>(kW, kH);  // drawn over the card
        // Coordinates below are page coordinates (the graph sits at 0, 0).
        auto caption = [&](double cx, double y, const char* t) {
            DrawAligned(p, t, cx - 110, cx + 110, y, pdf::HorizontalAlignment::Center,
                        "Helvetica", 9.0f, Hex(kSlate));
        };
        // 1. Rounded rectangle with a linear gradient and a dashed border.
        AddRect(g, 70, kH - 340, 200, 140,
                Style(Linear(kIndigo, kSky, pdf::Point(0, 140), pdf::Point(200, 0)),
                      Hex(kCoral), 2.0f, {6, 4}),
                22);
        caption(170, kH - 362, "Rounded rectangle, linear gradient, dashed border");
        // 2. Radial gradient disc.
        AddCircle(g, 420, kH - 270, 72, Style(Radial(0xFFF4E8, kCoral)));
        caption(420, kH - 362, "Circle with a radial gradient");
        // 3. Overlapping translucent circles.
        AddCircle(g, 140, kH - 470, 56, Style(Hex(kIndigo, 120)));
        AddCircle(g, 205, kH - 470, 56, Style(Hex(kCoral, 130)));
        AddCircle(g, 172, kH - 420, 56, Style(Hex(kMintDeep, 140)));
        caption(172, kH - 552, "Translucent fills (Color alpha)");
        // 4. Ellipse with a vertical gradient and an outline.
        {
            auto e = std::make_unique<draw::Ellipse>(340.0, kH - 520.0, 160.0, 100.0);
            e->GraphInfo(Style(Linear(0xDDF1E8, kMintDeep, pdf::Point(0, 100), pdf::Point(0, 0)),
                               Hex(0x2E9E6E), 1.5f));
            g.Shapes().push_back(std::move(e));
        }
        caption(420, kH - 552, "Ellipse, vertical gradient + outline");
        // 5. Lines: a dashed zig-zag and a thick wave.
        AddLine(g, {80.0f, float(kH - 620), 140.0f, float(kH - 580), 200.0f, float(kH - 620),
                    260.0f, float(kH - 580), 320.0f, float(kH - 620), 380.0f, float(kH - 580),
                    440.0f, float(kH - 620), 500.0f, float(kH - 580)},
                Style(pdf::Color(), Hex(kIndigo), 2.0f, {8, 5}));
        AddLine(g, {80.0f, float(kH - 650), 500.0f, float(kH - 650)},
                Style(pdf::Color(), Hex(kCoral), 5.0f));
        caption(290, kH - 672, "Lines: width, colour and dash pattern");
        p.Paragraphs().Add(g);
    }

    // ---- Page 9 : bar chart (landscape) ---------------------------------
    {
        pdf::Page& p = addPage();
        p.SetPageSize(kH, kW);  // landscape (kH wide × kW tall)
        const double W = kH, H = kW;
        auto& under = Frame(arena, p, 9, W, H);
        Heading(arena, p, under, "7", "Bar Chart", "Weekly visitors, drawn from gradient bars",
                H);

        const int values[] = {40, 95, 70, 120, 60, 150, 85};
        const char* labels[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        const double x0 = 110, y0 = 100, scale = 2.0, pitch = 90, barW = 50;
        // Grid: dashed lilac lines every 40 units, value labels on the left.
        for (int v = 0; v <= 160; v += 40) {
            const double y = y0 + v * scale;
            AddLine(under, {float(x0 - 10), float(y), float(x0 + 7 * pitch), float(y)},
                    Style(pdf::Color(), Hex(kLilac), v == 0 ? 1.2f : 0.6f,
                          v == 0 ? std::vector<int>{} : std::vector<int>{4, 4}));
            DrawAligned(p, std::to_string(v), x0 - 50, x0 - 16, y - 3,
                        pdf::HorizontalAlignment::Right, "Helvetica", 9.0f, Hex(kSlate));
        }
        auto& g = arena.make<draw::Graph>(W, H);
        int best = 0;
        for (int i = 1; i < 7; ++i)
            if (values[i] > values[best]) best = i;
        for (int i = 0; i < 7; ++i) {
            const double x = x0 + 20 + i * pitch, h = values[i] * scale;
            // Vertical gradient per bar (Start/End relative to the bar).
            const pdf::Color fill = i == best
                ? Linear(kCoral, kPeach, pdf::Point(0, 0), pdf::Point(0, h))
                : Linear(kIndigo, kLilac, pdf::Point(0, 0), pdf::Point(0, h));
            AddRect(g, x, y0, barW, h, Style(fill), 6);
            DrawAligned(p, std::to_string(values[i]), x, x + barW, y0 + h + 8,
                        pdf::HorizontalAlignment::Center, "Helvetica-Bold", 10.0f,
                        Hex(i == best ? 0xD9634F : kIndigo));
            DrawAligned(p, labels[i], x - 10, x + barW + 10, y0 - 20,
                        pdf::HorizontalAlignment::Center, "Helvetica", 10.0f, Hex(kInk));
        }
        p.Paragraphs().Add(g);
        Pill(under, p, W - 250, H - 168, "Peak: Saturday, 150", 0xFDE7E2, 0xD9634F, 9.0);
    }

    // ---- Page 10 : pre-filled field + call-out --------------------------
    {
        pdf::Page& p = addPage();
        auto& g = Frame(arena, p, 10);
        Heading(arena, p, g, "8", "Pre-filled Field", "Field values and a FloatingBox call-out");
        DrawText(p, "A second AcroForm text field, pre-filled with a value:", 64.0, kH - 170.0,
                 "Helvetica", 11.0f, Hex(kSlate));

        auto& ro = arena.make<frm::TextBoxField>(doc, Rect(64.0, kH - 215.0, 420.0, kH - 190.0));
        ro.PartialName("issuedBy");
        ro.Value("Aspose.PDF FOSS for C++");
        doc.Form().Add(ro, 10);

        // A FloatingBox call-out: gradient background, rounded coloured
        // border, under the text (negative ZIndex).
        auto& box = arena.make<pdf::FloatingBox>(float(kW - 128), 96.0f);
        pdf::MarginInfo m;
        m.Left(64);
        m.Top(250);
        box.Margin(m);
        box.BackgroundColor(Linear(0xE4F4EC, 0xF5F6FD));
        pdf::BorderInfo border(pdf::BorderSide::All, 1.0f, Hex(kMintDeep));
        border.RoundedBorderRadius(14);
        box.Border(border);
        box.ZIndex(-1);
        p.Paragraphs().Add(box);
        // The info chip goes on a second under-text graph, added after the
        // box so it paints on top of it.
        auto& chip = Under(arena, p, kW, kH);
        AddCircle(chip, 92, kH - 282, 12, Style(Hex(kMintDeep)));
        DrawAligned(p, "i", 80, 104, kH - 287, pdf::HorizontalAlignment::Center,
                    "Times-Bold", 14.0f, Hex(0xFFFFFF));
        DrawText(p, "Form::Flatten() bakes every field into static page content.", 116,
                 kH - 280.0, "Helvetica-Bold", 11.0f, Hex(kInk));
        DrawText(p, "It is document-wide, so it is not called here - it would also", 116,
                 kH - 300.0, "Times-Roman", 11.0f, Hex(kInk));
        DrawText(p, "flatten the interactive fields on page 5.", 116, kH - 316.0,
                 "Times-Roman", 11.0f, Hex(kInk));
    }

    // ---- bookmark outline (built last, references every page) -----------
    {
        auto& root = doc.Outlines();
        struct Entry { const char* title; int page; };
        const Entry entries[] = {
            {"Cover", 1},                 {"Contents", 2},
            {"1. Text Capabilities", 3},  {"2. Image Embedding", 4},
            {"3. AcroForm Fields", 5},    {"4. Annotation Gallery", 6},
            {"5. Tables", 7},             {"6. Vector Graphics", 8},
            {"7. Bar Chart", 9},          {"8. Pre-filled Field", 10},
        };
        for (const Entry& e : entries) {
            auto& item = arena.make<pdf::OutlineItemCollection>(root);
            item.Title(e.title);
            item.Bold(e.page == 1);
            pdf::Page& target = pages[std::size_t(e.page - 1)];
            // page 9 (bar chart) is landscape — its top edge is kW, not kH.
            const double top = (e.page == 9) ? kW : kH;
            item.Destination(ann::XYZExplicitDestination(target, 0.0, top, 0.0));
            root.Add(item);
        }
    }

    doc.Save(outPath);
    std::cout << "Wrote " << pages.size() << "-page showcase to " << outPath
              << "\n";
    return 0;
}
