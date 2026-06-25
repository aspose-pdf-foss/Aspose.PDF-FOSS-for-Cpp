// Smoke test for Aspose::Pdf::Devices::TextDevice.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/text_device.hpp>
#include <aspose/pdf/text_extraction_options.hpp>

#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
int passed = 0, failed = 0;
void Check(bool c, const char* n) {
    if (c) { std::cout << "PASS " << n << "\n"; ++passed; }
    else   { std::cout << "FAIL " << n << "\n"; ++failed; }
}
std::filesystem::path FixtureDir() {
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}
}

int main() {
    using namespace Aspose::Pdf;
    Document doc((FixtureDir() / "two_lines.pdf").string());

    Devices::TextDevice text;
    std::stringstream sink(
        std::ios::binary | std::ios::out | std::ios::in);
    text.Process(doc.Pages()[1], sink);
    const auto out = sink.str();
    Check(out.find("Line one") != std::string::npos,
          "TextDevice extracts 'Line one'");
    Check(out.find("Line two") != std::string::npos,
          "TextDevice extracts 'Line two'");

    // ExtractionOptions getter throws if no options set.
    Devices::TextDevice empty;
    bool threw = false;
    try { (void)empty.ExtractionOptions(); }
    catch (const std::runtime_error&) { threw = true; }
    Check(threw, "ExtractionOptions() throws when unset");

    // With options:
    Text::TextExtractionOptions opts(
        Text::TextExtractionOptions::TextFormattingMode::Pure);
    Devices::TextDevice with(opts);
    Check(&with.ExtractionOptions() == &opts,
          "ExtractionOptions() round-trips");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
