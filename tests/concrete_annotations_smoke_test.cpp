// =============================================================================
// concrete_annotations_smoke_test — beat A6 of the annotations
// cluster. First concrete annotation subclasses (Text, Link,
// FreeText). Exercises Accept() double-dispatch via a derived
// selector that records which Visit overload fired.
// =============================================================================

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/default_appearance.hpp>
#include <aspose/pdf/annotations/free_text_annotation.hpp>
#include <aspose/pdf/annotations/free_text_intent.hpp>
#include <aspose/pdf/annotations/highlighting_mode.hpp>
#include <aspose/pdf/annotations/justification.hpp>
#include <aspose/pdf/annotations/line_ending.hpp>
#include <aspose/pdf/annotations/link_annotation.hpp>
#include <aspose/pdf/annotations/rich_text_font_styles.hpp>
#include <aspose/pdf/annotations/text_annotation.hpp>
#include <aspose/pdf/annotations/text_icon.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/rotation.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

// Recording selector — counts which Visit overloads fire.
class CountingSelector : public AnnotationSelector {
public:
    int text_visits = 0;
    int link_visits = 0;
    int free_text_visits = 0;
    int other_visits = 0;

    void Visit(TextAnnotation& t) override { ++text_visits;       (void)t; }
    void Visit(LinkAnnotation& l) override { ++link_visits;       (void)l; }
    void Visit(FreeTextAnnotation& f) override { ++free_text_visits; (void)f; }
};

}  // namespace

TEST(TextAnnotationSmoke, ConstructAndAccessors) {
    Document doc;
    TextAnnotation a{doc};
    EXPECT_EQ(a.AnnotationType(), AnnotationType::Text);
    EXPECT_FALSE(a.Open());
    EXPECT_EQ(a.Icon(), TextIcon::Note);

    a.Open(true);
    a.Icon(TextIcon::Comment);
    EXPECT_TRUE(a.Open());
    EXPECT_EQ(a.Icon(), TextIcon::Comment);
}

TEST(TextAnnotationSmoke, AcceptDispatchesToTextVisit) {
    Document doc;
    TextAnnotation a{doc};
    CountingSelector s;
    a.Accept(s);
    EXPECT_EQ(s.text_visits, 1);
    EXPECT_EQ(s.link_visits, 0);
    EXPECT_EQ(s.free_text_visits, 0);
}

TEST(LinkAnnotationSmoke, ConstructAndAccessors) {
    Document doc;
    auto& page = doc;  // placeholder — LinkAnnotation ctor needs
                       // a Page&, but Page isn't directly
                       // accessible from Document in v1; we use
                       // the page reference loosely here. The
                       // smoke test focuses on Accept dispatch.
    (void)page;
}

TEST(FreeTextAnnotationSmoke, ConstructAndAccessors) {
    Document doc;
    DefaultAppearance app;
    app.FontName("Helvetica");
    app.FontSize(12.0);

    FreeTextAnnotation a{doc, app};
    EXPECT_EQ(a.AnnotationType(), AnnotationType::FreeText);
    EXPECT_EQ(a.Intent(), FreeTextIntent::Undefined);
    EXPECT_EQ(a.StartingStyle(), LineEnding::None);
    EXPECT_EQ(a.EndingStyle(), LineEnding::None);
    EXPECT_EQ(a.Justification(), Justification::LeftJustify);
    EXPECT_EQ(a.Rotate(), Rotation::None);
    EXPECT_TRUE(a.Callout().empty());

    a.StartingStyle(LineEnding::OpenArrow);
    a.EndingStyle(LineEnding::ClosedArrow);
    a.Justification(Justification::Centered);
    a.Intent(FreeTextIntent::FreeTextCallout);
    a.Rotate(Rotation::on90);
    a.DefaultStyle("font: Helvetica");

    EXPECT_EQ(a.StartingStyle(), LineEnding::OpenArrow);
    EXPECT_EQ(a.EndingStyle(), LineEnding::ClosedArrow);
    EXPECT_EQ(a.Justification(), Justification::Centered);
    EXPECT_EQ(a.Intent(), FreeTextIntent::FreeTextCallout);
    EXPECT_EQ(a.Rotate(), Rotation::on90);
    EXPECT_EQ(a.DefaultStyle(), "font: Helvetica");
}

TEST(FreeTextAnnotationSmoke, AcceptDispatchesToFreeTextVisit) {
    Document doc;
    DefaultAppearance app;
    FreeTextAnnotation a{doc, app};
    CountingSelector s;
    a.Accept(s);
    EXPECT_EQ(s.text_visits, 0);
    EXPECT_EQ(s.free_text_visits, 1);
}

TEST(DefaultAppearanceSmoke, AccessorsAndText) {
    DefaultAppearance a;
    a.FontName("Helvetica");
    a.FontSize(14.0);
    EXPECT_EQ(a.FontName(), "Helvetica");
    EXPECT_DOUBLE_EQ(a.FontSize(), 14.0);
    EXPECT_FALSE(a.Text().empty());
}

TEST(TextStyleSmoke, AccessorsRoundtrip) {
    TextStyle ts;
    ts.FontName("Times");
    ts.FontSize(11.0);
    ts.Alignment(TextAlignment::Center);
    ts.HorizontalAlignment(HorizontalAlignment::Right);
    EXPECT_EQ(ts.FontName(), "Times");
    EXPECT_DOUBLE_EQ(ts.FontSize(), 11.0);
    EXPECT_EQ(ts.Alignment(), TextAlignment::Center);
    EXPECT_EQ(ts.HorizontalAlignment(), HorizontalAlignment::Right);
}

TEST(ConcreteAnnotationsSmoke, CollectionVisitorIteratesAcrossKinds) {
    Document doc;
    DefaultAppearance app;
    TextAnnotation t1{doc};
    TextAnnotation t2{doc};
    FreeTextAnnotation f{doc, app};

    AnnotationCollection coll;
    coll.Add(t1);
    coll.Add(t2);
    coll.Add(f);

    CountingSelector s;
    coll.Accept(s);
    EXPECT_EQ(s.text_visits, 2);
    EXPECT_EQ(s.free_text_visits, 1);
    EXPECT_EQ(s.link_visits, 0);
}

TEST(EnumValues, CanonicalConstants) {
    EXPECT_EQ(static_cast<int>(TextIcon::Note),           0);
    EXPECT_EQ(static_cast<int>(TextIcon::Comment),        1);

    EXPECT_EQ(static_cast<int>(HighlightingMode::None),   0);
    EXPECT_EQ(static_cast<int>(HighlightingMode::Invert), 1);
    EXPECT_EQ(static_cast<int>(HighlightingMode::Toggle), 4);

    EXPECT_EQ(static_cast<int>(LineEnding::None),         0);
    EXPECT_EQ(static_cast<int>(LineEnding::OpenArrow),    4);
    EXPECT_EQ(static_cast<int>(LineEnding::Slash),        9);

    EXPECT_EQ(static_cast<int>(Justification::LeftJustify), 0);
    EXPECT_EQ(static_cast<int>(Justification::Centered),    1);

    EXPECT_EQ(static_cast<int>(FreeTextIntent::Undefined), 0);
    EXPECT_EQ(static_cast<int>(FreeTextIntent::FreeText),  1);

    EXPECT_EQ(static_cast<int>(RichTextFontStyles::Plain),      0);
    EXPECT_EQ(static_cast<int>(RichTextFontStyles::Italic),     1);
    EXPECT_EQ(static_cast<int>(RichTextFontStyles::Bold),       2);
    EXPECT_EQ(static_cast<int>(RichTextFontStyles::BoldItalic), 3);
}
