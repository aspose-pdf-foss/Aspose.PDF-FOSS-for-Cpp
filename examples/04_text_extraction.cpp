// Extract all text from a document via TextAbsorber.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/text_absorber.hpp>

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: 04_text_extraction <input.pdf>\n"; return 2; }
    Aspose::Pdf::Document doc(argv[1]);
    Aspose::Pdf::Text::TextAbsorber absorber;
    absorber.Visit(doc);
    std::cout << "--- text from " << argv[1] << " ---\n"
              << absorber.Text() << "\n"
              << "--- (HasErrors=" << (absorber.HasErrors() ? "true" : "false")
              << ") ---\n";
    return 0;
}
