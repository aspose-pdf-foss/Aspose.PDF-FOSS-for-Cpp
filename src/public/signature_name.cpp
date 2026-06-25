#include <aspose/pdf/facades/signature_name.hpp>

#include <functional>

namespace Aspose::Pdf::Facades {

std::string SignatureName::ToString() const {
    return FullName.empty() ? Name : FullName;
}

int SignatureName::GetHashCode() const {
    return static_cast<int>(std::hash<std::string>{}(FullName));
}

bool SignatureName::HasSignature() const noexcept {
    return has_signature_;
}

}  // namespace Aspose::Pdf::Facades
