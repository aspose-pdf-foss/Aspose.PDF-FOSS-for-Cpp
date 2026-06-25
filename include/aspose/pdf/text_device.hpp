#pragma once

// =============================================================================
// Aspose::Pdf::Devices::TextDevice — sealed concrete PageDevice that
// emits extracted text from a page into the output stream.
//
// v1 surface (per capabilities/text_device.yaml after the cpp Stream
// cascade in translations.yaml drops Process(Page, Stream)):
//   * default ctor + Encoding-taking ctor
//   * Encoding property (read-write)
//   * Process(Page, ostream&) hand-authored convenience (kept for
//     library users who need direct std::ostream output)
//
// Encoding-taking ctor + Encoding property activate the
// foundation::encoding charset codec primitive — see
// the project spec for the BCL contract. Default is
// UTF-8 (matches existing csharp lib state per minimum-change rule).
// =============================================================================

#include <ostream>

#include "page_device.hpp"
#include "../../internal/encoding.hpp"

namespace Aspose::Pdf {
class Page;
}

namespace Aspose::Pdf::Devices {

class TextDevice final : public PageDevice {
public:
    TextDevice();
    explicit TextDevice(const ::foundation::encoding::Encoding& encoding);

    const ::foundation::encoding::Encoding& GetEncoding() const noexcept;
    void SetEncoding(const ::foundation::encoding::Encoding& encoding) noexcept;

    void Process(const ::Aspose::Pdf::Page& page,
                 std::ostream& output) override;

private:
    const ::foundation::encoding::Encoding* encoding_;
};

}
