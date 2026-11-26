// =============================================================================
// pdf_content_editor_smoke_test — PdfContentEditor: attachments, document
// JavaScript actions, viewer preferences, ReplaceImage, and editing the
// stamps PdfFileStamp draws (delete / hide / show / move, by id or index).
// =============================================================================

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/embedded_file_collection.hpp>
#include <aspose/pdf/facades/pdf_content_editor.hpp>
#include <aspose/pdf/facades/pdf_file_stamp.hpp>
#include <aspose/pdf/facades/viewer_preference.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/resources.hpp>
#include <aspose/pdf/x_image.hpp>
#include <aspose/pdf/x_image_collection.hpp>

#include "flatten_test_support.hpp"
#include "trailer.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;
using flatten_test::DarkPixels;
using flatten_test::HelloWorldPdf;
using flatten_test::ReadFile;
using flatten_test::Render;
using flatten_test::TmpPath;
namespace fo = foundation::objects;

std::string Fixture(const std::string& dir, const std::string& name) {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" / dir / name).string();
}

// The catalog of the PDF at `path` (latest revision).
fo::Dict Catalog(const std::string& bytes, fo::Dump& dump) {
    const auto span = flatten_test::Bytes(bytes);
    const auto tr = foundation::trailer::Parse(span);
    dump = fo::Parse(span);
    const auto* obj = flatten_test::Find(dump, tr.root_id);
    return obj ? std::get<fo::Dict>(obj->value.v) : fo::Dict{};
}

}  // namespace

TEST(PdfContentEditor, AttachmentsAddAndDelete) {
    const auto with = TmpPath("ce_attach");
    const auto without = TmpPath("ce_attach_deleted");
    {
        Document doc{HelloWorldPdf()};
        PdfContentEditor editor{doc};
        editor.AddDocumentAttachment(Fixture("pkcs12", "content.bin"), "sample bytes");
        EXPECT_THROW(editor.AddDocumentAttachment(Fixture("pkcs12", "missing.bin"), ""),
                     std::runtime_error);
        doc.Save(with);
    }
    {
        Document doc{with};
        ASSERT_EQ(doc.EmbeddedFiles().Count(), 1);
        EXPECT_EQ(doc.EmbeddedFiles()[0].Name(), "content.bin");
        EXPECT_EQ(doc.EmbeddedFiles()[0].Description(), "sample bytes");
        PdfContentEditor editor{doc};
        editor.DeleteAttachments();
        doc.Save(without);
    }
    Document doc{without};
    EXPECT_EQ(doc.EmbeddedFiles().Count(), 0);
}

TEST(PdfContentEditor, DocumentJavaScriptActions) {
    const auto with = TmpPath("ce_actions");
    const auto removed = TmpPath("ce_actions_removed");
    {
        Document doc{HelloWorldPdf()};
        PdfContentEditor editor{doc};
        editor.AddDocumentAdditionalAction(PdfContentEditor::DocumentOpen, "app.alert('open');");
        editor.AddDocumentAdditionalAction(PdfContentEditor::DocumentWillPrint, "print();");
        EXPECT_THROW(editor.AddDocumentAdditionalAction("XX", "x"), std::invalid_argument);
        doc.Save(with);
    }
    {
        fo::Dump dump;
        const auto cat = Catalog(ReadFile(with), dump);
        const auto open = flatten_test::ResolveDict(dump, flatten_test::Get(cat, "OpenAction"));
        EXPECT_EQ(flatten_test::NameOf(flatten_test::Get(open, "S")), "JavaScript");
        const auto aa = flatten_test::ResolveDict(dump, flatten_test::Get(cat, "AA"));
        EXPECT_NE(flatten_test::Get(aa, "WP"), nullptr);
    }
    {
        Document doc{with};
        PdfContentEditor editor{doc};
        editor.RemoveDocumentOpenAction();
        doc.Save(removed);
    }
    fo::Dump dump;
    const auto cat = Catalog(ReadFile(removed), dump);
    EXPECT_EQ(flatten_test::Get(cat, "OpenAction"), nullptr);
    EXPECT_NE(flatten_test::Get(cat, "AA"), nullptr);  // other triggers stay
}

TEST(PdfContentEditor, ViewerPreferences) {
    const auto out = TmpPath("ce_viewer");
    const int flags = ViewerPreference::PageLayoutTwoColumnLeft | ViewerPreference::HideToolbar |
                      ViewerPreference::DuplexFlipLongEdge |
                      ViewerPreference::PageModeUseOutlines;
    {
        Document doc{HelloWorldPdf()};
        PdfContentEditor editor{doc};
        EXPECT_EQ(editor.GetViewerPreference(), 0);
        editor.ChangeViewerPreference(flags);
        editor.ChangeViewerPreference(ViewerPreference::CenterWindow);
        EXPECT_EQ(editor.GetViewerPreference(), flags | ViewerPreference::CenterWindow);
        doc.Save(out);
    }
    Document doc{out};
    PdfContentEditor editor{doc};
    EXPECT_EQ(editor.GetViewerPreference(), flags | ViewerPreference::CenterWindow);
    fo::Dump dump;
    const auto cat = Catalog(ReadFile(out), dump);
    EXPECT_EQ(flatten_test::NameOf(flatten_test::Get(cat, "PageLayout")), "TwoColumnLeft");
    EXPECT_EQ(flatten_test::NameOf(flatten_test::Get(cat, "PageMode")), "UseOutlines");
}

