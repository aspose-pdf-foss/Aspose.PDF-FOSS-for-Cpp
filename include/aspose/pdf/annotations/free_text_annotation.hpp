#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::FreeTextAnnotation — text-in-a-box
// annotation. Mirrors canonical Aspose.Pdf.Annotations.
// FreeTextAnnotation (Aspose.PDF 26.4.0); extends MarkupAnnotation.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/default_appearance.hpp>
#include <aspose/pdf/annotations/free_text_intent.hpp>
#include <aspose/pdf/annotations/justification.hpp>
#include <aspose/pdf/annotations/line_ending.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/annotations/rich_text_font_styles.hpp>
#include <aspose/pdf/annotations/text_style.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/rotation.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class FreeTextAnnotation : public MarkupAnnotation {
public:
    FreeTextAnnotation(Aspose::Pdf::Document& document,
                       Annotations::DefaultAppearance appearance);
    FreeTextAnnotation(Aspose::Pdf::Page& page,
                       Aspose::Pdf::Rectangle rect,
                       Annotations::DefaultAppearance appearance);

    void Accept(AnnotationSelector& visitor) override;

    // Apply RichTextFontStyles over the rich-text run between
    // fromInd and toInd (canonical inclusive-exclusive
    // semantics). The Drawing.Color-typed overload drops via
    // translations cascade.
    void SetTextStyle(int fromInd, int toInd,
                      RichTextFontStyles textStyles);

    LineEnding StartingStyle() const noexcept;
    void StartingStyle(LineEnding value) noexcept;

    LineEnding EndingStyle() const noexcept;
    void EndingStyle(LineEnding value) noexcept;

    Annotations::Justification Justification() const noexcept;
    void Justification(Annotations::Justification value) noexcept;

    const std::string& DefaultAppearance() const noexcept;
    void DefaultAppearance(std::string value) noexcept;

    const Annotations::DefaultAppearance& DefaultAppearanceObject() const noexcept;

    FreeTextIntent Intent() const noexcept;
    void Intent(FreeTextIntent value) noexcept;

    const std::string& DefaultStyle() const noexcept;
    void DefaultStyle(std::string value) noexcept;

    const Annotations::TextStyle& TextStyle() const noexcept;
    void TextStyle(Annotations::TextStyle value) noexcept;

    Aspose::Pdf::Rotation Rotate() const noexcept;
    void Rotate(Aspose::Pdf::Rotation value) noexcept;

    const std::vector<Aspose::Pdf::Point>& Callout() const noexcept;
    void Callout(std::vector<Aspose::Pdf::Point> value) noexcept;

    const Aspose::Pdf::Rectangle& TextRectangle() const noexcept;
    void TextRectangle(Aspose::Pdf::Rectangle value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    Annotations::DefaultAppearance default_appearance_;
    std::string default_appearance_string_;
    std::string default_style_;
    Annotations::TextStyle text_style_;
    LineEnding starting_style_ = LineEnding::None;
    LineEnding ending_style_ = LineEnding::None;
    Annotations::Justification justification_
        = Annotations::Justification::LeftJustify;
    FreeTextIntent intent_ = FreeTextIntent::Undefined;
    Aspose::Pdf::Rotation rotate_ = Aspose::Pdf::Rotation::None;
    std::vector<Aspose::Pdf::Point> callout_;
    Aspose::Pdf::Rectangle text_rectangle_
        = Aspose::Pdf::Rectangle::Trivial();
};

}  // namespace Aspose::Pdf::Annotations
