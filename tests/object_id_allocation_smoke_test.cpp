// =============================================================================
// object_id_allocation_smoke_test — save passes that add objects (attachments,
// XMP metadata) after an earlier incremental update (a PdfFileStamp stamp)
// must not reuse the ids of objects that update created: the trailer /Size
// read from a multi-revision file can be stale.
// =============================================================================

#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/embedded_file_collection.hpp>
#include <aspose/pdf/facades/pdf_content_editor.hpp>
#include <aspose/pdf/facades/pdf_file_stamp.hpp>
#include <aspose/pdf/metadata.hpp>
#include <aspose/pdf/text_absorber.hpp>
#include <aspose/pdf/xmp_value.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

using namespace Aspose::Pdf;

TEST(ObjectIdAllocation, AttachmentAndXmpAfterStampKeepTheStamp) {
    const auto out = flatten_test::TmpPath("id_alloc");
    {
        Document doc{flatten_test::HelloWorldPdf()};
        Facades::PdfFileStamp stamp{doc};
        stamp.AddFooter("STAMPED FOOTER", 20.0f);  // objects added at once
        Facades::PdfContentEditor content{doc};
        content.AddDocumentAttachment(flatten_test::HelloWorldPdf(), "copy");  // at Save
        doc.Metadata()["dc:title"] = XmpValue(std::string("Id test"));        // at Save
        doc.Save(out);
    }
    Document doc{out};
    Text::TextAbsorber absorber;
    absorber.Visit(doc);
    EXPECT_NE(absorber.Text().find("STAMPED FOOTER"), std::string::npos);
    EXPECT_EQ(doc.EmbeddedFiles().Count(), 1);
    EXPECT_EQ(doc.Metadata().at("dc:title").ToStringValue(), "Id test");
    // The stamp's content stream is still a stream.
    const auto page = flatten_test::ParsePage(flatten_test::ReadFile(out), 0);
    const auto* contents = flatten_test::Get(page.page, "Contents");
    ASSERT_NE(contents, nullptr);
    if (const auto* arr = std::get_if<foundation::objects::Array>(&contents->v))
        for (const auto& item : arr->items)
            if (const auto* r = std::get_if<foundation::objects::Ref>(&item.v)) {
                const auto* o = flatten_test::Find(page.dump, r->id);
                ASSERT_NE(o, nullptr);
                EXPECT_TRUE(std::holds_alternative<foundation::objects::Stream>(o->value.v))
                    << "content object " << r->id << " is not a stream";
            }
}
