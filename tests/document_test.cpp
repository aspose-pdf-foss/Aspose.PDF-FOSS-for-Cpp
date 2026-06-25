// Smoke test for Aspose::Pdf::Document.
//
// Exercises Document(filename) ctor + Save round-trip + Pages()
// against the bundled fixtures in this directory's fixtures/.
// Exercises the public Document API surface.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <stdexcept>

namespace {

int passed = 0;
int failed = 0;

void Check(bool cond, const char* name) {
    if (cond) { std::cout << "PASS " << name << "\n"; ++passed; }
    else      { std::cout << "FAIL " << name << "\n"; ++failed; }
}

std::filesystem::path FixtureDir() {
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::vector<char> ReadAll(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::vector<char>(std::istreambuf_iterator<char>(f), {});
}

}  // namespace

int main() {
    const auto fixtures = FixtureDir();

    // 1. Open + Pages.Count
    Aspose::Pdf::Document doc((fixtures / "two_pages.pdf").string());
    Check(doc.Pages().Count() == 2u, "two_pages.pdf has 2 pages");

    // 2. Save with no edits is byte-identical (verbatim path).
    const auto tmp_out = std::filesystem::temp_directory_path() /
                         "aspose_doc_smoke_out.pdf";
    doc.Save(tmp_out.string());
    const auto in = ReadAll(fixtures / "two_pages.pdf");
    const auto out = ReadAll(tmp_out);
    Check(in == out, "Save() byte-identical when not dirty");
    std::filesystem::remove(tmp_out);

    // 3. Missing file throws system_error.
    bool threw = false;
    try {
        Aspose::Pdf::Document missing("/no/such/path.pdf");
    } catch (const std::system_error&) {
        threw = true;
    } catch (...) {}
    Check(threw, "missing file throws system_error");

    // 4. Document() default ctor yields a 0-page document.
    {
        Aspose::Pdf::Document empty_doc;
        Check(empty_doc.Pages().Count() == 0u,
              "Document() default ctor yields 0-page document");
    }

    // 5. Save() no-arg writes back to source filename.
    {
        const auto src = fixtures / "two_pages.pdf";
        const auto tmp = std::filesystem::temp_directory_path() /
                         "aspose_doc_save_in_place.pdf";
        std::filesystem::copy_file(
            src, tmp,
            std::filesystem::copy_options::overwrite_existing);
        try {
            Aspose::Pdf::Document doc(tmp.string());
            doc.Save();
            const auto orig = ReadAll(src);
            const auto after = ReadAll(tmp);
            Check(orig == after,
                  "Save() round-trips bytes to source filename");
        } catch (...) {
            Check(false, "Save() no-arg threw unexpectedly");
        }
        std::filesystem::remove(tmp);
    }

    // 6. Save() no-arg throws when constructed via empty ctor.
    {
        bool save_threw = false;
        try {
            Aspose::Pdf::Document empty_doc;
            empty_doc.Save();
        } catch (const std::runtime_error&) {
            save_threw = true;
        } catch (...) {}
        Check(save_threw,
              "Save() no-arg throws on empty-ctor Document");
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
