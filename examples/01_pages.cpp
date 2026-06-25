// Open a PDF, print the page count, iterate through pages.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: 01_pages <input.pdf>\n";
        return 2;
    }
    Aspose::Pdf::Document doc(argv[1]);
    auto& pages = doc.Pages();
    std::cout << "File:  " << argv[1] << "\n"
              << "Pages: " << pages.Count() << "\n";
    for (int i = 1; i <= static_cast<int>(pages.Count()); ++i) {
        std::cout << "  page " << i
                  << " (Number=" << pages[i].Number() << ")\n";
    }
    return 0;
}
