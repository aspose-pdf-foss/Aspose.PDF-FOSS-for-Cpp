#include <aspose/pdf/annotations/widget_annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

WidgetAnnotation::WidgetAnnotation(Aspose::Pdf::Document& doc)
    : document_(&doc) {}

void WidgetAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

std::string WidgetAnnotation::GetCheckedStateName() const {
    return std::string{};
}

HighlightingMode WidgetAnnotation::Highlighting() const noexcept {
    return highlighting_;
}
void WidgetAnnotation::Highlighting(HighlightingMode v) noexcept {
    highlighting_ = v;
}

const Annotations::DefaultAppearance&
WidgetAnnotation::DefaultAppearance() const noexcept {
    return default_appearance_;
}
void WidgetAnnotation::DefaultAppearance(
        Annotations::DefaultAppearance v) noexcept {
    default_appearance_ = std::move(v);
}

bool WidgetAnnotation::ReadOnly() const noexcept { return read_only_; }
void WidgetAnnotation::ReadOnly(bool v) noexcept { read_only_ = v; }

bool WidgetAnnotation::Required() const noexcept { return required_; }
void WidgetAnnotation::Required(bool v) noexcept { required_ = v; }

bool WidgetAnnotation::Exportable() const noexcept { return exportable_; }
void WidgetAnnotation::Exportable(bool v) noexcept { exportable_ = v; }

Aspose::Pdf::Forms::Field*
WidgetAnnotation::Parent() const noexcept { return parent_; }
void WidgetAnnotation::Parent(
        Aspose::Pdf::Forms::Field* v) noexcept { parent_ = v; }

AnnotationType WidgetAnnotation::TypeImpl() const noexcept {
    return AnnotationType::Widget;
}

}  // namespace Aspose::Pdf::Annotations
