#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::MovieAnnotation — embedded-movie
// annotation (PDF /Subtype /Movie per §12.5.6.17). Mirrors
// canonical Aspose.Pdf.Annotations.MovieAnnotation; extends
// Annotation directly.
// =============================================================================

#include <string>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/file_specification.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class MovieAnnotation : public Annotation {
public:
    MovieAnnotation(Aspose::Pdf::Document& document,
                    std::string movieFile);
    MovieAnnotation(Aspose::Pdf::Page& page,
                    Aspose::Pdf::Rectangle rect,
                    std::string movieFile);

    void Accept(AnnotationSelector& visitor) override;

    const std::string& Title() const noexcept;
    void Title(std::string value) noexcept;

    const Aspose::Pdf::FileSpecification& File() const noexcept;
    void File(Aspose::Pdf::FileSpecification value) noexcept;

    bool Poster() const noexcept;
    void Poster(bool value) noexcept;

    const Aspose::Pdf::Point& Aspect() const noexcept;
    void Aspect(Aspose::Pdf::Point value) noexcept;

    int Rotate() const noexcept;
    void Rotate(int value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    std::string title_;
    Aspose::Pdf::FileSpecification file_;
    bool poster_ = false;
    Aspose::Pdf::Point aspect_{0.0, 0.0};
    int rotate_ = 0;
};

}  // namespace Aspose::Pdf::Annotations
