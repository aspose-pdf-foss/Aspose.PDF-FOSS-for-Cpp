// =============================================================================
// facades_section2_import_export_smoke_test — PdfBookmarkEditor XML / HTML,
// PdfAnnotationEditor XFDF / FDF / PDF import and ModifyAnnotations[Author],
// PdfFileSecurity::SetPrivilege without passwords.
// =============================================================================

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/annotations/square_annotation.hpp>
#include <aspose/pdf/annotations/text_annotation.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/bookmark.hpp>
#include <aspose/pdf/document_privilege.hpp>
#include <aspose/pdf/facades/pdf_annotation_editor.hpp>
#include <aspose/pdf/facades/pdf_bookmark_editor.hpp>
#include <aspose/pdf/facades/pdf_file_security.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;
using Aspose::Pdf::Annotations::AnnotationType;
using flatten_test::HelloWorldPdf;
using flatten_test::ReadFile;
using flatten_test::TmpPath;

void Write(const std::string& path, const std::string& text) {
    std::ofstream(path, std::ios::binary).write(text.data(), static_cast<std::streamsize>(text.size()));
}

std::vector<std::string> Titles(const Bookmarks& bs) {
    std::vector<std::string> t;
    for (const auto& b : bs) t.push_back(std::to_string(b.Level()) + ":" + b.Title());
    return t;
}

// two_pages.pdf with outline A (page 1, children A1 page 2) and B (page 2).
std::string WithBookmarks(const std::string& tag) {
    const auto path = TmpPath(tag);
    Document doc{(std::filesystem::path(HelloWorldPdf()).parent_path() / "two_pages.pdf").string()};
    PdfBookmarkEditor editor{doc};
    Bookmark a, a1, b;
    a.Title("A"); a.PageNumber(1);
    a1.Title("A1"); a1.PageNumber(2);
    b.Title("B"); b.PageNumber(2);
    Bookmarks kids;
    kids.push_back(a1);
    a.ChildItems(kids);
    editor.CreateBookmarks(a);
    editor.CreateBookmarks(b);
    doc.Save(path);
    return path;
}

std::vector<AnnotationType> Types(Document& doc, int page) {
    std::vector<AnnotationType> out;
    auto& annots = doc.Pages()[page].Annotations();
    for (int i = 0; i < annots.Count(); ++i) out.push_back(annots[i].AnnotationType());
    return out;
}

}  // namespace

// ---- bookmarks ---------------------------------------------------------------

TEST(Section2Bookmarks, XmlExportAndImportRoundTrip) {
    const auto src = WithBookmarks("bm_xml_src");
    const auto xml = TmpPath("bm_xml_export") + ".xml";
    {
        Document doc{src};
        PdfBookmarkEditor editor{doc};
        editor.ExportBookmarksToXML(xml);
    }
    const std::string text = ReadFile(xml);
    EXPECT_NE(text.find("<Bookmark>"), std::string::npos);
    EXPECT_NE(text.find("Action=\"GoTo\""), std::string::npos);
    EXPECT_NE(text.find("Page=\"2 "), std::string::npos);
    EXPECT_NE(text.find("Open="), std::string::npos);

    const auto out = TmpPath("bm_xml_import");
    {
        Document doc{(std::filesystem::path(HelloWorldPdf()).parent_path() / "two_pages.pdf").string()};
        PdfBookmarkEditor editor{doc};
        editor.ImportBookmarksWithXML(xml);
        doc.Save(out);
    }
    Document doc{out};
    PdfBookmarkEditor editor{doc};
    EXPECT_EQ(Titles(editor.ExtractBookmarks()),
              (std::vector<std::string>{"1:A", "2:A1", "1:B"}));
}

