#include <aspose/pdf/facades/vertical_alignment_type.hpp>

#include <utility>

namespace Aspose::Pdf::Facades {

VerticalAlignmentType::VerticalAlignmentType(std::string name)
    : name_(std::move(name)) {}

const VerticalAlignmentType& VerticalAlignmentType::Top() {
    static const VerticalAlignmentType v{"Top"};
    return v;
}
const VerticalAlignmentType& VerticalAlignmentType::Center() {
    static const VerticalAlignmentType v{"Center"};
    return v;
}
const VerticalAlignmentType& VerticalAlignmentType::Bottom() {
    static const VerticalAlignmentType v{"Bottom"};
    return v;
}

const std::string& VerticalAlignmentType::ToString() const noexcept {
    return name_;
}

bool VerticalAlignmentType::operator==(
        const VerticalAlignmentType& other) const noexcept {
    return name_ == other.name_;
}
bool VerticalAlignmentType::operator!=(
        const VerticalAlignmentType& other) const noexcept {
    return !(*this == other);
}

}  // namespace Aspose::Pdf::Facades
