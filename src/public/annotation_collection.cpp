#include <aspose/pdf/annotations/annotation_collection.hpp>

#include <algorithm>
#include <stdexcept>

#include <aspose/pdf/annotations/annotation.hpp>

namespace Aspose::Pdf::Annotations {

AnnotationCollection::AnnotationCollection() noexcept = default;
AnnotationCollection::~AnnotationCollection() = default;

void AnnotationCollection::Add(Annotation& annotation,
                                bool considerRotation) {
    items_.push_back(&annotation);
    consider_rotation_.push_back(considerRotation);
}

void AnnotationCollection::Add(Annotation& annotation) {
    Add(annotation, /*considerRotation=*/false);
}

void AnnotationCollection::Delete(int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= items_.size()) {
        throw std::out_of_range(
            "AnnotationCollection::Delete: index out of range");
    }
    items_.erase(items_.begin() + index);
    consider_rotation_.erase(consider_rotation_.begin() + index);
}

void AnnotationCollection::Delete() {
    Clear();
}

void AnnotationCollection::Delete(const Annotation& annotation) {
    auto it = std::find(items_.begin(), items_.end(), &annotation);
    if (it != items_.end()) {
        const auto idx = static_cast<std::size_t>(
            it - items_.begin());
        items_.erase(it);
        consider_rotation_.erase(
            consider_rotation_.begin() + idx);
    }
}

void AnnotationCollection::Clear() {
    items_.clear();
    consider_rotation_.clear();
}

bool AnnotationCollection::Remove(const Annotation& annotation) {
    auto it = std::find(items_.begin(), items_.end(), &annotation);
    if (it == items_.end()) return false;
    const auto idx = static_cast<std::size_t>(
        it - items_.begin());
    items_.erase(it);
    consider_rotation_.erase(consider_rotation_.begin() + idx);
    return true;
}

bool AnnotationCollection::Contains(
        const Annotation& annotation) const noexcept {
    return std::find(items_.begin(), items_.end(), &annotation)
        != items_.end();
}

int AnnotationCollection::Count() const noexcept {
    return static_cast<int>(items_.size());
}

bool AnnotationCollection::IsReadOnly() const noexcept {
    return false;
}

Annotation& AnnotationCollection::operator[](int index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= items_.size()) {
        throw std::out_of_range(
            "AnnotationCollection::operator[]: index out of range");
    }
    return *items_[index];
}

void AnnotationCollection::Accept(AnnotationSelector& visitor) {
    for (auto* a : items_) {
        if (a != nullptr) a->Accept(visitor);
    }
}

}  // namespace Aspose::Pdf::Annotations
