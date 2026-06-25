#include <aspose/pdf/forms/date_field.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

DateField::DateField(Aspose::Pdf::Document& doc)
    : TextBoxField(doc) {}

DateField::DateField(Aspose::Pdf::Page& page,
                      Aspose::Pdf::Rectangle rect)
    : TextBoxField(page, std::move(rect)) {}

DateField::DateField(Aspose::Pdf::Document& doc,
                      Aspose::Pdf::Rectangle rect)
    : TextBoxField(doc, std::move(rect)) {}

void DateField::Init(Aspose::Pdf::Page& page) {
    init_page_ = &page;
}

const std::string& DateField::DateFormat() const noexcept {
    return date_format_;
}
void DateField::DateFormat(std::string v) {
    date_format_ = std::move(v);
}

}  // namespace Aspose::Pdf::Forms
