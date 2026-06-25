// =============================================================================
// markup_annotation_smoke_test — beat A3 of the annotations
// cluster. Covers MarkupAnnotation + TextMarkupAnnotation
// (via concrete derived stubs) + the 3 cluster enums shipped
// this beat (AnnotationState, AnnotationStateModel, ReplyType).
// =============================================================================

#include <aspose/pdf/annotations/annotation_state.hpp>
#include <aspose/pdf/annotations/annotation_state_model.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/annotations/reply_type.hpp>
#include <aspose/pdf/annotations/text_markup_annotation.hpp>
#include <aspose/pdf/point.hpp>

#include <gtest/gtest.h>

#include <memory>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

class Markup : public MarkupAnnotation {
public:
    using MarkupAnnotation::MarkupAnnotation;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override {
        return AnnotationType::Text;
    }
};

class TextMarkup : public TextMarkupAnnotation {
public:
    using TextMarkupAnnotation::TextMarkupAnnotation;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override {
        return AnnotationType::Highlight;
    }
};

}  // namespace

TEST(AnnotationStateEnum, CanonicalValues) {
    EXPECT_EQ(static_cast<int>(AnnotationState::Marked),    0);
    EXPECT_EQ(static_cast<int>(AnnotationState::Unmarked),  1);
    EXPECT_EQ(static_cast<int>(AnnotationState::Accepted),  2);
    EXPECT_EQ(static_cast<int>(AnnotationState::Rejected),  3);
    EXPECT_EQ(static_cast<int>(AnnotationState::Cancelled), 4);
    EXPECT_EQ(static_cast<int>(AnnotationState::Completed), 5);
    EXPECT_EQ(static_cast<int>(AnnotationState::None),      6);
}

TEST(AnnotationStateModelEnum, CanonicalValues) {
    EXPECT_EQ(static_cast<int>(AnnotationStateModel::Marked), 0);
    EXPECT_EQ(static_cast<int>(AnnotationStateModel::Review), 1);
    EXPECT_EQ(static_cast<int>(AnnotationStateModel::None),   2);
}

TEST(ReplyTypeEnum, CanonicalValues) {
    EXPECT_EQ(static_cast<int>(ReplyType::Reply),     0);
    EXPECT_EQ(static_cast<int>(ReplyType::Group),     1);
    EXPECT_EQ(static_cast<int>(ReplyType::Undefined), 2);
}

TEST(MarkupAnnotationSmoke, DefaultsAndPropertyRoundtrip) {
    Markup m;
    EXPECT_TRUE(m.Title().empty());
    EXPECT_TRUE(m.RichText().empty());
    EXPECT_TRUE(m.Subject().empty());
    EXPECT_DOUBLE_EQ(m.Opacity(), 1.0);
    EXPECT_EQ(m.InReplyTo(), nullptr);
    EXPECT_EQ(m.ReplyType(), ReplyType::Reply);
    EXPECT_EQ(m.GetState(), AnnotationState::None);
    EXPECT_EQ(m.GetStateModel(), AnnotationStateModel::None);

    m.Title("Alice");
    m.RichText("<b>hi</b>");
    m.Subject("Notes");
    m.Opacity(0.5);
    m.ReplyType(ReplyType::Group);
    EXPECT_EQ(m.Title(), "Alice");
    EXPECT_EQ(m.RichText(), "<b>hi</b>");
    EXPECT_EQ(m.Subject(), "Notes");
    EXPECT_DOUBLE_EQ(m.Opacity(), 0.5);
    EXPECT_EQ(m.ReplyType(), ReplyType::Group);
}

TEST(MarkupAnnotationSmoke, SetReviewStateUpdatesStateAndModel) {
    Markup m;
    m.SetReviewState(AnnotationState::Accepted, "Bob");
    EXPECT_EQ(m.GetState(), AnnotationState::Accepted);
    EXPECT_EQ(m.GetStateModel(), AnnotationStateModel::Review);
    EXPECT_EQ(m.Title(), "Bob");

    m.ClearState();
    EXPECT_EQ(m.GetState(), AnnotationState::None);
    EXPECT_EQ(m.GetStateModel(), AnnotationStateModel::None);
}

TEST(MarkupAnnotationSmoke, SetMarkedStateUpdatesMarkedModel) {
    Markup m;
    m.SetMarkedState(true);
    EXPECT_EQ(m.GetState(), AnnotationState::Marked);
    EXPECT_EQ(m.GetStateModel(), AnnotationStateModel::Marked);

    m.SetMarkedState(false);
    EXPECT_EQ(m.GetState(), AnnotationState::Unmarked);
    EXPECT_EQ(m.GetStateModel(), AnnotationStateModel::Marked);
}

TEST(MarkupAnnotationSmoke, InReplyToRoundtrip) {
    Markup m;
    auto reply = std::make_shared<Markup>();
    m.InReplyTo(reply);
    EXPECT_EQ(m.InReplyTo(), reply);
}

TEST(TextMarkupAnnotationSmoke, QuadPointsAndStubText) {
    TextMarkup t;
    EXPECT_TRUE(t.QuadPoints().empty());
    EXPECT_TRUE(t.GetMarkedText().empty());

    std::vector<Point> pts{
        Point{0, 0}, Point{10, 0}, Point{10, 10}, Point{0, 10}};
    t.QuadPoints(pts);
    EXPECT_EQ(t.QuadPoints().size(), 4u);
    EXPECT_DOUBLE_EQ(t.QuadPoints()[2].X(), 10.0);

    EXPECT_EQ(t.AnnotationType(), AnnotationType::Highlight);
}
