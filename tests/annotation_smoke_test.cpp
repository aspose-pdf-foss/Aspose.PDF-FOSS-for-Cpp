// =============================================================================
// annotation_smoke_test — beat A2 of the annotations cluster.
// Covers Annotation (via a concrete derived stub), Border,
// Characteristics, the 3 cluster-leaf enums (AnnotationType,
// AnnotationFlags, TextAlignment), and the 2 Border deps
// (BorderStyle, BorderEffect).
// =============================================================================

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_flags.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/border.hpp>
#include <aspose/pdf/annotations/border_effect.hpp>
#include <aspose/pdf/annotations/border_style.hpp>
#include <aspose/pdf/annotations/characteristics.hpp>
#include <aspose/pdf/annotations/text_alignment.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

class Ann : public Annotation {
public:
    using Annotation::Annotation;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override {
        return AnnotationType::Text;
    }
};

constexpr double kEps = 1e-9;

}  // namespace

TEST(AnnotationTypeEnum, RepresentativeValues) {
    EXPECT_EQ(static_cast<int>(AnnotationType::Text),           0);
    EXPECT_EQ(static_cast<int>(AnnotationType::Link),           13);
    EXPECT_EQ(static_cast<int>(AnnotationType::Highlight),      7);
    EXPECT_EQ(static_cast<int>(AnnotationType::FreeText),       6);
    EXPECT_EQ(static_cast<int>(AnnotationType::FileAttachment), 15);
    EXPECT_EQ(static_cast<int>(AnnotationType::Watermark),      20);
}

TEST(AnnotationFlagsEnum, BitfieldValues) {
    EXPECT_EQ(static_cast<int>(AnnotationFlags::Default),        0);
    EXPECT_EQ(static_cast<int>(AnnotationFlags::Invisible),      1);
    EXPECT_EQ(static_cast<int>(AnnotationFlags::Hidden),         2);
    EXPECT_EQ(static_cast<int>(AnnotationFlags::Print),          4);
    EXPECT_EQ(static_cast<int>(AnnotationFlags::NoZoom),         8);
    EXPECT_EQ(static_cast<int>(AnnotationFlags::ReadOnly),       64);
    EXPECT_EQ(static_cast<int>(AnnotationFlags::LockedContents), 512);
}

TEST(AnnotationFlagsEnum, BitwiseComposition) {
    auto f = AnnotationFlags::Print | AnnotationFlags::ReadOnly;
    EXPECT_EQ(static_cast<int>(f), 4 | 64);
    EXPECT_EQ(static_cast<int>(f & AnnotationFlags::Print),
              static_cast<int>(AnnotationFlags::Print));
    f &= ~AnnotationFlags::Print;
    EXPECT_EQ(static_cast<int>(f),
              static_cast<int>(AnnotationFlags::ReadOnly));
}

TEST(TextAlignmentEnum, CanonicalValues) {
    EXPECT_EQ(static_cast<int>(TextAlignment::Left),   0);
    EXPECT_EQ(static_cast<int>(TextAlignment::Center), 1);
    EXPECT_EQ(static_cast<int>(TextAlignment::Right),  2);
}

TEST(BorderStyleEnum, CanonicalValues) {
    EXPECT_EQ(static_cast<int>(BorderStyle::Solid),     0);
    EXPECT_EQ(static_cast<int>(BorderStyle::Dashed),    1);
    EXPECT_EQ(static_cast<int>(BorderStyle::Beveled),   2);
    EXPECT_EQ(static_cast<int>(BorderStyle::Inset),     3);
    EXPECT_EQ(static_cast<int>(BorderStyle::Underline), 4);
}

TEST(BorderEffectEnum, CanonicalValues) {
    EXPECT_EQ(static_cast<int>(BorderEffect::None),   0);
    EXPECT_EQ(static_cast<int>(BorderEffect::Cloudy), 1);
}

