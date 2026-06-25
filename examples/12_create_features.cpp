// Build a multi-page "Feature Showcase" PDF *from scratch* with the
// Aspose::Pdf:: creation surface — the mirror image of the other
// examples here, which all open an existing file. It is the C++
// analogue of the bundled features.pdf showcase: one program that
// exercises every creation capability the library ships.
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
//   7  Tables: a restaurant bill with a spanned total row
//   8  Vector graphics: lines, rectangles, circle, ellipse
//   9  A bar chart drawn from rectangles (A4 landscape)
//  10  A second, pre-filled AcroForm field
//
// Output note: text / tables / vector graphics / images / watermark
// paint in black + image colour through our own rasteriser. The
// AcroForm fields (page 4 / 9) and the annotation gallery (page 5)
// are written as proper interactive objects with pre-rendered /AP
// appearance streams and linked into their page /Annots, so they
// render in a full viewer such as Acrobat (and in our own engine).
// The attachment is embedded document-level (catalog /Names
// /EmbeddedFiles) so it shows in the viewer's attachment panel.
// Render any page with 07_render_png to see what our engine paints.
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

#include <aspose/pdf/paragraphs.hpp>
#include <aspose/pdf/table.hpp>
#include <aspose/pdf/rows.hpp>
#include <aspose/pdf/row.hpp>
#include <aspose/pdf/cells.hpp>
#include <aspose/pdf/cell.hpp>
#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/border_side.hpp>

#include <aspose/pdf/drawing/graph.hpp>
#include <aspose/pdf/drawing/line.hpp>
#include <aspose/pdf/drawing/rectangle.hpp>
#include <aspose/pdf/drawing/circle.hpp>
#include <aspose/pdf/drawing/ellipse.hpp>
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

// One positioned line of text. TextBuilder::AppendText reads the
// fragment synchronously, so it can be a temporary.
void DrawText(pdf::Page& page, const std::string& s, double x, double y,
              const std::string& font = "Helvetica", float size = 11.0f,
              pdf::Color color = pdf::Color::Black()) {
    txt::TextFragment frag(s);
    frag.TextState().Font(txt::FontRepository::FindFont(font));
    frag.TextState().FontSize(size);
    frag.TextState().ForegroundColor(color);
    frag.Position(txt::Position(x, y));
    txt::TextBuilder{page}.AppendText(frag);
}

