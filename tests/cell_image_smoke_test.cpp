// =============================================================================
// cell_image_smoke_test — beat C-6 of the tables cluster. A table cell's
// background image (Cell::BackgroundImage bytes, or BackgroundImageFile path)
// is embedded as an /XObject and drawn within the cell at Save.
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
#include <aspose/pdf/x_image_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return std::filesystem::path(env);
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string JpegFixture() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "dctdecode" / "gradient_32.jpg")
        .string();
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

}  // namespace

// A cell background image (bytes) is embedded as a page image XObject.
TEST(CellImageSmoke, BackgroundImageBytesEmbed) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_cellimg.pdf").string();
    const std::vector<std::byte> jpeg = ReadAllBytes(JpegFixture());
    ASSERT_GE(jpeg.size(), 2u);
    {
        Document doc{(FixtureRoot() / "hello_world.pdf").string()};
        Table table;
        table.Left(72.0f);
        table.Top(700.0f);
        table.ColumnWidths("120 120");
        table.DefaultCellBorder(BorderInfo{BorderSide::All, 1.0f});

        Row& row = table.Rows().Add();
        Cell& imgCell = row.Cells().Add();
        imgCell.BackgroundImage(jpeg);
        row.Cells().Add("label");

        doc.Pages()[1].Paragraphs().Add(table);  // table outlives Save
        doc.Save(out);
    }

    Document doc{out};
    // The cell image is now an image resource of the page.
    EXPECT_GE(doc.Pages()[1].Resources().Images().Count(), 1);
    std::filesystem::remove(out);
}

// A cell background image supplied by file path also embeds.
TEST(CellImageSmoke, BackgroundImageFileEmbeds) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_cellimgf.pdf")
            .string();
    {
        Document doc{(FixtureRoot() / "hello_world.pdf").string()};
        Table table;
        table.ColumnWidths("150");
        Row& row = table.Rows().Add();
        Cell& cell = row.Cells().Add();
        cell.BackgroundImageFile(JpegFixture());
        doc.Pages()[1].Paragraphs().Add(table);
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_GE(doc.Pages()[1].Resources().Images().Count(), 1);
    std::filesystem::remove(out);
}

// BackgroundImage round-trips on the in-memory object.
TEST(CellImageSmoke, BackgroundImageProperty) {
    Cell cell;
    EXPECT_TRUE(cell.BackgroundImage().empty());
    const std::vector<std::byte> bytes{std::byte{1}, std::byte{2}, std::byte{3}};
    cell.BackgroundImage(bytes);
    EXPECT_EQ(cell.BackgroundImage().size(), 3u);
}
