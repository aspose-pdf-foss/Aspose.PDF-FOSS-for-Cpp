// gtest cases for the Document() no-arg ctor + Save() no-arg
// overload added in the cross-lib parity beat.
//
// Sibling of document_test.cpp which exercises
// the same surfaces. This file lets
// the same checks run via ctest so the gtest harness flags
// regressions during normal cmake builds.

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace {

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    // Mirrors document_test.cpp: <lib>/tests/<file> → <lib>/pdfs/.
    return std::filesystem::path(__FILE__)
        .parent_path().parent_path() / "pdfs";
}

std::vector<char> ReadAll(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    return std::vector<char>(std::istreambuf_iterator<char>(f), {});
}

}  // namespace

TEST(DocumentSmoke, EmptyCtor_YieldsZeroPageDocument) {
    Aspose::Pdf::Document doc;
    EXPECT_EQ(doc.Pages().Count(), 0u);
}

TEST(DocumentSmoke, SaveNoArg_RoundTripsBytesToSourceFilename) {
    const auto src = FixtureRoot() / "two_pages.pdf";
    const auto tmp = std::filesystem::temp_directory_path() /
                     "aspose_doc_save_in_place.pdf";
    std::filesystem::copy_file(
        src, tmp, std::filesystem::copy_options::overwrite_existing);

    try {
        Aspose::Pdf::Document doc(tmp.string());
        doc.Save();

        const auto orig = ReadAll(src);
        const auto after = ReadAll(tmp);
        EXPECT_EQ(orig, after);
    } catch (...) {
        std::filesystem::remove(tmp);
        throw;
    }
    std::filesystem::remove(tmp);
}

TEST(DocumentSmoke, SaveNoArg_ThrowsWhenConstructedViaEmptyCtor) {
    Aspose::Pdf::Document doc;
    EXPECT_THROW(doc.Save(), std::runtime_error);
}
