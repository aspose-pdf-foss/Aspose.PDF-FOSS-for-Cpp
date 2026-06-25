#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::LineAnnotation — straight-line
// annotation (PDF /Subtype /Line per §12.5.6.7). Mirrors
// canonical Aspose.Pdf.Annotations.LineAnnotation; extends
// MarkupAnnotation directly (not CommonFigureAnnotation).
//
// Phased drops on this beat (drops.yaml):
//   * ChangeAfterResize(Matrix) — Matrix deferred
//   * Measure — Aspose.Pdf.Annotations.Measure deferred (has
//     nested Measure.NumberFormatList class cascade)
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/caption_position.hpp>
#include <aspose/pdf/annotations/line_ending.hpp>
#include <aspose/pdf/annotations/line_intent.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class LineAnnotation : public MarkupAnnotation {
public:
    LineAnnotation(Aspose::Pdf::Document& document,
                   Aspose::Pdf::Point start,
                   Aspose::Pdf::Point end);
    LineAnnotation(Aspose::Pdf::Page& page,
                   Aspose::Pdf::Rectangle rect,
                   Aspose::Pdf::Point start,
                   Aspose::Pdf::Point end);

    void Accept(AnnotationSelector& visitor) override;

    const Aspose::Pdf::Point& Starting() const noexcept;
    void Starting(Aspose::Pdf::Point value) noexcept;

    LineEnding StartingStyle() const noexcept;
    void StartingStyle(LineEnding value) noexcept;

    const Aspose::Pdf::Point& Ending() const noexcept;
    void Ending(Aspose::Pdf::Point value) noexcept;

    LineEnding EndingStyle() const noexcept;
    void EndingStyle(LineEnding value) noexcept;

    Aspose::Pdf::Color InteriorColor() const noexcept;
    void InteriorColor(Aspose::Pdf::Color value) noexcept;

    double LeaderLine() const noexcept;
    void LeaderLine(double value) noexcept;

    double LeaderLineExtension() const noexcept;
    void LeaderLineExtension(double value) noexcept;

    bool ShowCaption() const noexcept;
    void ShowCaption(bool value) noexcept;

    double LeaderLineOffset() const noexcept;
    void LeaderLineOffset(double value) noexcept;

    const Aspose::Pdf::Point& CaptionOffset() const noexcept;
    void CaptionOffset(Aspose::Pdf::Point value) noexcept;

    Annotations::CaptionPosition CaptionPosition() const noexcept;
    void CaptionPosition(Annotations::CaptionPosition value) noexcept;

    LineIntent Intent() const noexcept;
    void Intent(LineIntent value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    Aspose::Pdf::Point starting_{0.0, 0.0};
    Aspose::Pdf::Point ending_{0.0, 0.0};
    LineEnding starting_style_ = LineEnding::None;
    LineEnding ending_style_ = LineEnding::None;
    Aspose::Pdf::Color interior_color_;
    double leader_line_ = 0.0;
    double leader_line_extension_ = 0.0;
    double leader_line_offset_ = 0.0;
    bool show_caption_ = false;
    Aspose::Pdf::Point caption_offset_{0.0, 0.0};
    Annotations::CaptionPosition caption_position_
        = Annotations::CaptionPosition::Inline;
    LineIntent intent_ = LineIntent::Undefined;
};

}  // namespace Aspose::Pdf::Annotations
