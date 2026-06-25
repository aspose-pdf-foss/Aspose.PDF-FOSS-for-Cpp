// =============================================================================
// facades_pdf_bookmark_editor_smoke_test — beat Fa6 of the Facades
// cluster. PdfBookmarkEditor + the Bookmark / Bookmarks value types.
// Bookmark/Bookmarks are real value types; CreateBookmarks/Extract are
// now REAL (parity gap 6) — staged /Outlines write at Save + readback
// via foundation::outlines. HTML/XML import/export remain stubs.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/bookmark.hpp>
#include <aspose/pdf/facades/bookmarks.hpp>
#include <aspose/pdf/facades/pdf_bookmark_editor.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::string TwoPagesPdf() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "text_extractor" / "two_pages.pdf").string();
}

std::string BmTmp(const std::string& n) {
    return (std::filesystem::temp_directory_path() /
            ("aspose_bookmark_" + n)).string();
}

}  // namespace

TEST(FacadesBookmarkSmoke, BookmarkPodDefaultsAndRoundtrip) {
    Bookmark bm;
    EXPECT_TRUE(bm.Title().empty());
    EXPECT_EQ(bm.PageNumber(), 1);
    EXPECT_EQ(bm.Level(), 1);
    EXPECT_FALSE(bm.BoldFlag());
    EXPECT_FALSE(bm.ItalicFlag());
    EXPECT_FALSE(bm.Open());

    bm.Title("Chapter 1");
    bm.PageNumber(5);
    bm.Level(2);
    bm.BoldFlag(true);
    bm.ItalicFlag(true);
    bm.Open(true);
    bm.Action("GoTo");
    bm.Destination("dest1");
    bm.PageDisplay("FitH");
    bm.PageDisplay_Left(10);
    bm.PageDisplay_Top(700);
    bm.PageDisplay_Zoom(100);
    bm.RemoteFile("other.pdf");

    EXPECT_EQ(bm.Title(), "Chapter 1");
    EXPECT_EQ(bm.PageNumber(), 5);
    EXPECT_EQ(bm.Level(), 2);
    EXPECT_TRUE(bm.BoldFlag());
    EXPECT_TRUE(bm.ItalicFlag());
    EXPECT_TRUE(bm.Open());
    EXPECT_EQ(bm.Action(), "GoTo");
    EXPECT_EQ(bm.Destination(), "dest1");
    EXPECT_EQ(bm.PageDisplay(), "FitH");
    EXPECT_EQ(bm.PageDisplay_Left(), 10);
    EXPECT_EQ(bm.PageDisplay_Top(), 700);
    EXPECT_EQ(bm.PageDisplay_Zoom(), 100);
    EXPECT_EQ(bm.RemoteFile(), "other.pdf");
}

TEST(FacadesBookmarkSmoke, BookmarksCollectionIsAVector) {
    Bookmarks list;
    EXPECT_TRUE(list.empty());

    Bookmark a;
    a.Title("A");
    Bookmark b;
    b.Title("B");
    list.push_back(a);
    list.push_back(b);

    ASSERT_EQ(list.size(), 2u);
    EXPECT_EQ(list[0].Title(), "A");
    EXPECT_EQ(list[1].Title(), "B");
}

TEST(FacadesBookmarkSmoke, ChildItemsRoundtrip) {
    Bookmark parent;
    parent.Title("Parent");

    Bookmarks children;
    Bookmark child;
    child.Title("Child");
    children.push_back(child);

    parent.ChildItems(children);
    Bookmarks back = parent.ChildItems();
    ASSERT_EQ(back.size(), 1u);
    EXPECT_EQ(back[0].Title(), "Child");

    // ChildItem is a canonical alias over the same backing collection.
    EXPECT_EQ(parent.ChildItem().size(), 1u);
}

TEST(FacadesBookmarkSmoke, EditorStubsDoNotThrow) {
    Document doc{HelloWorldPdf()};
    PdfBookmarkEditor editor{doc};

    Bookmark bm;
    bm.Title("Intro");
    bm.PageNumber(1);

    editor.CreateBookmarks();
    editor.CreateBookmarks(bm);
    editor.CreateBookmarkOfPage("Intro", 1);
    editor.CreateBookmarkOfPage(std::vector<std::string>{"A", "B"},
                                std::vector<int>{1, 2});
    editor.ModifyBookmarks("old", "new");
    editor.DeleteBookmarks("Intro");
    editor.DeleteBookmarks();
    editor.ExportBookmarksToXML("out.xml");
    editor.ImportBookmarksWithXML("in.xml");
    editor.ExportBookmarksToHtml("dir", "out.html");
    editor.ExtractBookmarksToHTML("dir", "out.html");
    SUCCEED();
}

TEST(FacadesBookmarkSmoke, ExtractReturnsEmptyBookmarks) {
    Document doc{HelloWorldPdf()};
    PdfBookmarkEditor editor{doc};
    EXPECT_TRUE(editor.ExtractBookmarks().empty());
    EXPECT_TRUE(editor.ExtractBookmarks(true).empty());
    EXPECT_TRUE(editor.ExtractBookmarks("Intro").empty());

    Bookmark parent;
    EXPECT_TRUE(editor.ExtractBookmarks(parent).empty());
}


TEST(PdfBookmarkEditorRealSmoke, CreateSaveReloadExtract) {
    const std::string out = BmTmp("create.pdf");
    {
        Document doc{TwoPagesPdf()};
        PdfBookmarkEditor ed{doc};
        ed.CreateBookmarkOfPage("Chapter 1", 1);
        ed.CreateBookmarkOfPage("Chapter 2", 2);
        ed.Save(out);
    }
    Document re{out};
    PdfBookmarkEditor ed2{re};
    Bookmarks bms = ed2.ExtractBookmarks();
    ASSERT_GE(bms.size(), 2u);
    EXPECT_EQ(bms[0].Title(), "Chapter 1");
    EXPECT_EQ(bms[1].Title(), "Chapter 2");
    std::filesystem::remove(out);
}

TEST(PdfBookmarkEditorRealSmoke, NestedBookmarksRoundTrip) {
    const std::string out = BmTmp("nested.pdf");
    {
        Document doc{TwoPagesPdf()};
        PdfBookmarkEditor ed{doc};
        Bookmark parent;
        parent.Title("Part I");
        parent.PageNumber(1);
        Bookmarks kids;
        Bookmark child;
        child.Title("Section 1.1");
        child.PageNumber(2);
        kids.push_back(child);
        parent.ChildItems(kids);
        ed.CreateBookmarks(parent);
        ed.Save(out);
    }
    Document re{out};
    PdfBookmarkEditor ed2{re};
    Bookmarks bms = ed2.ExtractBookmarks();
    ASSERT_GE(bms.size(), 2u);
    EXPECT_EQ(bms[0].Title(), "Part I");
    EXPECT_EQ(bms[1].Title(), "Section 1.1");
    EXPECT_GT(bms[1].Level(), bms[0].Level());
    std::filesystem::remove(out);
}

TEST(PdfBookmarkEditorRealSmoke, DeleteClearsOutlines) {
    const std::string out = BmTmp("del.pdf");
    {
        Document doc{TwoPagesPdf()};
        PdfBookmarkEditor ed{doc};
        ed.CreateBookmarkOfPage("Temp", 1);
        ed.DeleteBookmarks();
        ed.Save(out);
    }
    Document re{out};
    PdfBookmarkEditor ed2{re};
    EXPECT_TRUE(ed2.ExtractBookmarks().empty());
    std::filesystem::remove(out);
}
