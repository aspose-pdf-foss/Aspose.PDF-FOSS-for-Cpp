// =============================================================================
// annotation_collection_smoke_test — beat A4 of the annotations
// cluster. Covers AnnotationCollection (add/delete/contains/
// remove/clear/indexer) using a concrete Annotation stub.
// =============================================================================

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_collection.hpp>

#include <gtest/gtest.h>

#include <stdexcept>

namespace {

using namespace Aspose::Pdf::Annotations;

class Ann : public Aspose::Pdf::Annotations::Annotation {
public:
    using Aspose::Pdf::Annotations::Annotation::Annotation;

protected:
    Aspose::Pdf::Annotations::AnnotationType TypeImpl() const noexcept override {
        return Aspose::Pdf::Annotations::AnnotationType::Text;
    }
};

}  // namespace

TEST(AnnotationCollectionSmoke, DefaultIsEmpty) {
    AnnotationCollection c;
    EXPECT_EQ(c.Count(), 0);
    EXPECT_FALSE(c.IsReadOnly());
}

TEST(AnnotationCollectionSmoke, AddAndAccess) {
    Ann a1, a2, a3;
    AnnotationCollection c;
    c.Add(a1);
    c.Add(a2);
    c.Add(a3, /*considerRotation=*/true);
    EXPECT_EQ(c.Count(), 3);
    EXPECT_EQ(&c[0], &a1);
    EXPECT_EQ(&c[1], &a2);
    EXPECT_EQ(&c[2], &a3);
}

TEST(AnnotationCollectionSmoke, IndexOutOfRangeThrows) {
    AnnotationCollection c;
    Ann a;
    c.Add(a);
    EXPECT_THROW(c[1], std::out_of_range);
    EXPECT_THROW(c[-1], std::out_of_range);
    EXPECT_THROW(c.Delete(2), std::out_of_range);
}

TEST(AnnotationCollectionSmoke, ContainsAndRemove) {
    Ann a1, a2;
    AnnotationCollection c;
    c.Add(a1);
    EXPECT_TRUE(c.Contains(a1));
    EXPECT_FALSE(c.Contains(a2));

    EXPECT_TRUE(c.Remove(a1));
    EXPECT_FALSE(c.Contains(a1));
    EXPECT_FALSE(c.Remove(a1));  // already gone
    EXPECT_EQ(c.Count(), 0);
}

TEST(AnnotationCollectionSmoke, DeleteVariants) {
    Ann a1, a2, a3;
    AnnotationCollection c;
    c.Add(a1); c.Add(a2); c.Add(a3);

    c.Delete(1);            // index-based
    EXPECT_EQ(c.Count(), 2);
    EXPECT_EQ(&c[0], &a1);
    EXPECT_EQ(&c[1], &a3);

    c.Delete(a3);           // value-based
    EXPECT_EQ(c.Count(), 1);
    EXPECT_EQ(&c[0], &a1);

    c.Delete();             // all
    EXPECT_EQ(c.Count(), 0);
}

TEST(AnnotationCollectionSmoke, Clear) {
    Ann a1, a2;
    AnnotationCollection c;
    c.Add(a1); c.Add(a2);
    c.Clear();
    EXPECT_EQ(c.Count(), 0);
    EXPECT_FALSE(c.Contains(a1));
}

TEST(AnnotationCollectionSmoke, DeleteByValueIsNoopWhenAbsent) {
    Ann a1, a2;
    AnnotationCollection c;
    c.Add(a1);
    c.Delete(a2);   // a2 not in collection — no-op
    EXPECT_EQ(c.Count(), 1);
    EXPECT_TRUE(c.Contains(a1));
}