TEST(Section2Bookmarks, ImportsHandWrittenXml) {
    const auto xml = TmpPath("bm_hand") + ".xml";
    Write(xml,
          "<?xml version=\"1.0\" encoding=\"ISO8859-1\"?>\n"
          "<Bookmark>\n"
          "  <Title Style=\"bold\" Page=\"1 XYZ 0 792 0\" Action=\"GoTo\" Open=\"True\">"
          "Caf&#233; \xE9t\xE9\n"
          "    <Title Action=\"URI\" URI=\"https://example.com/\">Link &amp; more</Title>\n"
          "  </Title>\n"
          "</Bookmark>\n");
    const auto out = TmpPath("bm_hand_out");
    {
        Document doc{HelloWorldPdf()};
        PdfBookmarkEditor editor{doc};
        editor.ImportBookmarksWithXML(xml);
        doc.Save(out);
    }
    Document doc{out};
    PdfBookmarkEditor editor{doc};
    EXPECT_EQ(Titles(editor.ExtractBookmarks()),
              (std::vector<std::string>{"1:Caf\xC3\xA9 \xC3\xA9t\xC3\xA9", "2:Link & more"}));
    Write(xml, "<Bookmark><Title>unterminated</Bookmark>");
    PdfBookmarkEditor again{doc};
    EXPECT_THROW(again.ImportBookmarksWithXML(xml), std::runtime_error);
}

TEST(Section2Bookmarks, HtmlExport) {
    const auto src = WithBookmarks("bm_html_src");
    const auto html = TmpPath("bm_html") + ".html";
    PdfBookmarkEditor editor;
    editor.ExportBookmarksToHtml(src, html);
    const std::string text = ReadFile(html);
    const std::string file = std::filesystem::path(src).filename().string();
    EXPECT_NE(text.find("<!DOCTYPE html>"), std::string::npos);
    EXPECT_NE(text.find("href=\"" + file + "#page=2\""), std::string::npos);
    EXPECT_NE(text.find(">A1</a>"), std::string::npos);

    editor.ExtractBookmarksToHTML(src, "");
    const auto index = std::filesystem::path(src).parent_path() /
                       (std::filesystem::path(src).stem().string() + "-index.html");
    EXPECT_TRUE(std::filesystem::exists(index));

    const auto none = TmpPath("bm_html_none") + ".html";
    editor.ExportBookmarksToHtml(HelloWorldPdf(), none);
    EXPECT_NE(ReadFile(none).find("This document has no bookmarks."), std::string::npos);
}

// ---- annotation import / modify ----------------------------------------------------

namespace {

const char* kXfdf = R"(<?xml version="1.0" encoding="UTF-8"?>
<xfdf xmlns="http://ns.adobe.com/xfdf/" xml:space="preserve">
  <annots>
    <text page="0" rect="100,700,120,720" color="#FFFF00" title="Reviewer" name="n1"
          subject="Note" icon="Comment" date="D:20261126120000Z">
      <contents>Please check &amp; fix</contents>
      <popup page="0" rect="130,650,300,720" open="yes"/>
    </text>
    <highlight page="0" rect="90,690,200,710" color="#00FF00" title="Reviewer"
               coords="90,710,200,710,90,690,200,690"/>
    <square page="0" rect="300,300,400,400" color="#FF0000" width="2" title="Other"/>
    <ink page="0" rect="50,50,150,150" color="#0000FF" title="Other">
      <inklist><gesture>50,50;100,100;150,50</gesture></inklist>
    </ink>
    <line page="0" rect="10,10,200,200" start="10,10" end="200,200" head="OpenArrow" title="Other"/>
    <text page="5" rect="1,1,2,2" title="Off the end"/>
  </annots>
</xfdf>
)";

}  // namespace

