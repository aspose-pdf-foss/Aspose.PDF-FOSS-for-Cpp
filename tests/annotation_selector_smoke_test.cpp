// =============================================================================
// annotation_selector_smoke_test — beat A5 of the annotations
// cluster. Covers AnnotationSelector default + 1-arg ctor + the
// double-dispatch wire-through via Annotation::Accept +
// AnnotationCollection::Accept.
//
// Concrete annotation subclasses don't exist yet (A6..A13); this
// test uses Annotation stubs whose Accept() default is no-op
// (matching the v1 abstract-base contract). When concrete
// subclasses land they'll override Accept to call
// visitor.Visit(*this) and the dispatch will exercise the right
// Visit overload.
// =============================================================================

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>

#include <gtest/gtest.h>

namespace {

class Ann : public Aspose::Pdf::Annotations::Annotation {
public:
    using Aspose::Pdf::Annotations::Annotation::Annotation;

    int accept_calls = 0;

    void Accept(
            Aspose::Pdf::Annotations::AnnotationSelector& v) override {
        ++accept_calls;
        // No double-dispatch yet — concrete subclasses at A6+
        // will call v.Visit(*this) here.
        (void)v;
    }

protected:
    Aspose::Pdf::Annotations::AnnotationType TypeImpl() const noexcept override {
        return Aspose::Pdf::Annotations::AnnotationType::Text;
    }
};

}  // namespace

TEST(AnnotationSelectorSmoke, DefaultConstructible) {
    Aspose::Pdf::Annotations::AnnotationSelector s;
    (void)s;
    SUCCEED();
}

TEST(AnnotationSelectorSmoke, OneArgCtorAcceptsAnnotation) {
    Ann a;
    Aspose::Pdf::Annotations::AnnotationSelector s{a};
    (void)s;
    SUCCEED();
}

TEST(AnnotationSelectorSmoke, AnnotationAcceptDispatches) {
    Ann a;
    Aspose::Pdf::Annotations::AnnotationSelector s;
    a.Accept(s);
    EXPECT_EQ(a.accept_calls, 1);
    a.Accept(s);
    a.Accept(s);
    EXPECT_EQ(a.accept_calls, 3);
}

TEST(AnnotationSelectorSmoke, AnnotationCollectionAcceptIterates) {
    Ann a1, a2, a3;
    Aspose::Pdf::Annotations::AnnotationCollection coll;
    coll.Add(a1);
    coll.Add(a2);
    coll.Add(a3);

    Aspose::Pdf::Annotations::AnnotationSelector s;
    coll.Accept(s);

    EXPECT_EQ(a1.accept_calls, 1);
    EXPECT_EQ(a2.accept_calls, 1);
    EXPECT_EQ(a3.accept_calls, 1);
}

TEST(AnnotationSelectorSmoke, EmptyCollectionAcceptIsNoop) {
    Aspose::Pdf::Annotations::AnnotationCollection coll;
    Aspose::Pdf::Annotations::AnnotationSelector s;
    coll.Accept(s);  // should not throw
    SUCCEED();
}

TEST(AnnotationSelectorSmoke, BaseClassVisitDefaultsAreNoops) {
    // The 30 Visit overloads are virtual no-ops at the base
    // class level — calling any of them on a default selector
    // is well-defined even though the concrete annotation types
    // don't exist yet (forward declarations are sufficient for
    // taking parameters by reference). v1 smoke is a "doesn't
    // throw / doesn't crash" sanity check.
    Aspose::Pdf::Annotations::AnnotationSelector s;
    (void)s;
    SUCCEED();
}
