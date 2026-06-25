// =============================================================================
// base_paragraph_smoke_test — beat A1 of the annotations cluster.
// Covers BaseParagraph (via a minimal derived stub), MarginInfo,
// Hyperlink, and the VerticalAlignment + HorizontalAlignment
// enums.
// =============================================================================

#include <aspose/pdf/base_paragraph.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/hyperlink.hpp>
#include <aspose/pdf/margin_info.hpp>
#include <aspose/pdf/vertical_alignment.hpp>

#include <gtest/gtest.h>

#include <memory>

namespace {

// BaseParagraph is abstract — derive a concrete shell so the
// smoke test can exercise the inherited accessor surface.
class Para : public Aspose::Pdf::BaseParagraph {
public:
    using Aspose::Pdf::BaseParagraph::BaseParagraph;
};

constexpr double kEps = 1e-9;

}  // namespace

TEST(MarginInfoSmoke, DefaultConstructedIsZero) {
    Aspose::Pdf::MarginInfo m;
    EXPECT_NEAR(m.Left(),   0.0, kEps);
    EXPECT_NEAR(m.Right(),  0.0, kEps);
    EXPECT_NEAR(m.Top(),    0.0, kEps);
    EXPECT_NEAR(m.Bottom(), 0.0, kEps);
}

TEST(MarginInfoSmoke, ParamCtorSetsAllFour) {
    // Canonical ctor parameter order is (left, bottom, right, top).
    Aspose::Pdf::MarginInfo m{1.0, 2.0, 3.0, 4.0};
    EXPECT_NEAR(m.Left(),   1.0, kEps);
    EXPECT_NEAR(m.Bottom(), 2.0, kEps);
    EXPECT_NEAR(m.Right(),  3.0, kEps);
    EXPECT_NEAR(m.Top(),    4.0, kEps);
}

TEST(MarginInfoSmoke, AccessorRoundtrip) {
    Aspose::Pdf::MarginInfo m;
    m.Left(10.0);
    m.Right(20.0);
    m.Top(30.0);
    m.Bottom(40.0);
    EXPECT_NEAR(m.Left(),   10.0, kEps);
    EXPECT_NEAR(m.Right(),  20.0, kEps);
    EXPECT_NEAR(m.Top(),    30.0, kEps);
    EXPECT_NEAR(m.Bottom(), 40.0, kEps);
}

TEST(VerticalAlignmentEnum, CanonicalValues) {
    using Aspose::Pdf::VerticalAlignment;
    EXPECT_EQ(static_cast<int>(VerticalAlignment::None),   0);
    EXPECT_EQ(static_cast<int>(VerticalAlignment::Top),    1);
    EXPECT_EQ(static_cast<int>(VerticalAlignment::Center), 2);
    EXPECT_EQ(static_cast<int>(VerticalAlignment::Bottom), 3);
}

TEST(HorizontalAlignmentEnum, CanonicalValues) {
    using Aspose::Pdf::HorizontalAlignment;
    EXPECT_EQ(static_cast<int>(HorizontalAlignment::None),        0);
    EXPECT_EQ(static_cast<int>(HorizontalAlignment::Left),        1);
    EXPECT_EQ(static_cast<int>(HorizontalAlignment::Center),      2);
    EXPECT_EQ(static_cast<int>(HorizontalAlignment::Right),       3);
    EXPECT_EQ(static_cast<int>(HorizontalAlignment::Justify),     4);
    EXPECT_EQ(static_cast<int>(HorizontalAlignment::FullJustify), 5);
}

TEST(HyperlinkSmoke, DefaultConstructible) {
    Aspose::Pdf::Hyperlink h;
    (void)h;
    SUCCEED();
}

TEST(BaseParagraphSmoke, DefaultAccessors) {
    Para p;
    EXPECT_EQ(p.VerticalAlignment(),
              Aspose::Pdf::VerticalAlignment::None);
    EXPECT_EQ(p.HorizontalAlignment(),
              Aspose::Pdf::HorizontalAlignment::None);
    EXPECT_FALSE(p.IsFirstParagraphInColumn());
    EXPECT_FALSE(p.IsKeptWithNext());
    EXPECT_FALSE(p.IsInNewPage());
    EXPECT_FALSE(p.IsInLineParagraph());
    EXPECT_EQ(p.ZIndex(), 0);
    EXPECT_EQ(p.Hyperlink(), nullptr);
    EXPECT_NEAR(p.Margin().Left(), 0.0, kEps);
}

TEST(BaseParagraphSmoke, AccessorRoundtrip) {
    Para p;
    p.VerticalAlignment(Aspose::Pdf::VerticalAlignment::Center);
    p.HorizontalAlignment(Aspose::Pdf::HorizontalAlignment::Right);
    p.IsFirstParagraphInColumn(true);
    p.IsKeptWithNext(true);
    p.IsInNewPage(true);
    p.IsInLineParagraph(true);
    p.ZIndex(7);
    p.Margin(Aspose::Pdf::MarginInfo{1.0, 2.0, 3.0, 4.0});

    EXPECT_EQ(p.VerticalAlignment(),
              Aspose::Pdf::VerticalAlignment::Center);
    EXPECT_EQ(p.HorizontalAlignment(),
              Aspose::Pdf::HorizontalAlignment::Right);
    EXPECT_TRUE(p.IsFirstParagraphInColumn());
    EXPECT_TRUE(p.IsKeptWithNext());
    EXPECT_TRUE(p.IsInNewPage());
    EXPECT_TRUE(p.IsInLineParagraph());
    EXPECT_EQ(p.ZIndex(), 7);
    EXPECT_NEAR(p.Margin().Left(),   1.0, kEps);
    EXPECT_NEAR(p.Margin().Bottom(), 2.0, kEps);
    EXPECT_NEAR(p.Margin().Right(),  3.0, kEps);
    EXPECT_NEAR(p.Margin().Top(),    4.0, kEps);
}

TEST(BaseParagraphSmoke, HyperlinkRoundtrip) {
    Para p;
    auto h = std::make_shared<Aspose::Pdf::Hyperlink>();
    p.Hyperlink(h);
    EXPECT_EQ(p.Hyperlink(), h);
}
