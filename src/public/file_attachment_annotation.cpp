#include <aspose/pdf/annotations/file_attachment_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

FileAttachmentAnnotation::FileAttachmentAnnotation(
        Aspose::Pdf::Page& page,
        Aspose::Pdf::Rectangle rect,
        Aspose::Pdf::FileSpecification fileSpec)
    : page_(&page),
      file_(std::move(fileSpec)) {
    Rect(std::move(rect));
}

void FileAttachmentAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

const Aspose::Pdf::FileSpecification&
FileAttachmentAnnotation::File() const noexcept {
    return file_;
}
void FileAttachmentAnnotation::File(
        Aspose::Pdf::FileSpecification v) noexcept {
    file_ = std::move(v);
}

FileIcon FileAttachmentAnnotation::Icon() const noexcept { return icon_; }
void FileAttachmentAnnotation::Icon(FileIcon v) noexcept { icon_ = v; }

double FileAttachmentAnnotation::Opacity() const noexcept {
    return MarkupAnnotation::Opacity();
}
void FileAttachmentAnnotation::Opacity(double v) noexcept {
    MarkupAnnotation::Opacity(v);
}

AnnotationType FileAttachmentAnnotation::TypeImpl() const noexcept {
    return AnnotationType::FileAttachment;
}

}  // namespace Aspose::Pdf::Annotations
