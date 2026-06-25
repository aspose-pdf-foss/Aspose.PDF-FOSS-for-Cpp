// =============================================================================
// form_widget_family_smoke_test — beat A12 of the annotations
// cluster. PopupAnnotation + ScreenAnnotation + WidgetAnnotation
// — three concrete annotations that all extend Annotation
// directly (not MarkupAnnotation). Beat also undrops
// MarkupAnnotation.Popup which had been deferred since A3.
// =============================================================================

#include <memory>

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/default_appearance.hpp>
#include <aspose/pdf/annotations/highlighting_mode.hpp>
#include <aspose/pdf/annotations/popup_annotation.hpp>
#include <aspose/pdf/annotations/screen_annotation.hpp>
#include <aspose/pdf/annotations/text_annotation.hpp>
#include <aspose/pdf/annotations/widget_annotation.hpp>
#include <aspose/pdf/document.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

class CountingSelector : public AnnotationSelector {
public:
    int popup = 0, screen = 0, widget = 0;
    void Visit(PopupAnnotation& p)  override { ++popup;  (void)p; }
    void Visit(ScreenAnnotation& s) override { ++screen; (void)s; }
    void Visit(WidgetAnnotation& w) override { ++widget; (void)w; }
};

}  // namespace

TEST(FormWidgetFamilySmoke, EnumValuesPinned) {
    EXPECT_EQ(static_cast<int>(AnnotationType::Popup),  14);
    EXPECT_EQ(static_cast<int>(AnnotationType::Screen), 18);
    EXPECT_EQ(static_cast<int>(AnnotationType::Widget), 19);
}

TEST(FormWidgetFamilySmoke, PopupConstructAndVisit) {
    Document doc;
    PopupAnnotation p{doc};
    EXPECT_EQ(p.AnnotationType(), AnnotationType::Popup);
    EXPECT_FALSE(p.Open());

    p.Open(true);
    EXPECT_TRUE(p.Open());

    CountingSelector sel;
    p.Accept(sel);
    EXPECT_EQ(sel.popup,  1);
    EXPECT_EQ(sel.screen, 0);
    EXPECT_EQ(sel.widget, 0);
}

TEST(FormWidgetFamilySmoke, PopupParentRoundtrip) {
    Document doc;
    PopupAnnotation popup{doc};
    auto host = std::make_shared<TextAnnotation>(doc);

    EXPECT_EQ(popup.Parent().get(), nullptr);
    popup.Parent(host);
    ASSERT_EQ(popup.Parent().get(), host.get());
    EXPECT_EQ(popup.Parent()->AnnotationType(), AnnotationType::Text);
}

TEST(FormWidgetFamilySmoke, MarkupAnnotationPopupWireThrough) {
    Document doc;
    TextAnnotation host{doc};
    EXPECT_EQ(host.Popup().get(), nullptr);

    auto popup = std::make_shared<PopupAnnotation>(doc);
    popup->Open(true);
    host.Popup(popup);

    ASSERT_EQ(host.Popup().get(), popup.get());
    EXPECT_TRUE(host.Popup()->Open());
}

TEST(FormWidgetFamilySmoke, WidgetConstructAndAccessors) {
    Document doc;
    WidgetAnnotation w{doc};
    EXPECT_EQ(w.AnnotationType(), AnnotationType::Widget);
    EXPECT_EQ(w.Highlighting(), HighlightingMode::Invert);
    EXPECT_FALSE(w.ReadOnly());
    EXPECT_FALSE(w.Required());
    EXPECT_TRUE(w.Exportable());
    EXPECT_TRUE(w.GetCheckedStateName().empty());

    w.Highlighting(HighlightingMode::Push);
    w.ReadOnly(true);
    w.Required(true);
    w.Exportable(false);
    EXPECT_EQ(w.Highlighting(), HighlightingMode::Push);
    EXPECT_TRUE(w.ReadOnly());
    EXPECT_TRUE(w.Required());
    EXPECT_FALSE(w.Exportable());

    DefaultAppearance da;
    da.FontSize(14.0);
    w.DefaultAppearance(da);
    EXPECT_DOUBLE_EQ(w.DefaultAppearance().FontSize(), 14.0);

    CountingSelector sel;
    w.Accept(sel);
    EXPECT_EQ(sel.widget, 1);
    EXPECT_EQ(sel.popup,  0);
    EXPECT_EQ(sel.screen, 0);
}

// ScreenAnnotation ctor takes Page&; v1 doesn't expose Page
// standalone construction (lands at A14). Exercise the Accept /
// TypeImpl signatures via pointer-to-member-function takes.
TEST(FormWidgetFamilySmoke, ScreenHeadersCompile) {
    void (ScreenAnnotation::* sc_accept)(AnnotationSelector&)
        = &ScreenAnnotation::Accept;
    (void)sc_accept;
    SUCCEED();
}
