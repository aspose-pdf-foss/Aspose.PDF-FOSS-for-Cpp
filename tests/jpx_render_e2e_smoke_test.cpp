// =============================================================================
// jpx_render_e2e_smoke_test — proves the page_renderer /JPXDecode dispatch path
// end-to-end: assemble a one-page PDF whose sole image XObject is the
// rgb_5x3_32.j2k JPEG 2000 fixture, render the page to an (uncompressed) BMP,
// and assert the four quadrants decode to their distinct fixture colours. This
// exercises foundation::jpx::Decode through the real image-XObject pipeline,
// not just the unit decoder.
// =============================================================================

#include <aspose/pdf/bmp_device.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/resolution.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::filesystem::path JpxFixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path() / "fixtures" / "jpx";
}

std::string ReadBinary(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open fixture: " + path.string());
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Minimal hand-assembled PDF wrapping `j2k` as a 32x32 DeviceRGB /JPXDecode
// image XObject drawn across the whole MediaBox. Object byte offsets are
// recorded as the body is built so the xref table is exact (the parser reads
// the image stream by its /Length, so the binary codestream is embedded
// verbatim).
std::string BuildJpxImagePdf(const std::string& j2k) {
    std::string pdf = "%PDF-1.5\n";
    std::vector<std::size_t> off(6, 0);

    auto add_obj = [&](int n, const std::string& body) {
        off[static_cast<std::size_t>(n)] = pdf.size();
        pdf += std::to_string(n) + " 0 obj\n" + body + "\nendobj\n";
    };

    add_obj(1, "<< /Type /Catalog /Pages 2 0 R >>");
    add_obj(2, "<< /Type /Pages /Kids [3 0 R] /Count 1 >>");
    add_obj(3,
            "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 32 32] "
            "/Resources << /XObject << /Im0 4 0 R >> >> /Contents 5 0 R >>");

    off[4] = pdf.size();
    pdf += "4 0 obj\n<< /Type /XObject /Subtype /Image /Width 32 /Height 32 "
           "/ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /JPXDecode "
           "/Length " + std::to_string(j2k.size()) + " >>\nstream\n";
    pdf += j2k;
    pdf += "\nendstream\nendobj\n";

    const std::string content = "q 32 0 0 32 0 0 cm /Im0 Do Q\n";
    off[5] = pdf.size();
    pdf += "5 0 obj\n<< /Length " + std::to_string(content.size()) +
           " >>\nstream\n" + content + "endstream\nendobj\n";

    const std::size_t xref_off = pdf.size();
    pdf += "xref\n0 6\n0000000000 65535 f \n";
    for (int i = 1; i <= 5; ++i) {
        char buf[24];
        std::snprintf(buf, sizeof buf, "%010zu 00000 n \n", off[static_cast<std::size_t>(i)]);
        pdf += buf;
    }
    pdf += "trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n" +
           std::to_string(xref_off) + "\n%%EOF\n";
    return pdf;
}

std::uint32_t LE32(const std::string& s, std::size_t at) {
    std::uint32_t v = 0;
    std::memcpy(&v, s.data() + at, 4);
    return v;
}

enum class Quad { Red, Green, Blue, Yellow, Other };

Quad Classify(int r, int g, int b) {
    if (r > 120 && g < 120 && b < 120) return Quad::Red;
    if (g > 120 && r < 120 && b < 120) return Quad::Green;
    if (b > 120 && r < 120 && g < 120) return Quad::Blue;
    if (r > 120 && g > 120 && b < 120) return Quad::Yellow;
    return Quad::Other;
}

}  // namespace

TEST(JpxRenderE2E, Rgb53ImageRendersQuadrantColours) {
    const std::string j2k = ReadBinary(JpxFixtureRoot() / "rgb_5x3_32.j2k");

    const auto tmp = std::filesystem::temp_directory_path() / "jpx_render_e2e.pdf";
    {
        std::ofstream out(tmp, std::ios::binary);
        const std::string pdf = BuildJpxImagePdf(j2k);
        out.write(pdf.data(), static_cast<std::streamsize>(pdf.size()));
    }

    Aspose::Pdf::Document doc(tmp.string());
    auto page = doc.Pages()[1];

    Aspose::Pdf::Devices::BmpDevice bmp(Aspose::Pdf::Devices::Resolution(72));
    std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
    bmp.Process(page, sink);
    const std::string out = sink.str();

    std::filesystem::remove(tmp);

    ASSERT_GE(out.size(), 54u);
    ASSERT_EQ(out[0], 'B');
    ASSERT_EQ(out[1], 'M');
    const std::uint32_t pixel_off = LE32(out, 10);
    const std::int32_t width = static_cast<std::int32_t>(LE32(out, 18));
    const std::int32_t raw_h = static_cast<std::int32_t>(LE32(out, 22));
    std::uint16_t bpp = 0;
    std::memcpy(&bpp, out.data() + 28, 2);
    ASSERT_EQ(bpp, 24) << "expected a 24-bit BMP from BmpDevice";
    ASSERT_GT(width, 0);

    const bool top_down = raw_h < 0;
    const std::int32_t height = top_down ? -raw_h : raw_h;
    const std::size_t stride = (static_cast<std::size_t>(width) * 3 + 3) & ~std::size_t{3};

    // Sample a representative pixel near each quadrant centre, in image
    // (top-down) coordinates, mapping to the BMP row order.
    auto sample = [&](std::int32_t img_x, std::int32_t img_y) -> Quad {
        const std::int32_t file_row = top_down ? img_y : (height - 1 - img_y);
        const std::size_t at = pixel_off + static_cast<std::size_t>(file_row) * stride +
                               static_cast<std::size_t>(img_x) * 3;
        if (at + 3 > out.size()) return Quad::Other;
        const int b = static_cast<unsigned char>(out[at + 0]);
        const int g = static_cast<unsigned char>(out[at + 1]);
        const int r = static_cast<unsigned char>(out[at + 2]);
        return Classify(r, g, b);
    };

    const std::int32_t qx0 = width / 4, qx1 = 3 * width / 4;
    const std::int32_t qy0 = height / 4, qy1 = 3 * height / 4;
    const std::set<Quad> seen = {
        sample(qx0, qy0), sample(qx1, qy0), sample(qx0, qy1), sample(qx1, qy1)};

    // The four fixture quadrants are red / green / blue / yellow; all four
    // distinct colours must appear (orientation-agnostic), proving the JPX
    // codestream decoded and rendered through the /JPXDecode dispatch path
    // rather than producing a blank or single-colour page.
    EXPECT_EQ(seen.count(Quad::Other), 0u) << "a quadrant sampled an unexpected colour";
    EXPECT_EQ(seen.size(), 4u) << "expected four distinct quadrant colours";
    EXPECT_EQ(seen.count(Quad::Red), 1u);
    EXPECT_EQ(seen.count(Quad::Green), 1u);
    EXPECT_EQ(seen.count(Quad::Blue), 1u);
    EXPECT_EQ(seen.count(Quad::Yellow), 1u);
}
