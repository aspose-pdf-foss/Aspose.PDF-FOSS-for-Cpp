#include <aspose/pdf/forms/number_field.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

NumberField::NumberField(Aspose::Pdf::Page& page,
                          Aspose::Pdf::Rectangle rect)
    : TextBoxField(page, std::move(rect)) {}

NumberField::NumberField(Aspose::Pdf::Document& doc,
                          Aspose::Pdf::Rectangle rect)
    : TextBoxField(doc, std::move(rect)) {}

const std::string& NumberField::AllowedChars() const noexcept {
    return allowed_chars_;
}
void NumberField::AllowedChars(std::string v) {
    allowed_chars_ = std::move(v);
}

}  // namespace Aspose::Pdf::Forms
