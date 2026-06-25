#include <aspose/pdf/facades/alignment_type.hpp>

#include <utility>

namespace Aspose::Pdf::Facades {

AlignmentType::AlignmentType(std::string name)
    : name_(std::move(name)) {}

const AlignmentType& AlignmentType::Left() {
    static const AlignmentType v{"Left"};
    return v;
}
const AlignmentType& AlignmentType::Center() {
    static const AlignmentType v{"Center"};
    return v;
}
const AlignmentType& AlignmentType::Right() {
    static const AlignmentType v{"Right"};
    return v;
}

const std::string& AlignmentType::ToString() const noexcept {
    return name_;
}

bool AlignmentType::operator==(const AlignmentType& other) const noexcept {
    return name_ == other.name_;
}
bool AlignmentType::operator!=(const AlignmentType& other) const noexcept {
    return !(*this == other);
}

}  // namespace Aspose::Pdf::Facades
