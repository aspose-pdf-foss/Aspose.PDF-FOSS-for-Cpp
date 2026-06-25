#include <aspose/pdf/forms/list_box_field.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

ListBoxField::ListBoxField(Aspose::Pdf::Page& /*page*/,
                            Aspose::Pdf::Rectangle rect) {
    Rect(std::move(rect));
}

ListBoxField::ListBoxField(Aspose::Pdf::Document& doc,
                            Aspose::Pdf::Rectangle rect)
    : ChoiceField(doc) {
    Rect(std::move(rect));
}

int ListBoxField::TopIndex() const noexcept { return top_index_; }
void ListBoxField::TopIndex(int v) noexcept { top_index_ = v; }

}  // namespace Aspose::Pdf::Forms
