// =============================================================================
// explicit_destination_subtypes_smoke_test — beat N-1 of the navigation
// cluster. The eight typed ExplicitDestination subclasses (XYZ / Fit / FitH /
// FitV / FitR / FitB / FitBH / FitBV) plus the ExplicitDestinationType enum.
// Exercises construction (page / page-number ctors), the mode-specific
// coordinate accessors, polymorphic PageNumber()/ToString(), and the
// CloneAppointment() path through GoToAction::Destination.
// =============================================================================

#include <aspose/pdf/annotations/explicit_destination.hpp>
#include <aspose/pdf/annotations/explicit_destination_type.hpp>
#include <aspose/pdf/annotations/fit_b_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_bh_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_bv_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_h_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_r_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_v_explicit_destination.hpp>
#include <aspose/pdf/annotations/go_to_action.hpp>
#include <aspose/pdf/annotations/xyz_explicit_destination.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

TEST(ExplicitDestinationTypeSmoke, CanonicalValues) {
    EXPECT_EQ(static_cast<int>(ExplicitDestinationType::XYZ), 0);
    EXPECT_EQ(static_cast<int>(ExplicitDestinationType::Fit), 1);
    EXPECT_EQ(static_cast<int>(ExplicitDestinationType::FitH), 2);
    EXPECT_EQ(static_cast<int>(ExplicitDestinationType::FitV), 3);
    EXPECT_EQ(static_cast<int>(ExplicitDestinationType::FitR), 4);
    EXPECT_EQ(static_cast<int>(ExplicitDestinationType::FitB), 5);
    EXPECT_EQ(static_cast<int>(ExplicitDestinationType::FitBH), 6);
    EXPECT_EQ(static_cast<int>(ExplicitDestinationType::FitBV), 7);
}

TEST(ExplicitDestinationSubtypesSmoke, XyzAccessorsAndToString) {
    XYZExplicitDestination d{3, 100.0, 700.0, 2.0};
    EXPECT_EQ(d.PageNumber(), 3);
    EXPECT_DOUBLE_EQ(d.Left(), 100.0);
    EXPECT_DOUBLE_EQ(d.Top(), 700.0);
    EXPECT_DOUBLE_EQ(d.Zoom(), 2.0);
    EXPECT_EQ(d.ToString(), "3 XYZ 100 700 2");
}

TEST(ExplicitDestinationSubtypesSmoke, FitFamilyToString) {
    EXPECT_EQ(FitExplicitDestination{2}.ToString(), "2 Fit");
    EXPECT_EQ((FitHExplicitDestination{2, 500.0}).ToString(), "2 FitH 500");
    EXPECT_EQ((FitVExplicitDestination{2, 50.0}).ToString(), "2 FitV 50");
    EXPECT_EQ((FitRExplicitDestination{2, 10.0, 20.0, 30.0, 40.0}).ToString(),
              "2 FitR 10 20 30 40");
    EXPECT_EQ(FitBExplicitDestination{2}.ToString(), "2 FitB");
    EXPECT_EQ((FitBHExplicitDestination{2, 500.0}).ToString(), "2 FitBH 500");
    EXPECT_EQ((FitBVExplicitDestination{2, 50.0}).ToString(), "2 FitBV 50");
}

TEST(ExplicitDestinationSubtypesSmoke, FitRAccessors) {
    FitRExplicitDestination d{1, 10.0, 20.0, 30.0, 40.0};
    EXPECT_DOUBLE_EQ(d.Left(), 10.0);
    EXPECT_DOUBLE_EQ(d.Bottom(), 20.0);
    EXPECT_DOUBLE_EQ(d.Right(), 30.0);
    EXPECT_DOUBLE_EQ(d.Top(), 40.0);
}

TEST(ExplicitDestinationSubtypesSmoke, PageConstructorUsesPageNumber) {
    Document doc;
    doc.Pages().Add();
    Page second = doc.Pages().Add();
    ASSERT_EQ(second.Number(), 2);

    XYZExplicitDestination d{second, 0.0, 0.0, 1.0};
    EXPECT_EQ(d.PageNumber(), 2);
}

TEST(ExplicitDestinationSubtypesSmoke, PolymorphicBasePointer) {
    XYZExplicitDestination xyz{5, 1.0, 2.0, 3.0};
    ExplicitDestination& base = xyz;
    EXPECT_EQ(base.PageNumber(), 5);
    EXPECT_EQ(base.ToString(), "5 XYZ 1 2 3");
}

TEST(ExplicitDestinationSubtypesSmoke, ClonesThroughGoToActionDestination) {
    // GoToAction copies the destination via CloneAppointment(); the clone must
    // preserve the derived subtype (its ToString verb), not slice to the base.
    FitHExplicitDestination dest{4, 612.0};
    GoToAction action{dest};

    IAppointment* stored = action.Destination();
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->ToString(), "4 FitH 612");
}

TEST(GoToActionTypedDestinationSmoke, BuildsTypedDestinationFromVerb) {
    // N-2: GoToAction(Page, ExplicitDestinationType, double[]) builds the
    // matching typed destination from the fit-mode verb + coordinate operands.
    Document doc;
    Page page = doc.Pages().Add();  // page 1
    ASSERT_EQ(page.Number(), 1);

    GoToAction xyz{page, ExplicitDestinationType::XYZ, {100.0, 700.0, 2.0}};
    ASSERT_NE(xyz.Destination(), nullptr);
    EXPECT_EQ(xyz.Destination()->ToString(), "1 XYZ 100 700 2");

    GoToAction fitr{page, ExplicitDestinationType::FitR,
                    {10.0, 20.0, 30.0, 40.0}};
    EXPECT_EQ(fitr.Destination()->ToString(), "1 FitR 10 20 30 40");

    GoToAction fit{page, ExplicitDestinationType::Fit, {}};
    EXPECT_EQ(fit.Destination()->ToString(), "1 Fit");

    // Short operand list → missing operands default to 0.
    GoToAction fith{page, ExplicitDestinationType::FitH, {}};
    EXPECT_EQ(fith.Destination()->ToString(), "1 FitH 0");
}

}  // namespace
