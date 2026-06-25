// =============================================================================
// outlines_smoke_test — beat N-4a of the navigation cluster. The public outline
// tree: Outlines (base), OutlineCollection (the /Outlines root) and
// OutlineItemCollection (a node + child collection), plus Document::Outlines().
// Covers in-memory tree building / navigation (Count / First / Last / Next /
// Prev / Parent / Level / VisibleCount / Title / Bold / Italic / Open /
// Destination) and load-on-open parsing of an existing /Outlines tree.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/xyz_explicit_destination.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_bookmark_editor.hpp>
#include <aspose/pdf/outline_collection.hpp>
#include <aspose/pdf/outline_item_collection.hpp>
#include <aspose/pdf/outlines.hpp>
#include <aspose/pdf/page_collection.hpp>

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

}  // namespace

TEST(OutlinesSmoke, BuildTraverseAndNavigate) {
    Document doc{(FixtureRoot() / "two_pages.pdf").string()};
    OutlineCollection& root = doc.Outlines();
    EXPECT_EQ(root.Count(), 0);

    OutlineItemCollection a{root};
    a.Title("Chapter 1");
    a.Bold(true);
    root.Add(a);
    OutlineItemCollection b{root};
    b.Title("Chapter 2");
    b.Italic(true);
    root.Add(b);
    ASSERT_EQ(root.Count(), 2);

    EXPECT_EQ(root[1].Title(), "Chapter 1");
    EXPECT_TRUE(root[1].Bold());
    EXPECT_FALSE(root[1].Italic());
    EXPECT_TRUE(root[2].Italic());
    EXPECT_EQ(root.First()->Title(), "Chapter 1");
    EXPECT_EQ(root.Last()->Title(), "Chapter 2");

    // Sibling navigation.
    EXPECT_EQ(root[1].Next()->Title(), "Chapter 2");
    EXPECT_EQ(root[2].Prev()->Title(), "Chapter 1");
    EXPECT_TRUE(root[1].HasNext());
    EXPECT_FALSE(root[2].HasNext());
    EXPECT_EQ(root[1].Level(), 1);
    EXPECT_EQ(root[1].Parent(), static_cast<Outlines*>(&root));

    // Nesting.
    OutlineItemCollection sec{root};
    sec.Title("Section 1.1");
    root[1].Add(sec);
    ASSERT_EQ(root[1].Count(), 1);
    OutlineItemCollection* child = root[1].First();
    ASSERT_NE(child, nullptr);
    EXPECT_EQ(child->Title(), "Section 1.1");
    EXPECT_EQ(child->Level(), 2);
    EXPECT_EQ(child->Parent(), static_cast<Outlines*>(&root[1]));

    // VisibleCount: 2 top-level + 1 visible descendant of the open Chapter 1.
    EXPECT_EQ(root.VisibleCount(), 3);
    root[1].Open(false);
    EXPECT_EQ(root.VisibleCount(), 2);

    // A node targeting an explicit destination.
    OutlineItemCollection jump{root};
    jump.Title("Jump");
    jump.Destination(XYZExplicitDestination{1, 0.0, 700.0, 1.0});
    root.Add(jump);
    ASSERT_NE(root[3].Destination(), nullptr);
    EXPECT_EQ(root[3].Destination()->ToString(), "1 XYZ 0 700 1");
}

TEST(OutlinesSmoke, LoadExistingOutlineTree) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_outlines_load.pdf")
            .string();

    // Write a /Outlines tree via the bookmark editor (staged-outline save).
    {
        Document doc{(FixtureRoot() / "two_pages.pdf").string()};
        Facades::PdfBookmarkEditor editor{doc};
        editor.CreateBookmarkOfPage("Chapter 1", 1);
        editor.CreateBookmarkOfPage("Chapter 2", 2);
        doc.Save(out);
    }

    // Read it back through the public outline tree.
    {
        Document doc{out};
        OutlineCollection& root = doc.Outlines();
        ASSERT_GE(root.Count(), 2);

        std::vector<std::string> titles;
        for (int i = 1; i <= root.Count(); ++i)
            titles.push_back(root[i].Title());
        EXPECT_NE(std::find(titles.begin(), titles.end(), "Chapter 1"),
                  titles.end());
        EXPECT_NE(std::find(titles.begin(), titles.end(), "Chapter 2"),
                  titles.end());

        // The loaded nodes carry a navigation target parsed from /Dest.
        EXPECT_NE(root.First()->Destination(), nullptr);
    }

    std::filesystem::remove(out);
}
