#include <aspose/pdf/text_device.hpp>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>

#include "text_extractor.hpp"
#include "encoding.hpp"

#include <cstring>
#include <ostream>
#include <span>
#include <vector>

namespace Aspose::Pdf::Devices {

TextDevice::TextDevice()
    : encoding_(&::foundation::encoding::Encoding::UTF8()) {}

TextDevice::TextDevice(const ::foundation::encoding::Encoding& encoding)
    : encoding_(&encoding) {}

const ::foundation::encoding::Encoding& TextDevice::GetEncoding() const noexcept {
    return *encoding_;
}

void TextDevice::SetEncoding(
    const ::foundation::encoding::Encoding& encoding) noexcept {
    encoding_ = &encoding;
}

void TextDevice::Process(const ::Aspose::Pdf::Page& page,
                         std::ostream& output) {
    const auto& bytes = page.doc_->bytes_;
    auto pages = foundation::text_extractor::Parse(
        std::span<const std::byte>(bytes.data(), bytes.size()));
    if (page.leaf_index_ < pages.size()) {
        const auto& text = pages[page.leaf_index_];
        auto encoded = encoding_->GetBytes(text);
        if (!encoded.empty()) {
            output.write(reinterpret_cast<const char*>(encoded.data()),
                         static_cast<std::streamsize>(encoded.size()));
        }
    }
}

}  // namespace Aspose::Pdf::Devices
