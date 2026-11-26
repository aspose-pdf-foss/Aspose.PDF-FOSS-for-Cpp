// =============================================================================
// backlog_silent_stubs_smoke_test — former silent stubs that now answer for
// real: PdfFileInfo password state and /Collection, PdfFileStamp page size,
// and the PdfBookmarkEditor::ExtractBookmarks overloads.
// =============================================================================

#include <filesystem>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_bookmark_editor.hpp>
#include <aspose/pdf/facades/pdf_file_info.hpp>
#include <aspose/pdf/facades/pdf_file_stamp.hpp>
#include <aspose/pdf/password_type.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;
using flatten_test::TmpPath;
using flatten_test::WritePdf;

// qpdf-made fixtures (tests/fixtures/encrypted/generate.sh).
std::string Encrypted(const std::string& name) {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" / "encrypted" / name)
        .string();
}

}  // namespace

TEST(BacklogSilentStubs, PasswordStateWithUserPassword) {
    for (const char* name : {"user_owner_aes256.pdf", "user_owner_rc4.pdf"}) {
        {
            Document doc{Encrypted(name), "user"};
            PdfFileInfo info{doc};
            EXPECT_TRUE(info.HasOpenPassword()) << name;
            EXPECT_TRUE(info.HasEditPassword()) << name;
            EXPECT_EQ(info.PasswordType(), PasswordType::User) << name;
        }
        {
            Document doc{Encrypted(name), "owner"};
            PdfFileInfo info{doc};
            EXPECT_TRUE(info.HasOpenPassword()) << name;
            EXPECT_EQ(info.PasswordType(), PasswordType::Owner) << name;
        }
    }
}

TEST(BacklogSilentStubs, PasswordStateWithoutUserPassword) {
    Document doc{Encrypted("blank_user_aes128.pdf"), ""};
    PdfFileInfo info{doc};
    EXPECT_FALSE(info.HasOpenPassword());
    EXPECT_TRUE(info.HasEditPassword());
    EXPECT_EQ(info.PasswordType(), PasswordType::User);
}

TEST(BacklogSilentStubs, UnencryptedHasNoPasswords) {
    Document doc{flatten_test::HelloWorldPdf()};
    PdfFileInfo info{doc};
    EXPECT_FALSE(info.HasOpenPassword());
    EXPECT_FALSE(info.HasEditPassword());
    EXPECT_EQ(info.PasswordType(), PasswordType::None);
    EXPECT_FALSE(info.HasCollection());
}

TEST(BacklogSilentStubs, HasCollection) {
    const auto path = TmpPath("collection");
    WritePdf(path, {
        "<< /Type /Catalog /Pages 2 0 R /Collection << /Type /Collection >> >>",
        "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 200 300] >>",
    });
    Document doc{path};
    PdfFileInfo info{doc};
    EXPECT_TRUE(info.HasCollection());
}

TEST(BacklogSilentStubs, FileStampPageSizeIsTheFirstPages) {
    const auto path = TmpPath("stamp_size");
    WritePdf(path, {
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 420 595] >>",
    });
    Document doc{path};
    PdfFileStamp stamp{doc};
    EXPECT_FLOAT_EQ(stamp.PageWidth(), 420.0f);
    EXPECT_FLOAT_EQ(stamp.PageHeight(), 595.0f);
}

TEST(BacklogSilentStubs, ExtractBookmarksOverloads) {
    const auto path = TmpPath("bookmarks_tree");
    {
        Document doc{flatten_test::HelloWorldPdf()};
        PdfBookmarkEditor editor{doc};
        Bookmark a, a1, a2, b, b1;
        a.Title("A"); a.PageNumber(1);
        a1.Title("A1"); a1.PageNumber(1);
        a2.Title("A2"); a2.PageNumber(1);
        b.Title("B"); b.PageNumber(1);
        b1.Title("B1"); b1.PageNumber(1);
        Bookmarks ak, bk;
        ak.push_back(a1);
        ak.push_back(a2);
        bk.push_back(b1);
        a.ChildItems(ak);
        b.ChildItems(bk);
        editor.CreateBookmarks(a);
        editor.CreateBookmarks(b);
        doc.Save(path);
    }
    Document doc{path};
    PdfBookmarkEditor editor{doc};
    ASSERT_EQ(editor.ExtractBookmarks().size(), 5u);

    auto titles = [](const Bookmarks& bs) {
        std::vector<std::string> t;
        for (const auto& b : bs) t.push_back(b.Title());
        return t;
    };
    EXPECT_EQ(titles(editor.ExtractBookmarks(true)), (std::vector<std::string>{"A", "B"}));
    EXPECT_EQ(titles(editor.ExtractBookmarks(false)).size(), 5u);
    EXPECT_EQ(titles(editor.ExtractBookmarks(std::string("A"))),
              (std::vector<std::string>{"A", "A1", "A2"}));
    Bookmark parent;
    parent.Title("B");
    EXPECT_EQ(titles(editor.ExtractBookmarks(parent)), (std::vector<std::string>{"B1"}));
}
