// Smoke test for Aspose::Pdf::PageCollection.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <filesystem>
#include <iostream>
#include <stdexcept>

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
    Aspose::Pdf::Document doc(
        (FixtureDir() / "two_pages.pdf").string());
    auto& pages = doc.Pages();

    Check(pages.Count() == 2u, "Count returns 2 for two_pages.pdf");
    Check(pages[1].Number() == 1, "indexer [1] returns first page");
    Check(pages[2].Number() == 2, "indexer [2] returns second page");

    bool threw_zero = false;
    try { (void)pages[0]; }
    catch (const std::out_of_range&) { threw_zero = true; }
    Check(threw_zero, "indexer [0] throws (1-based)");

    bool threw_high = false;
    try { (void)pages[3]; }
    catch (const std::out_of_range&) { threw_high = true; }
    Check(threw_high, "indexer [Count+1] throws");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
