#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::ScreenAnnotation — screen annotation
// for embedded media playback (PDF /Subtype /Screen per
// §12.5.6.18). Mirrors canonical
// Aspose.Pdf.Annotations.ScreenAnnotation; extends Annotation
// directly.
//
// Phased drops:
//   * Action (PdfAction) — PdfAction cascade deferred
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class ScreenAnnotation : public Annotation {
public:
    ScreenAnnotation(Aspose::Pdf::Page& page,
                     Aspose::Pdf::Rectangle rect,
                     std::string mediaFile);

    void Accept(AnnotationSelector& visitor) override;

    const std::string& Title() const noexcept;
    void Title(std::string value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    std::string media_file_;
    std::string title_;
};

}  // namespace Aspose::Pdf::Annotations
