// =============================================================================
// Go-parity: page_test.go → canonical C++ Page / PageCollection.
//
// Ported (canonical): page enumeration + Number, single-page access,
// page geometry (Rect), per-page Rotation get/set, box fallback to MediaBox
// (TrimBox/BleedBox/ArtBox/CropBox), invalid page index.
// Skipped: PageSizes() free helper (invented aggregate); the Rotate-accumulation
// tail of TestPageRotation (Go varargs semantics — canonical Page.Rotate is an
// absolute property). Page sizes are asserted relative to each page's MediaBox
// rather than the Go fixture's hardcoded letter dimensions.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>
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

}  // namespace

// TestDocumentPages — enumerate pages; each Number() in range, Rect non-empty.
TEST(GoPage, EnumeratePages) {
    Document doc{TwoPages()};
    auto& pages = doc.Pages();
    ASSERT_EQ(pages.Count(), 2);
    for (int i = 1; i <= pages.Count(); ++i) {
        EXPECT_GE(pages[i].Number(), 1);
        EXPECT_LE(pages[i].Number(), pages.Count());
        EXPECT_GT(pages[i].Rect().Width(), 0.0);
        EXPECT_GT(pages[i].Rect().Height(), 0.0);
    }
}

// TestDocumentPage — single-page access by 1-based index.
TEST(GoPage, SinglePageAccess) {
    Document doc{TwoPages()};
    Page p = doc.Pages()[2];
    EXPECT_EQ(p.Number(), 2);
    EXPECT_GT(p.Rect().Width(), 0.0);
}

// TestPageRotation — initial None; set page 1 → on90; page 2 unaffected.
TEST(GoPage, Rotation) {
    Document doc{TwoPages()};
    for (int i = 1; i <= doc.Pages().Count(); ++i)
        EXPECT_EQ(doc.Pages()[i].Rotate(), Rotation::None);

    doc.Pages()[1].Rotate(Rotation::on90);
    EXPECT_EQ(doc.Pages()[1].Rotate(), Rotation::on90);
    EXPECT_EQ(doc.Pages()[2].Rotate(), Rotation::None);
}

// TestPageBoxesFallbackToMediaBox — Crop/Trim/Bleed/ArtBox fall back to Media.
TEST(GoPage, BoxesFallBackToMediaBox) {
    Document doc{TwoPages()};
    Page p = doc.Pages()[1];
    const Rectangle media = p.MediaBox();
    ASSERT_GT(media.Width(), 0.0);

    for (const Rectangle box : {p.CropBox(), p.TrimBox(), p.BleedBox(),
                               p.ArtBox()}) {
        EXPECT_NEAR(box.Width(), media.Width(), 1e-6);
        EXPECT_NEAR(box.Height(), media.Height(), 1e-6);
    }
}

// TestDocumentPageInvalidNumber — out-of-range index throws.
TEST(GoPage, InvalidPageNumberThrows) {
    Document doc{TwoPages()};
    EXPECT_THROW(doc.Pages()[0], std::out_of_range);
    EXPECT_THROW(doc.Pages()[99], std::out_of_range);
}
