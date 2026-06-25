#include <aspose/pdf/facades/pdf_extractor.hpp>

#include <fstream>
#include <utility>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/embedded_file_collection.hpp>
#include <aspose/pdf/file_specification.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/text_absorber.hpp>

namespace Aspose::Pdf::Facades {

namespace {

void WriteTextFile(const std::string& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
}

}  // namespace

PdfExtractor::PdfExtractor(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

// ===== Text extraction =======================================================

void PdfExtractor::EnsureExtracted() {
    if (extracted_) {
        return;
    }
    full_text_.clear();
    page_texts_.clear();
    page_cursor_ = 0;

    if (document_ == nullptr) {
        extracted_ = true;
        return;
    }

    Aspose::Pdf::PageCollection& pages = document_->Pages();
    const int count = static_cast<int>(pages.Count());
    const int first = start_page_ < 1 ? 1 : start_page_;
    const int last =
        (end_page_ <= 0 || end_page_ > count) ? count : end_page_;

    for (int i = first; i <= last; ++i) {
        Aspose::Pdf::Text::TextAbsorber absorber;
        Aspose::Pdf::Page page = pages[i];
        absorber.Visit(page);
        std::string text = absorber.Text();
        full_text_ += text;
        page_texts_.push_back(std::move(text));
    }
    extracted_ = true;
}

void PdfExtractor::ExtractText() {
    EnsureExtracted();
}

void PdfExtractor::GetText(const std::string& outputFile) {
    EnsureExtracted();
    WriteTextFile(outputFile, full_text_);
}

bool PdfExtractor::HasNextPageText() {
    EnsureExtracted();
    return page_cursor_ < page_texts_.size();
}

void PdfExtractor::GetNextPageText(const std::string& outputFile) {
    EnsureExtracted();
    std::string text;
    if (page_cursor_ < page_texts_.size()) {
        text = page_texts_[page_cursor_++];
    }
    WriteTextFile(outputFile, text);
}

// ===== Image extraction (real — image XObjects via Document) =================
// Walks the document's image XObjects; GetNextImage writes the next one
// to a file (DCTDecode/JPEG streams verbatim) and advances the cursor.

void PdfExtractor::ExtractImage() {
    image_cursor_ = 0;  // (re)start the image walk
}

bool PdfExtractor::HasNextImage() {
    if (document_ == nullptr) return false;
    return image_cursor_ < document_->CountImages(0);
}

bool PdfExtractor::GetNextImage(const std::string& outputFile) {
    if (document_ == nullptr) return false;
    if (image_cursor_ >= document_->CountImages(0)) return false;
    const bool ok =
        document_->ExtractImageToFile(image_cursor_, outputFile);
    if (ok) ++image_cursor_;
    return ok;
}

// ===== Attachment extraction — v1 stubs ======================================
// /EmbeddedFile stream extraction lands with embedded-file plumbing
// (the same beat that wires FileSpecification stream contents through
// PdfWriter). The surface returns the "no attachments" contract.

std::vector<std::string> PdfExtractor::GetAttachNames() {
    if (document_ == nullptr) return {};
    return document_->AttachmentNamesInternal();
}

void PdfExtractor::ExtractAttachment() {}

void PdfExtractor::ExtractAttachment(const std::string& attachmentFileName) {
    // Write the named attachment's bytes to a file of that name.
    if (document_ == nullptr) return;
    document_->ExtractAttachmentInternal(attachmentFileName,
                                         attachmentFileName);
}

void PdfExtractor::GetAttachment(const std::string& outputPath) {
    // Write the first attachment's bytes to `outputPath`.
    if (document_ == nullptr) return;
    document_->ExtractAttachmentInternal(std::string(), outputPath);
}

std::vector<Aspose::Pdf::FileSpecification>
PdfExtractor::GetAttachmentInfo() {
    if (document_ == nullptr) return {};
    auto& coll = document_->EmbeddedFiles();
    std::vector<Aspose::Pdf::FileSpecification> out;
    for (int i = 0; i < coll.Count(); ++i) out.push_back(coll[i]);
    return out;
}

// ===== Properties ============================================================

int PdfExtractor::StartPage() const noexcept {
    return start_page_;
}
void PdfExtractor::StartPage(int value) noexcept {
    start_page_ = value;
    extracted_ = false;  // range changed — re-extract on next read
}

int PdfExtractor::EndPage() const noexcept {
    return end_page_;
}
void PdfExtractor::EndPage(int value) noexcept {
    end_page_ = value;
    extracted_ = false;
}

int PdfExtractor::ExtractTextMode() const noexcept {
    return extract_text_mode_;
}
void PdfExtractor::ExtractTextMode(int value) noexcept {
    extract_text_mode_ = value;
}

int PdfExtractor::Resolution() const noexcept {
    return resolution_;
}
void PdfExtractor::Resolution(int value) noexcept {
    resolution_ = value;
}

const std::string& PdfExtractor::Password() const noexcept {
    return password_;
}
void PdfExtractor::Password(std::string value) {
    password_ = std::move(value);
}

bool PdfExtractor::IsBidi() const noexcept {
    return is_bidi_;
}

}  // namespace Aspose::Pdf::Facades
