// =============================================================================
// cells_smoke_test — beat C-2 of the tables cluster. Cell and the Cells
// collection: cell properties (border / background / span / alignment) and the
// Add overloads that create text-filled cells.
// =============================================================================

#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/border_side.hpp>
#include <aspose/pdf/cell.hpp>
#include <aspose/pdf/cells.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

}  // namespace

TEST(CellSmoke, Defaults) {
    Cell c;
    EXPECT_FALSE(c.IsNoBorder());
    EXPECT_EQ(c.ColSpan(), 1);
    EXPECT_EQ(c.RowSpan(), 1);
    EXPECT_TRUE(c.IsWordWrapped());
    EXPECT_EQ(c.Alignment(), HorizontalAlignment::Left);

    Cell sized{Rectangle{0.0, 0.0, 120.0, 40.0, false}};
    EXPECT_DOUBLE_EQ(sized.Width(), 120.0);
}

TEST(CellSmoke, Properties) {
    Cell c;
    c.Border(BorderInfo{BorderSide::All, 1.5f});
    c.BackgroundColor(Color::FromRgb(0.9, 0.9, 0.9));
    c.Alignment(HorizontalAlignment::Center);
    c.ColSpan(2);
    c.RowSpan(3);
    c.IsNoBorder(true);

    EXPECT_FLOAT_EQ(c.Border().Top().LineWidth(), 1.5f);
    EXPECT_DOUBLE_EQ(c.BackgroundColor().A(), 1.0);
    EXPECT_EQ(c.Alignment(), HorizontalAlignment::Center);
    EXPECT_EQ(c.ColSpan(), 2);
    EXPECT_EQ(c.RowSpan(), 3);
    EXPECT_TRUE(c.IsNoBorder());
}

TEST(CellsSmoke, AddOverloadsAndIndexer) {
    Cells cells;
    EXPECT_EQ(cells.Count(), 0);

    cells.Add();
    cells.Add("hello");
    Cell& styled = cells.Add("world");
    styled.Alignment(HorizontalAlignment::Right);

    ASSERT_EQ(cells.Count(), 3);
    // 1-based indexer returns the same stored cells.
    EXPECT_EQ(cells[3].Alignment(), HorizontalAlignment::Right);

    cells.RemoveRange(1, 1);  // drop the first empty cell
    EXPECT_EQ(cells.Count(), 2);
}
