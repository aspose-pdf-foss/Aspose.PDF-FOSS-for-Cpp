// Bodies for the page-label surface: PageLabel (a range descriptor) and
// PageLabelCollection (the document's /PageLabels number tree). Mirrors
// canonical Aspose.Pdf.PageLabel + Aspose.Pdf.PageLabelCollection.

#include <aspose/pdf/page_label_collection.hpp>

#include <aspose/pdf/page_label.hpp>

#include <string>
#include <utility>
#include <vector>

namespace Aspose::Pdf {

// ---- PageLabel -------------------------------------------------------------

PageLabel::PageLabel() = default;

int PageLabel::StartingValue() const noexcept { return starting_value_; }
void PageLabel::StartingValue(int value) { starting_value_ = value; }

Aspose::Pdf::NumberingStyle PageLabel::NumberingStyle() const noexcept {
    return numbering_style_;
}
void PageLabel::NumberingStyle(Aspose::Pdf::NumberingStyle value) {
    numbering_style_ = value;
}

const std::string& PageLabel::Prefix() const noexcept { return prefix_; }
void PageLabel::Prefix(std::string value) { prefix_ = std::move(value); }

// ---- PageLabelCollection ---------------------------------------------------

PageLabelCollection::PageLabelCollection() noexcept = default;

PageLabel PageLabelCollection::GetLabel(int pageIndex) const {
    // The active range is the one with the largest start <= pageIndex.
    auto it = ranges_.upper_bound(pageIndex);
    if (it == ranges_.begin()) return PageLabel{};
    --it;
    return it->second;
}

void PageLabelCollection::UpdateLabel(int pageIndex,
                                      const PageLabel& pageLabel) {
    ranges_[pageIndex] = pageLabel;
}

bool PageLabelCollection::RemoveLabel(int pageIndex) {
    return ranges_.erase(pageIndex) > 0;
}

std::vector<int> PageLabelCollection::GetPages() const {
    std::vector<int> pages;
    pages.reserve(ranges_.size());
    for (const auto& [start, label] : ranges_) pages.push_back(start);
    return pages;
}

}  // namespace Aspose::Pdf
