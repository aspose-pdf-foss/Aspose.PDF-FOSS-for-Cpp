// Bodies for the positioned-text-insertion cluster: Position and TextBuilder.
// TextBuilder::AppendText writes the fragment's text into the page content
// stream via Document::AddTextToPage (the same incremental-update path that
// backs Page::AddImage).

#include <aspose/pdf/text_builder.hpp>

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include <aspose/pdf/color.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/font.hpp>
#include <aspose/pdf/font_repository.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/position.hpp>
#include <aspose/pdf/text_fragment.hpp>
#include <aspose/pdf/text_fragment_state.hpp>
#include <aspose/pdf/text_paragraph.hpp>
#include <aspose/pdf/text_state.hpp>

namespace Aspose::Pdf::Text {

// ---- Position --------------------------------------------------------------

Position::Position(double xIndent, double yIndent) noexcept
    : x_indent_(xIndent), y_indent_(yIndent) {}

double Position::XIndent() const noexcept { return x_indent_; }
void Position::XIndent(double value) noexcept { x_indent_ = value; }

double Position::YIndent() const noexcept { return y_indent_; }
void Position::YIndent(double value) noexcept { y_indent_ = value; }

// ---- TextBuilder -----------------------------------------------------------

TextBuilder::TextBuilder(Aspose::Pdf::Page& page) : page_(&page) {}

void TextBuilder::AppendText(Aspose::Pdf::Text::TextFragment& textFragment) {
    if (page_ == nullptr || page_->doc_ == nullptr) return;
    const auto pos = textFragment.Position();
    const float size = textFragment.TextState().FontSize();
    const Aspose::Pdf::Text::Font& font = textFragment.TextState().Font();
    // The fragment's authored foreground colour (opaque ⇒ emitted as the text
    // fill; the default transparent colour leaves the text black).
    const Aspose::Pdf::Color& fg = textFragment.TextState().ForegroundColor();
    // A font loaded via FontRepository::OpenFont carries its program — embed
    // it (FontFile2) so the text renders in that font; otherwise draw with the
    // named Standard-14 font.
    if (font.is_embedded_ && !font.program_.empty()) {
        page_->doc_->AddEmbeddedTextToPage(
            page_->leaf_index_, textFragment.Text(), pos.XIndent(),
            pos.YIndent(), font.program_, font.FontName(),
            static_cast<double>(size), font.is_subset_, &fg);
    } else {
        page_->doc_->AddTextToPage(page_->leaf_index_, textFragment.Text(),
                                   pos.XIndent(), pos.YIndent(),
                                   font.FontName(), static_cast<double>(size),
                                   &fg);
    }
}

void TextBuilder::AppendParagraph(
        Aspose::Pdf::Text::TextParagraph& textParagraph) {
    if (page_ == nullptr || page_->doc_ == nullptr) return;
    Aspose::Pdf::Document* doc = page_->doc_;
    const std::size_t leaf = page_->leaf_index_;
    const TextParagraph& p = textParagraph;

    // Measure a string in a given font at a given size (points) using the
    // canonical TextState.MeasureString path (Standard-14 exact).
    auto measure = [](const Aspose::Pdf::Text::Font& f, double fs,
                      const std::string& s) -> double {
        Aspose::Pdf::Text::TextState ts;
        // MeasureString reads only the font name (Standard-14 metrics), so
        // resolve by name — avoids copying an embedded font's program buffer
        // on every wrap/justify measurement. An empty name (unstyled line)
        // defaults to Helvetica, matching what emit() renders.
        const std::string mname =
            f.FontName().empty() ? "Helvetica" : f.FontName();
        ts.Font(Aspose::Pdf::Text::FontRepository::FindFont(mname));
        ts.FontSize(static_cast<float>(fs));
        return ts.MeasureString(s);
    };

    // Emit one run, mirroring AppendText: embed the program when present,
    // else draw with the named (or default Helvetica) font; carry the colour.
    auto emit = [&](const std::string& text, const Aspose::Pdf::Text::Font& font,
                    double fs, const Aspose::Pdf::Color& fg, double x,
                    double yy) {
        if (text.empty()) return;
        if (font.is_embedded_ && !font.program_.empty()) {
            doc->AddEmbeddedTextToPage(leaf, text, x, yy, font.program_,
                                       font.FontName(), fs, font.is_subset_,
                                       &fg);
        } else {
            const std::string name =
                font.FontName().empty() ? "Helvetica" : font.FontName();
            doc->AddTextToPage(leaf, text, x, yy, name, fs, &fg);
        }
    };

    // Box geometry: a Rectangle (origin + wrap width) when set, else Position.
    const bool has_box = p.rect_.has_value();
    double x0 = has_box ? p.rect_->LLX() : p.position_.XIndent();
    double top_y = has_box ? p.rect_->URY() : p.position_.YIndent();
    double width = has_box ? p.rect_->Width() : 0.0;
    x0 += p.margin_.Left();
    top_y -= p.margin_.Top();
    if (width > 0.0) {
        width -= p.margin_.Left() + p.margin_.Right();
        if (width < 0.0) width = 0.0;
    }

    // Word-wrap each logical line into physical lines using real metrics.
    struct PLine {
        std::string text;
        Aspose::Pdf::Text::Font font;
        Aspose::Pdf::Color color;
        double fs;
        double adv;
    };
    std::vector<PLine> phys;
    for (const auto& ln : p.lines_) {
        const double fs = ln.font_size > 0.0f ? ln.font_size : 12.0;
        const double adv = ln.line_spacing > 0.0f ? ln.line_spacing : fs * 1.2;
        if (width <= 0.0 || ln.text.empty()) {
            phys.push_back({ln.text, ln.font, ln.color, fs, adv});
            continue;
        }
        std::istringstream iss(ln.text);
        std::string word, cur;
        while (iss >> word) {
            const std::string trial = cur.empty() ? word : cur + " " + word;
            if (measure(ln.font, fs, trial) > width && !cur.empty()) {
                phys.push_back({cur, ln.font, ln.color, fs, adv});
                cur = word;
            } else {
                cur = trial;
            }
        }
        phys.push_back({cur, ln.font, ln.color, fs, adv});
    }
    if (phys.empty()) return;

    // Vertical alignment shifts the whole block within the box height.
    double start_y = top_y;
    if (has_box && p.valign_ != Aspose::Pdf::VerticalAlignment::Top) {
        const double box_h =
            p.rect_->Height() - (p.margin_.Top() + p.margin_.Bottom());
        double total_h = 0.0;
        for (const auto& pl : phys) total_h += pl.adv;
        double slack = box_h - total_h;
        if (slack < 0.0) slack = 0.0;
        if (p.valign_ == Aspose::Pdf::VerticalAlignment::Center)
            start_y = top_y - slack / 2.0;
        else if (p.valign_ == Aspose::Pdf::VerticalAlignment::Bottom)
            start_y = top_y - slack;
    }

    const bool justify =
        p.halign_ == Aspose::Pdf::HorizontalAlignment::Justify ||
        p.halign_ == Aspose::Pdf::HorizontalAlignment::FullJustify;

    double y = start_y;
    for (std::size_t i = 0; i < phys.size(); ++i) {
        y -= phys[i].fs;  // drop to this line's baseline
        const PLine& pl = phys[i];
        if (!pl.text.empty()) {
            const double indent =
                i == 0 ? p.first_line_indent_ : p.subsequent_lines_indent_;
            const double avail = width > 0.0 ? width - indent : 0.0;
            const double line_w = measure(pl.font, pl.fs, pl.text);
            // Last PHYSICAL line of the whole paragraph (across all AppendLine
            // logical lines) — this is the line plain Justify leaves ragged.
            const bool is_last = (i + 1 == phys.size());

            // Justify (except the final line of plain Justify) distributes the
            // slack across inter-word gaps; otherwise place the whole line.
            const bool do_justify =
                width > 0.0 && justify && avail > line_w &&
                !(p.halign_ == Aspose::Pdf::HorizontalAlignment::Justify &&
                  is_last);

            if (do_justify) {
                std::vector<std::string> words;
                std::istringstream wss(pl.text);
                std::string w;
                while (wss >> w) words.push_back(w);
                if (words.size() <= 1) {
                    emit(pl.text, pl.font, pl.fs, pl.color, x0 + indent, y);
                } else {
                    double words_w = 0.0;
                    for (const auto& wd : words)
                        words_w += measure(pl.font, pl.fs, wd);
                    const double gap =
                        (avail - words_w) /
                        static_cast<double>(words.size() - 1);
                    // Justify emits each word as its own positioned run. For an
                    // embedded font this re-embeds the program per word (file
                    // bloat); acceptable for the common named-font path. A
                    // future refinement could batch via word-spacing (Tw).
                    double cx = x0 + indent;
                    for (const auto& wd : words) {
                        emit(wd, pl.font, pl.fs, pl.color, cx, y);
                        cx += measure(pl.font, pl.fs, wd) + gap;
                    }
                }
            } else {
                double x = x0 + indent;
                if (width > 0.0 &&
                    p.halign_ == Aspose::Pdf::HorizontalAlignment::Center)
                    x = x0 + (width - line_w) / 2.0;
                else if (width > 0.0 &&
                         p.halign_ == Aspose::Pdf::HorizontalAlignment::Right)
                    x = x0 + width - line_w;
                emit(pl.text, pl.font, pl.fs, pl.color, x, y);
            }
        }
        y -= phys[i].adv - phys[i].fs;  // complete the line advance
    }
}

}  // namespace Aspose::Pdf::Text
