// =============================================================================
// stamp_image_smoke_test — beat P-3. StampAnnotation.Image — the stamp's custom
// image content as raw bytes (get/set). Ships on the cpp track via a per-member
// translation override (System.IO.Stream -> std::vector<std::byte>).
// =============================================================================

#include <cstddef>
#include <vector>

#include <aspose/pdf/annotations/stamp_annotation.hpp>
#include <aspose/pdf/document.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

}  // namespace

TEST(StampImageSmoke, ImageBytesRoundTrip) {
    Document doc;
    StampAnnotation stamp{doc};
    EXPECT_TRUE(stamp.Image().empty());

    std::vector<std::byte> bytes;
    for (int i = 0; i < 8; ++i)
        bytes.push_back(static_cast<std::byte>(i * 16));
    stamp.Image(bytes);

    ASSERT_EQ(stamp.Image().size(), 8u);
    EXPECT_EQ(stamp.Image()[1], static_cast<std::byte>(16));
    EXPECT_EQ(stamp.Image()[7], static_cast<std::byte>(112));
}
