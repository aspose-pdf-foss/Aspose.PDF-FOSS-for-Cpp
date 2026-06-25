#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::FileAttachmentAnnotation — embedded
// file annotation (PDF /Subtype /FileAttachment per §12.5.6.15).
// Mirrors canonical Aspose.Pdf.Annotations.FileAttachmentAnnotation;
// extends MarkupAnnotation.
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/file_icon.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/file_specification.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Annotations {

class FileAttachmentAnnotation : public MarkupAnnotation {
public:
    FileAttachmentAnnotation(Aspose::Pdf::Page& page,
                             Aspose::Pdf::Rectangle rect,
                             Aspose::Pdf::FileSpecification fileSpec);

    void Accept(AnnotationSelector& visitor) override;

    const Aspose::Pdf::FileSpecification& File() const noexcept;
    void File(Aspose::Pdf::FileSpecification value) noexcept;

    FileIcon Icon() const noexcept;
    void Icon(FileIcon value) noexcept;

    double Opacity() const noexcept;
    void Opacity(double value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    Aspose::Pdf::FileSpecification file_;
    FileIcon icon_ = FileIcon::PushPin;
};

}  // namespace Aspose::Pdf::Annotations
