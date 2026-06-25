#pragma once

// =============================================================================
// Aspose::Pdf::Devices::TiffDevice — sealed concrete DocumentDevice
// emitting multi-page TIFF. DLL surface (Aspose.PDF 26.4.0):
// 18 ctors, BinarizeBradley instance method, two Process overrides
// (Document range + single Page), 6 properties.
//
// `class Resolution` and `class TiffSettings` use the elaborated
// type-specifier form because the same-named accessor methods on
// this class shadow the type names inside class scope.
// =============================================================================

#include <istream>
#include <ostream>

#include "document_device.hpp"
#include "form_presentation_mode.hpp"
#include "rendering_options.hpp"
#include "resolution.hpp"
#include "tiff_settings.hpp"

namespace Aspose::Pdf {
class Document;
class IIndexBitmapConverter;
class Page;
class PageSize;
}

namespace Aspose::Pdf::Devices {

class TiffDevice final : public DocumentDevice {
public:
    explicit TiffDevice(const class Resolution& resolution);
    TiffDevice(const class Resolution& resolution,
               const class TiffSettings& settings);
    explicit TiffDevice(const class TiffSettings& settings);
    TiffDevice();
    TiffDevice(int width, int height,
               const class Resolution& resolution,
               const class TiffSettings& settings);
    TiffDevice(const ::Aspose::Pdf::PageSize& pageSize,
               const class Resolution& resolution,
               const class TiffSettings& settings);
    TiffDevice(int width, int height,
               const class Resolution& resolution);
    TiffDevice(const ::Aspose::Pdf::PageSize& pageSize,
               const class Resolution& resolution);
    TiffDevice(int width, int height,
               const class TiffSettings& settings);
    TiffDevice(const ::Aspose::Pdf::PageSize& pageSize,
               const class TiffSettings& settings);
    TiffDevice(int width, int height);
    explicit TiffDevice(const ::Aspose::Pdf::PageSize& pageSize);

    // Caller-pluggable index-bitmap converter overloads. The
    // converter is consulted only when ColorDepth selects an
    // indexed mode (Format1bpp / Format4bpp / Format8bpp); for
    // Default / Format24bpp the device emits direct RGB and the
    // converter is unused. The converter's lifetime must outlive
    // the device — it's stored as a non-owning pointer.
    TiffDevice(const class Resolution& resolution,
               const class TiffSettings& settings,
               ::Aspose::Pdf::IIndexBitmapConverter& converter);
    TiffDevice(const class TiffSettings& settings,
               ::Aspose::Pdf::IIndexBitmapConverter& converter);
    TiffDevice(int width, int height,
               const class Resolution& resolution,
               const class TiffSettings& settings,
               ::Aspose::Pdf::IIndexBitmapConverter& converter);
    TiffDevice(const ::Aspose::Pdf::PageSize& pageSize,
               const class Resolution& resolution,
               const class TiffSettings& settings,
               ::Aspose::Pdf::IIndexBitmapConverter& converter);
    TiffDevice(int width, int height,
               const class TiffSettings& settings,
               ::Aspose::Pdf::IIndexBitmapConverter& converter);
    TiffDevice(const ::Aspose::Pdf::PageSize& pageSize,
               const class TiffSettings& settings,
               ::Aspose::Pdf::IIndexBitmapConverter& converter);

    void BinarizeBradley(std::istream& inputImageStream,
                         std::ostream& outputImageStream,
                         double threshold);

    void Process(const ::Aspose::Pdf::Document& document,
                 int fromPage, int toPage,
                 std::ostream& output) override;

    void Process(const ::Aspose::Pdf::Page& page,
                 std::ostream& output) override;

    ::Aspose::Pdf::RenderingOptions& RenderingOptions();
    void RenderingOptions(const ::Aspose::Pdf::RenderingOptions& value);

    ::Aspose::Pdf::Devices::FormPresentationMode
        FormPresentationMode() const noexcept;
    void FormPresentationMode(
        ::Aspose::Pdf::Devices::FormPresentationMode value) noexcept;

    const class TiffSettings& Settings() const noexcept;

    const class Resolution& Resolution() const noexcept;

    int Width() const noexcept;
    int Height() const noexcept;

private:
    int width_ = 0;
    int height_ = 0;
    class Resolution resolution_{72};
    class TiffSettings settings_{};
    ::Aspose::Pdf::RenderingOptions rendering_options_{};
    ::Aspose::Pdf::Devices::FormPresentationMode form_presentation_mode_ =
        ::Aspose::Pdf::Devices::FormPresentationMode::Production;
    ::Aspose::Pdf::IIndexBitmapConverter* converter_ = nullptr;
};

}
