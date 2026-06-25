#include <aspose/pdf/facades/facade.hpp>

#include <memory>

#include <aspose/pdf/document.hpp>

namespace Aspose::Pdf::Facades {

void Facade::BindPdf(const std::string& srcFile) {
    Close();
    owned_ = std::make_unique<Aspose::Pdf::Document>(srcFile);
    document_ = owned_.get();
}

void Facade::BindPdf(Aspose::Pdf::Document& srcDoc) {
    Close();
    document_ = &srcDoc;
}

void Facade::Close() {
    owned_.reset();
    document_ = nullptr;
}

void Facade::Dispose() { Close(); }

Aspose::Pdf::Document* Facade::Document() noexcept { return document_; }
const Aspose::Pdf::Document* Facade::Document() const noexcept {
    return document_;
}

}  // namespace Aspose::Pdf::Facades
