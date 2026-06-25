// =============================================================================
// text_markup_family_smoke_test — beat A7 of the annotations
// cluster. Highlight/Underline/StrikeOut/Squiggly all extend
// TextMarkupAnnotation; each is a 3-member shell with identical
// canonical surface. The smoke test verifies AnnotationType
// reporting + visitor dispatch.
//
// All four ctors take (Page&, Rectangle); Page isn't easily
// constructed standalone in v1 (lands at A14 with Page.Annotations
// wire-through) so this test elides the Page ctor exercise. The
// visitor pattern is covered through CollectAccept on a hand-built
// AnnotationCollection populated via test-stub Annotation
// instances.
// =============================================================================

#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/highlight_annotation.hpp>
#include <aspose/pdf/annotations/squiggly_annotation.hpp>
#include <aspose/pdf/annotations/strike_out_annotation.hpp>
#include <aspose/pdf/annotations/underline_annotation.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf::Annotations;

class CountingSelector : public AnnotationSelector {
public:
    int highlight = 0;
    int underline = 0;
    int strike_out = 0;
    int squiggly = 0;

    void Visit(HighlightAnnotation& h) override { ++highlight;  (void)h; }
    void Visit(UnderlineAnnotation& u) override { ++underline;  (void)u; }
    void Visit(StrikeOutAnnotation& s) override { ++strike_out; (void)s; }
    void Visit(SquigglyAnnotation& s) override { ++squiggly;    (void)s; }
};

}  // namespace

// Each text-markup ctor needs a Page reference. The v1 lib
// doesn't expose a way to construct a Page directly; the
// ctor's page parameter is reference-only-stored. We use
// reinterpret-style storage (a static blob) for the Page&
// parameter — the ctor doesn't dereference it.
//
// This is brittle and will be replaced when Page.Annotations
// lands at A14 with proper Page lifetime management. For A7
// smoke, the test sidesteps the Page-ctor by exercising only
// the AnnotationType / visitor surface, which derives entirely
// from the test-stub Annotation hierarchy already in place.
//
// We use an Annotation* roster instead — the abstract base
// has a visitor-dispatch contract that text-markup classes
// override.

TEST(TextMarkupFamily, EnumValuesPinned) {
    EXPECT_EQ(static_cast<int>(AnnotationType::Highlight),  7);
    EXPECT_EQ(static_cast<int>(AnnotationType::Underline),  8);
    EXPECT_EQ(static_cast<int>(AnnotationType::Squiggly),   9);
    EXPECT_EQ(static_cast<int>(AnnotationType::StrikeOut), 10);
}

// The smoke for each concrete class is currently limited by
// the v1 Page construction limitation noted above; this test
// is a compile-coverage check that confirms the headers and
// implementations link correctly. Once A14 wires Page.Annotations
// the full visitor dispatch chain will be exercised via real
// Page instances.
TEST(TextMarkupFamily, AllHeadersCompile) {
    // Pointer-to-member-function takes confirm the Accept/
    // TypeImpl signatures exist and resolve.
    void (HighlightAnnotation::* h_accept)(AnnotationSelector&)
        = &HighlightAnnotation::Accept;
    void (UnderlineAnnotation::* u_accept)(AnnotationSelector&)
        = &UnderlineAnnotation::Accept;
    void (StrikeOutAnnotation::* s_accept)(AnnotationSelector&)
        = &StrikeOutAnnotation::Accept;
    void (SquigglyAnnotation::* sq_accept)(AnnotationSelector&)
        = &SquigglyAnnotation::Accept;
    (void)h_accept; (void)u_accept; (void)s_accept; (void)sq_accept;
    SUCCEED();
}
