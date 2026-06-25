// =============================================================================
// Go-parity: table_test.go → canonical C++ Table / Row / Cell + Page.Paragraphs.
//
// Ported (canonical): empty table, add rows + cells, column widths, defaults
// (RepeatingRowsCount, Cell.ColSpan), cell text round-trip through Save,
// IsBroken overflow adds a continuation page (C-5), and an image cell embeds an
// XObject (C-6).
// Skipped: Go content-stream byte assertions (BackgroundFillEmitted,
// BorderSidesMask, dedup, SerializeContentOps), builder-chaining return-value
// tests, and Go-specific Add-validation error cases.
// =============================================================================

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/border_side.hpp>
#include <aspose/pdf/cell.hpp>
#include <aspose/pdf/cells.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/paragraphs.hpp>
#include <aspose/pdf/resources.hpp>
#include <aspose/pdf/row.hpp>
#include <aspose/pdf/rows.hpp>
#include <aspose/pdf/table.hpp>
#include <aspose/pdf/text_fragment_absorber.hpp>
#include <aspose/pdf/text_fragment_collection.hpp>
#include <aspose/pdf/x_image_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Text::TextFragmentAbsorber;

std::filesystem::path TestsRoot() {
    return std::filesystem::path(__FILE__).parent_path().parent_path();
}
std::string HelloWorld() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return (std::filesystem::path(env) / "hello_world.pdf").string();
    return (TestsRoot().parent_path() / "pdfs" / "hello_world.pdf").string();
}
std::string Jpeg() {
    return (TestsRoot() / "fixtures" / "dctdecode" / "gradient_32.jpg").string();
}
std::vector<std::byte> ReadAllBytes(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    const std::string raw((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
    std::vector<std::byte> out(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i)
        out[i] = static_cast<std::byte>(static_cast<unsigned char>(raw[i]));
    return out;
}
std::string TmpOut(const std::string& tag) {
    return (std::filesystem::temp_directory_path() / ("go_tbl_" + tag + ".pdf"))
        .string();
}

}  // namespace

// TestTable_NewIsEmpty / TestTable_RepeatingRowsCountDefault / Cell defaults.
TEST(GoTable, DefaultsAndConstruction) {
    Table t;
    EXPECT_EQ(t.Rows().Count(), 0);
    EXPECT_EQ(t.RepeatingRowsCount(), 0);
    EXPECT_FALSE(t.IsBroken());
    Cell c;
    EXPECT_EQ(c.ColSpan(), 1);
    EXPECT_TRUE(c.BackgroundImage().empty());
}

// TestTable_AddRowAndCells / TestRow_AddCellsConvenience.
TEST(GoTable, AddRowsAndCells) {
    Table t;
    Row& r = t.Rows().Add();
    r.Cells().Add("a");
    r.Cells().Add("b");
    EXPECT_EQ(t.Rows().Count(), 1);
    EXPECT_EQ(t.Rows()[1].Cells().Count(), 2);
}

// TestAddTable_RoundTripText — cell text survives placement + save/reload.
TEST(GoTable, CellTextRoundTrip) {
    const std::string out = TmpOut("text");
    {
        Document doc{HelloWorld()};
        Table t;
        t.ColumnWidths("80 80");
        Row& r = t.Rows().Add();
        r.Cells().Add("GoCellAlpha");
        r.Cells().Add("GoCellBeta");
        doc.Pages()[1].Paragraphs().Add(t);
        doc.Save(out);
    }
    TextFragmentAbsorber a{"GoCellAlpha"};
    Document doc{out};
    a.Visit(doc);
    EXPECT_GE(a.TextFragments().Count(), 1);
    std::filesystem::remove(out);
}

// TestAddTable_OverflowAddsPage — broken table overflows onto a new page (C-5).
TEST(GoTable, OverflowAddsPage) {
    const std::string out = TmpOut("overflow");
    {
        Document doc{HelloWorld()};
        ASSERT_EQ(doc.Pages().Count(), 1);
        Table t;
        t.IsBroken(true);
        t.ColumnWidths("200");
        t.DefaultCellBorder(BorderInfo{BorderSide::All, 1.0f});
        for (int i = 1; i <= 70; ++i) t.Rows().Add().Cells().Add("R");
        doc.Pages()[1].Paragraphs().Add(t);
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_GE(doc.Pages().Count(), 2);
    std::filesystem::remove(out);
}

// TestAddTable_ImageCellRendered — a cell background image embeds (C-6).
TEST(GoTable, ImageCellEmbeds) {
    const std::string out = TmpOut("imgcell");
    {
        Document doc{HelloWorld()};
        Table t;
        t.ColumnWidths("120");
        Cell& c = t.Rows().Add().Cells().Add();
        c.BackgroundImage(ReadAllBytes(Jpeg()));
        doc.Pages()[1].Paragraphs().Add(t);
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_GE(doc.Pages()[1].Resources().Images().Count(), 1);
    std::filesystem::remove(out);
}
