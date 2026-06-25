#include <aspose/pdf/annotations/free_text_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

FreeTextAnnotation::FreeTextAnnotation(
        Aspose::Pdf::Document& document,
        Annotations::DefaultAppearance appearance)
    : MarkupAnnotation(document),
      default_appearance_(std::move(appearance)) {}

FreeTextAnnotation::FreeTextAnnotation(
        Aspose::Pdf::Page& page,
        Aspose::Pdf::Rectangle rect,
        Annotations::DefaultAppearance appearance)
    : page_(&page),
      default_appearance_(std::move(appearance)) {
    Rect(std::move(rect));
}

void FreeTextAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

void FreeTextAnnotation::SetTextStyle(
        int fromInd, int toInd,
        RichTextFontStyles textStyles) {
    // v1 stub — full rich-text-run mutation pipeline lands with
    // a future appearance-stream generator beat. Stores nothing
    // observable on the public surface.
    (void)fromInd;
    (void)toInd;
    (void)textStyles;
}

LineEnding FreeTextAnnotation::StartingStyle() const noexcept {
    return starting_style_;
}
void FreeTextAnnotation::StartingStyle(LineEnding v) noexcept {
    starting_style_ = v;
}

LineEnding FreeTextAnnotation::EndingStyle() const noexcept {
    return ending_style_;
}
void FreeTextAnnotation::EndingStyle(LineEnding v) noexcept {
    ending_style_ = v;
}

Justification FreeTextAnnotation::Justification() const noexcept {
    return justification_;
}
void FreeTextAnnotation::Justification(
        Annotations::Justification v) noexcept {
    justification_ = v;
}

const std::string& FreeTextAnnotation::DefaultAppearance() const noexcept {
    return default_appearance_string_;
}
void FreeTextAnnotation::DefaultAppearance(std::string v) noexcept {
    default_appearance_string_ = std::move(v);
}

const Annotations::DefaultAppearance&
        FreeTextAnnotation::DefaultAppearanceObject() const noexcept {
    return default_appearance_;
}

FreeTextIntent FreeTextAnnotation::Intent() const noexcept {
    return intent_;
}
void FreeTextAnnotation::Intent(FreeTextIntent v) noexcept {
    intent_ = v;
}

const std::string& FreeTextAnnotation::DefaultStyle() const noexcept {
    return default_style_;
}
void FreeTextAnnotation::DefaultStyle(std::string v) noexcept {
    default_style_ = std::move(v);
}

const Annotations::TextStyle&
        FreeTextAnnotation::TextStyle() const noexcept {
    return text_style_;
}
void FreeTextAnnotation::TextStyle(Annotations::TextStyle v) noexcept {
    text_style_ = std::move(v);
}

Aspose::Pdf::Rotation FreeTextAnnotation::Rotate() const noexcept {
    return rotate_;
}
void FreeTextAnnotation::Rotate(Aspose::Pdf::Rotation v) noexcept {
    rotate_ = v;
}

const std::vector<Aspose::Pdf::Point>&
        FreeTextAnnotation::Callout() const noexcept {
    return callout_;
}
void FreeTextAnnotation::Callout(
        std::vector<Aspose::Pdf::Point> v) noexcept {
    callout_ = std::move(v);
}

const Aspose::Pdf::Rectangle&
        FreeTextAnnotation::TextRectangle() const noexcept {
    return text_rectangle_;
}
void FreeTextAnnotation::TextRectangle(
        Aspose::Pdf::Rectangle v) noexcept {
    text_rectangle_ = std::move(v);
}

AnnotationType FreeTextAnnotation::TypeImpl() const noexcept {
    return AnnotationType::FreeText;
}

}  // namespace Aspose::Pdf::Annotations
