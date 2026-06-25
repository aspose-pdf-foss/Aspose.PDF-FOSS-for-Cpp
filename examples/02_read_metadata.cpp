// Read DocumentInfo (PDF /Info dictionary).
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_info.hpp>

#include <iostream>
#include <string>

void Show(const std::string& label, const std::string& value) {
    std::cout << "  " << label << ": "
              << (value.empty() ? "<empty>" : value) << "\n";
}

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: 02_read_metadata <input.pdf>\n"; return 2; }
    Aspose::Pdf::Document doc(argv[1]);
    auto& info = doc.Info();
    std::cout << "Metadata of " << argv[1] << ":\n";
    Show("Title",    info.Title());
    Show("Author",   info.Author());
    Show("Creator",  info.Creator());
    Show("Producer", info.Producer());
    Show("Subject",  info.Subject());
    Show("Keywords", info.Keywords());
    return 0;
}
