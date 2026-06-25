#include <aspose/pdf/forms/field.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace Aspose::Pdf::Forms {

Field::Field(Aspose::Pdf::Document& doc)
    : Aspose::Pdf::Annotations::WidgetAnnotation(doc) {}

bool Field::Recalculate() { return true; }

void Field::Flatten() {
    Aspose::Pdf::Annotations::Annotation::Flatten();
}

void Field::SetPosition(Aspose::Pdf::Point point) {
    const auto& current = Rect();
    const double w = current.Width();
    const double h = current.Height();
    Aspose::Pdf::Rectangle moved{
        point.X(), point.Y(),
        point.X() + w, point.Y() + h,
        false};
    Rect(std::move(moved));
}

const std::string& Field::PartialName() const noexcept {
    return partial_name_;
}
void Field::PartialName(std::string v) { partial_name_ = std::move(v); }

const std::string& Field::AlternateName() const noexcept {
    return alternate_name_;
}
void Field::AlternateName(std::string v) { alternate_name_ = std::move(v); }

const std::string& Field::MappingName() const noexcept {
    return mapping_name_;
}
void Field::MappingName(std::string v) { mapping_name_ = std::move(v); }

const std::string& Field::Value() const noexcept { return value_; }
void Field::Value(std::string v) { value_ = std::move(v); }

int Field::Count() const noexcept {
    // 1 (this Field IS the default rendition) + any owned kids.
    return 1 + static_cast<int>(kids_.size());
}

bool Field::IsGroup() const noexcept {
    return Count() > 1;
}

Aspose::Pdf::Annotations::WidgetAnnotation&
Field::operator[](const std::string& name) {
    // The Field itself is the default rendition — its PartialName
    // matches `name` for unshared fields. v1 doesn't carry a name
    // on widget renditions other than the field itself; falls
    // back to throwing when no kid matches.
    if (name == partial_name_) return *this;
    for (auto* kid : kids_) {
        if (kid != nullptr) {
            // Widget renditions don't carry partial names by
            // themselves in canonical .NET; the comparison hook
            // is here for symmetry with the indexer overload.
            (void)kid;
        }
    }
    throw std::out_of_range(
        "Aspose::Pdf::Forms::Field::operator[](name): "
        "no widget rendition named '" + name + "'");
}

Aspose::Pdf::Annotations::WidgetAnnotation&
Field::operator[](int index) {
    if (index == 0) return *this;
    const int kid_index = index - 1;
    if (kid_index < 0 ||
        static_cast<std::size_t>(kid_index) >= kids_.size()) {
        throw std::out_of_range(
            "Aspose::Pdf::Forms::Field::operator[](int): "
            "index " + std::to_string(index) +
            " out of range [0.." + std::to_string(Count()) + ")");
    }
    auto* kid = kids_[static_cast<std::size_t>(kid_index)];
    if (kid == nullptr) {
        throw std::out_of_range(
            "Aspose::Pdf::Forms::Field::operator[](int): "
            "null kid at index " + std::to_string(index));
    }
    return *kid;
}

int Field::AnnotationIndex() const noexcept { return annotation_index_; }
void Field::AnnotationIndex(int v) noexcept { annotation_index_ = v; }

int Field::PageIndex() const noexcept { return page_index_; }

bool Field::IsSharedField() const noexcept { return is_shared_field_; }
void Field::IsSharedField(bool v) noexcept { is_shared_field_ = v; }

bool Field::FitIntoRectangle() const noexcept {
    return fit_into_rectangle_;
}
void Field::FitIntoRectangle(bool v) noexcept {
    fit_into_rectangle_ = v;
}

double Field::MaxFontSize() const noexcept { return max_font_size_; }
void Field::MaxFontSize(double v) noexcept { max_font_size_ = v; }

double Field::MinFontSize() const noexcept { return min_font_size_; }
void Field::MinFontSize(double v) noexcept { min_font_size_ = v; }

int Field::TabOrder() const noexcept { return tab_order_; }
void Field::TabOrder(int v) noexcept { tab_order_ = v; }

}  // namespace Aspose::Pdf::Forms
