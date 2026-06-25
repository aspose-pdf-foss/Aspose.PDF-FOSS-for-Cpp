// =============================================================================
// text_device_e2e_smoke_test — drives Process(Page, ostream) end-to-end
// for TextDevice. Asserts the extracted text matches the fixture's
// known content.
// =============================================================================

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/text_device.hpp>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>

namespace {
std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path()
        / "fixtures" / "text_extractor";
}
}

TEST(TextDeviceE2E, HelloWorld_ExtractsHelloWorld) {
    const auto pdf = FixtureRoot() / "hello_world.pdf";
    Aspose::Pdf::Document doc(pdf.string());
    auto page = doc.Pages()[1];

    Aspose::Pdf::Devices::TextDevice text;
    std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
    text.Process(page, sink);

    EXPECT_EQ(sink.str(), "Hello World");
}

TEST(TextDeviceE2E, TwoLines_PerPageMatchesAbsorber) {
    const auto pdf = FixtureRoot() / "two_lines.pdf";
    Aspose::Pdf::Document doc(pdf.string());

    Aspose::Pdf::Devices::TextDevice text;
    std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
    text.Process(doc.Pages()[1], sink);

    const auto out = sink.str();
    EXPECT_NE(out.find("Line one"), std::string::npos);
    EXPECT_NE(out.find("Line two"), std::string::npos);
}
