#include <aspose/pdf/facades/pdf_converter.hpp>

#include <fstream>
#include <utility>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/tiff_device.hpp>

namespace Aspose::Pdf::Facades {

PdfConverter::PdfConverter(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

int PdfConverter::FirstPage() const {
    return start_page_ < 1 ? 1 : start_page_;
}

int PdfConverter::LastPage() const {
    if (document_ == nullptr) {
        return 0;
    }
    const int count = static_cast<int>(document_->Pages().Count());
    return (end_page_ <= 0 || end_page_ > count) ? count : end_page_;
}

void PdfConverter::DoConvert() {
    next_image_ = document_ == nullptr ? 0 : FirstPage();
}

// ===== TIFF (real) ===========================================================

void PdfConverter::RenderTiff(
    const std::string& outputFile,
    const Aspose::Pdf::Devices::Resolution& resolution,
    const Aspose::Pdf::Devices::TiffSettings* settings) {
    if (document_ == nullptr) {
        return;
    }
    std::ofstream out(outputFile, std::ios::binary | std::ios::trunc);
    if (settings != nullptr) {
        Aspose::Pdf::Devices::TiffDevice dev(resolution, *settings);
        dev.Process(*document_, FirstPage(), LastPage(), out);
    } else {
        Aspose::Pdf::Devices::TiffDevice dev(resolution);
        dev.Process(*document_, FirstPage(), LastPage(), out);
    }
}

void PdfConverter::SaveAsTIFF(const std::string& outputFile) {
    RenderTiff(outputFile, resolution_, nullptr);
}
void PdfConverter::SaveAsTIFF(
    const std::string& outputFile,
    Aspose::Pdf::Devices::CompressionType compression) {
    const Aspose::Pdf::Devices::TiffSettings settings(compression);
    RenderTiff(outputFile, resolution_, &settings);
}
void PdfConverter::SaveAsTIFF(const std::string& outputFile, int xResolution,
                              int yResolution) {
    RenderTiff(outputFile,
               Aspose::Pdf::Devices::Resolution(xResolution, yResolution),
               nullptr);
}
void PdfConverter::SaveAsTIFF(const std::string& outputFile,
                              Aspose::Pdf::PageSize) {
    // PageSize fitting deferred — v1 renders at the configured resolution.
    RenderTiff(outputFile, resolution_, nullptr);
}
void PdfConverter::SaveAsTIFF(
    const std::string& outputFile, Aspose::Pdf::PageSize,
    const Aspose::Pdf::Devices::TiffSettings& settings) {
    RenderTiff(outputFile, resolution_, &settings);
}
void PdfConverter::SaveAsTIFF(
    const std::string& outputFile, int xResolution, int yResolution,
    Aspose::Pdf::Devices::CompressionType compression) {
    const Aspose::Pdf::Devices::TiffSettings settings(compression);
    RenderTiff(outputFile,
               Aspose::Pdf::Devices::Resolution(xResolution, yResolution),
               &settings);
}
void PdfConverter::SaveAsTIFF(
    const std::string& outputFile, int xResolution, int yResolution,
    const Aspose::Pdf::Devices::TiffSettings& settings) {
    RenderTiff(outputFile,
               Aspose::Pdf::Devices::Resolution(xResolution, yResolution),
               &settings);
}
void PdfConverter::SaveAsTIFF(
    const std::string& outputFile, int xResolution, int yResolution,
    const Aspose::Pdf::Devices::TiffSettings& settings,
    Aspose::Pdf::IIndexBitmapConverter&) {
    // Indexed-bitmap conversion deferred — render normally.
    RenderTiff(outputFile,
               Aspose::Pdf::Devices::Resolution(xResolution, yResolution),
               &settings);
}
void PdfConverter::SaveAsTIFF(
    const std::string& outputFile,
    const Aspose::Pdf::Devices::TiffSettings& settings) {
    RenderTiff(outputFile, resolution_, &settings);
}
void PdfConverter::SaveAsTIFF(
    const std::string& outputFile,
    const Aspose::Pdf::Devices::TiffSettings& settings,
    Aspose::Pdf::IIndexBitmapConverter&) {
    RenderTiff(outputFile, resolution_, &settings);
}

void PdfConverter::SaveAsTIFFClassF(const std::string& outputFile) {
    // ClassF (CCITT-G4 1-bit fax) refinement deferred — render to TIFF.
    RenderTiff(outputFile, resolution_, nullptr);
}
void PdfConverter::SaveAsTIFFClassF(const std::string& outputFile,
                                    int xResolution, int yResolution) {
    RenderTiff(outputFile,
               Aspose::Pdf::Devices::Resolution(xResolution, yResolution),
               nullptr);
}
void PdfConverter::SaveAsTIFFClassF(const std::string& outputFile,
                                    Aspose::Pdf::PageSize) {
    RenderTiff(outputFile, resolution_, nullptr);
}

// ===== Page-by-page image extraction (real → PNG) ============================

bool PdfConverter::HasNextImage() {
    if (document_ == nullptr) {
        return false;
    }
    if (next_image_ == 0) {
        next_image_ = FirstPage();
    }
    return next_image_ <= LastPage();
}

void PdfConverter::GetNextImage(const std::string& outputFile) {
    if (!HasNextImage()) {
        return;
    }
    Aspose::Pdf::Page page = document_->Pages()[next_image_];
    std::ofstream out(outputFile, std::ios::binary | std::ios::trunc);
    Aspose::Pdf::Devices::PngDevice dev(resolution_);
    dev.Process(page, out);
    ++next_image_;
}

void PdfConverter::GetNextImage(const std::string& outputFile,
                                Aspose::Pdf::PageSize) {
    // PageSize fitting deferred — render at the configured resolution.
    GetNextImage(outputFile);
}

// ===== Properties ============================================================

Aspose::Pdf::PageCoordinateType PdfConverter::CoordinateType()
    const noexcept {
    return coordinate_type_;
}
void PdfConverter::CoordinateType(
    Aspose::Pdf::PageCoordinateType value) noexcept {
    coordinate_type_ = value;
}
bool PdfConverter::ShowHiddenAreas() const noexcept {
    return show_hidden_areas_;
}
void PdfConverter::ShowHiddenAreas(bool value) noexcept {
    show_hidden_areas_ = value;
}
const Aspose::Pdf::RenderingOptions& PdfConverter::RenderingOptions()
    const noexcept {
    return rendering_options_;
}
void PdfConverter::RenderingOptions(Aspose::Pdf::RenderingOptions value) {
    rendering_options_ = std::move(value);
}
Aspose::Pdf::Devices::FormPresentationMode PdfConverter::FormPresentationMode()
    const noexcept {
    return form_presentation_mode_;
}
void PdfConverter::FormPresentationMode(
    Aspose::Pdf::Devices::FormPresentationMode value) noexcept {
    form_presentation_mode_ = value;
}
const Aspose::Pdf::Devices::Resolution& PdfConverter::Resolution()
    const noexcept {
    return resolution_;
}
void PdfConverter::Resolution(Aspose::Pdf::Devices::Resolution value) {
    resolution_ = value;
}
int PdfConverter::StartPage() const noexcept { return start_page_; }
void PdfConverter::StartPage(int value) noexcept { start_page_ = value; }
int PdfConverter::EndPage() const noexcept { return end_page_; }
void PdfConverter::EndPage(int value) noexcept { end_page_ = value; }
const std::string& PdfConverter::Password() const noexcept {
    return password_;
}
void PdfConverter::Password(std::string value) {
    password_ = std::move(value);
}
const std::string& PdfConverter::UserPassword() const noexcept {
    return user_password_;
}
void PdfConverter::UserPassword(std::string value) {
    user_password_ = std::move(value);
}
int PdfConverter::PageCount() const {
    return document_ == nullptr
               ? 0
               : static_cast<int>(document_->Pages().Count());
}

}  // namespace Aspose::Pdf::Facades
