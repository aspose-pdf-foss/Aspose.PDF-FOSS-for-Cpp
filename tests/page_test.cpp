// Smoke test for Aspose::Pdf::Page.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <filesystem>
#include <iostream>

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

    auto p1 = doc.Pages()[1];
    auto p2 = doc.Pages()[2];

    Check(p1.Number() == 1, "first page Number == 1");
    Check(p2.Number() == 2, "second page Number == 2");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
