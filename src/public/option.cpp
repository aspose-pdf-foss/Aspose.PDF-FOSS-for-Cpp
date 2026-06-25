#include <aspose/pdf/forms/option.hpp>

#include <utility>

namespace Aspose::Pdf::Forms {

const std::string& Option::Value() const noexcept { return value_; }
void Option::Value(std::string v) { value_ = std::move(v); }

const std::string& Option::Name() const noexcept { return name_; }
void Option::Name(std::string v) { name_ = std::move(v); }

bool Option::Selected() const noexcept { return selected_; }
void Option::Selected(bool v) noexcept { selected_ = v; }

int Option::Index() const noexcept { return index_; }
void Option::IndexInternal(int v) noexcept { index_ = v; }

}  // namespace Aspose::Pdf::Forms
