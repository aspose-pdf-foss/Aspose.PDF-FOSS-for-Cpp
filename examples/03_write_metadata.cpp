// Add /Info metadata and save via incremental update.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_info.hpp>

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: 03_write_metadata <input.pdf> <output.pdf>\n";
        return 2;
    }
    Aspose::Pdf::Document doc(argv[1]);

    doc.SetTitle("Hello from Aspose.Pdf C++ FOSS");

    auto& info = doc.Info();
    info.Author("Demo author");
    info.Creator("Aspose.Pdf FOSS C++ v0");
    info.Subject("DocumentInfo round-trip demo");
    info.Add("Custom-Key", "Custom value via Add()");

    doc.Save(argv[2]);

    std::cout << "Wrote " << argv[2]
              << " with /Info edits flushed via incremental update.\n";
    return 0;
}
