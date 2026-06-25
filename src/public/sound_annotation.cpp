#include <aspose/pdf/annotations/sound_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

SoundAnnotation::SoundAnnotation(Aspose::Pdf::Page& page,
                                  Aspose::Pdf::Rectangle rect,
                                  std::string soundFile)
    : page_(&page),
      sound_file_(std::move(soundFile)) {
    Rect(std::move(rect));
}

void SoundAnnotation::Accept(AnnotationSelector& visitor) {
    // Canonical Aspose.Pdf.Annotations.AnnotationSelector has no
    // Visit(SoundAnnotation&) overload, so dispatch falls through
    // to the base Annotation::Accept default no-op.
    Annotation::Accept(visitor);
}

SoundIcon SoundAnnotation::Icon() const noexcept { return icon_; }
void SoundAnnotation::Icon(SoundIcon v) noexcept { icon_ = v; }

AnnotationType SoundAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Sound;
}

}  // namespace Aspose::Pdf::Annotations