TEST(BorderSmoke, AccessorRoundtrip) {
    Border b{nullptr};
    EXPECT_EQ(b.Width(), 1);
    EXPECT_EQ(b.Style(), BorderStyle::Solid);
    EXPECT_EQ(b.Effect(), BorderEffect::None);

    b.Width(3);
    b.HCornerRadius(5.0);
    b.VCornerRadius(7.0);
    b.EffectIntensity(2);
    b.Style(BorderStyle::Dashed);
    b.Effect(BorderEffect::Cloudy);

    EXPECT_EQ(b.Width(), 3);
    EXPECT_NEAR(b.HCornerRadius(), 5.0, kEps);
    EXPECT_NEAR(b.VCornerRadius(), 7.0, kEps);
    EXPECT_EQ(b.EffectIntensity(), 2);
    EXPECT_EQ(b.Style(), BorderStyle::Dashed);
    EXPECT_EQ(b.Effect(), BorderEffect::Cloudy);
}

TEST(CharacteristicsSmoke, RotateRoundtrip) {
    Characteristics c;
    EXPECT_EQ(c.Rotate(), Aspose::Pdf::Rotation::None);
    c.Rotate(Aspose::Pdf::Rotation::on180);
    EXPECT_EQ(c.Rotate(), Aspose::Pdf::Rotation::on180);
}

TEST(AnnotationSmoke, DefaultAccessors) {
    Ann a;
    EXPECT_EQ(a.AnnotationType(), AnnotationType::Text);
    EXPECT_EQ(a.Flags(), AnnotationFlags::Default);
    EXPECT_FALSE(a.UpdateAppearanceOnConvert());
    EXPECT_FALSE(a.UseFontSubset());
    EXPECT_TRUE(a.Contents().empty());
    EXPECT_TRUE(a.Name().empty());
    EXPECT_TRUE(a.ActiveState().empty());
    EXPECT_TRUE(a.FullName().empty());
    EXPECT_EQ(a.PageIndex(), 0);
    EXPECT_EQ(a.Alignment(), TextAlignment::Left);
    EXPECT_EQ(a.HorizontalAlignment(),
              Aspose::Pdf::HorizontalAlignment::None);
}

TEST(AnnotationSmoke, RectAndDimensionAccessors) {
    Rectangle r{10.0, 20.0, 30.0, 50.0, false};
    Ann a{r};
    EXPECT_NEAR(a.Width(),  20.0, kEps);
    EXPECT_NEAR(a.Height(), 30.0, kEps);
    EXPECT_NEAR(a.Rect().LLX(), 10.0, kEps);

    a.Width(40.0);
    EXPECT_NEAR(a.Rect().URX(), 50.0, kEps);
    a.Height(15.0);
    EXPECT_NEAR(a.Rect().URY(), 35.0, kEps);
}

TEST(AnnotationSmoke, PropertyRoundtrip) {
    Ann a;
    a.Flags(AnnotationFlags::Print | AnnotationFlags::ReadOnly);
    a.UpdateAppearanceOnConvert(true);
    a.UseFontSubset(true);
    a.Contents("hello");
    a.Name("note1");
    a.ActiveState("On");
    a.Color(Color::Red());
    a.Alignment(TextAlignment::Right);
    a.HorizontalAlignment(Aspose::Pdf::HorizontalAlignment::Center);

    EXPECT_EQ(static_cast<int>(a.Flags()),
              static_cast<int>(AnnotationFlags::Print
                  | AnnotationFlags::ReadOnly));
    EXPECT_TRUE(a.UpdateAppearanceOnConvert());
    EXPECT_TRUE(a.UseFontSubset());
    EXPECT_EQ(a.Contents(), "hello");
    EXPECT_EQ(a.Name(), "note1");
    EXPECT_EQ(a.ActiveState(), "On");
    EXPECT_EQ(a.Alignment(), TextAlignment::Right);
    EXPECT_EQ(a.HorizontalAlignment(),
              Aspose::Pdf::HorizontalAlignment::Center);
}

TEST(AnnotationSmoke, BorderAccessIsLValueReference) {
    Ann a;
    a.Border().Width(5);
    EXPECT_EQ(a.Border().Width(), 5);

    Border b{nullptr};
    b.Style(BorderStyle::Inset);
    a.Border(b);
    EXPECT_EQ(a.Border().Style(), BorderStyle::Inset);
}
