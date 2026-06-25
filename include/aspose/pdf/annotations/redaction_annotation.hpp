#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::RedactionAnnotation — redaction
// annotation (PDF /Subtype /Redact per §12.5.6.21 / Aspose
// extension to the spec for content-redaction). Mirrors canonical
// Aspose.Pdf.Annotations.RedactionAnnotation; extends
// MarkupAnnotation.
//
// Redact() performs the actual content removal; Flatten()
// removes the annotation but leaves underlying content intact.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class RedactionAnnotation : public MarkupAnnotation {
public:
    explicit RedactionAnnotation(Aspose::Pdf::Document& document);
    RedactionAnnotation(Aspose::Pdf::Page& page,
                        Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

    // Removes the annotation from the page, leaving the underlying
    // page content unchanged.
    void Flatten();

    // Performs the redaction — removes the content under the
    // annotation's QuadPoint region(s) and optionally fills with
    // FillColor + OverlayText.
    void Redact();

    // ---- Properties ----

    const std::vector<Aspose::Pdf::Point>& QuadPoint() const noexcept;
    void QuadPoint(std::vector<Aspose::Pdf::Point> value) noexcept;

    const std::string& DefaultAppearance() const noexcept;
    void DefaultAppearance(std::string value) noexcept;

    const Aspose::Pdf::Color& FillColor() const noexcept;
    void FillColor(Aspose::Pdf::Color value) noexcept;

    const Aspose::Pdf::Color& BorderColor() const noexcept;
    void BorderColor(Aspose::Pdf::Color value) noexcept;

    const std::string& OverlayText() const noexcept;
    void OverlayText(std::string value) noexcept;

    float FontSize() const noexcept;
    void FontSize(float value) noexcept;

    bool Repeat() const noexcept;
    void Repeat(bool value) noexcept;

    Aspose::Pdf::HorizontalAlignment TextAlignment() const noexcept;
    void TextAlignment(Aspose::Pdf::HorizontalAlignment value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    std::vector<Aspose::Pdf::Point> quad_points_;
    std::string default_appearance_;
    Aspose::Pdf::Color fill_color_ = Aspose::Pdf::Color::Black();
    Aspose::Pdf::Color border_color_ = Aspose::Pdf::Color::Black();
    std::string overlay_text_;
    float font_size_ = 0.0f;
    bool repeat_ = false;
    Aspose::Pdf::HorizontalAlignment text_alignment_ =
        Aspose::Pdf::HorizontalAlignment::Left;
};

}  // namespace Aspose::Pdf::Annotations
