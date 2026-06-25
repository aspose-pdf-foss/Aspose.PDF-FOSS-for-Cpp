// =============================================================================
// rows_smoke_test — beat C-3 of the tables cluster. Row and the Rows
// collection: per-row styling, the row's Cells, and the Rows Add / indexer /
// IndexOf / Remove operations.
// =============================================================================

#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/border_side.hpp>
#include <aspose/pdf/cell.hpp>
#include <aspose/pdf/cells.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/row.hpp>
#include <aspose/pdf/rows.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

}  // namespace

TEST(RowSmoke, CellsAndProperties) {
    Row row;
    row.Cells().Add("a");
    row.Cells().Add("b");
    EXPECT_EQ(row.Cells().Count(), 2);

    row.BackgroundColor(Color::FromRgb(0.8, 0.8, 0.8));
    row.MinRowHeight(20.0);
    row.FixedRowHeight(25.0);
    row.Border(BorderInfo{BorderSide::All});
    EXPECT_DOUBLE_EQ(row.MinRowHeight(), 20.0);
    EXPECT_DOUBLE_EQ(row.FixedRowHeight(), 25.0);
    EXPECT_FLOAT_EQ(row.Border().Left().LineWidth(), 1.0f);
}

TEST(RowsSmoke, AddIndexerIndexOfRemove) {
    Rows rows;
    EXPECT_EQ(rows.Count(), 0);

    Row& r1 = rows.Add();
    r1.Cells().Add("r1c1");
    Row& r2 = rows.Add();
    r2.Cells().Add("r2c1");
    ASSERT_EQ(rows.Count(), 2);

    EXPECT_EQ(rows.IndexOf(r2), 2);
    EXPECT_EQ(rows[1].Cells().Count(), 1);

    rows.RemoveAt(1);
    EXPECT_EQ(rows.Count(), 1);
    EXPECT_EQ(rows.IndexOf(r1), 0);  // r1 removed
}
