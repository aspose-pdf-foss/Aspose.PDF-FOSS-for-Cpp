#include <aspose/pdf/annotations/line_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

LineAnnotation::LineAnnotation(Aspose::Pdf::Document& document,
                                Aspose::Pdf::Point start,
                                Aspose::Pdf::Point end)
    : MarkupAnnotation(document),
      starting_(start),
      ending_(end) {}

LineAnnotation::LineAnnotation(Aspose::Pdf::Page& page,
                                Aspose::Pdf::Rectangle rect,
                                Aspose::Pdf::Point start,
                                Aspose::Pdf::Point end)
    : page_(&page),
      starting_(start),
      ending_(end) {
    Rect(std::move(rect));
}

void LineAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

const Aspose::Pdf::Point& LineAnnotation::Starting() const noexcept {
    return starting_;
}
void LineAnnotation::Starting(Aspose::Pdf::Point v) noexcept {
    starting_ = v;
}

LineEnding LineAnnotation::StartingStyle() const noexcept {
    return starting_style_;
}
void LineAnnotation::StartingStyle(LineEnding v) noexcept {
    starting_style_ = v;
}

const Aspose::Pdf::Point& LineAnnotation::Ending() const noexcept {
    return ending_;
}
void LineAnnotation::Ending(Aspose::Pdf::Point v) noexcept {
    ending_ = v;
}

LineEnding LineAnnotation::EndingStyle() const noexcept {
    return ending_style_;
}
void LineAnnotation::EndingStyle(LineEnding v) noexcept {
    ending_style_ = v;
}

Aspose::Pdf::Color LineAnnotation::InteriorColor() const noexcept {
    return interior_color_;
}
void LineAnnotation::InteriorColor(Aspose::Pdf::Color v) noexcept {
    interior_color_ = std::move(v);
}

double LineAnnotation::LeaderLine() const noexcept {
    return leader_line_;
}
void LineAnnotation::LeaderLine(double v) noexcept {
    leader_line_ = v;
}

double LineAnnotation::LeaderLineExtension() const noexcept {
    return leader_line_extension_;
}
void LineAnnotation::LeaderLineExtension(double v) noexcept {
    leader_line_extension_ = v;
}

bool LineAnnotation::ShowCaption() const noexcept {
    return show_caption_;
}
void LineAnnotation::ShowCaption(bool v) noexcept {
    show_caption_ = v;
}

double LineAnnotation::LeaderLineOffset() const noexcept {
    return leader_line_offset_;
}
void LineAnnotation::LeaderLineOffset(double v) noexcept {
    leader_line_offset_ = v;
}

const Aspose::Pdf::Point& LineAnnotation::CaptionOffset() const noexcept {
    return caption_offset_;
}
void LineAnnotation::CaptionOffset(Aspose::Pdf::Point v) noexcept {
    caption_offset_ = v;
}

CaptionPosition LineAnnotation::CaptionPosition() const noexcept {
    return caption_position_;
}
void LineAnnotation::CaptionPosition(
        Annotations::CaptionPosition v) noexcept {
    caption_position_ = v;
}

LineIntent LineAnnotation::Intent() const noexcept { return intent_; }
void LineAnnotation::Intent(LineIntent v) noexcept { intent_ = v; }

AnnotationType LineAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Line;
}

}  // namespace Aspose::Pdf::Annotations
