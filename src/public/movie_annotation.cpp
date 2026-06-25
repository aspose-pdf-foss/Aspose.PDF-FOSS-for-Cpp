#include <aspose/pdf/annotations/movie_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

MovieAnnotation::MovieAnnotation(Aspose::Pdf::Document& /*document*/,
                                  std::string movieFile)
    : file_(std::move(movieFile)) {}

MovieAnnotation::MovieAnnotation(Aspose::Pdf::Page& page,
                                  Aspose::Pdf::Rectangle rect,
                                  std::string movieFile)
    : page_(&page),
      file_(std::move(movieFile)) {
    Rect(std::move(rect));
}

void MovieAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

const std::string& MovieAnnotation::Title() const noexcept { return title_; }
void MovieAnnotation::Title(std::string v) noexcept { title_ = std::move(v); }

const Aspose::Pdf::FileSpecification&
MovieAnnotation::File() const noexcept { return file_; }
void MovieAnnotation::File(Aspose::Pdf::FileSpecification v) noexcept {
    file_ = std::move(v);
}

bool MovieAnnotation::Poster() const noexcept { return poster_; }
void MovieAnnotation::Poster(bool v) noexcept { poster_ = v; }

const Aspose::Pdf::Point& MovieAnnotation::Aspect() const noexcept {
    return aspect_;
}
void MovieAnnotation::Aspect(Aspose::Pdf::Point v) noexcept {
    aspect_ = std::move(v);
}

int MovieAnnotation::Rotate() const noexcept { return rotate_; }
void MovieAnnotation::Rotate(int v) noexcept { rotate_ = v; }

AnnotationType MovieAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Movie;
}

}  // namespace Aspose::Pdf::Annotations
