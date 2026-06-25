#include <aspose/pdf/text_fragment_absorber.hpp>

#include <cstddef>
#include <utility>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>

namespace Aspose::Pdf::Text {

void TextFragmentAbsorber::BuildFromShows(
        const std::vector<Aspose::Pdf::Document::TextShow>& shows,
        Aspose::Pdf::Document* doc) {
    // 0.5-em-per-char width approximation (no per-glyph font metrics in v1).
    auto box = [](TextFragment& frag, const Document::TextShow& show,
                  double x, std::size_t nchars) {
        const double adv = show.font_size * 0.5;
        frag.rx_ = x;
        frag.ry_ = show.y;
        frag.rw_ = static_cast<double>(nchars) * adv;
        frag.rh_ = show.font_size;
        frag.position_ = Position{x, show.y};
        frag.state_.FontSize(static_cast<float>(show.font_size));
    };

    if (match_all_) {
        // Extract-all: each positioned text run becomes one fragment.
        for (const auto& show : shows) {
            if (show.text.empty()) continue;
            TextFragment frag;
            frag.text_ = show.text;
            frag.doc_ = doc;
            frag.stream_id_ = show.stream_id;
            frag.occurrence_ = show.occurrence;
            frag.original_literal_ = show.text;
            frag.match_offset_ = 0;
            frag.match_len_ = show.text.size();
            box(frag, show, show.x, show.text.size());
            fragments_.items_.push_back(std::move(frag));
        }
        return;
    }

    if (phrase_.empty()) return;
    for (const auto& show : shows) {
        std::size_t pos = 0;
        while ((pos = show.text.find(phrase_, pos)) != std::string::npos) {
            TextFragment frag;
            frag.text_ = phrase_;
            frag.doc_ = doc;
            frag.stream_id_ = show.stream_id;
            frag.occurrence_ = show.occurrence;
            frag.original_literal_ = show.text;
            frag.match_offset_ = pos;
            frag.match_len_ = phrase_.size();
            box(frag, show, show.x + static_cast<double>(pos) * show.font_size *
                                        0.5,
                phrase_.size());
            fragments_.items_.push_back(std::move(frag));
            pos += phrase_.size();
        }
    }
}

TextFragmentAbsorber::TextFragmentAbsorber() : match_all_(true) {}

TextFragmentAbsorber::TextFragmentAbsorber(const std::string& phrase)
    : phrase_(phrase) {}

void TextFragmentAbsorber::Visit(const Aspose::Pdf::Document& pdf) {
    auto* doc = const_cast<Aspose::Pdf::Document*>(&pdf);
    BuildFromShows(doc->ScanTextShowsInternal(0, true), doc);
}

void TextFragmentAbsorber::Visit(const Aspose::Pdf::Page& page) {
    Aspose::Pdf::Document* doc = page.doc_;
    BuildFromShows(doc->ScanTextShowsInternal(page.leaf_index_, false), doc);
}

TextFragmentCollection& TextFragmentAbsorber::TextFragments() noexcept {
    return fragments_;
}

const TextFragmentCollection&
TextFragmentAbsorber::TextFragments() const noexcept {
    return fragments_;
}

}  // namespace Aspose::Pdf::Text
