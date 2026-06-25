// =============================================================================
// table_render_smoke_test — beat C-4b of the tables cluster. Page.Paragraphs +
// the render-on-save layout engine: a Table added to a page draws its grid
// (borders) and cell text into the page content stream.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
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

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string ReadAll(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)),
                       std::istreambuf_iterator<char>());
}

}  // namespace

TEST(TableRenderSmoke, RendersGridAndTextOnSave) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_table_render.pdf")
            .string();

    {
        Document doc{(FixtureRoot() / "two_pages.pdf").string()};
        Table table;
        table.Left(72.0f);
        table.Top(700.0f);
        table.ColumnWidths("80 80 80");
        table.DefaultCellBorder(BorderInfo{BorderSide::All, 1.0f});
        table.Border(BorderInfo{BorderSide::All, 1.5f});

        const char* labels[2][3] = {{"a1", "b1", "c1"}, {"a2", "b2", "c2"}};
        for (int r = 0; r < 2; ++r) {
            Row& row = table.Rows().Add();
            for (int c = 0; c < 3; ++c) row.Cells().Add(labels[r][c]);
        }
        ASSERT_EQ(table.Rows().Count(), 2);

        doc.Pages()[1].Paragraphs().Add(table);
        EXPECT_EQ(doc.Pages()[1].Paragraphs().Count(), 1);
        doc.Save(out);  // table is still alive here
    }

    // Reopen — structure intact.
    {
        Document doc{out};
        EXPECT_EQ(doc.Pages().Count(), 2);
    }

    // The content stream carries the table: a font resource, cell text show
    // operators, and stroked grid lines.
    const std::string bytes = ReadAll(out);
    EXPECT_NE(bytes.find("AspHelv"), std::string::npos);
    EXPECT_NE(bytes.find("(a1)"), std::string::npos);
    EXPECT_NE(bytes.find("(c2)"), std::string::npos);
    EXPECT_NE(bytes.find(" l "), std::string::npos);  // line-to (grid edge)
    EXPECT_NE(bytes.find(" Tj"), std::string::npos);  // text show

    std::filesystem::remove(out);
}
