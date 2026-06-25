// =============================================================================
// table_break_smoke_test — beat C-5 of the tables cluster. A broken table
// (Table::IsBroken) taller than the page flows its overflow rows onto a
// newly-created continuation page at Save, repeating the first
// RepeatingRowsCount rows as a header.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/border_side.hpp>
#include <aspose/pdf/cell.hpp>
#include <aspose/pdf/cells.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/paragraphs.hpp>
#include <aspose/pdf/row.hpp>
#include <aspose/pdf/rows.hpp>
#include <aspose/pdf/table.hpp>
#include <aspose/pdf/text_fragment_absorber.hpp>
#include <aspose/pdf/text_fragment_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Text::TextFragmentAbsorber;

std::string HelloWorldPdf() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return (std::filesystem::path(env) / "hello_world.pdf").string();
    return (std::filesystem::path(__FILE__).parent_path().parent_path() /
            "pdfs" / "hello_world.pdf")
        .string();
}

void FillRows(Table& table, int n) {
    table.ColumnWidths("200");
    table.DefaultCellBorder(BorderInfo{BorderSide::All, 1.0f});
    for (int i = 1; i <= n; ++i) {
        Row& row = table.Rows().Add();
        row.Cells().Add("Row" + std::to_string(i));
    }
}

}  // namespace

// A broken table with more rows than fit on one page creates a continuation
// page; the late rows land there.
TEST(TableBreakSmoke, BrokenTableFlowsToNewPage) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_tbreak.pdf").string();
    {
        Document doc{HelloWorldPdf()};
        ASSERT_EQ(doc.Pages().Count(), 1);
        Table table;
        table.IsBroken(true);
        table.RepeatingRowsCount(1);
        FillRows(table, 70);
        doc.Pages()[1].Paragraphs().Add(table);  // table outlives Save
        doc.Save(out);
    }

    Document doc{out};
    EXPECT_GE(doc.Pages().Count(), 2);  // a continuation page was created

    TextFragmentAbsorber late{"Row70"};
    late.Visit(doc);
    EXPECT_GE(late.TextFragments().Count(), 1);  // late row flowed over
    std::filesystem::remove(out);
}

// Without IsBroken the table stays on its page (no continuation page).
TEST(TableBreakSmoke, UnbrokenTableStaysSinglePage) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_tnobrk.pdf").string();
    {
        Document doc{HelloWorldPdf()};
        Table table;  // IsBroken defaults to false
        FillRows(table, 70);
        doc.Pages()[1].Paragraphs().Add(table);
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), 1);
    std::filesystem::remove(out);
}

TEST(TableBreakSmoke, IsBrokenProperty) {
    Table table;
    EXPECT_FALSE(table.IsBroken());
    table.IsBroken(true);
    EXPECT_TRUE(table.IsBroken());
}
