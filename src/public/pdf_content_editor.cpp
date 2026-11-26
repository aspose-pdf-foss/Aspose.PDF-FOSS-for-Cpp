#include <aspose/pdf/facades/pdf_content_editor.hpp>

#include <fstream>
#include <iterator>
#include <stdexcept>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/embedded_file_collection.hpp>
#include <aspose/pdf/file_specification.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resources.hpp>
#include <aspose/pdf/x_image_collection.hpp>

namespace Aspose::Pdf::Facades {

PdfContentEditor::PdfContentEditor(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

namespace {

using StampEdit = Aspose::Pdf::Document::StampEdit;

StampEdit MakeEdit(StampEdit::Kind kind, bool by_id, int key, double x = 0, double y = 0) {
    StampEdit e;
    e.kind = kind;
    e.by_id = by_id;
    e.key = key;
    e.x = x;
    e.y = y;
    return e;
}

}  // namespace

void PdfContentEditor::AddDocumentAttachment(const std::string& path,
                                             const std::string& description) {
    if (document_ == nullptr) return;
    document_->AddAttachmentInternal(path, description);
}
void PdfContentEditor::DeleteAttachments() {
    if (document_ == nullptr) return;
    document_->EmbeddedFiles().Delete();
}

void PdfContentEditor::AddDocumentAdditionalAction(const std::string& actionName,
                                                   const std::string& javaScript) {
    if (document_ == nullptr) return;
    if (actionName != DocumentOpen && actionName != DocumentClose &&
        actionName != DocumentWillSave && actionName != DocumentSaved &&
        actionName != DocumentWillPrint && actionName != DocumentPrinted)
        throw std::invalid_argument("PdfContentEditor: unknown document action " + actionName);
    Aspose::Pdf::Document::CatalogOp op;
    op.kind = Aspose::Pdf::Document::CatalogOp::Kind::JavaScriptAction;
    op.trigger = actionName;
    op.javascript = javaScript;
    document_->StageCatalogOpInternal(std::move(op));
}
void PdfContentEditor::RemoveDocumentOpenAction() {
    if (document_ == nullptr) return;
    Aspose::Pdf::Document::CatalogOp op;
    op.kind = Aspose::Pdf::Document::CatalogOp::Kind::RemoveOpenAction;
    document_->StageCatalogOpInternal(std::move(op));
}

void PdfContentEditor::ChangeViewerPreference(int viewerAttribution) {
    if (document_ == nullptr) return;
    Aspose::Pdf::Document::CatalogOp op;
    op.kind = Aspose::Pdf::Document::CatalogOp::Kind::ViewerFlag;
    op.flag = viewerAttribution;
    document_->StageCatalogOpInternal(std::move(op));
}
int PdfContentEditor::GetViewerPreference() {
    return document_ == nullptr ? 0 : document_->ViewerPreferenceInternal();
}

void PdfContentEditor::ReplaceImage(int pageNum, int imageNum, const std::string& fileName) {
    if (document_ == nullptr) return;
    std::ifstream in(fileName, std::ios::binary);
    if (!in) throw std::runtime_error("PdfContentEditor: cannot read image " + fileName);
    std::vector<char> raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<std::byte> bytes(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) bytes[i] = static_cast<std::byte>(raw[i]);
    if (pageNum < 1 || pageNum > document_->Pages().Count())
        throw std::out_of_range("PdfContentEditor::ReplaceImage: page");
    document_->Pages()[pageNum].Resources().Images().Replace(imageNum, bytes);
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

void PdfContentEditor::DeleteStamp(int pageNum, const std::vector<int>& stampIndex) {
    if (document_ == nullptr || pageNum < 1) return;
    std::vector<StampEdit> edits;
    for (int i : stampIndex) edits.push_back(MakeEdit(StampEdit::Kind::Delete, false, i));
    document_->EditStampsInternal(pageNum, edits);
}
void PdfContentEditor::DeleteStampByIds(const std::vector<int>& stampIds) {
    DeleteStampByIds(0, stampIds);
}
void PdfContentEditor::DeleteStampByIds(int pageNum, const std::vector<int>& stampIds) {
    if (document_ == nullptr) return;
    std::vector<StampEdit> edits;
    for (int id : stampIds) edits.push_back(MakeEdit(StampEdit::Kind::Delete, true, id));
    document_->EditStampsInternal(pageNum, edits);
}
void PdfContentEditor::DeleteStampById(int pageNum, int stampId) {
    DeleteStampByIds(pageNum, {stampId});
}
void PdfContentEditor::DeleteStampById(int stampId) { DeleteStampByIds(0, {stampId}); }
void PdfContentEditor::HideStampById(int pageNum, int stampId) {
    if (document_ == nullptr || pageNum < 1) return;
    document_->EditStampsInternal(pageNum, {MakeEdit(StampEdit::Kind::Hide, true, stampId)});
}
void PdfContentEditor::ShowStampById(int pageNum, int stampId) {
    if (document_ == nullptr || pageNum < 1) return;
    document_->EditStampsInternal(pageNum, {MakeEdit(StampEdit::Kind::Show, true, stampId)});
}
void PdfContentEditor::MoveStampById(int pageNum, int stampId, double x, double y) {
    if (document_ == nullptr || pageNum < 1) return;
    document_->EditStampsInternal(pageNum,
                                  {MakeEdit(StampEdit::Kind::Move, true, stampId, x, y)});
}
void PdfContentEditor::MoveStamp(int pageNum, int stampIndex, double x, double y) {
    if (document_ == nullptr) return;
    if (pageNum < 1 ||
        document_->EditStampsInternal(
            pageNum, {MakeEdit(StampEdit::Kind::Move, false, stampIndex, x, y)}) == 0)
        throw std::out_of_range("PdfContentEditor::MoveStamp: no stamp at that index");
}

}  // namespace Aspose::Pdf::Facades
