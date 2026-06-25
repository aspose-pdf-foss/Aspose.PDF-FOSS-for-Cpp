// =============================================================================
// named_destinations_persistence_smoke_test — beat N-3b of the navigation
// cluster. Wires NamedDestinationCollection through Document::Save (writing the
// Catalog /Names /Dests name tree) and load-on-open (parsing it back into typed
// ExplicitDestination subtypes). Proves the full round-trip: add → save →
// reopen → read.
// =============================================================================

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/fit_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_h_explicit_destination.hpp>
#include <aspose/pdf/annotations/xyz_explicit_destination.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/named_destination_collection.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

}  // namespace

TEST(NamedDestinationsPersistenceSmoke, SaveAndReopenRoundTrip) {
    const std::string src = (FixtureRoot() / "two_pages.pdf").string();
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_named_dests.pdf")
            .string();

    // 1. Open a 2-page doc and register three named destinations spanning
    //    different fit verbs and both pages.
    {
        Document doc{src};
        ASSERT_EQ(doc.Pages().Count(), 2);
        NamedDestinationCollection& dests = doc.NamedDestinations();
        dests.Add("Intro", XYZExplicitDestination{1, 100.0, 700.0, 2.0});
        dests.Add("Middle", FitHExplicitDestination{1, 540.0});
        dests.Add("Outro", FitExplicitDestination{2});
        ASSERT_EQ(dests.Count(), 3);
        doc.Save(out);
    }

    // 2. Reopen the saved file — the named destinations load back from the
    //    /Names /Dests name tree, reconstructed into typed destinations.
    {
        Document doc{out};
        NamedDestinationCollection& dests = doc.NamedDestinations();
        EXPECT_EQ(dests.Count(), 3);

        std::vector<std::string> names = dests.Names();
        ASSERT_EQ(names.size(), 3u);
        // Insertion order in the name tree is preserved.
        EXPECT_EQ(names[0], "Intro");
        EXPECT_EQ(names[1], "Middle");
        EXPECT_EQ(names[2], "Outro");

        ASSERT_NE(dests["Intro"], nullptr);
        EXPECT_EQ(dests["Intro"]->ToString(), "1 XYZ 100 700 2");
        EXPECT_EQ(dests["Middle"]->ToString(), "1 FitH 540");
        EXPECT_EQ(dests["Outro"]->ToString(), "2 Fit");
        EXPECT_EQ(dests["absent"], nullptr);
    }

    std::filesystem::remove(out);
}
