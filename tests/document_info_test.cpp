// Smoke test for Aspose::Pdf::DocumentInfo.
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_info.hpp>

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
    // 1. Read existing /Info entries.
    Aspose::Pdf::Document doc(
        (FixtureDir() / "with_info.pdf").string());
    auto& info = doc.Info();
    Check(!info.Title().empty(),    "with_info.pdf has Title");
    Check(!info.Author().empty(),   "with_info.pdf has Author");
    Check(!info.Producer().empty(), "with_info.pdf has Producer");

    // 2. SetTitle persists across save + reopen.
    const auto out = std::filesystem::temp_directory_path() /
                     "aspose_doc_info_smoke.pdf";
    doc.SetTitle("Smoke");
    doc.Save(out.string());
    Aspose::Pdf::Document round_trip(out.string());
    Check(round_trip.Info().Title() == "Smoke",
          "SetTitle persists after Save+reopen");

    // 3. Untouched fields preserved verbatim.
    Check(round_trip.Info().Producer() == info.Producer(),
          "Producer unchanged when only Title set");

    std::filesystem::remove(out);

    // 4. IsPredefinedKey static predicate.
    Check(Aspose::Pdf::DocumentInfo::IsPredefinedKey("Title"),
          "IsPredefinedKey('Title') is true");
    Check(!Aspose::Pdf::DocumentInfo::IsPredefinedKey("CustomKey"),
          "IsPredefinedKey('CustomKey') is false");

    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}
