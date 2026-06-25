// =============================================================================
// Go-parity: annotation_test.go / annotation_text_test.go → canonical C++
// Annotations cluster (Page.Annotations, TextAnnotation, text-markup
// annotations, LinkAnnotation + actions, AnnotationCollection, AnnotationType).
//
// Ported (canonical): collection presence/add/delete/count, annotation types
// and type filtering, TextAnnotation icon, text-markup quad points, link
// GoToURIAction. Save-through round-trips are covered by this lib's
// annotation_save_through / annotation_load_on_open suites.
// Skipped: Go-internal appearance tests (AP-XObject-leak), panic-on-nil-page
// ctors, and constant-enumeration tests that assert Go's enum integer layout.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/go_to_uri_action.hpp>
#include <aspose/pdf/annotations/highlight_annotation.hpp>
#include <aspose/pdf/annotations/link_annotation.hpp>
#include <aspose/pdf/annotations/text_annotation.hpp>
#include <aspose/pdf/annotations/text_icon.hpp>
#include <aspose/pdf/annotations/underline_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

std::filesystem::path PdfDir() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return std::filesystem::path(env);
    return std::filesystem::path(__FILE__)
        .parent_path()
        .parent_path()
        .parent_path() /
        "pdfs";
}

std::string HelloWorld() { return (PdfDir() / "hello_world.pdf").string(); }

Rectangle R() { return Rectangle{50.0, 50.0, 150.0, 100.0, true}; }

}  // namespace

// TestPageAnnotationsNonNilOnPlainDoc — a plain page has an empty collection.
TEST(GoAnnotation, EmptyCollectionOnPlainPage) {
    Document doc{HelloWorld()};
    EXPECT_EQ(doc.Pages()[1].Annotations().Count(), 0);
}

// TestTextAnnotationRoundTrip / TestTextAnnotationDefaultIcon — icon get/set.
TEST(GoAnnotation, TextAnnotationIcon) {
    Document doc{HelloWorld()};
    TextAnnotation a{doc};
    EXPECT_EQ(a.Icon(), TextIcon::Note);
    a.Icon(TextIcon::Comment);
    EXPECT_EQ(a.Icon(), TextIcon::Comment);
    EXPECT_EQ(a.AnnotationType(), AnnotationType::Text);
}

// TestHighlightAnnotationRoundTrip / TestUnderlineAnnotationRoundTrip —
// text-markup quad points + type.
TEST(GoAnnotation, TextMarkupQuadPoints) {
    Document doc{HelloWorld()};
    Page page = doc.Pages()[1];

    HighlightAnnotation hl{page, R()};
    const std::vector<Point> quads{Point{50, 50}, Point{150, 50},
                                   Point{150, 100}, Point{50, 100}};
    hl.QuadPoints(quads);
    EXPECT_EQ(hl.QuadPoints().size(), 4u);
    EXPECT_EQ(hl.AnnotationType(), AnnotationType::Highlight);

    UnderlineAnnotation ul{page, R()};
    EXPECT_EQ(ul.AnnotationType(), AnnotationType::Underline);
}

// TestLinkAnnotationGoToURIAction — link carries a URI action.
TEST(GoAnnotation, LinkGoToURIAction) {
    Document doc{HelloWorld()};
    Page page = doc.Pages()[1];
    LinkAnnotation link{page, R()};
    GoToURIAction action{"https://example.com"};
    link.Action(action);
    ASSERT_NE(link.Action(), nullptr);
    EXPECT_EQ(link.AnnotationType(), AnnotationType::Link);
}

// TestAnnotationCollectionAddLinkRoundTrip / DeleteAt — add + delete + count.
TEST(GoAnnotation, CollectionAddAndDelete) {
    Document doc{HelloWorld()};
    Page page = doc.Pages()[1];
    TextAnnotation a{doc};
    TextAnnotation b{doc};

    auto& annots = page.Annotations();
    annots.Add(a);
    annots.Add(b);
    EXPECT_EQ(annots.Count(), 2);
    annots.Delete(0);  // 0-based (canonical AnnotationCollection)
    EXPECT_EQ(annots.Count(), 1);
}

// TestAnnotationFilterByType — type discrimination across mixed annotations.
TEST(GoAnnotation, FilterByType) {
    Document doc{HelloWorld()};
    Page page = doc.Pages()[1];
    TextAnnotation note{doc};
    HighlightAnnotation hl{page, R()};

    auto& annots = page.Annotations();
    annots.Add(note);
    annots.Add(hl);

    int highlights = 0;
    for (int i = 0; i < annots.Count(); ++i)  // AnnotationCollection is 0-based
        if (annots[i].AnnotationType() == AnnotationType::Highlight)
            ++highlights;
    EXPECT_EQ(highlights, 1);
}
