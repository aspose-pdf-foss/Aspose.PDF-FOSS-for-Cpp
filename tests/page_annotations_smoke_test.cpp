// =============================================================================
// page_annotations_smoke_test — beat A14 (closing) of the
// annotations cluster. Page.Annotations() exposes the per-page
// AnnotationCollection; annotation instances live on the caller
// and are referenced by the collection. This beat is the cluster
// integration check — concrete annotation classes (A6..A13) now
// thread through a real Page object.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/circle_annotation.hpp>
#include <aspose/pdf/annotations/free_text_annotation.hpp>
#include <aspose/pdf/annotations/text_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

class CountingSelector : public AnnotationSelector {
public:
    int text = 0, circle = 0, free_text = 0;
    void Visit(TextAnnotation& t)     override { ++text;       (void)t; }
    void Visit(CircleAnnotation& c)   override { ++circle;     (void)c; }
    void Visit(FreeTextAnnotation& f) override { ++free_text;  (void)f; }
};

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

}  // namespace

TEST(PageAnnotationsSmoke, EmptyByDefault) {
    Document doc{HelloWorldPdf()};
    auto page = doc.Pages()[1];
    auto& annots = page.Annotations();
    EXPECT_EQ(annots.Count(), 0);
}

TEST(PageAnnotationsSmoke, StableIdentityAcrossCalls) {
    Document doc{HelloWorldPdf()};
    // Page is value-returned by PageCollection::operator[], so the
    // Page objects differ across calls — but Annotations() must
    // resolve to the *same* underlying collection both times.
    auto& a1 = doc.Pages()[1].Annotations();
    auto& a2 = doc.Pages()[1].Annotations();
    EXPECT_EQ(&a1, &a2);
}

TEST(PageAnnotationsSmoke, AddRoundtripsThroughCollection) {
    Document doc{HelloWorldPdf()};
    TextAnnotation host{doc};
    CircleAnnotation shape{doc};

    doc.Pages()[1].Annotations().Add(host);
    doc.Pages()[1].Annotations().Add(shape);

    auto& annots = doc.Pages()[1].Annotations();
    ASSERT_EQ(annots.Count(), 2);
    EXPECT_EQ(annots[0].AnnotationType(), AnnotationType::Text);
    EXPECT_EQ(annots[1].AnnotationType(), AnnotationType::Circle);

    CountingSelector sel;
    annots.Accept(sel);
    EXPECT_EQ(sel.text,   1);
    EXPECT_EQ(sel.circle, 1);
    EXPECT_EQ(sel.free_text, 0);
}

TEST(PageAnnotationsSmoke, RemoveAndContains) {
    Document doc{HelloWorldPdf()};
    TextAnnotation a{doc};
    TextAnnotation b{doc};

    auto& annots = doc.Pages()[1].Annotations();
    annots.Add(a);
    annots.Add(b);
    EXPECT_TRUE(annots.Contains(a));
    EXPECT_TRUE(annots.Contains(b));
    EXPECT_EQ(annots.Count(), 2);

    EXPECT_TRUE(annots.Remove(a));
    EXPECT_FALSE(annots.Contains(a));
    EXPECT_TRUE(annots.Contains(b));
    EXPECT_EQ(annots.Count(), 1);
}

// Empty Document still answers Annotations() — slot vector grows
// lazily to leaf_index_ + 1 the first time the accessor is hit.
// This test exercises the resize path on a doc with multiple
// canonical leaves (here the v1 default ctor; the empty doc
// reports 0 leaves so Pages()[1] would throw — exercise the lazy
// grow through a real load instead).
TEST(PageAnnotationsSmoke, FreshDocumentNoLeafThrows) {
    Document doc;  // empty Pages
    EXPECT_EQ(doc.Pages().Count(), 0u);
    EXPECT_THROW(doc.Pages()[1].Annotations(), std::out_of_range);
}
