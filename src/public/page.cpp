#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/artifact_collection.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/paragraphs.hpp>
#include <aspose/pdf/resources.hpp>

#include "pages_tree.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace Aspose::Pdf {

Page::Page(Document& doc, std::size_t leaf_index) noexcept
    : doc_(&doc), leaf_index_(leaf_index) {}

std::int64_t Page::Number() const noexcept {
    return static_cast<std::int64_t>(
        doc_->tree_->leaves[leaf_index_].pagenum);
}

Rectangle Page::Rect() const {
    return doc_->GetPageRectInternal(leaf_index_);
}
void Page::Rect(const Rectangle& value) {
    doc_->SetPageRectInternal(leaf_index_, value);
}
Rectangle Page::MediaBox() const {
    return doc_->GetPageRectInternal(leaf_index_);
}
void Page::MediaBox(const Rectangle& value) {
    doc_->SetPageRectInternal(leaf_index_, value);
}
Rectangle Page::CropBox() const {
    return doc_->GetPageCropBoxInternal(leaf_index_);
}
void Page::CropBox(const Rectangle& value) {
    doc_->SetPageCropBoxInternal(leaf_index_, value);
}
Rectangle Page::TrimBox() const {
    return doc_->GetPageTrimBoxInternal(leaf_index_);
}
void Page::TrimBox(const Rectangle& value) {
    doc_->SetPageTrimBoxInternal(leaf_index_, value);
}
Rectangle Page::BleedBox() const {
    return doc_->GetPageBleedBoxInternal(leaf_index_);
}
void Page::BleedBox(const Rectangle& value) {
    doc_->SetPageBleedBoxInternal(leaf_index_, value);
}
Rectangle Page::ArtBox() const {
    return doc_->GetPageArtBoxInternal(leaf_index_);
}
void Page::ArtBox(const Rectangle& value) {
    doc_->SetPageArtBoxInternal(leaf_index_, value);
}
Rotation Page::Rotate() const {
    const int deg = doc_->GetPageRotationInternal(leaf_index_);
    return static_cast<Rotation>((deg / 90) % 4);
}
void Page::Rotate(Rotation value) {
    doc_->SetPageRotationInternal(leaf_index_,
                                  static_cast<int>(value) * 90);
}
Rectangle Page::GetPageRect(bool isMediaBox) const {
    return isMediaBox ? doc_->GetPageRectInternal(leaf_index_)
                      : doc_->GetPageCropBoxInternal(leaf_index_);
}
void Page::SetPageSize(double width, double height) {
    doc_->SetPageRectInternal(leaf_index_,
                              Rectangle(0.0, 0.0, width, height, false));
}

bool Page::AddImage(const std::string& imageFile, const Rectangle& rect) {
    return doc_->AddImageToPage(leaf_index_, imageFile, rect.LLX(),
                                rect.LLY(), rect.URX(), rect.URY());
}

void Page::AddImage(const std::vector<std::byte>& imageStream,
                    const Rectangle& imageRect, const Rectangle& bbox,
                    bool autoAdjustRectangle) {
    const bool ok = doc_->AddImageBytesToPage(
        leaf_index_, imageStream, imageRect.LLX(), imageRect.LLY(),
        imageRect.URX(), imageRect.URY(), autoAdjustRectangle,
        bbox.IsEmpty() ? nullptr : &bbox);
    if (!ok)
        throw std::runtime_error(
            "Aspose::Pdf::Page::AddImage: unsupported or unreadable image");
}

Annotations::AnnotationCollection& Page::Annotations() const {
    auto& slots = doc_->page_annotations_;
    if (slots.size() <= leaf_index_) {
        slots.resize(leaf_index_ + 1);
    }
    auto& slot = slots[leaf_index_];
    if (!slot) {
        slot = std::make_unique<Annotations::AnnotationCollection>();
        doc_->LoadPageAnnotations(leaf_index_, *slot,
                                  const_cast<Page&>(*this));
    }
    return *slot;
}

Aspose::Pdf::Paragraphs& Page::Paragraphs() const {
    auto& slots = doc_->page_paragraphs_;
    if (slots.size() <= leaf_index_) slots.resize(leaf_index_ + 1);
    auto& slot = slots[leaf_index_];
    if (!slot) slot = std::make_unique<Aspose::Pdf::Paragraphs>();
    return *slot;
}

Aspose::Pdf::Resources& Page::Resources() const {
    auto& slots = doc_->page_resources_;
    if (slots.size() <= leaf_index_) slots.resize(leaf_index_ + 1);
    auto& slot = slots[leaf_index_];
    if (!slot) {
        slot = std::make_unique<Aspose::Pdf::Resources>();
        doc_->LoadPageImages(leaf_index_, slot->Images());
    }
    return *slot;
}

Aspose::Pdf::ArtifactCollection& Page::Artifacts() const {
    auto& slots = doc_->page_artifacts_;
    if (slots.size() <= leaf_index_) slots.resize(leaf_index_ + 1);
    auto& slot = slots[leaf_index_];
    if (!slot) slot = std::make_unique<Aspose::Pdf::ArtifactCollection>();
    return *slot;
}

}  // namespace Aspose::Pdf
