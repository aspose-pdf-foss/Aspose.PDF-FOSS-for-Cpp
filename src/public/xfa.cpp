#include <aspose/pdf/forms/xfa.hpp>

namespace Aspose::Pdf::Forms {

std::string XFA::operator[](const std::string& path) const {
    for (const auto& e : entries_) {
        if (e.first == path) return e.second;
    }
    return {};
}

std::vector<std::string> XFA::FieldNames() const {
    std::vector<std::string> out;
    out.reserve(entries_.size());
    for (const auto& e : entries_) out.push_back(e.first);
    return out;
}

}  // namespace Aspose::Pdf::Forms
