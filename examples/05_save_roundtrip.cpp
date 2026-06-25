// Open + Save with no edits — should byte-roundtrip exactly.
#include <aspose/pdf/document.hpp>

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

std::vector<char> ReadAll(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return std::vector<char>(std::istreambuf_iterator<char>(f), {});
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: 05_save_roundtrip <input.pdf> <output.pdf>\n";
        return 2;
    }
    Aspose::Pdf::Document doc(argv[1]);
    doc.Save(argv[2]);

    auto in_bytes  = ReadAll(argv[1]);
    auto out_bytes = ReadAll(argv[2]);
    std::cout << "input:  " << in_bytes.size()  << " bytes\n"
              << "output: " << out_bytes.size() << " bytes\n"
              << "byte-identical: "
              << (in_bytes == out_bytes ? "yes" : "no") << "\n";
    return 0;
}
