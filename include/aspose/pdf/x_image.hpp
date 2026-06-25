#pragma once

// =============================================================================
// Aspose::Pdf::XImage — an embedded image XObject (PDF §8.9). Mirrors canonical
// Aspose.Pdf.XImage (Aspose.PDF 26.4.0): the image's name, pixel dimensions,
// mask / transparency flags and its raw stream bytes. Obtained by enumerating
// a page's Resources().Images().
// =============================================================================

#include <cstddef>
#include <string>
#include <vector>

namespace Aspose::Pdf {

class XImage {
public:
    // Canonical XImage.Width / Height — pixel dimensions.
    int Width() const noexcept;
    int Height() const noexcept;

    // Canonical XImage.Name — the image's /XObject resource name.
    const std::string& Name() const noexcept;
    void Name(std::string value);

    // Canonical XImage.ContainsTransparency — whether the image has a soft mask.
    bool ContainsTransparency() const noexcept;

    // Canonical XImage.ImageMask — whether the image is a stencil mask.
    bool ImageMask() const noexcept;

    // Canonical XImage.GetRawImageData — the image's raw (filter-encoded)
    // stream bytes (e.g. the JPEG bytes for a DCTDecode image).
    std::vector<std::byte> GetRawImageData() const;

private:
    std::string name_;
    int width_ = 0;
    int height_ = 0;
    bool contains_transparency_ = false;
    bool image_mask_ = false;
    std::vector<std::byte> data_;
    // True for an image staged by XImageCollection::Add: `data_` then holds
    // the source image-file bytes (PNG / JPEG) to embed as a new XObject at
    // Document::Save, rather than the filter-encoded body of a loaded image.
    bool is_new_ = false;
    // True for a loaded image whose content was replaced via
    // XImageCollection::Replace: `data_` then holds the new source image-file
    // bytes, re-embedded under the same resource name at Document::Save.
    bool is_replaced_ = false;

    friend class Document;
    friend class XImageCollection;
};

}  // namespace Aspose::Pdf
