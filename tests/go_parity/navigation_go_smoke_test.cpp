// =============================================================================
// Go-parity: named_destinations_test.go / page_labels_test.go → canonical C++
// Document::NamedDestinations (NamedDestinationCollection) and
// Document::PageLabels (PageLabelCollection / PageLabel / NumberingStyle).
//
// Ported (canonical): named-dest empty/add/get/remove/names + round-trip,
// page-label defaults + range update + page list.
// Skipped: Go-specific add-validation errors (nil/empty-name/wrong-value),
// writer-internal and pypdf/aspose byte-parity tests.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/annotations/xyz_explicit_destination.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/named_destination_collection.hpp>
#include <aspose/pdf/numbering_style.hpp>
#include <aspose/pdf/page_label.hpp>
#include <aspose/pdf/page_label_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Annotations::XYZExplicitDestination;

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
    return (std::filesystem::temp_directory_path() / ("go_nav_" + tag + ".pdf"))
        .string();
}

}  // namespace

// TestNamedDestinations_EmptyDoc
TEST(GoNavigation, NamedDestEmpty) {
    Document doc{TwoPages()};
    EXPECT_EQ(doc.NamedDestinations().Count(), 0);
}

// TestNamedDestinations_AddGet / TestNamedDestinations_Remove / NamesSorted
TEST(GoNavigation, NamedDestAddRemove) {
    Document doc{TwoPages()};
    NamedDestinationCollection& dests = doc.NamedDestinations();
    dests.Add("Intro", XYZExplicitDestination{1, 100.0, 700.0, 2.0});
    dests.Add("Outro", XYZExplicitDestination{2, 0.0, 0.0, 1.0});
    EXPECT_EQ(dests.Count(), 2);
    ASSERT_NE(dests["Intro"], nullptr);
    EXPECT_EQ(dests.Names().size(), 2u);

    dests.Remove("Intro");
    EXPECT_EQ(dests.Count(), 1);
}

// TestNamedDestinations_RoundTrip_SingleEntry
TEST(GoNavigation, NamedDestRoundTrip) {
    const std::string out = TmpOut("nd");
    {
        Document doc{TwoPages()};
        doc.NamedDestinations().Add(
            "Chapter1", XYZExplicitDestination{1, 50.0, 600.0, 1.0});
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(doc.NamedDestinations().Count(), 1);
    EXPECT_NE(doc.NamedDestinations()["Chapter1"], nullptr);
    std::filesystem::remove(out);
}

// TestPageLabelDefaultDecimal — a default PageLabel is arabic with no prefix.
TEST(GoNavigation, PageLabelDefault) {
    PageLabel d;
    EXPECT_EQ(d.NumberingStyle(), NumberingStyle::NumeralsArabic);
    EXPECT_TRUE(d.Prefix().empty());
}

// TestSetPageLabels_RomanThenDecimal / PrefixAndCustomStart
TEST(GoNavigation, PageLabelRanges) {
    Document doc{TwoPages()};
    PageLabelCollection& labels = doc.PageLabels();

    PageLabel roman;
    roman.NumberingStyle(NumberingStyle::NumeralsRomanLowercase);
    roman.Prefix("pre-");
    labels.UpdateLabel(0, roman);

    PageLabel arabic;
    arabic.StartingValue(1);
    labels.UpdateLabel(1, arabic);

    const std::vector<int> pages = labels.GetPages();
    ASSERT_EQ(pages.size(), 2u);
    EXPECT_EQ(labels.GetLabel(0).NumberingStyle(),
              NumberingStyle::NumeralsRomanLowercase);
    EXPECT_EQ(labels.GetLabel(0).Prefix(), "pre-");
}
