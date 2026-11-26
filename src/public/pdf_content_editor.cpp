#include <aspose/pdf/facades/pdf_content_editor.hpp>

#include "_pending.hpp"

#include <aspose/pdf/document.hpp>

namespace Aspose::Pdf::Facades {

PdfContentEditor::PdfContentEditor(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

// ReplaceText (text replacement) and DeleteImage (image removal) are real,
// wired to Document. The remaining editors — attachments, document actions,
// viewer preference, ReplaceImage, and stamp ops — throw std::logic_error
// (not implemented yet).

void PdfContentEditor::AddDocumentAttachment(const std::string&,
                                             const std::string&) {
    PENDING("content-editor-attachments");
}
void PdfContentEditor::DeleteAttachments() {
    PENDING("content-editor-attachments");
}

void PdfContentEditor::AddDocumentAdditionalAction(const std::string&,
                                                   const std::string&) {
    PENDING("content-editor-document-actions");
}
void PdfContentEditor::RemoveDocumentOpenAction() {
    PENDING("content-editor-document-actions");
}

void PdfContentEditor::ChangeViewerPreference(int) {
    PENDING("content-editor-viewer-preferences");
}
int PdfContentEditor::GetViewerPreference() {
    PENDING("content-editor-viewer-preferences");
}

void PdfContentEditor::ReplaceImage(int, int, const std::string&) {
    PENDING("content-editor-replace-image");
}
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

void PdfContentEditor::DeleteStamp(int, const std::vector<int>&) {
    PENDING("content-editor-stamps");
}
void PdfContentEditor::DeleteStampByIds(const std::vector<int>&) {
    PENDING("content-editor-stamps");
}
void PdfContentEditor::DeleteStampByIds(int, const std::vector<int>&) {
    PENDING("content-editor-stamps");
}
void PdfContentEditor::DeleteStampById(int, int) { PENDING("content-editor-stamps"); }
void PdfContentEditor::DeleteStampById(int) { PENDING("content-editor-stamps"); }
void PdfContentEditor::HideStampById(int, int) { PENDING("content-editor-stamps"); }
void PdfContentEditor::ShowStampById(int, int) { PENDING("content-editor-stamps"); }
void PdfContentEditor::MoveStampById(int, int, double, double) {
    PENDING("content-editor-stamps");
}
void PdfContentEditor::MoveStamp(int, int, double, double) {
    PENDING("content-editor-stamps");
}

}  // namespace Aspose::Pdf::Facades
