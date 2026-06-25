// =============================================================================
// Go-parity: document_test.go → canonical C++ Document/PageCollection.
//
// Ported (canonical): Open, Save, page-rotate (all + specific page),
// RemoveUnusedObjects → Document::OptimizeResources.
// Skipped: Rotate accumulation / duplicate-page-num / invalid-angle (Go varargs
// semantics; canonical Page.Rotate is an absolute property), Reorder (invented),
// ExtractPages / WriteTo (no canonical stream-extract path in the cpp subset).
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rotation.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path PdfDir() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return std::filesystem::path(env);
    return std::filesystem::path(__FILE__)
        .parent_path()
        .parent_path()
        .parent_path() /
        "pdfs";
}

std::string TwoPages() { return (PdfDir() / "two_pages.pdf").string(); }

std::string TmpOut(const std::string& tag) {
    return (std::filesystem::temp_directory_path() / ("go_doc_" + tag + ".pdf"))
        .string();
}

constexpr int kMarketingPages = 2;  // two_pages.pdf

}  // namespace

// TestDocumentOpen
TEST(GoDocument, Open) {
    Document doc{TwoPages()};
    EXPECT_EQ(doc.Pages().Count(), kMarketingPages);
}

// TestDocumentSave
TEST(GoDocument, Save) {
    const std::string out = TmpOut("save");
    {
        Document doc{TwoPages()};
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), kMarketingPages);
    std::filesystem::remove(out);
}

// TestDocumentRotate (rotate every page) — canonical per-page Page.Rotate.
TEST(GoDocument, RotateAllPages) {
    const std::string out = TmpOut("rotate");
    {
        Document doc{TwoPages()};
        for (int i = 1; i <= doc.Pages().Count(); ++i)
            doc.Pages()[i].Rotate(Rotation::on90);
        doc.Save(out);
    }
    Document doc{out};
    for (int i = 1; i <= doc.Pages().Count(); ++i)
        EXPECT_EQ(doc.Pages()[i].Rotate(), Rotation::on90);
    std::filesystem::remove(out);
}

// TestDocumentRotateSpecificPage — only page 1 rotated.
TEST(GoDocument, RotateSpecificPage) {
    const std::string out = TmpOut("rotate_one");
    {
        Document doc{TwoPages()};
        doc.Pages()[1].Rotate(Rotation::on180);
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(doc.Pages()[1].Rotate(), Rotation::on180);
    EXPECT_EQ(doc.Pages()[2].Rotate(), Rotation::None);
    std::filesystem::remove(out);
}

// TestRemoveUnusedObjectsRoundTrip → Document::OptimizeResources.
TEST(GoDocument, OptimizeResourcesRoundTrip) {
    const std::string out = TmpOut("optimize");
    {
        Document doc{TwoPages()};
        doc.OptimizeResources();
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), kMarketingPages);
    std::filesystem::remove(out);
}
