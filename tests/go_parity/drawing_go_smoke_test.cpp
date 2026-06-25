// =============================================================================
// Go-parity: vector_test.go → canonical C++ Aspose::Pdf::Drawing (Graph + Line
// / Rectangle / Circle / Ellipse), placed via Page.Paragraphs.
//
// Go exposes an idiomatic page.DrawLine/DrawCircle API; the canonical surface
// is a Graph container holding Shape objects. Ported here is the canonical
// shape-drawing capability: build a Graph of shapes and render it on the page.
// Skipped: Go-invented DrawPath / DrawPolyline / DrawPolygon / DrawRoundedRectangle
// (no canonical Path/Polygon/RoundedRect shape), LineCap/LineJoin/LineStyle
// constant structs, and content-byte / ExtGState-alpha assertions.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/drawing/circle.hpp>
#include <aspose/pdf/drawing/ellipse.hpp>
#include <aspose/pdf/drawing/graph.hpp>
#include <aspose/pdf/drawing/line.hpp>
#include <aspose/pdf/drawing/rectangle.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/paragraphs.hpp>

#include <gtest/gtest.h>

namespace {

namespace draw = Aspose::Pdf::Drawing;
using Aspose::Pdf::Document;

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
    return (std::filesystem::temp_directory_path() / ("go_draw_" + tag + ".pdf"))
        .string();
}

}  // namespace

// TestDrawLine / TestDrawRectangle / TestDrawCircle / TestDrawEllipse — a graph
// of mixed shapes renders and the document round-trips.
TEST(GoDrawing, GraphOfShapesRenders) {
    const std::string out = TmpOut("shapes");
    {
        Document doc{HelloWorld()};
        draw::Graph graph{400.0, 400.0};
        graph.Shapes().push_back(
            std::make_unique<draw::Line>(std::vector<float>{0, 0, 100, 100}));
        graph.Shapes().push_back(
            std::make_unique<draw::Rectangle>(10.0f, 10.0f, 80.0f, 40.0f));
        graph.Shapes().push_back(
            std::make_unique<draw::Circle>(200.0f, 200.0f, 50.0f));
        graph.Shapes().push_back(
            std::make_unique<draw::Ellipse>(150.0, 150.0, 120.0, 60.0));
        EXPECT_EQ(graph.Shapes().size(), 4u);
        doc.Pages()[1].Paragraphs().Add(graph);
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), 1);  // renders without corrupting the doc
    std::filesystem::remove(out);
}

// TestDrawRectangle_StrokeOnly etc. — a single shape graph is constructible
// and placeable.
TEST(GoDrawing, SingleRectangleGraph) {
    Document doc{HelloWorld()};
    draw::Graph graph{200.0, 150.0};
    graph.Shapes().push_back(
        std::make_unique<draw::Rectangle>(0.0f, 0.0f, 50.0f, 50.0f));
    ASSERT_EQ(graph.Shapes().size(), 1u);
    doc.Pages()[1].Paragraphs().Add(graph);
    EXPECT_EQ(doc.Pages()[1].Paragraphs().Count(), 1);
}
