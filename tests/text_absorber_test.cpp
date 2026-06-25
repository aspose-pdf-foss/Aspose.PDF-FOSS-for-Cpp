// Smoke test for Aspose::Pdf::Text::TextAbsorber.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/text_absorber.hpp>

#include <filesystem>
#include <iostream>
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
    // 1. Visit(Document) — accumulates all pages' text.
    Aspose::Pdf::Document doc(
        (FixtureDir() / "two_lines.pdf").string());
    Aspose::Pdf::Text::TextAbsorber doc_absorber;
    doc_absorber.Visit(doc);
    const auto doc_text = doc_absorber.Text();
    Check(doc_text.find("Line one") != std::string::npos,
          "Visit(Document) finds 'Line one'");
    Check(doc_text.find("Line two") != std::string::npos,
          "Visit(Document) finds 'Line two'");
    Check(!doc_absorber.HasErrors(), "no errors after clean Visit");

    // 2. Visit(Page) — single page only.
    Aspose::Pdf::Text::TextAbsorber page_absorber;
    page_absorber.Visit(doc.Pages()[1]);
    Check(!page_absorber.Text().empty(), "Visit(Page) extracts text");

    // 3. Fresh absorber starts empty.
    Aspose::Pdf::Text::TextAbsorber fresh;
    Check(fresh.Text().empty(), "fresh TextAbsorber.Text is empty");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
