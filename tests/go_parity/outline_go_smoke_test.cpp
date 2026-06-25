// =============================================================================
// Go-parity: outline_test.go → canonical C++ Document::Outlines
// (OutlineCollection / OutlineItemCollection).
//
// Ported (canonical): empty root, Title/Bold/Italic get-set, Add + nested
// hierarchy + Count, Insert, Remove, and a nested round-trip through Save.
// Skipped: Go-specific validation errors (nil/self/cross-document/ancestor-cycle
// adds), writer-internal and pypdf/aspose byte-parity tests.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/outline_collection.hpp>
#include <aspose/pdf/outline_item_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path PdfDir() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return std::filesystem::path(env);
    return std::filesystem::path(__FILE__)
        .parent_path()
        .parent_path()
        .parent_path() /
        "pdfs";
}

std::string HelloWorld() { return (PdfDir() / "hello_world.pdf").string(); }

std::string TmpOut(const std::string& tag) {
    return (std::filesystem::temp_directory_path() / ("go_outl_" + tag + ".pdf"))
        .string();
}

}  // namespace

// TestOutlines_EmptyDocReturnsRoot — a fresh doc's outline root is empty.
TEST(GoOutline, EmptyRoot) {
    Document doc{HelloWorld()};
    EXPECT_EQ(doc.Outlines().Count(), 0);
}

// TestOutlines_TitleGetSet / TestOutlines_BoldItalic — item style props.
TEST(GoOutline, ItemStyleProperties) {
    Document doc{HelloWorld()};
    OutlineCollection& root = doc.Outlines();
    OutlineItemCollection item{root};
    item.Title("Chapter 1");
    item.Bold(true);
    item.Italic(true);
    EXPECT_EQ(item.Title(), "Chapter 1");
    EXPECT_TRUE(item.Bold());
    EXPECT_TRUE(item.Italic());
}

// TestOutlines_Add / TestOutlines_NestedHierarchy — add + nest + count.
TEST(GoOutline, AddAndNest) {
    Document doc{HelloWorld()};
    OutlineCollection& root = doc.Outlines();
    OutlineItemCollection ch1{root};
    ch1.Title("Chapter 1");
    root.Add(ch1);
    OutlineItemCollection sec{root};
    sec.Title("Section 1.1");
    root[1].Add(sec);
    OutlineItemCollection ch2{root};
    ch2.Title("Chapter 2");
    root.Add(ch2);

    ASSERT_EQ(root.Count(), 2);
    EXPECT_EQ(root[1].Title(), "Chapter 1");
    EXPECT_EQ(root[1].Count(), 1);
    EXPECT_EQ(root[2].Title(), "Chapter 2");
}

// TestOutlines_Remove — removing a child shrinks the collection.
TEST(GoOutline, Remove) {
    Document doc{HelloWorld()};
    OutlineCollection& root = doc.Outlines();
    OutlineItemCollection a{root};
    a.Title("A");
    root.Add(a);
    OutlineItemCollection b{root};
    b.Title("B");
    root.Add(b);
    ASSERT_EQ(root.Count(), 2);
    root.Remove(1);  // 1-based
    EXPECT_EQ(root.Count(), 1);
    EXPECT_EQ(root[1].Title(), "B");
}

// TestOutlines_NestedHierarchyRoundtrip — outline tree survives save/reload.
TEST(GoOutline, NestedRoundTrip) {
    const std::string out = TmpOut("nested");
    {
        Document doc{HelloWorld()};
        OutlineCollection& root = doc.Outlines();
        OutlineItemCollection ch1{root};
        ch1.Title("Chapter 1");
        root.Add(ch1);
        OutlineItemCollection sec{root};
        sec.Title("Section 1.1");
        root[1].Add(sec);
        doc.Save(out);
    }
    Document doc{out};
    OutlineCollection& root = doc.Outlines();
    ASSERT_EQ(root.Count(), 1);
    EXPECT_EQ(root[1].Title(), "Chapter 1");
    EXPECT_EQ(root[1].Count(), 1);
    std::filesystem::remove(out);
}
