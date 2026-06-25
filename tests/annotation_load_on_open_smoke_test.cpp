// =============================================================================
// annotation_load_on_open_smoke_test — Page::Annotations() reads an
// existing page's /Annots back into the collection (load-on-open).
// Verifies add → save → reopen → read-back (count / subtype / contents),
// plus delete-on-disk and delete-all-on-disk via re-save.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/circle_annotation.hpp>
#include <aspose/pdf/annotations/square_annotation.hpp>
#include <aspose/pdf/annotations/text_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::string TmpPath(const std::string& tag) {
    return (std::filesystem::temp_directory_path() /
            ("asp_annot_loo_" + tag + ".pdf"))
        .string();
}

Rectangle R(double a, double b, double c, double d) {
    return Rectangle(a, b, c, d, false);
}

// Make a 3-annotation PDF (Text / Square / Circle) at `out`.
void MakeThreeAnnotPdf(const std::string& out) {
    Document doc{HelloWorldPdf()};
    TextAnnotation t{doc};
    t.Rect(R(100, 700, 120, 720));
    t.Contents("note one");
    SquareAnnotation s{doc};
    s.Rect(R(150, 700, 200, 740));
    s.Contents("box two");
    CircleAnnotation c{doc};
    c.Rect(R(220, 700, 260, 740));
    doc.Pages()[1].Annotations().Add(t);
    doc.Pages()[1].Annotations().Add(s);
    doc.Pages()[1].Annotations().Add(c);
    doc.Save(out);
}

}  // namespace

TEST(AnnotationLoadOnOpen, RoundTripCountTypesContents) {
    const std::string out = TmpPath("rt");
    MakeThreeAnnotPdf(out);

    Document re{out};
    auto& annots = re.Pages()[1].Annotations();
    ASSERT_EQ(annots.Count(), 3);

    // Subtypes and contents survive (order preserved).
    EXPECT_EQ(annots[0].AnnotationType(), AnnotationType::Text);
    EXPECT_EQ(annots[0].Contents(), "note one");
    EXPECT_EQ(annots[1].AnnotationType(), AnnotationType::Square);
    EXPECT_EQ(annots[1].Contents(), "box two");
    EXPECT_EQ(annots[2].AnnotationType(), AnnotationType::Circle);
    std::filesystem::remove(out);
}

TEST(AnnotationLoadOnOpen, DeleteOnDiskReducesCount) {
    const std::string out = TmpPath("del1");
    const std::string out2 = TmpPath("del2");
    MakeThreeAnnotPdf(out);

    {
        Document re{out};
        auto& annots = re.Pages()[1].Annotations();
        ASSERT_EQ(annots.Count(), 3);
        annots.Delete(1);  // drop the Square (index 1)
        EXPECT_EQ(annots.Count(), 2);
        re.Save(out2);
    }

    Document re2{out2};
    auto& a2 = re2.Pages()[1].Annotations();
    EXPECT_EQ(a2.Count(), 2);
    EXPECT_EQ(a2[0].AnnotationType(), AnnotationType::Text);
    EXPECT_EQ(a2[1].AnnotationType(), AnnotationType::Circle);
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}

TEST(AnnotationLoadOnOpen, DeleteAllOnDiskClears) {
    const std::string out = TmpPath("clr1");
    const std::string out2 = TmpPath("clr2");
    MakeThreeAnnotPdf(out);

    {
        Document re{out};
        auto& annots = re.Pages()[1].Annotations();
        ASSERT_EQ(annots.Count(), 3);
        annots.Delete();  // clear all
        EXPECT_EQ(annots.Count(), 0);
        re.Save(out2);
    }

    Document re2{out2};
    EXPECT_EQ(re2.Pages()[1].Annotations().Count(), 0);
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}

TEST(AnnotationLoadOnOpen, ReadOnlyResaveIsLossless) {
    const std::string out = TmpPath("loss1");
    const std::string out2 = TmpPath("loss2");
    MakeThreeAnnotPdf(out);

    {
        Document re{out};
        ASSERT_EQ(re.Pages()[1].Annotations().Count(), 3);
        re.Save(out2);  // no changes
    }

    Document re2{out2};
    auto& a2 = re2.Pages()[1].Annotations();
    EXPECT_EQ(a2.Count(), 3);
    EXPECT_EQ(a2[0].Contents(), "note one");
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}
