#include <aspose/pdf/facades/pdf_content_editor.hpp>

#include <aspose/pdf/document.hpp>

namespace Aspose::Pdf::Facades {

PdfContentEditor::PdfContentEditor(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

// ReplaceText (text replacement) and DeleteImage (image removal) are real,
// wired to Document. The remaining editors — attachments, document actions,
// viewer preference, ReplaceImage, and stamp ops — are still surface-only
// stubs (no-op / 0) pending their foundation paths.

void PdfContentEditor::AddDocumentAttachment(const std::string&,
                                             const std::string&) {}
void PdfContentEditor::DeleteAttachments() {}

void PdfContentEditor::AddDocumentAdditionalAction(const std::string&,
                                                   const std::string&) {}
void PdfContentEditor::RemoveDocumentOpenAction() {}

void PdfContentEditor::ChangeViewerPreference(int) {}
int PdfContentEditor::GetViewerPreference() { return 0; }

void PdfContentEditor::ReplaceImage(int, int, const std::string&) {}
void PdfContentEditor::DeleteImage(int pageNum,
                                   const std::vector<int>& imageNum) {
    if (document_ == nullptr) return;
    document_->DeleteImagesFromPage(pageNum, imageNum);
}
void PdfContentEditor::DeleteImage() {
    if (document_ == nullptr) return;
    document_->DeleteImagesFromPage(0, {});  // every image on every page
}

bool PdfContentEditor::ReplaceText(const std::string& srcText,
                                   const std::string& destText) {
    if (document_ == nullptr) return false;
    return document_->ReplaceTextInContent(srcText, destText, 0) > 0;
}
bool PdfContentEditor::ReplaceText(const std::string& srcText, int pageNum,
                                   const std::string& destText) {
    if (document_ == nullptr) return false;
    return document_->ReplaceTextInContent(srcText, destText, pageNum) > 0;
}
bool PdfContentEditor::ReplaceText(const std::string& srcText,
                                   const std::string& destText, int pageNum) {
    if (document_ == nullptr) return false;
    return document_->ReplaceTextInContent(srcText, destText, pageNum) > 0;
}

void PdfContentEditor::DeleteStamp(int, const std::vector<int>&) {}
void PdfContentEditor::DeleteStampByIds(const std::vector<int>&) {}
void PdfContentEditor::DeleteStampByIds(int, const std::vector<int>&) {}
void PdfContentEditor::DeleteStampById(int, int) {}
void PdfContentEditor::DeleteStampById(int) {}
void PdfContentEditor::HideStampById(int, int) {}
void PdfContentEditor::ShowStampById(int, int) {}
void PdfContentEditor::MoveStampById(int, int, double, double) {}
void PdfContentEditor::MoveStamp(int, int, double, double) {}

}  // namespace Aspose::Pdf::Facades