// Footer stamped on every page.
void Footer(pdf::Page& page, int number) {
    DrawText(page, "Aspose.PDF FOSS for C++  -  Feature Showcase", 50.0, 30.0,
             "Helvetica", 8.0f, pdf::Color::Gray());
    DrawText(page, "Page " + std::to_string(number), kW - 90.0, 30.0,
             "Helvetica", 8.0f, pdf::Color::Gray());
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

// Section heading + thin rule. `pageTop` / `pageW` default to portrait A4;
// the landscape bar-chart page passes its swapped dimensions so the heading
// sits at the visible top edge and the rule spans the full width.
void Heading(Arena& arena, pdf::Page& page, const std::string& title,
             double pageTop = kH, double pageW = kW) {
    DrawText(page, title, 50.0, pageTop - 80.0, "Helvetica", 16.0f);
    auto& g = arena.make<draw::Graph>(pageW - 100.0, 1.0);
    g.Left(50.0);
    g.Top(pageTop - 90.0);
    g.Shapes().push_back(std::make_unique<draw::Line>(
        std::vector<float>{0.0f, 0.0f, float(pageW - 100.0), 0.0f}));
    page.Paragraphs().Add(g);
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

    // (Document metadata via doc.Info() is intentionally omitted: a
    //  from-scratch document has no /Info dictionary for the incremental
    //  save to patch, so setting it would throw at Save time.)

    // ---- Page 1 : cover --------------------------------------------------
    {
        pdf::Page& p = addPage();
        DrawText(p, "Aspose.PDF FOSS for C++", 70.0, kH - 200.0, "Helvetica", 32.0f);
        DrawText(p, "Feature Showcase", 70.0, kH - 245.0, "Helvetica", 24.0f,
                 pdf::Color::DarkBlue());
        DrawText(p, "Built entirely from scratch with the public creation API.",
                 70.0, kH - 290.0, "Times-Roman", 13.0f);
        DrawText(p, "Use the bookmark panel or the Contents page to navigate.",
                 70.0, kH - 312.0, "Times-Roman", 13.0f);
        Footer(p, 1);
    }

    // ---- Page 2 : Contents (table of contents) --------------------------
    struct Toc { const char* title; int page; };
    static const Toc kToc[] = {
        {"Text Capabilities", 3}, {"Image Embedding", 4},
        {"AcroForm Fields", 5},   {"Annotation Gallery", 6},
        {"Tables", 7},            {"Vector Graphics", 8},
        {"Bar Chart", 9},         {"Pre-filled Field", 10},
    };
    {
        pdf::Page& p = addPage();
        // Deep navy used for the title, the row numbers and the page numbers
        // (matches the Feature Showcase contents page).
        const pdf::Color navy = pdf::Color::FromArgb(31, 45, 107);

        // Centred title + a thin rule beneath it.
        {
            txt::TextState ts;
            ts.Font(txt::FontRepository::FindFont("Helvetica-Bold"));
            ts.FontSize(32.0f);
            ts.ForegroundColor(navy);
            txt::TextParagraph title;
            title.Rectangle(Rect(0.0, kH - 105.0, kW, kH - 105.0 + 32.0));
            title.HorizontalAlignment(pdf::HorizontalAlignment::Center);
            title.AppendLine("Contents", ts);
            txt::TextBuilder{p}.AppendParagraph(title);
        }
        auto& rule = arena.make<draw::Graph>(kW - 168.0, 1.0);
        rule.Left(84.0);
        rule.Top(kH - 128.0);
        rule.Shapes().push_back(std::make_unique<draw::Line>(
            std::vector<float>{0.0f, 0.0f, float(kW - 168.0), 0.0f}));
        p.Paragraphs().Add(rule);

        // Column geometry: numbers right-align at numRight, titles start at
        // titleX, a centred dot-leader run, page numbers right-align at pnRight.
        const double numRight = 110.0, titleX = 122.0, pnRight = kW - 84.0;
        double y = kH - 172.0;
        int idx = 0;
        for (const Toc& e : kToc) {
            ++idx;
            const std::string num = std::to_string(idx) + ".";
            {
                txt::TextState ts;
                ts.Font(txt::FontRepository::FindFont("Helvetica-Bold"));
                ts.FontSize(13.0f);
                ts.ForegroundColor(navy);
                txt::TextParagraph numPara;
                numPara.Rectangle(Rect(0.0, y, numRight, y + 13.0));
                numPara.HorizontalAlignment(pdf::HorizontalAlignment::Right);
                numPara.AppendLine(num, ts);
                txt::TextBuilder{p}.AppendParagraph(numPara);
            }
            DrawText(p, e.title, titleX, y, "Helvetica", 13.0f,
                     pdf::Color::FromArgb(35, 35, 35));
            // Centred spaced dot-leader run (fixed width, like the reference).
            std::string leader;
            for (int d = 0; d < 20; ++d) leader += ". ";
            DrawText(p, leader, 305.0, y, "Helvetica", 13.0f,
                     pdf::Color::FromArgb(170, 170, 170));
            {
                txt::TextState ts;
                ts.Font(txt::FontRepository::FindFont("Helvetica-Bold"));
                ts.FontSize(13.0f);
                ts.ForegroundColor(navy);
                txt::TextParagraph pnPara;
                pnPara.Rectangle(Rect(titleX, y, pnRight, y + 13.0));
                pnPara.HorizontalAlignment(pdf::HorizontalAlignment::Right);
                pnPara.AppendLine(std::to_string(e.page), ts);
                txt::TextBuilder{p}.AppendParagraph(pnPara);
            }
            // Clickable link spanning the whole row — jumps to the section page.
            auto& link = arena.make<ann::LinkAnnotation>(
                p, Rect(84.0, y - 6.0, pnRight, y + 16.0));
            link.Action(ann::GoToAction(e.page));
            link.Contents(e.title);
            p.Annotations().Add(link);
            y -= 33.0;
        }
        {
            txt::TextState ts;
            ts.Font(txt::FontRepository::FindFont("Helvetica-Oblique"));
            ts.FontSize(11.0f);
            ts.ForegroundColor(pdf::Color::FromArgb(130, 130, 130));
            txt::TextParagraph note;
            note.Rectangle(Rect(0.0, 122.0, kW, 122.0 + 11.0));
            note.HorizontalAlignment(pdf::HorizontalAlignment::Center);
            note.AppendLine("Click any title above to jump to the section.", ts);
            txt::TextBuilder{p}.AppendParagraph(note);
        }
        Footer(p, 2);
    }

    // ---- Page 3 : text capabilities -------------------------------------
    {
        pdf::Page& p = addPage();
        Heading(arena, p, "1. Text Capabilities");

        // Standard-14 font grid.
        const char* fonts[] = {"Helvetica", "Helvetica-Bold", "Times-Roman",
                               "Times-Italic", "Courier", "Courier-Bold"};
        double y = kH - 110.0;
        for (const char* f : fonts) {
            DrawText(p, std::string(f) + " - The quick brown fox jumps over the lazy dog",
                     60.0, y, f, 13.0f);
            y -= 26.0;
        }

        // Decorated text (underline / strikeout set on the state).
        {
            txt::TextFragment frag("Underlined + struck-out (decorations set via TextState)");
            frag.TextState().Font(txt::FontRepository::FindFont("Helvetica"));
            frag.TextState().FontSize(13.0f);
            frag.TextState().Underline(true);
            frag.TextState().StrikeOut(true);
            frag.Position(txt::Position(60.0, y));
            txt::TextBuilder{p}.AppendText(frag);
            y -= 34.0;
        }

        // Word-wrapped paragraph inside a fixed box.
        {
            txt::TextParagraph para;
            para.AppendLine(
                "This paragraph is laid out with TextParagraph: the text is "
                "wrapped to fit a fixed rectangle, so long runs break across "
                "several lines automatically rather than overflowing the page. "
                "It is the building block for flowing body copy.");
            para.Rectangle(Rect(60.0, y - 90.0, kW - 60.0, y));
            para.Position(txt::Position(60.0, y));
            txt::TextBuilder{p}.AppendParagraph(para);
        }

        // A diagonal watermark behind the content.
        auto& wm = arena.make<pdf::WatermarkArtifact>();
        txt::TextState wts;
        wts.FontSize(72.0f);
        wts.ForegroundColor(pdf::Color::LightGray());
        wm.SetTextAndState("DRAFT", wts);
        wm.Position(pdf::Point(160.0, 360.0));
        wm.Rotation(45.0);
        wm.Opacity(0.35);
        wm.IsBackground(true);
        p.Artifacts().Add(wm);

        Footer(p, 3);
    }

    // ---- Page 4 : image embedding ---------------------------------------
    {
        pdf::Page& p = addPage();
        Heading(arena, p, "2. Image Embedding");
        DrawText(p, "The bundled pdflib.png (1254x1254 RGB) embedded as a stream:",
                 60.0, kH - 110.0, "Helvetica", 12.0f);
        const std::vector<std::byte> png = LoadPng(CREATE_FEATURES_PNG);
        // Square source → square 220x220 pt placement.
        p.AddImage(png, Rect(60.0, kH - 360.0, 280.0, kH - 140.0));
        DrawText(p, "Page::AddImage accepts PNG and JPEG byte streams.", 60.0,
                 kH - 390.0, "Times-Roman", 12.0f);
        Footer(p, 4);
    }

    // ---- Page 5 : AcroForm fields (mirrors features.pdf p5) -------------
    {
        pdf::Page& p = addPage();
        Heading(arena, p, "3. AcroForm Fields");
        DrawText(p,
                 "Text - checkbox - radio group - combo box - list box - push "
                 "button",
                 60.0, kH - 108.0, "Helvetica", 9.0f, pdf::Color::Gray());
        auto& form = doc.Form();
        double y;

        y = kH - 135.0;
        DrawText(p, "Full name:", 60.0, y, "Helvetica", 12.0f);
        auto& name = arena.make<frm::TextBoxField>(doc, Rect(190.0, y - 6.0, 470.0, y + 14.0));
        name.PartialName("fullName");
        name.Value("Alice Sample");
        form.Add(name, 5);

        y = kH - 180.0;
        DrawText(p, "Subscribe:", 60.0, y, "Helvetica", 12.0f);
        auto& cb = arena.make<frm::CheckboxField>(doc, Rect(190.0, y - 4.0, 208.0, y + 14.0));
        cb.PartialName("subscribe");
        cb.Checked(true);
        form.Add(cb, 5);

        y = kH - 225.0;
        DrawText(p, "Plan:", 60.0, y, "Helvetica", 12.0f);
        auto& radio = arena.make<frm::RadioButtonField>(doc);
        radio.PartialName("plan");
        radio.AddOption("Basic", Rect(190.0, y - 3.0, 208.0, y + 15.0));
        radio.AddOption("Pro", Rect(300.0, y - 3.0, 318.0, y + 15.0));
        radio.AddOption("Enterprise", Rect(410.0, y - 3.0, 428.0, y + 15.0));
        radio.Value("Pro");  // selected option
        DrawText(p, "Basic", 214.0, y, "Helvetica", 11.0f);
        DrawText(p, "Pro", 324.0, y, "Helvetica", 11.0f);
        DrawText(p, "Enterprise", 434.0, y, "Helvetica", 11.0f);
        form.Add(radio, 5);

        y = kH - 270.0;
        DrawText(p, "Country:", 60.0, y, "Helvetica", 12.0f);
        auto& combo = arena.make<frm::ComboBoxField>(doc, Rect(190.0, y - 4.0, 360.0, y + 14.0));
        combo.PartialName("country");
        combo.AddOption("US");
        combo.AddOption("UK");
        combo.AddOption("Germany");
        combo.Value("US");
        form.Add(combo, 5);

        y = kH - 320.0;
        DrawText(p, "Interests:", 60.0, y, "Helvetica", 12.0f);
        auto& list = arena.make<frm::ListBoxField>(doc, Rect(190.0, y - 78.0, 360.0, y + 14.0));
        list.PartialName("interests");
        list.AddOption("PDF Engineering");
        list.AddOption("Cryptography");
        list.AddOption("Typography");
        list.AddOption("Color Science");
        list.Selected(0);  // first item selected
        form.Add(list, 5);

        y = kH - 450.0;
        DrawText(p, "Submit:", 60.0, y, "Helvetica", 12.0f);
        auto& btn = arena.make<frm::ButtonField>(doc, Rect(190.0, y - 28.0, 320.0, y + 18.0));
        btn.PartialName("submit");
        btn.NormalCaption("Submit");
        form.Add(btn, 5);

        Footer(p, 5);
    }

    // ---- Page 6 : annotation gallery (mirrors features.pdf p6) ----------
    {
        pdf::Page& p = addPage();
        Heading(arena, p, "4. Annotation Gallery");
        const double leftX = 60.0, rightX = 410.0, pitch = 92.0;
        const double startY = kH - 120.0;
        auto label = [&](double x, double topY, const char* name,
                         const char* desc) {
            DrawText(p, name, x, topY, "Helvetica-Bold", 12.0f,
                     pdf::Color::DarkBlue());
            DrawText(p, desc, x, topY - 13.0, "Helvetica", 8.0f,
                     pdf::Color::Gray());
        };
        auto rgb = [](double r, double g, double b) {
            return pdf::Color::FromRgb(r, g, b);
        };

        // ---- left column: text markup + link + sticky note + free text ----
        double ly = startY;
        // Highlight
        label(leftX, ly, "Highlight", "yellow background over text");
        DrawText(p, "Highlight this phrase", leftX, ly - 45.0, "Helvetica", 13.0f);
        {
            auto& a = arena.make<ann::HighlightAnnotation>(
                p, Rect(leftX, ly - 48.0, leftX + 150.0, ly - 32.0));
            a.Color(rgb(1.0, 0.95, 0.0));
            a.Contents("Highlight");
            p.Annotations().Add(a);
        }
        ly -= pitch;
        // Underline
        label(leftX, ly, "Underline", "single line under the text");
        DrawText(p, "Underline this phrase", leftX, ly - 45.0, "Helvetica", 13.0f);
        {
            auto& a = arena.make<ann::UnderlineAnnotation>(
                p, Rect(leftX, ly - 48.0, leftX + 150.0, ly - 32.0));
            a.Color(rgb(0.0, 0.0, 0.8));
            a.Contents("Underline");
            p.Annotations().Add(a);
        }
        ly -= pitch;
        // Squiggly
        label(leftX, ly, "Squiggly", "wavy underline for proofreaders");
        DrawText(p, "Squiggle this phrase", leftX, ly - 45.0, "Helvetica", 13.0f);
        {
            auto& a = arena.make<ann::SquigglyAnnotation>(
                p, Rect(leftX, ly - 48.0, leftX + 150.0, ly - 32.0));
            a.Color(rgb(0.9, 0.5, 0.0));
            a.Contents("Squiggly");
            p.Annotations().Add(a);
        }
        ly -= pitch;
        // StrikeOut
        label(leftX, ly, "StrikeOut", "line through the text");
        DrawText(p, "Strike this phrase out", leftX, ly - 45.0, "Helvetica", 13.0f);
        {
            auto& a = arena.make<ann::StrikeOutAnnotation>(
                p, Rect(leftX, ly - 48.0, leftX + 150.0, ly - 32.0));
            a.Color(rgb(0.85, 0.0, 0.0));
            a.Contents("StrikeOut");
            p.Annotations().Add(a);
        }
        ly -= pitch;
        // Link
        label(leftX, ly, "Link", "clickable URL with a GoToURI action");
        DrawText(p, "Open example.com", leftX, ly - 45.0, "Helvetica", 13.0f,
                 rgb(0.0, 0.0, 0.8));
        {
            auto& a = arena.make<ann::LinkAnnotation>(
                p, Rect(leftX, ly - 48.0, leftX + 130.0, ly - 32.0));
            a.Action(ann::GoToURIAction("https://example.com/"));
            a.Contents("Open example.com");
            p.Annotations().Add(a);
        }
        ly -= pitch;
        // Text sticky note
        label(leftX, ly, "Text - sticky note", "viewer draws a note icon");
        {
            auto& a = arena.make<ann::TextAnnotation>(doc);
            a.Rect(Rect(leftX, ly - 50.0, leftX + 18.0, ly - 32.0));
            a.Icon(ann::TextIcon::Note);
            a.Contents("A sticky note comment.");
            p.Annotations().Add(a);
        }
        ly -= pitch;
        // FreeText
        label(leftX, ly, "FreeText", "text drawn directly on the page");
        {
            ann::DefaultAppearance da;
            da.FontName("Helvetica");
            da.FontSize(11.0);
            auto& a = arena.make<ann::FreeTextAnnotation>(
                p, Rect(leftX, ly - 58.0, leftX + 200.0, ly - 30.0), da);
            a.Contents("FreeText sample");
            p.Annotations().Add(a);
        }

        // ---- right column: figures + line + ink + stamp + attachment ----
        double ry = startY;
        // Square
        label(rightX, ry, "Square", "filled rectangle with a border");
        {
            auto& a = arena.make<ann::SquareAnnotation>(
                p, Rect(rightX + 30.0, ry - 58.0, rightX + 160.0, ry - 28.0));
            a.Color(rgb(0.85, 0.0, 0.0));
            a.Contents("Square");
            p.Annotations().Add(a);
        }
        ry -= pitch;
        // Circle
        label(rightX, ry, "Circle", "dashed border, no fill");
        {
            auto& a = arena.make<ann::CircleAnnotation>(
                p, Rect(rightX + 30.0, ry - 58.0, rightX + 160.0, ry - 28.0));
            a.Color(rgb(0.0, 0.6, 0.0));
            a.Contents("Circle");
            p.Annotations().Add(a);
        }
        ry -= pitch;
        // Line
        label(rightX, ry, "Line", "line with start / end arrow endings");
        {
            const double ya = ry - 42.0;
            // End at rightX+160 (same right edge as Square/Circle) so the
            // arrowhead stays inside the page; rightX+200 ran off the border.
            auto& a = arena.make<ann::LineAnnotation>(
                p, Rect(rightX + 30.0, ya - 8.0, rightX + 160.0, ya + 8.0),
                pdf::Point(rightX + 30.0, ya), pdf::Point(rightX + 160.0, ya));
            a.Color(rgb(0.0, 0.0, 0.8));
            a.Contents("Line");
            p.Annotations().Add(a);
        }
        ry -= pitch;
        // Ink
        label(rightX, ry, "Ink", "free-hand pen strokes");
        {
            const double bx = rightX + 30.0, by = ry - 45.0;
            ann::InkAnnotation::StrokeList strokes{{
                pdf::Point(bx, by), pdf::Point(bx + 35.0, by + 14.0),
                pdf::Point(bx + 70.0, by), pdf::Point(bx + 105.0, by + 14.0),
                pdf::Point(bx + 140.0, by)}};
            auto& a = arena.make<ann::InkAnnotation>(
                p, Rect(bx, by - 6.0, bx + 145.0, by + 20.0), strokes);
            a.Color(rgb(0.6, 0.0, 0.6));
            a.Contents("Ink");
            p.Annotations().Add(a);
        }
        ry -= pitch;
        // Stamp
        label(rightX, ry, "Stamp", "predefined or custom text stamp");
        {
            auto& a = arena.make<ann::StampAnnotation>(doc);
            a.Rect(Rect(rightX + 30.0, ry - 56.0, rightX + 170.0, ry - 28.0));
            a.Icon(ann::StampIcon::Approved);
            a.Color(rgb(0.0, 0.55, 0.0));
            a.Contents("APPROVED");
            p.Annotations().Add(a);
        }
        ry -= pitch;
        // FileAttachment + document-level embedded file
        label(rightX, ry, "FileAttachment", "embedded file behind an icon");
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
            a.Contents("Attachment");
            p.Annotations().Add(a);
        }

        Footer(p, 6);
    }

    // ---- Page 7 : tables -------------------------------------------------
    {
        pdf::Page& p = addPage();
        Heading(arena, p, "5. Tables");
        DrawText(p, "A restaurant bill - note the column-spanning total row:",
                 60.0, kH - 110.0, "Helvetica", 12.0f);

        auto& table = arena.make<pdf::Table>();
        table.ColumnWidths("250 120 120");
        table.Left(60.0f);
        table.Top(680.0f);  // y of the table's top edge, measured from the page bottom
        table.Border(pdf::BorderInfo(pdf::BorderSide::All, 0.5f));
        table.DefaultCellBorder(pdf::BorderInfo(pdf::BorderSide::All, 0.5f));

        auto header = [&](const char* a, const char* b, const char* c) {
            pdf::Row& r = table.Rows().Add();
            r.BackgroundColor(pdf::Color::LightGray());
            r.Cells().Add(a);
            r.Cells().Add(b);
            r.Cells().Add(c);
        };
        auto line = [&](const char* a, const char* b, const char* c) {
            pdf::Row& r = table.Rows().Add();
            r.Cells().Add(a);
            r.Cells().Add(b);
            r.Cells().Add(c);
        };
        header("Item", "Qty", "Price");
        line("Margherita pizza", "2", "$24.00");
        line("Caesar salad", "1", "$9.50");
        line("Sparkling water", "3", "$7.50");

        pdf::Row& total = table.Rows().Add();
        pdf::Cell& tc = total.Cells().Add("Total");
        tc.ColSpan(2);
        tc.BackgroundColor(pdf::Color::LightYellow());
        total.Cells().Add("$41.00");

        p.Paragraphs().Add(table);
        Footer(p, 7);
    }

    // ---- Page 8 : vector graphics ---------------------------------------
    {
        pdf::Page& p = addPage();
        Heading(arena, p, "6. Vector Graphics");

        auto& g = arena.make<draw::Graph>(kW - 100.0, 420.0);
        g.Left(40.0);
        g.Top(40.0);
        auto& shapes = g.Shapes();
        // Frame.
        shapes.push_back(std::make_unique<draw::Rectangle>(60.0f, 320.0f, 460.0f, 360.0f));
        // Diagonals.
        shapes.push_back(std::make_unique<draw::Line>(
            std::vector<float>{60.0f, 320.0f, 520.0f, 680.0f}));
        shapes.push_back(std::make_unique<draw::Line>(
            std::vector<float>{60.0f, 680.0f, 520.0f, 320.0f}));
        // A circle and an ellipse.
        shapes.push_back(std::make_unique<draw::Circle>(180.0f, 500.0f, 70.0f));
        shapes.push_back(std::make_unique<draw::Ellipse>(320.0, 460.0, 180.0, 90.0));
        p.Paragraphs().Add(g);

        DrawText(p, "Graph holds Line / Rectangle / Circle / Ellipse shapes.",
                 60.0, kH - 110.0, "Helvetica", 12.0f);
        Footer(p, 8);
    }

    // ---- Page 9 : bar chart (landscape) ---------------------------------
    {
        pdf::Page& p = addPage();
        p.SetPageSize(kH, kW);  // landscape (kH wide × kW tall)
        Heading(arena, p, "7. Bar Chart", kW, kH);  // landscape dims

        const int values[] = {40, 95, 70, 120, 60, 150, 85};
        const char* labels[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        auto& g = arena.make<draw::Graph>(kH - 100.0, kW - 140.0);
        g.Left(60.0);
        g.Top(90.0);
        constexpr float kPitch = 70.0f, kBarW = 40.0f, kBarX0 = 20.0f;
        // Baseline spanning just past the last bar.
        g.Shapes().push_back(std::make_unique<draw::Line>(
            std::vector<float>{0.0f, 0.0f, kBarX0 + 7 * kPitch, 0.0f}));
        float x = kBarX0;
        for (int v : values) {
            g.Shapes().push_back(std::make_unique<draw::Rectangle>(
                x, 0.0f, kBarW, float(v) * 2.0f));
            x += kPitch;
        }
        p.Paragraphs().Add(g);
        // Labels centred under each bar.
        double lx = 60.0 + kBarX0 + 10.0;
        for (const char* l : labels) {
            DrawText(p, l, lx, 75.0, "Helvetica", 10.0f);
            lx += kPitch;
        }
        Footer(p, 9);
    }

    // ---- Page 10 : read-only / pre-filled field -------------------------
    {
        pdf::Page& p = addPage();
        Heading(arena, p, "8. Pre-filled Field");
        DrawText(p, "A second AcroForm text field, pre-filled with a value:",
                 60.0, kH - 110.0, "Helvetica", 12.0f);

        auto& ro = arena.make<frm::TextBoxField>(doc, Rect(60.0, kH - 150.0, 400.0, kH - 125.0));
        ro.PartialName("issuedBy");
        ro.Value("Aspose.PDF FOSS for C++");
        doc.Form().Add(ro, 10);

        DrawText(p, "Form::Flatten() also exists to bake every field into static",
                 60.0, kH - 185.0, "Times-Roman", 11.0f);
        DrawText(p, "page content - it is document-wide, so it is not called here",
                 60.0, kH - 202.0, "Times-Roman", 11.0f);
        DrawText(p, "(it would also flatten the interactive fields on page 5).",
                 60.0, kH - 219.0, "Times-Roman", 11.0f);

        Footer(p, 10);
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
