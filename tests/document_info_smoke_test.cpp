#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_info.hpp>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstddef>

namespace {

std::filesystem::path FixtureDir() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path()             // tests/
        / "fixtures" / "metadata";
}

}  // namespace

TEST(DocumentInfoSmoke, ReadsExistingInfoFromWithInfoFixture) {
    const auto src = FixtureDir() / "with_info.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;

    Aspose::Pdf::Document doc(src.string());
    auto& info = doc.Info();

    EXPECT_FALSE(info.Title().empty());
    EXPECT_FALSE(info.Author().empty());
    EXPECT_FALSE(info.Producer().empty());
}

TEST(DocumentInfoSmoke, SetTitleRoundTrips) {
    const auto src = FixtureDir() / "with_info.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;

    const std::string new_title = "Updated by FOSS Aspose.Pdf";
    const auto out = std::filesystem::temp_directory_path() /
                     "aspose_doc_info_set_title.pdf";

    {
        Aspose::Pdf::Document doc(src.string());
        doc.SetTitle(new_title);
        doc.Save(out.string());
    }

    Aspose::Pdf::Document reopened(out.string());
    EXPECT_EQ(reopened.Info().Title(), new_title);

    std::filesystem::remove(out);
}

TEST(DocumentInfoSmoke, InfoSetterRoundTripsAcrossSave) {
    const auto src = FixtureDir() / "with_info.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;

    const auto out = std::filesystem::temp_directory_path() /
                     "aspose_doc_info_setters.pdf";

    {
        Aspose::Pdf::Document doc(src.string());
        auto& info = doc.Info();
        info.Author("Smoke Test Author");
        info.Subject("Subject");
        info.Keywords("k1; k2; k3");
        doc.Save(out.string());
    }

    Aspose::Pdf::Document reopened(out.string());
    auto& info = reopened.Info();
    EXPECT_EQ(info.Author(), "Smoke Test Author");
    EXPECT_EQ(info.Subject(), "Subject");
    EXPECT_EQ(info.Keywords(), "k1; k2; k3");

    std::filesystem::remove(out);
}

TEST(DocumentInfoSmoke, NoMutationsSaveIsByteIdentical) {
    const auto src = FixtureDir() / "with_info.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;

    const auto out = std::filesystem::temp_directory_path() /
                     "aspose_doc_info_pristine.pdf";

    {
        Aspose::Pdf::Document doc(src.string());
        // Reading Info() must not mark dirty.
        (void)doc.Info().Title();
        doc.Save(out.string());
    }

    auto read_all = [](const std::filesystem::path& p) {
        std::ifstream f(p, std::ios::binary | std::ios::ate);
        std::vector<char> b(static_cast<std::size_t>(f.tellg()));
        f.seekg(0);
        f.read(b.data(), static_cast<std::streamsize>(b.size()));
        return b;
    };
    EXPECT_EQ(read_all(src), read_all(out))
        << "Save() with no mutations must round-trip byte-identical";

    std::filesystem::remove(out);
}

TEST(DocumentInfoSmoke, IsPredefinedKey) {
    EXPECT_TRUE(Aspose::Pdf::DocumentInfo::IsPredefinedKey("Title"));
    EXPECT_TRUE(Aspose::Pdf::DocumentInfo::IsPredefinedKey("Author"));
    EXPECT_TRUE(Aspose::Pdf::DocumentInfo::IsPredefinedKey("CreationDate"));
    EXPECT_FALSE(Aspose::Pdf::DocumentInfo::IsPredefinedKey("CustomKey"));
    EXPECT_FALSE(Aspose::Pdf::DocumentInfo::IsPredefinedKey(""));
}

TEST(DocumentInfoSmoke, SaveOnDocumentWithoutInfoThrows) {
    const auto src = FixtureDir() / "without_info.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;

    Aspose::Pdf::Document doc(src.string());
    doc.SetTitle("attempted");

    const auto out = std::filesystem::temp_directory_path() /
                     "aspose_doc_info_noinfo.pdf";
    EXPECT_THROW(doc.Save(out.string()), std::runtime_error);
}