TEST(Section2Annotations, XfdfImport) {
    const auto xfdf = TmpPath("xfdf_in") + ".xfdf";
    Write(xfdf, kXfdf);
    const auto out = TmpPath("xfdf_out");
    {
        Document doc{HelloWorldPdf()};
        PdfAnnotationEditor editor{doc};
        editor.ImportAnnotationsFromXfdf(xfdf);
        doc.Save(out);
    }
    const std::string bytes = ReadFile(out);
    EXPECT_NE(bytes.find("/QuadPoints"), std::string::npos);
    EXPECT_NE(bytes.find("/InkList"), std::string::npos);
    EXPECT_NE(bytes.find("/Popup"), std::string::npos);
    Document doc{out};
    const auto types = Types(doc, 1);
    auto count = [&](AnnotationType t) { return std::count(types.begin(), types.end(), t); };
    EXPECT_EQ(count(AnnotationType::Text), 1);
    EXPECT_EQ(count(AnnotationType::Highlight), 1);
    EXPECT_EQ(count(AnnotationType::Square), 1);
    // Ink and Line are not modelled by the annotation collection: check the file.
    EXPECT_NE(bytes.find("/Subtype /Ink"), std::string::npos);
    EXPECT_NE(bytes.find("/Subtype /Line"), std::string::npos);
    EXPECT_NE(bytes.find("/OpenArrow"), std::string::npos);
    auto& annots = doc.Pages()[1].Annotations();
    for (int i = 0; i < annots.Count(); ++i)
        if (annots[i].AnnotationType() == AnnotationType::Text) {
            EXPECT_EQ(annots[i].Contents(), "Please check & fix");
            auto* m = dynamic_cast<Annotations::MarkupAnnotation*>(&annots[i]);
            ASSERT_NE(m, nullptr);
            EXPECT_EQ(m->Title(), "Reviewer");
        }
}

TEST(Section2Annotations, XfdfImportFilteredByType) {
    const auto xfdf = TmpPath("xfdf_filter") + ".xfdf";
    Write(xfdf, kXfdf);
    const auto out = TmpPath("xfdf_filter_out");
    {
        Document doc{HelloWorldPdf()};
        PdfAnnotationEditor editor{doc};
        editor.ImportAnnotationFromXfdf(xfdf, {AnnotationType::Highlight, AnnotationType::Square});
        doc.Save(out);
    }
    Document doc{out};
    const auto types = Types(doc, 1);
    ASSERT_EQ(types.size(), 2u);
    EXPECT_EQ(std::count(types.begin(), types.end(), AnnotationType::Text), 0);

    Write(xfdf, "<notxfdf/>");
    Document again{HelloWorldPdf()};
    PdfAnnotationEditor editor{again};
    EXPECT_THROW(editor.ImportAnnotationsFromXfdf(xfdf), std::runtime_error);
    EXPECT_THROW(editor.ImportAnnotationsFromXfdf(TmpPath("missing") + ".xfdf"), std::runtime_error);
}

TEST(Section2Annotations, FdfImportWithoutXref) {
    const auto fdf = TmpPath("fdf_in") + ".fdf";
    Write(fdf,
          "%FDF-1.2\n"
          "1 0 obj\n<< /FDF << /Annots [2 0 R] >> >>\nendobj\n"
          "2 0 obj\n<< /Type /Annot /Subtype /Square /Page 0 /Rect [100 100 200 200] "
          "/C [1 0 0] /T (Fdf Author) /Contents (from fdf) >>\nendobj\n"
          "trailer\n<< /Root 1 0 R >>\n%%EOF\n");
    const auto out = TmpPath("fdf_out");
    {
        Document doc{HelloWorldPdf()};
        PdfAnnotationEditor editor{doc};
        editor.ImportAnnotationsFromFdf(fdf);
        doc.Save(out);
    }
    Document doc{out};
    auto& annots = doc.Pages()[1].Annotations();
    ASSERT_EQ(annots.Count(), 1);
    EXPECT_EQ(annots[0].AnnotationType(), AnnotationType::Square);
    EXPECT_EQ(annots[0].Contents(), "from fdf");
    EXPECT_EQ(ReadFile(out).find("/Page 0"), std::string::npos);
}

