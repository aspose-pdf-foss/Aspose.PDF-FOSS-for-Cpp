// =============================================================================
// specialty_family_smoke_test — beat A11 of the annotations
// cluster. FileAttachmentAnnotation (carries a FileSpecification),
// RedactionAnnotation (extends MarkupAnnotation with quad-region
// content-removal hooks), WatermarkAnnotation (extends Annotation
// directly with an opacity-only surface; v1 drops the text-state
// + facades-text overloads).
// =============================================================================

#include <aspose/pdf/af_relationship.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/file_attachment_annotation.hpp>
#include <aspose/pdf/annotations/file_icon.hpp>
#include <aspose/pdf/annotations/redaction_annotation.hpp>
#include <aspose/pdf/annotations/watermark_annotation.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/file_encoding.hpp>
#include <aspose/pdf/file_specification.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/point.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;

class CountingSelector : public AnnotationSelector {
public:
    int file_attachment = 0, redaction = 0, watermark = 0;
    void Visit(FileAttachmentAnnotation& f) override {
        ++file_attachment; (void)f;
    }
    void Visit(RedactionAnnotation& r) override { ++redaction; (void)r; }
    void Visit(WatermarkAnnotation& w) override { ++watermark; (void)w; }
};

}  // namespace

TEST(SpecialtyFamilySmoke, EnumValuesPinned) {
    EXPECT_EQ(static_cast<int>(AnnotationType::FileAttachment), 15);
    EXPECT_EQ(static_cast<int>(AnnotationType::Redaction),      23);
    EXPECT_EQ(static_cast<int>(AnnotationType::Watermark),      20);

    EXPECT_EQ(static_cast<int>(FileIcon::PushPin),   0);
    EXPECT_EQ(static_cast<int>(FileIcon::Graph),     1);
    EXPECT_EQ(static_cast<int>(FileIcon::Paperclip), 2);
    EXPECT_EQ(static_cast<int>(FileIcon::Tag),       3);

    EXPECT_EQ(static_cast<int>(FileEncoding::None), 0);
    EXPECT_EQ(static_cast<int>(FileEncoding::Zip),  1);

    EXPECT_EQ(static_cast<int>(AFRelationship::Source),           0);
    EXPECT_EQ(static_cast<int>(AFRelationship::Data),             1);
    EXPECT_EQ(static_cast<int>(AFRelationship::Alternative),      2);
    EXPECT_EQ(static_cast<int>(AFRelationship::Supplement),       3);
    EXPECT_EQ(static_cast<int>(AFRelationship::Unspecified),      4);
    EXPECT_EQ(static_cast<int>(AFRelationship::EncryptedPayload), 5);
    EXPECT_EQ(static_cast<int>(AFRelationship::None),             6);
}

TEST(SpecialtyFamilySmoke, FileSpecificationDataCarrier) {
    FileSpecification fs{"path/to/data.json", "schema descriptor"};
    EXPECT_EQ(fs.Description(), "schema descriptor");
    EXPECT_EQ(fs.Name(), "path/to/data.json");
    EXPECT_EQ(fs.UnicodeName(), "path/to/data.json");
    EXPECT_EQ(fs.Encoding(), FileEncoding::None);
    EXPECT_EQ(fs.AFRelationship(), AFRelationship::Unspecified);
    EXPECT_TRUE(fs.IncludeContents());

    fs.Encoding(FileEncoding::Zip);
    fs.AFRelationship(AFRelationship::Data);
    fs.MIMEType("application/json");
    fs.IncludeContents(false);
    EXPECT_EQ(fs.Encoding(), FileEncoding::Zip);
    EXPECT_EQ(fs.AFRelationship(), AFRelationship::Data);
    EXPECT_EQ(fs.MIMEType(), "application/json");
    EXPECT_FALSE(fs.IncludeContents());

    fs.SetValue("Author", "QA");
    fs.SetValue("Version", "1.2");
    EXPECT_EQ(fs.GetValue("Author"),  "QA");
    EXPECT_EQ(fs.GetValue("Version"), "1.2");
    EXPECT_EQ(fs.GetValue("Missing"), "");

    fs.Dispose();
    EXPECT_EQ(fs.Description(), "");
    EXPECT_EQ(fs.Name(), "");
}

TEST(SpecialtyFamilySmoke, RedactionConstructAndVisit) {
    Document doc;
    RedactionAnnotation r{doc};
    EXPECT_EQ(r.AnnotationType(), AnnotationType::Redaction);

    EXPECT_NEAR(r.FillColor().A(),   1.0, 1e-9);
    EXPECT_NEAR(r.BorderColor().A(), 1.0, 1e-9);
    EXPECT_FALSE(r.Repeat());
    EXPECT_EQ(r.FontSize(), 0.0f);
    EXPECT_EQ(r.TextAlignment(), HorizontalAlignment::Left);

    r.OverlayText("[REDACTED]");
    r.DefaultAppearance("/Helv 0 Tf 0 g");
    r.FontSize(10.0f);
    r.Repeat(true);
    r.TextAlignment(HorizontalAlignment::Center);
    r.FillColor(Color::Red());

    EXPECT_EQ(r.OverlayText(),       "[REDACTED]");
    EXPECT_EQ(r.DefaultAppearance(), "/Helv 0 Tf 0 g");
    EXPECT_EQ(r.FontSize(),          10.0f);
    EXPECT_TRUE(r.Repeat());
    EXPECT_EQ(r.TextAlignment(),     HorizontalAlignment::Center);

    std::vector<Point> quads{Point{0, 0}, Point{10, 0}, Point{10, 10},
                             Point{0, 10}};
    r.QuadPoint(quads);
    EXPECT_EQ(r.QuadPoint().size(), 4u);

    // Flatten + Redact must not throw on a freshly-constructed
    // annotation; v1 bodies are no-ops on the page side until the
    // Document edit-flow is wired.
    r.Flatten();
    r.Redact();

    CountingSelector sel;
    r.Accept(sel);
    EXPECT_EQ(sel.redaction, 1);
    EXPECT_EQ(sel.file_attachment, 0);
    EXPECT_EQ(sel.watermark, 0);
}

// FileAttachmentAnnotation + WatermarkAnnotation ctors require a
// Page reference. The Page object cannot be standalone-constructed
// in v1 (lands at A14 with Page.Annotations wire-through), so we
// exercise pointer-to-member-function takes to confirm the Accept /
// TypeImpl signatures resolve cleanly. Same pattern as the
// TextMarkupFamily.AllHeadersCompile test from A7.
TEST(SpecialtyFamilySmoke, FileAttachmentAndWatermarkHeadersCompile) {
    void (FileAttachmentAnnotation::* fa_accept)(AnnotationSelector&)
        = &FileAttachmentAnnotation::Accept;
    void (WatermarkAnnotation::* wm_accept)(AnnotationSelector&)
        = &WatermarkAnnotation::Accept;
    (void)fa_accept; (void)wm_accept;
    SUCCEED();
}
