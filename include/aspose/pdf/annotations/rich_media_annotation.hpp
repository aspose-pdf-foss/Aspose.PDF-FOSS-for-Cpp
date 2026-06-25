#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::RichMediaAnnotation — rich-media
// annotation (PDF 1.7 ExtensionLevel 3 /Subtype /RichMedia).
// Mirrors canonical Aspose.Pdf.Annotations.RichMediaAnnotation;
// extends Annotation directly.
//
// Phased drops:
//   * AddCustomData(string, Stream) — Stream cascade
//   * SetContent(string, Stream) — Stream cascade
//   * SetPoster(Stream) — Stream cascade
//   * CustomPlayer (Stream property) — Stream cascade
//   * Content (Stream property) — Stream cascade
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class RichMediaAnnotation : public Annotation {
public:
    enum class ContentType : int {
        Audio = 0,
        Video = 1,
        Unknown = 2,
    };

    enum class ActivationEvent : int {
        Click = 0,
        PageOpen = 1,
        PageVisible = 2,
    };

    RichMediaAnnotation(Aspose::Pdf::Page& page,
                        Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

    // Re-emit the annotation's stream content from the configured
    // sources. v1 is a no-op; full update wiring lands with the
    // Document save-time edit-flow.
    void Update();

    const std::string& CustomFlashVariables() const noexcept;
    void CustomFlashVariables(std::string value) noexcept;

    ContentType Type() const noexcept;
    void Type(ContentType value) noexcept;

    ActivationEvent ActivateOn() const noexcept;
    void ActivateOn(ActivationEvent value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    std::string custom_flash_variables_;
    ContentType type_ = ContentType::Unknown;
    ActivationEvent activate_on_ = ActivationEvent::Click;
};

}  // namespace Aspose::Pdf::Annotations