TEST(Section2Annotations, ImportFromOtherPdfs) {
    const auto src = TmpPath("annot_src");
    {
        Document doc{HelloWorldPdf()};
        Annotations::TextAnnotation t{doc};
        t.Rect(Rectangle{100, 700, 120, 720, false});
        t.Contents("from pdf");
        doc.Pages()[1].Annotations().Add(t);
        auto page = doc.Pages()[1];
        Annotations::SquareAnnotation sq{page, Rectangle{200, 200, 300, 300, false}};
        doc.Pages()[1].Annotations().Add(sq);
        doc.Save(src);
    }
    const auto all = TmpPath("annot_all");
    const auto only = TmpPath("annot_only_square");
    {
        Document doc{HelloWorldPdf()};
        PdfAnnotationEditor editor{doc};
        editor.ImportAnnotations({src});
        doc.Save(all);
    }
    {
        Document doc{HelloWorldPdf()};
        PdfAnnotationEditor editor{doc};
        editor.ImportAnnotations({src}, {AnnotationType::Square});
        doc.Save(only);
    }
    Document a{all};
    EXPECT_EQ(Types(a, 1).size(), 2u);
    Document o{only};
    EXPECT_EQ(Types(o, 1), (std::vector<AnnotationType>{AnnotationType::Square}));
}

TEST(Section2Annotations, ModifyAnnotationsAndAuthor) {
    const auto src = TmpPath("modify_src");
    {
        Document doc{HelloWorldPdf()};
        // The collection keeps pointers: the annotations live until Save.
        Annotations::TextAnnotation ann{doc}, bob{doc};
        for (auto* t : {&ann, &bob}) {
            t->Rect(Rectangle{100, 700, 120, 720, false});
            t->Contents("old");
            doc.Pages()[1].Annotations().Add(*t);
        }
        ann.Title("Ann");
        bob.Title("Bob");
        doc.Save(src);
    }
    const auto out = TmpPath("modify_out");
    {
        Document doc{src};
        PdfAnnotationEditor editor{doc};
        editor.ModifyAnnotationsAuthor(1, 1, "Ann", "Anna");
        Annotations::TextAnnotation tpl{doc};
        tpl.Contents("new");
        tpl.Title("Editor");
        tpl.Color(Color::FromRgb(1.0, 0.0, 0.0));
        editor.ModifyAnnotations(1, 1, tpl);
        doc.Save(out);
    }
    Document doc{out};
    auto& annots = doc.Pages()[1].Annotations();
    ASSERT_EQ(annots.Count(), 2);
    for (int i = 0; i < 2; ++i) {
        EXPECT_EQ(annots[i].Contents(), "new");
        EXPECT_EQ(dynamic_cast<Annotations::MarkupAnnotation&>(annots[i]).Title(), "Editor");
    }

    const auto renamed = TmpPath("modify_author");
    {
        Document d{src};
        PdfAnnotationEditor editor{d};
        editor.ModifyAnnotationsAuthor(1, 1, "Ann", "Anna");
        d.Save(renamed);
    }
    Document r{renamed};
    auto& ra = r.Pages()[1].Annotations();
    std::vector<std::string> authors;
    for (int i = 0; i < ra.Count(); ++i)
        authors.push_back(dynamic_cast<Annotations::MarkupAnnotation&>(ra[i]).Title());
    EXPECT_EQ(authors, (std::vector<std::string>{"Anna", "Bob"}));
}

// ---- security ----------------------------------------------------------------

TEST(Section2Security, SetPrivilegeWithoutPasswords) {
    const auto out = TmpPath("setpriv_nopw");
    PdfFileSecurity sec{HelloWorldPdf(), out};
    EXPECT_TRUE(sec.SetPrivilege(DocumentPrivilege::ForbidAll()));
    Document doc{out};  // opens without a password
    EXPECT_TRUE(doc.IsEncrypted());
}
