// =============================================================================
// drawing_render_smoke_test — beat D-2 of the drawing cluster. Render-on-save:
// a Graph added to Page.Paragraphs draws its shapes (lines / rectangles /
// circles) into the page content stream.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/drawing/circle.hpp>
#include <aspose/pdf/drawing/graph.hpp>
#include <aspose/pdf/drawing/line.hpp>
#include <aspose/pdf/drawing/rectangle.hpp>
#include <aspose/pdf/graph_info.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/paragraphs.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Drawing;

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

TEST(DrawingRenderSmoke, RendersShapesOnSave) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_drawing_render.pdf")
            .string();

    {
        Document doc{(FixtureRoot() / "two_pages.pdf").string()};
        Graph graph{400.0, 400.0};

        auto line =
            std::make_unique<Line>(std::vector<float>{72.0f, 72.0f, 272.0f,
                                                     272.0f});
        GraphInfo gi;
        gi.LineWidth(2.0f);
        line->GraphInfo(gi);
        graph.Shapes().push_back(std::move(line));
        graph.Shapes().push_back(
            std::make_unique<Drawing::Rectangle>(100.0f, 100.0f, 150.0f,
                                                 80.0f));
        graph.Shapes().push_back(
            std::make_unique<Circle>(300.0f, 300.0f, 40.0f));

        doc.Pages()[1].Paragraphs().Add(graph);
        doc.Save(out);  // graph still alive
    }

    {
        Document doc{out};
        EXPECT_EQ(doc.Pages().Count(), 2);
    }

    const std::string bytes = ReadAll(out);
    EXPECT_NE(bytes.find(" m "), std::string::npos);   // line move-to
    EXPECT_NE(bytes.find(" l "), std::string::npos);   // line line-to
    EXPECT_NE(bytes.find(" re "), std::string::npos);  // rectangle
    EXPECT_NE(bytes.find(" c "), std::string::npos);   // circle Bézier
    EXPECT_NE(bytes.find(" S "), std::string::npos);   // stroke

    std::filesystem::remove(out);
}
