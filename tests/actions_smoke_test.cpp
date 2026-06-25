// =============================================================================
// actions_smoke_test — beat B of the actions cluster. The new PdfAction
// subclasses NamedAction (PredefinedAction), JavascriptAction and
// SubmitFormAction, plus the PredefinedAction enum. Covers construction /
// accessors and the polymorphic clone-through-LinkAnnotation::Action path.
// =============================================================================

#include <aspose/pdf/annotations/javascript_action.hpp>
#include <aspose/pdf/annotations/link_annotation.hpp>
#include <aspose/pdf/annotations/named_action.hpp>
#include <aspose/pdf/annotations/pdf_action.hpp>
#include <aspose/pdf/annotations/predefined_action.hpp>
#include <aspose/pdf/annotations/submit_form_action.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/file_specification.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

}  // namespace

TEST(ActionsSmoke, NamedActionFromPredefined) {
    EXPECT_EQ(static_cast<int>(PredefinedAction::FirstPage), 0);
    EXPECT_EQ(static_cast<int>(PredefinedAction::Window_FullScreenMode), 70);

    NamedAction a{PredefinedAction::NextPage};
    EXPECT_EQ(a.Name(), "NextPage");
    EXPECT_EQ(a.ToString(), "NextPage");
    a.Name("FirstPage");
    EXPECT_EQ(a.Name(), "FirstPage");
}

TEST(ActionsSmoke, JavascriptAction) {
    JavascriptAction a{"app.alert('hi');"};
    EXPECT_EQ(a.Script(), "app.alert('hi');");
    EXPECT_EQ(a.GetECMAScriptString(), "app.alert('hi');");
    a.Script("console.println('x');");
    EXPECT_EQ(a.GetECMAScriptString(), "console.println('x');");
}

TEST(ActionsSmoke, SubmitFormAction) {
    SubmitFormAction a;
    EXPECT_EQ(a.Flags(), 0);
    a.Flags(4);
    a.Url(FileSpecification{"https://example.org/submit"});
    EXPECT_EQ(a.Flags(), 4);
    EXPECT_EQ(a.Url().Name(), "https://example.org/submit");
}

TEST(ActionsSmoke, ClonesThroughLinkAnnotationAction) {
    Document doc;
    Page page = doc.Pages().Add();
    LinkAnnotation link{page, Rectangle{0.0, 0.0, 100.0, 20.0, false}};

    // Each action type survives the clone done by LinkAnnotation::Action.
    link.Action(NamedAction{PredefinedAction::LastPage});
    ASSERT_NE(link.Action(), nullptr);
    EXPECT_EQ(link.Action()->ToString(), "LastPage");
    EXPECT_NE(dynamic_cast<NamedAction*>(link.Action()), nullptr);

    link.Action(JavascriptAction{"this.print();"});
    EXPECT_EQ(link.Action()->GetECMAScriptString(), "this.print();");
    EXPECT_NE(dynamic_cast<JavascriptAction*>(link.Action()), nullptr);

    SubmitFormAction sf;
    sf.Url(FileSpecification{"https://example.org/x"});
    link.Action(sf);
    auto* got = dynamic_cast<SubmitFormAction*>(link.Action());
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(got->Url().Name(), "https://example.org/x");
}
