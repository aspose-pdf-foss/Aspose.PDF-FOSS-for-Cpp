// =============================================================================
// stamp_caret_ink_family_smoke_test — beat A10 of the annotations
// cluster. StampAnnotation + CaretAnnotation + InkAnnotation all
// extend MarkupAnnotation directly. Exercises visitor dispatch +
// AnnotationType reporting + per-class properties.
// =============================================================================

#include <vector>

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/cap_style.hpp>
#include <aspose/pdf/annotations/caret_annotation.hpp>
#include <aspose/pdf/annotations/caret_symbol.hpp>
#include <aspose/pdf/annotations/ink_annotation.hpp>
#include <aspose/pdf/annotations/stamp_annotation.hpp>
#include <aspose/pdf/annotations/stamp_icon.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

class CountingSelector : public AnnotationSelector {
public:
    int stamp = 0, caret = 0, ink = 0;
    void Visit(StampAnnotation& s) override { ++stamp; (void)s; }
    void Visit(CaretAnnotation& c) override { ++caret; (void)c; }
    void Visit(InkAnnotation& i)   override { ++ink;   (void)i; }
};

}  // namespace

TEST(StampCaretInkFamilySmoke, EnumValuesPinned) {
    EXPECT_EQ(static_cast<int>(AnnotationType::Caret), 11);
    EXPECT_EQ(static_cast<int>(AnnotationType::Ink),   12);
    EXPECT_EQ(static_cast<int>(AnnotationType::Stamp), 24);

    EXPECT_EQ(static_cast<int>(StampIcon::Draft),            0);
    EXPECT_EQ(static_cast<int>(StampIcon::Approved),         1);
    EXPECT_EQ(static_cast<int>(StampIcon::ForPublicRelease), 12);
    EXPECT_EQ(static_cast<int>(StampIcon::TopSecret),        13);

    EXPECT_EQ(static_cast<int>(CaretSymbol::None),      0);
    EXPECT_EQ(static_cast<int>(CaretSymbol::Paragraph), 1);

    EXPECT_EQ(static_cast<int>(CapStyle::Butt),    0);
    EXPECT_EQ(static_cast<int>(CapStyle::Rounded), 1);
}

TEST(StampCaretInkFamilySmoke, StampConstructAndVisit) {
    Document doc;
    StampAnnotation s{doc};
    EXPECT_EQ(s.AnnotationType(), AnnotationType::Stamp);
    EXPECT_EQ(s.Icon(), StampIcon::Draft);

    s.Icon(StampIcon::Approved);
    EXPECT_EQ(s.Icon(), StampIcon::Approved);

    CountingSelector sel;
    s.Accept(sel);
    EXPECT_EQ(sel.stamp, 1);
    EXPECT_EQ(sel.caret, 0);
    EXPECT_EQ(sel.ink,   0);
}

TEST(StampCaretInkFamilySmoke, CaretConstructAndVisit) {
    Document doc;
    CaretAnnotation c{doc};
    EXPECT_EQ(c.AnnotationType(), AnnotationType::Caret);
    EXPECT_EQ(c.Symbol(), CaretSymbol::None);

    c.Symbol(CaretSymbol::Paragraph);
    EXPECT_EQ(c.Symbol(), CaretSymbol::Paragraph);

    Rectangle frame{1.0, 2.0, 10.0, 20.0, false};
    c.Frame(frame);
    EXPECT_NEAR(c.Frame().LLX(), 1.0,  1e-9);
    EXPECT_NEAR(c.Frame().URY(), 20.0, 1e-9);

    CountingSelector sel;
    c.Accept(sel);
    EXPECT_EQ(sel.caret, 1);
    EXPECT_EQ(sel.stamp, 0);
    EXPECT_EQ(sel.ink,   0);
}

TEST(StampCaretInkFamilySmoke, InkConstructAndVisit) {
    Document doc;
    InkAnnotation::StrokeList strokes{
        InkAnnotation::Stroke{Point{0, 0}, Point{1, 1}, Point{2, 0}},
        InkAnnotation::Stroke{Point{10, 10}, Point{11, 12}},
    };
    InkAnnotation k{doc, strokes};

    EXPECT_EQ(k.AnnotationType(), AnnotationType::Ink);
    EXPECT_EQ(k.CapStyle(), CapStyle::Butt);

    ASSERT_EQ(k.InkList().size(), 2u);
    EXPECT_EQ(k.InkList()[0].size(), 3u);
    EXPECT_EQ(k.InkList()[1].size(), 2u);
    EXPECT_NEAR(k.InkList()[1][0].X(), 10.0, 1e-9);

    k.CapStyle(CapStyle::Rounded);
    EXPECT_EQ(k.CapStyle(), CapStyle::Rounded);

    CountingSelector sel;
    k.Accept(sel);
    EXPECT_EQ(sel.ink,   1);
    EXPECT_EQ(sel.stamp, 0);
    EXPECT_EQ(sel.caret, 0);
}

TEST(StampCaretInkFamilySmoke, InkListRoundtrip) {
    Document doc;
    InkAnnotation::StrokeList initial{
        InkAnnotation::Stroke{Point{0, 0}, Point{1, 1}},
    };
    InkAnnotation k{doc, initial};

    InkAnnotation::StrokeList updated{
        InkAnnotation::Stroke{Point{5, 5}, Point{6, 7}},
        InkAnnotation::Stroke{Point{8, 4}, Point{9, 2}, Point{10, 0}},
    };
    k.InkList(updated);
    ASSERT_EQ(k.InkList().size(), 2u);
    EXPECT_EQ(k.InkList()[1].size(), 3u);
    EXPECT_NEAR(k.InkList()[1][2].X(), 10.0, 1e-9);
}
