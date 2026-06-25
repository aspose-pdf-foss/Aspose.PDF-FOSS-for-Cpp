#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::SoundAnnotation — embedded-sound
// annotation (PDF /Subtype /Sound per §12.5.6.16). Mirrors
// canonical Aspose.Pdf.Annotations.SoundAnnotation; extends
// MarkupAnnotation.
//
// Canonical does not declare a Visit overload for SoundAnnotation;
// Accept() therefore falls through to Annotation's default no-op
// behaviour rather than dispatching to a Visit(SoundAnnotation&).
//
// Phased drops:
//   * .ctor(Page&, Rect, soundFile, SoundSampleData) — SoundSampleData
//     class deferred
//   * SoundData property — SoundData class deferred
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/annotations/sound_icon.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class SoundAnnotation : public MarkupAnnotation {
public:
    SoundAnnotation(Aspose::Pdf::Page& page,
                    Aspose::Pdf::Rectangle rect,
                    std::string soundFile);

    void Accept(AnnotationSelector& visitor) override;

    SoundIcon Icon() const noexcept;
    void Icon(SoundIcon value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    std::string sound_file_;
    SoundIcon icon_ = SoundIcon::Speaker;
};

}  // namespace Aspose::Pdf::Annotations
