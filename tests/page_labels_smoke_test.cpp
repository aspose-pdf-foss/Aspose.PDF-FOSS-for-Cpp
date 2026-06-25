// =============================================================================
// page_labels_smoke_test — beat N-5 of the navigation cluster. PageLabel,
// PageLabelCollection and the NumberingStyle enum, plus Document::PageLabels().
// Covers in-session range semantics (GetLabel range lookup / UpdateLabel /
// RemoveLabel / GetPages) and the /PageLabels number-tree round-trip.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/numbering_style.hpp>
#include <aspose/pdf/page_label.hpp>
#include <aspose/pdf/page_label_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

}  // namespace

TEST(PageLabelSmoke, EnumAndDefaults) {
    EXPECT_EQ(static_cast<int>(NumberingStyle::NumeralsArabic), 0);
    EXPECT_EQ(static_cast<int>(NumberingStyle::NumeralsRomanLowercase), 2);
    EXPECT_EQ(static_cast<int>(NumberingStyle::None), 5);

    PageLabel d;
    EXPECT_EQ(d.StartingValue(), 1);
    EXPECT_EQ(d.NumberingStyle(), NumberingStyle::NumeralsArabic);
    EXPECT_TRUE(d.Prefix().empty());
}

TEST(PageLabelCollectionSmoke, RangeLookupAndMutation) {
    Document doc{(FixtureRoot() / "two_pages.pdf").string()};
    PageLabelCollection& labels = doc.PageLabels();
    EXPECT_TRUE(labels.GetPages().empty());

    PageLabel roman;
    roman.NumberingStyle(NumberingStyle::NumeralsRomanLowercase);
    roman.Prefix("pre-");
    labels.UpdateLabel(0, roman);

    PageLabel arabic;
    arabic.StartingValue(1);
    labels.UpdateLabel(2, arabic);

    std::vector<int> pages = labels.GetPages();
    ASSERT_EQ(pages.size(), 2u);
    EXPECT_EQ(pages[0], 0);
    EXPECT_EQ(pages[1], 2);

    // GetLabel resolves to the active range (largest start <= index).
    EXPECT_EQ(labels.GetLabel(0).NumberingStyle(),
              NumberingStyle::NumeralsRomanLowercase);
    EXPECT_EQ(labels.GetLabel(1).Prefix(), "pre-");  // still in range [0,2)
    EXPECT_EQ(labels.GetLabel(2).NumberingStyle(),
              NumberingStyle::NumeralsArabic);

    EXPECT_TRUE(labels.RemoveLabel(2));
    EXPECT_FALSE(labels.RemoveLabel(2));
    EXPECT_EQ(labels.GetPages().size(), 1u);
}

TEST(PageLabelCollectionSmoke, SaveAndReopenRoundTrip) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_page_labels.pdf")
            .string();

    {
        Document doc{(FixtureRoot() / "two_pages.pdf").string()};
        PageLabelCollection& labels = doc.PageLabels();

        PageLabel front;
        front.NumberingStyle(NumberingStyle::NumeralsRomanLowercase);
        labels.UpdateLabel(0, front);

        PageLabel body;
        body.NumberingStyle(NumberingStyle::NumeralsArabic);
        body.StartingValue(1);
        body.Prefix("A-");
        labels.UpdateLabel(1, body);

        doc.Save(out);
    }

    {
        Document doc{out};
        PageLabelCollection& labels = doc.PageLabels();
        ASSERT_EQ(labels.GetPages().size(), 2u);

        EXPECT_EQ(labels.GetLabel(0).NumberingStyle(),
                  NumberingStyle::NumeralsRomanLowercase);
        const PageLabel b = labels.GetLabel(1);
        EXPECT_EQ(b.NumberingStyle(), NumberingStyle::NumeralsArabic);
        EXPECT_EQ(b.Prefix(), "A-");
    }

    std::filesystem::remove(out);
}
