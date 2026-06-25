#include <aspose/pdf/annotations/redaction_annotation.hpp>

#include <algorithm>
#include <limits>
#include <utility>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>

namespace Aspose::Pdf::Annotations {

RedactionAnnotation::RedactionAnnotation(Aspose::Pdf::Document& document)
    : MarkupAnnotation(document) {}

RedactionAnnotation::RedactionAnnotation(Aspose::Pdf::Page& page,
                                          Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void RedactionAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

void RedactionAnnotation::Flatten() {
    Annotation::Flatten();
}

void RedactionAnnotation::Redact() {
    // A redaction must REMOVE the underlying content, not merely cover it.
    // Delegate to the Document: text shows intersecting the region are
    // dropped from the content stream (so they are no longer extractable),
    // then an opaque cover box + overlay text are painted on save. Requires
    // the page-bound ctor (a Document-only annotation has no page to redact).
    if (page_ == nullptr || page_->doc_ == nullptr) return;

    Aspose::Pdf::Rectangle region = Rect();
    // Fall back to the QuadPoints bounding box when Rect is unset.
    if ((region.Width() <= 0.0 || region.Height() <= 0.0) &&
        !quad_points_.empty()) {
        double minx = std::numeric_limits<double>::max();
        double miny = std::numeric_limits<double>::max();
        double maxx = -std::numeric_limits<double>::max();
        double maxy = -std::numeric_limits<double>::max();
        for (const auto& p : quad_points_) {
            minx = std::min(minx, p.X());
            miny = std::min(miny, p.Y());
            maxx = std::max(maxx, p.X());
            maxy = std::max(maxy, p.Y());
        }
        region = Aspose::Pdf::Rectangle(minx, miny, maxx, maxy, true);
    }

    page_->doc_->ApplyRedactionInternal(
        page_->leaf_index_, region, overlay_text_,
        static_cast<double>(font_size_), static_cast<int>(text_alignment_));
}

const std::vector<Aspose::Pdf::Point>&
RedactionAnnotation::QuadPoint() const noexcept {
    return quad_points_;
}
void RedactionAnnotation::QuadPoint(
        std::vector<Aspose::Pdf::Point> v) noexcept {
    quad_points_ = std::move(v);
}

const std::string& RedactionAnnotation::DefaultAppearance() const noexcept {
    return default_appearance_;
}
void RedactionAnnotation::DefaultAppearance(std::string v) noexcept {
    default_appearance_ = std::move(v);
}

const Aspose::Pdf::Color& RedactionAnnotation::FillColor() const noexcept {
    return fill_color_;
}
void RedactionAnnotation::FillColor(Aspose::Pdf::Color v) noexcept {
    fill_color_ = std::move(v);
}

const Aspose::Pdf::Color& RedactionAnnotation::BorderColor() const noexcept {
    return border_color_;
}
void RedactionAnnotation::BorderColor(Aspose::Pdf::Color v) noexcept {
    border_color_ = std::move(v);
}

const std::string& RedactionAnnotation::OverlayText() const noexcept {
    return overlay_text_;
}
void RedactionAnnotation::OverlayText(std::string v) noexcept {
    overlay_text_ = std::move(v);
}

float RedactionAnnotation::FontSize() const noexcept { return font_size_; }
void RedactionAnnotation::FontSize(float v) noexcept { font_size_ = v; }

bool RedactionAnnotation::Repeat() const noexcept { return repeat_; }
void RedactionAnnotation::Repeat(bool v) noexcept { repeat_ = v; }

Aspose::Pdf::HorizontalAlignment
RedactionAnnotation::TextAlignment() const noexcept {
    return text_alignment_;
}
void RedactionAnnotation::TextAlignment(
        Aspose::Pdf::HorizontalAlignment v) noexcept {
    text_alignment_ = v;
}

AnnotationType RedactionAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Redaction;
}

}  // namespace Aspose::Pdf::Annotations
