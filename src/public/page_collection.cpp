#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include "pages_tree.hpp"

#include <stdexcept>
#include <string>

namespace Aspose::Pdf {

PageCollection::PageCollection(Document& doc) noexcept : doc_(&doc) {}

std::size_t PageCollection::Count() const noexcept {
    return doc_->tree_->leaves.size();
}

Page PageCollection::operator[](int index) const {
    const auto count = Count();
    if (index < 1 || static_cast<std::size_t>(index) > count) {
        throw std::out_of_range(
            "Aspose::Pdf::PageCollection::operator[]: 1-based index "
            + std::to_string(index) + " out of range [1.." +
            std::to_string(count) + "]");
    }
    return Page(*doc_, static_cast<std::size_t>(index - 1));
}

Page PageCollection::Add() {
    const std::size_t idx = doc_->AddPageInternal(0, false, 0, 612.0, 792.0);
    return Page(*doc_, idx);
}

Page PageCollection::ImportFrom(const Page& page, int pageNumber) {
    const std::size_t before = Count();
    const std::size_t idx = doc_->ImportPagesFrom(
        *page.doc_, {static_cast<int>(page.leaf_index_) + 1}, pageNumber);
    if (Count() != before + 1) {
        throw std::runtime_error(
            "Aspose::Pdf::PageCollection: cross-document page import "
            "failed");
    }
    return Page(*doc_, idx);
}

Page PageCollection::Add(const Page& page) {
    if (page.doc_ != doc_) return ImportFrom(page, 0);
    const std::size_t idx =
        doc_->AddPageInternal(0, true, page.leaf_index_, 0.0, 0.0);
    return Page(*doc_, idx);
}

Page PageCollection::Insert(int pageNumber) {
    const std::size_t idx =
        doc_->AddPageInternal(pageNumber, false, 0, 612.0, 792.0);
    return Page(*doc_, idx);
}

Page PageCollection::Insert(int pageNumber, const Page& page) {
    if (page.doc_ != doc_) return ImportFrom(page, pageNumber);
    const std::size_t idx =
        doc_->AddPageInternal(pageNumber, true, page.leaf_index_, 0.0, 0.0);
    return Page(*doc_, idx);
}

void PageCollection::Delete() { doc_->DeleteAllPagesInternal(); }

void PageCollection::Delete(int pageNumber) {
    doc_->DeletePagesInternal({pageNumber});
}

void PageCollection::Delete(const std::vector<int>& pageNumbers) {
    doc_->DeletePagesInternal(pageNumbers);
}

}  // namespace Aspose::Pdf
