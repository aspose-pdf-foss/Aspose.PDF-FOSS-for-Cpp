#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PdfConverter — rasterises a PDF to images / TIFF.
// Mirrors canonical Aspose.Pdf.Facades.PdfConverter; extends Facade.
//
// v1 wires the file-output conversion through the REAL rendering
// pipeline: SaveAsTIFF(file …) renders the StartPage..EndPage range to a
// multi-page TIFF via Devices::TiffDevice; HasNextImage / GetNextImage
// (file …) walk the pages one at a time, each rendered to a PNG via
// Devices::PngDevice; PageCount is the real page count. SaveAsTIFFClassF
// renders through the same TIFF path (the CCITT-G4 1-bit refinement is
// deferred). The Stream-output and System.Drawing.Imaging.ImageFormat
// overloads + MergeImages drop.
//
// Phased drops:
//   * Every Stream-output overload (SaveAsTIFF / SaveAsTIFFClassF /
//     GetNextImage / BindPdf with a Stream) — Stream cascade.
//   * GetNextImage(*, System.Drawing.Imaging.ImageFormat, …) —
//     ImageFormat cascade.
//   * MergeImages / MergeImagesAsTiff (Stream return) — Stream cascade.
// =============================================================================

#include <string>

#include <aspose/pdf/compression_type.hpp>
#include <aspose/pdf/facades/facade.hpp>
#include <aspose/pdf/form_presentation_mode.hpp>
#include <aspose/pdf/i_index_bitmap_converter.hpp>
#include <aspose/pdf/page_coordinate_type.hpp>
#include <aspose/pdf/page_size.hpp>
#include <aspose/pdf/rendering_options.hpp>
#include <aspose/pdf/resolution.hpp>
#include <aspose/pdf/tiff_settings.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Facades {

class PdfConverter : public Facade {
public:
    PdfConverter() noexcept = default;
    explicit PdfConverter(Aspose::Pdf::Document& document);

    // Prepare the page cursor for HasNextImage / GetNextImage.
    void DoConvert();

    // ---- TIFF (real, file output) ----

    void SaveAsTIFF(const std::string& outputFile);
    void SaveAsTIFF(const std::string& outputFile,
                    Aspose::Pdf::Devices::CompressionType compression);
    void SaveAsTIFF(const std::string& outputFile, int xResolution,
                    int yResolution);
    void SaveAsTIFF(const std::string& outputFile,
                    Aspose::Pdf::PageSize pageSize);
    void SaveAsTIFF(const std::string& outputFile,
                    Aspose::Pdf::PageSize pageSize,
                    const Aspose::Pdf::Devices::TiffSettings& settings);
    void SaveAsTIFF(const std::string& outputFile, int xResolution,
                    int yResolution,
                    Aspose::Pdf::Devices::CompressionType compression);
    void SaveAsTIFF(const std::string& outputFile, int xResolution,
                    int yResolution,
                    const Aspose::Pdf::Devices::TiffSettings& settings);
    void SaveAsTIFF(const std::string& outputFile, int xResolution,
                    int yResolution,
                    const Aspose::Pdf::Devices::TiffSettings& settings,
                    Aspose::Pdf::IIndexBitmapConverter& converter);
    void SaveAsTIFF(const std::string& outputFile,
                    const Aspose::Pdf::Devices::TiffSettings& settings);
    void SaveAsTIFF(const std::string& outputFile,
                    const Aspose::Pdf::Devices::TiffSettings& settings,
                    Aspose::Pdf::IIndexBitmapConverter& converter);

    void SaveAsTIFFClassF(const std::string& outputFile);
    void SaveAsTIFFClassF(const std::string& outputFile, int xResolution,
                          int yResolution);
    void SaveAsTIFFClassF(const std::string& outputFile,
                          Aspose::Pdf::PageSize pageSize);

    // ---- Page-by-page image extraction (real, file output → PNG) ----

    bool HasNextImage();
    void GetNextImage(const std::string& outputFile);
    void GetNextImage(const std::string& outputFile,
                      Aspose::Pdf::PageSize pageSize);

    // ---- Properties ----

    Aspose::Pdf::PageCoordinateType CoordinateType() const noexcept;
    void CoordinateType(Aspose::Pdf::PageCoordinateType value) noexcept;
    bool ShowHiddenAreas() const noexcept;
    void ShowHiddenAreas(bool value) noexcept;
    const Aspose::Pdf::RenderingOptions& RenderingOptions() const noexcept;
    void RenderingOptions(Aspose::Pdf::RenderingOptions value);
    Aspose::Pdf::Devices::FormPresentationMode FormPresentationMode()
        const noexcept;
    void FormPresentationMode(
        Aspose::Pdf::Devices::FormPresentationMode value) noexcept;
    const Aspose::Pdf::Devices::Resolution& Resolution() const noexcept;
    void Resolution(Aspose::Pdf::Devices::Resolution value);
    int StartPage() const noexcept;
    void StartPage(int value) noexcept;
    int EndPage() const noexcept;
    void EndPage(int value) noexcept;
    const std::string& Password() const noexcept;
    void Password(std::string value);
    const std::string& UserPassword() const noexcept;
    void UserPassword(std::string value);
    int PageCount() const;

private:
    void RenderTiff(const std::string& outputFile,
                    const Aspose::Pdf::Devices::Resolution& resolution,
                    const Aspose::Pdf::Devices::TiffSettings* settings);
    int FirstPage() const;
    int LastPage() const;

    Aspose::Pdf::PageCoordinateType coordinate_type_ =
        Aspose::Pdf::PageCoordinateType::CropBox;
    bool show_hidden_areas_ = false;
    Aspose::Pdf::RenderingOptions rendering_options_;
    Aspose::Pdf::Devices::FormPresentationMode form_presentation_mode_ =
        Aspose::Pdf::Devices::FormPresentationMode::Production;
    Aspose::Pdf::Devices::Resolution resolution_{150};
    int start_page_ = 1;
    int end_page_ = 0;  // 0 == last page
    std::string password_;
    std::string user_password_;
    int next_image_ = 0;  // 0 == not started
};

}  // namespace Aspose::Pdf::Facades
