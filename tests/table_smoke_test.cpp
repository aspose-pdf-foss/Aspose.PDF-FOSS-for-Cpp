// =============================================================================
// table_smoke_test — beat C-4a of the tables cluster. The Table class: a
// BaseParagraph holding Rows plus column widths, borders and default cell
// styling. (Page placement + render-on-save land with C-4b.)
// =============================================================================

#include <aspose/pdf/base_paragraph.hpp>
#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/border_side.hpp>
#include <aspose/pdf/cell.hpp>
#include <aspose/pdf/cells.hpp>
#include <aspose/pdf/row.hpp>
#include <aspose/pdf/rows.hpp>
#include <aspose/pdf/table.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

}  // namespace

TEST(TableSmoke, BuildGridAndProperties) {
    Table table;
    table.ColumnWidths("100 100 100");
    table.Border(BorderInfo{BorderSide::All, 1.0f});
    table.DefaultCellBorder(BorderInfo{BorderSide::All});

    // Build a 2x3 grid.
    for (int r = 0; r < 2; ++r) {
        Row& row = table.Rows().Add();
        row.Cells().Add("a");
        row.Cells().Add("b");
        row.Cells().Add("c");
    }

    EXPECT_EQ(table.ColumnWidths(), "100 100 100");
    ASSERT_EQ(table.Rows().Count(), 2);
    EXPECT_EQ(table.Rows()[1].Cells().Count(), 3);
    EXPECT_FLOAT_EQ(table.Border().Top().LineWidth(), 1.0f);

    // Table is a BaseParagraph.
    BaseParagraph& bp = table;
    bp.IsInNewPage(true);
    EXPECT_TRUE(bp.IsInNewPage());

    table.Left(72.0f);
    table.Top(700.0f);
    EXPECT_FLOAT_EQ(table.Left(), 72.0f);
    EXPECT_TRUE(table.IsBordersIncluded());
}
