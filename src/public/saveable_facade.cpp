#include <aspose/pdf/facades/saveable_facade.hpp>

#include <stdexcept>

#include <aspose/pdf/document.hpp>

namespace Aspose::Pdf::Facades {

void SaveableFacade::Save(const std::string& destFile) {
    if (document_ == nullptr) {
        throw std::runtime_error(
            "Aspose::Pdf::Facades::SaveableFacade::Save: no document "
            "bound — call BindPdf first.");
    }
    document_->Save(destFile);
}

}  // namespace Aspose::Pdf::Facades
