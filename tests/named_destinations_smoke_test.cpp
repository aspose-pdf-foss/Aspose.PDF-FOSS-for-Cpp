// =============================================================================
// named_destinations_smoke_test — beat N-3a of the navigation cluster.
// NamedDestination (a name-keyed IAppointment) + NamedDestinationCollection
// (the document's /Names /Dests map) + Document::NamedDestinations(). Covers
// in-session add / replace / remove / query semantics and that the collection
// owns polymorphic copies of the destinations it stores.
// =============================================================================

#include <aspose/pdf/annotations/fit_explicit_destination.hpp>
#include <aspose/pdf/annotations/named_destination.hpp>
#include <aspose/pdf/annotations/xyz_explicit_destination.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/named_destination_collection.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <gtest/gtest.h>

#include <vector>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

TEST(NamedDestinationSmoke, NameAndToString) {
    Document doc;
    NamedDestination nd{doc, "Chapter1"};
    EXPECT_EQ(nd.Name(), "Chapter1");
    EXPECT_EQ(nd.ToString(), "Chapter1");
}

TEST(NamedDestinationCollectionSmoke, AddQueryNamesCount) {
    Document doc;
    NamedDestinationCollection& dests = doc.NamedDestinations();
    EXPECT_EQ(dests.Count(), 0);
    EXPECT_EQ(dests["missing"], nullptr);

    dests.Add("Intro", XYZExplicitDestination{1, 0.0, 800.0, 1.0});
    dests.Add("Body", FitExplicitDestination{2});
    EXPECT_EQ(dests.Count(), 2);

    // Names preserve insertion order.
    std::vector<std::string> names = dests.Names();
    ASSERT_EQ(names.size(), 2u);
    EXPECT_EQ(names[0], "Intro");
    EXPECT_EQ(names[1], "Body");

    // Stored as owned polymorphic copies — ToString reflects the subtype.
    IAppointment* intro = dests["Intro"];
    ASSERT_NE(intro, nullptr);
    EXPECT_EQ(intro->ToString(), "1 XYZ 0 800 1");
    EXPECT_EQ(dests["Body"]->ToString(), "2 Fit");

    // The accessor returns the same live collection.
    EXPECT_EQ(doc.NamedDestinations().Count(), 2);
}

TEST(NamedDestinationCollectionSmoke, ReplaceSameNameAndRemove) {
    Document doc;
    NamedDestinationCollection& dests = doc.NamedDestinations();

    dests.Add("Spot", XYZExplicitDestination{1, 0.0, 0.0, 1.0});
    dests.Add("Spot", XYZExplicitDestination{3, 10.0, 20.0, 2.0});  // replace
    EXPECT_EQ(dests.Count(), 1);
    EXPECT_EQ(dests["Spot"]->ToString(), "3 XYZ 10 20 2");

    dests.Remove("Spot");
    EXPECT_EQ(dests.Count(), 0);
    EXPECT_EQ(dests["Spot"], nullptr);
    dests.Remove("Spot");  // no-op on absent name
}

TEST(NamedDestinationCollectionSmoke, StoredCopyOutlivesSourceObject) {
    Document doc;
    NamedDestinationCollection& dests = doc.NamedDestinations();
    {
        XYZExplicitDestination tmp{7, 1.0, 2.0, 3.0};
        dests.Add("Kept", tmp);
    }  // tmp destroyed — the collection holds its own copy
    ASSERT_NE(dests["Kept"], nullptr);
    EXPECT_EQ(dests["Kept"]->ToString(), "7 XYZ 1 2 3");
}

}  // namespace