TEST(PdfContentEditor, ReplaceImage) {
    const auto seeded = TmpPath("ce_img_seed");
    const auto out = TmpPath("ce_img_out");
    {
        Document doc{HelloWorldPdf()};
        ASSERT_TRUE(doc.Pages()[1].AddImage(Fixture("dctdecode", "gradient_32.jpg"),
                                            Rectangle{100.0, 100.0, 300.0, 300.0, true}));
        doc.Save(seeded);
    }
    {
        Document doc{seeded};
        PdfContentEditor editor{doc};
        editor.ReplaceImage(1, 1, Fixture("png_decoder", "rgb_checker_16.png"));
        EXPECT_THROW(editor.ReplaceImage(1, 2, Fixture("png_decoder", "rgb_checker_16.png")),
                     std::out_of_range);
        EXPECT_THROW(editor.ReplaceImage(1, 1, Fixture("png_decoder", "missing.png")),
                     std::runtime_error);
        doc.Save(out);
    }
    Document doc{out};
    auto& images = doc.Pages()[1].Resources().Images();
    ASSERT_EQ(images.Count(), 1);
    EXPECT_EQ(images[1].Width(), 16);
}

namespace {

// hello_world.pdf (612 x 792) with a header (id 7) near the top and a
// footer (id 9) near the bottom, both drawn by PdfFileStamp.
std::string Stamped(const std::string& tag) {
    const auto path = TmpPath(tag);
    Document doc{HelloWorldPdf()};
    PdfFileStamp stamp{doc};
    stamp.StampId(7);
    stamp.AddHeader("HEADER STAMP", 20.0f);   // baseline y = 772, x = 72
    stamp.StampId(9);
    stamp.AddFooter("FOOTER STAMP", 20.0f);   // baseline y = 20, x = 72
    doc.Save(path);
    return path;
}

int HeaderInk(Document& doc) { return DarkPixels(Render(doc, 1), 72, 768, 200, 785); }
int FooterInk(Document& doc) { return DarkPixels(Render(doc, 1), 72, 16, 200, 33); }

}  // namespace

TEST(PdfContentEditor, HideAndShowStampById) {
    const auto src = Stamped("ce_stamp_hide");
    Document doc{src};
    ASSERT_GT(HeaderInk(doc), 0);
    PdfContentEditor editor{doc};
    editor.HideStampById(1, 7);
    EXPECT_EQ(HeaderInk(doc), 0);
    EXPECT_GT(FooterInk(doc), 0);
    editor.ShowStampById(1, 7);
    EXPECT_GT(HeaderInk(doc), 0);
    editor.HideStampById(1, 0);  // id 0 = no id: matches nothing
    EXPECT_GT(HeaderInk(doc), 0);
}

TEST(PdfContentEditor, MoveStampByIdAndIndex) {
    const auto src = Stamped("ce_stamp_move");
    const auto out = TmpPath("ce_stamp_moved");
    {
        Document doc{src};
        PdfContentEditor editor{doc};
        editor.MoveStampById(1, 9, 300, 400);       // footer -> (300, 400)
        editor.MoveStamp(1, 0, 300, 600);           // first stamp (header) -> (300, 600)
        EXPECT_THROW(editor.MoveStamp(1, 5, 0, 0), std::out_of_range);
        doc.Save(out);
    }
    Document doc{out};
    const auto img = Render(doc, 1);
    EXPECT_EQ(DarkPixels(img, 72, 16, 200, 33), 0);
    EXPECT_EQ(DarkPixels(img, 72, 768, 200, 785), 0);
    EXPECT_GT(DarkPixels(img, 300, 396, 430, 413), 0);
    EXPECT_GT(DarkPixels(img, 300, 596, 430, 613), 0);
}

TEST(PdfContentEditor, DeleteStamps) {
    const auto src = Stamped("ce_stamp_delete");
    {
        Document doc{src};
        PdfContentEditor editor{doc};
        editor.DeleteStampById(7);  // every page
        EXPECT_EQ(HeaderInk(doc), 0);
        EXPECT_GT(FooterInk(doc), 0);
        editor.DeleteStamp(1, {0});  // the footer is now index 0
        EXPECT_EQ(FooterInk(doc), 0);
        EXPECT_EQ(ReadFile(src).find("HEADER STAMP") != std::string::npos, true);
    }
    {
        Document doc{src};
        PdfContentEditor editor{doc};
        editor.DeleteStampByIds({7, 9});
        EXPECT_EQ(HeaderInk(doc), 0);
        EXPECT_EQ(FooterInk(doc), 0);
    }
}
