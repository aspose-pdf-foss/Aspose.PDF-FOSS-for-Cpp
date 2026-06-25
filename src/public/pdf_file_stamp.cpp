#include <aspose/pdf/facades/pdf_file_stamp.hpp>

#include <string>
#include <vector>
#include <utility>

#include <aspose/pdf/document.hpp>

namespace Aspose::Pdf::Facades {

PdfFileStamp::PdfFileStamp(const std::string& inputFile,
                           const std::string& outputFile)
    : input_file_(inputFile), output_file_(outputFile) {
    BindPdf(inputFile);
}

PdfFileStamp::PdfFileStamp(const std::string& inputFile,
                           const std::string& outputFile, bool keepSecurity)
    : keep_security_(keepSecurity),
      input_file_(inputFile),
      output_file_(outputFile) {
    BindPdf(inputFile);
}

PdfFileStamp::PdfFileStamp(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

PdfFileStamp::PdfFileStamp(Aspose::Pdf::Document& document,
                           const std::string& outputFile)
    : output_file_(outputFile) {
    BindPdf(document);
}

// ===== Stamping — real (content-stream text via Document::ApplyTextStamps) ===

namespace {

std::string FormatPageNumber(const std::string& fmt, int num) {
    std::string out;
    bool replaced = false;
    for (char c : fmt) {
        if (c == '#') {
            out += std::to_string(num);
            replaced = true;
        } else {
            out.push_back(c);
        }
    }
    if (!replaced) out += std::to_string(num);
    return out;
}

}  // namespace

void PdfFileStamp::DrawHeader(const std::string& text, float topMargin,
                              float leftMargin) {
    if (document_ == nullptr) return;
    std::vector<Aspose::Pdf::Document::TextStampReq> stamps;
    const int count = document_->PageCountInternal();
    for (int i = 0; i < count; ++i) {
        Aspose::Pdf::Document::TextStampReq r;
        r.leaf = static_cast<std::size_t>(i);
        r.text = text;
        r.x = leftMargin;
        r.y = document_->PageHeightInternal(static_cast<std::size_t>(i)) -
              topMargin;
        r.size = 12.0;
        stamps.push_back(std::move(r));
    }
    document_->ApplyTextStamps(stamps);
}

void PdfFileStamp::DrawFooter(const std::string& text, float bottomMargin,
                              float leftMargin) {
    if (document_ == nullptr) return;
    std::vector<Aspose::Pdf::Document::TextStampReq> stamps;
    const int count = document_->PageCountInternal();
    for (int i = 0; i < count; ++i) {
        Aspose::Pdf::Document::TextStampReq r;
        r.leaf = static_cast<std::size_t>(i);
        r.text = text;
        r.x = leftMargin;
        r.y = bottomMargin;
        r.size = 12.0;
        stamps.push_back(std::move(r));
    }
    document_->ApplyTextStamps(stamps);
}

void PdfFileStamp::DrawPageNumber(const std::string& format, int startingNumber,
                                  float x, float y) {
    if (document_ == nullptr) return;
    std::vector<Aspose::Pdf::Document::TextStampReq> stamps;
    const int count = document_->PageCountInternal();
    for (int i = 0; i < count; ++i) {
        Aspose::Pdf::Document::TextStampReq r;
        r.leaf = static_cast<std::size_t>(i);
        r.text = FormatPageNumber(format, startingNumber + i);
        r.x = (x >= 0.0f)
                  ? x
                  : document_->PageWidthInternal(
                        static_cast<std::size_t>(i)) / 2.0 - 20.0;
        r.y = (y >= 0.0f) ? y : 30.0;
        r.size = 12.0;
        stamps.push_back(std::move(r));
    }
    document_->ApplyTextStamps(stamps);
}

void PdfFileStamp::AddPageNumber(const std::string& format) {
    DrawPageNumber(format, starting_number_, -1.0f, -1.0f);
}
void PdfFileStamp::AddPageNumber(const std::string& format,
                                 int startingNumber) {
    DrawPageNumber(format, startingNumber, -1.0f, -1.0f);
}
void PdfFileStamp::AddPageNumber(const std::string& format, float left,
                                 float top) {
    DrawPageNumber(format, starting_number_, left, top);
}
void PdfFileStamp::AddPageNumber(const std::string& format,
                                 int startingNumber, float llx, float lly,
                                 float /*urx*/, float /*ury*/) {
    DrawPageNumber(format, startingNumber, llx, lly);
}
void PdfFileStamp::AddHeader(const std::string& text, float topMargin) {
    DrawHeader(text, topMargin, 72.0f);
}
void PdfFileStamp::AddHeader(const std::string& text, float topMargin,
                             float leftMargin, float /*rightMargin*/) {
    DrawHeader(text, topMargin, leftMargin);
}
void PdfFileStamp::AddFooter(const std::string& text, float bottomMargin) {
    DrawFooter(text, bottomMargin, 72.0f);
}
void PdfFileStamp::AddFooter(const std::string& text, float bottomMargin,
                             float leftMargin, float /*rightMargin*/) {
    DrawFooter(text, bottomMargin, leftMargin);
}

// ===== Properties ============================================================

bool PdfFileStamp::OptimizeSize() const noexcept { return optimize_size_; }
void PdfFileStamp::OptimizeSize(bool value) noexcept {
    optimize_size_ = value;
}
bool PdfFileStamp::KeepSecurity() const noexcept { return keep_security_; }
void PdfFileStamp::KeepSecurity(bool value) noexcept {
    keep_security_ = value;
}
const std::string& PdfFileStamp::InputFile() const noexcept {
    return input_file_;
}
void PdfFileStamp::InputFile(std::string value) {
    input_file_ = std::move(value);
}
const std::string& PdfFileStamp::OutputFile() const noexcept {
    return output_file_;
}
void PdfFileStamp::OutputFile(std::string value) {
    output_file_ = std::move(value);
}
float PdfFileStamp::PageNumberRotation() const noexcept {
    return page_number_rotation_;
}
void PdfFileStamp::PageNumberRotation(float value) noexcept {
    page_number_rotation_ = value;
}
void PdfFileStamp::ConvertTo(Aspose::Pdf::PdfFormat value) noexcept {
    convert_to_ = value;
}
float PdfFileStamp::PageHeight() const noexcept {
    return 792.0f;  // canonical US-Letter default (per-page geometry deferred)
}
float PdfFileStamp::PageWidth() const noexcept { return 612.0f; }
int PdfFileStamp::StartingNumber() const noexcept { return starting_number_; }
void PdfFileStamp::StartingNumber(int value) noexcept {
    starting_number_ = value;
}
int PdfFileStamp::StampId() const noexcept { return stamp_id_; }
void PdfFileStamp::StampId(int value) noexcept { stamp_id_ = value; }

}  // namespace Aspose::Pdf::Facades
