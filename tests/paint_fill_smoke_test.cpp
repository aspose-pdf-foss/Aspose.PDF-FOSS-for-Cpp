// =============================================================================
// paint_fill_smoke_test — colours, fills and gradients of generated content:
// Graph shape stroke / fill / dash / rounded corners, gradient fills
// (GradientAxialShading / GradientRadialShading via Color::PatternColorSpace),
// translucent fills, Page::Background, negative ZIndex (drawn under the page
// text), table / row / cell backgrounds and cell text styling, FloatingBox
// background. Checked through the saved operators and through our renderer.
// =============================================================================

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/border_side.hpp>
#include <aspose/pdf/cell.hpp>
#include <aspose/pdf/cells.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/drawing/circle.hpp>
#include <aspose/pdf/drawing/gradient_axial_shading.hpp>
#include <aspose/pdf/drawing/gradient_radial_shading.hpp>
#include <aspose/pdf/drawing/graph.hpp>
#include <aspose/pdf/drawing/line.hpp>
#include <aspose/pdf/drawing/rectangle.hpp>
#include <aspose/pdf/floating_box.hpp>
#include <aspose/pdf/font_repository.hpp>
#include <aspose/pdf/graph_info.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/margin_info.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/paragraphs.hpp>
#include <aspose/pdf/position.hpp>
#include <aspose/pdf/row.hpp>
#include <aspose/pdf/rows.hpp>
#include <aspose/pdf/table.hpp>
#include <aspose/pdf/text_builder.hpp>
#include <aspose/pdf/text_fragment.hpp>
#include <aspose/pdf/text_state.hpp>
#include <aspose/pdf/vertical_alignment.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
namespace dr = Aspose::Pdf::Drawing;

namespace {

using Img = foundation::png_decoder::DecodedImage;

// RGB at page point (x, y) of a 72-dpi render (1 px == 1 pt).
std::array<int, 3> Px(const Img& img, double x, double y) {
    const int col = static_cast<int>(x);
    const int row = static_cast<int>(img.height - y);
    const std::size_t i = (static_cast<std::size_t>(row) * img.width + col) * 4;
    return {std::to_integer<int>(img.pixels[i]), std::to_integer<int>(img.pixels[i + 1]),
            std::to_integer<int>(img.pixels[i + 2])};
}

bool Near(const std::array<int, 3>& got, int r, int g, int b, int tol = 12) {
    return std::abs(got[0] - r) <= tol && std::abs(got[1] - g) <= tol &&
           std::abs(got[2] - b) <= tol;
}

Color Gradient(std::shared_ptr<dr::PatternColorSpace> pcs) {
    Color c;
    c.PatternColorSpace(std::move(pcs));
    return c;
}

// A 200 x 200 page with whatever `build` adds, saved and reopened.
template <class Build>
std::string MakeDoc(const std::string& tag, Build build) {
    const auto out = flatten_test::TmpPath(tag);
    Document doc;
    Page page = doc.Pages().Add();
    page.SetPageSize(200, 200);
    build(doc, page);
    doc.Save(out);
    return out;
}

}  // namespace

TEST(PaintFill, ShapeStrokeFillDashAndRoundedCorners) {
    dr::Graph g(200, 200);
    auto rect = std::make_unique<dr::Rectangle>(20.0f, 20.0f, 80.0f, 60.0f);
    GraphInfo gi;
    gi.Color(Color::Blue());
    gi.FillColor(Color::FromArgb(255, 0, 0));
    gi.LineWidth(3.0f);
    gi.DashArray({4, 2});
    rect->GraphInfo(gi);
    rect->RoundedCornerRadius(10.0);
    g.Shapes().push_back(std::move(rect));
    auto circle = std::make_unique<dr::Circle>(150.0f, 150.0f, 30.0f);
    GraphInfo ci;
    ci.FillColor(Color::FromArgb(0, 160, 0));
    ci.LineWidth(0.0f);  // fill without an outline
    circle->GraphInfo(ci);
    g.Shapes().push_back(std::move(circle));

    const auto out = MakeDoc("paint_shapes", [&](Document&, Page& p) { p.Paragraphs().Add(g); });
    const std::string bytes = flatten_test::ReadFile(out);
    EXPECT_NE(bytes.find("0 0 1 RG"), std::string::npos);
    EXPECT_NE(bytes.find("1 0 0 rg"), std::string::npos);
    EXPECT_NE(bytes.find("[4 2] 0 d"), std::string::npos);
    EXPECT_NE(bytes.find(" c "), std::string::npos);  // rounded corners
    EXPECT_NE(bytes.find("re"), std::string::npos);
    Document doc{out};
    const Img img = flatten_test::Render(doc, 1);
    EXPECT_TRUE(Near(Px(img, 60, 50), 255, 0, 0)) << "rect fill";
    EXPECT_TRUE(Near(Px(img, 150, 150), 0, 160, 0)) << "circle fill";
    EXPECT_TRUE(Near(Px(img, 21, 21), 255, 255, 255, 40)) << "rounded corner left blank";
    EXPECT_TRUE(Near(Px(img, 179, 150), 0, 160, 0, 40)) << "no outline on the circle";
}

TEST(PaintFill, AxialGradientFillRendersAcrossTheShape) {
    dr::Graph g(200, 200);
    auto rect = std::make_unique<dr::Rectangle>(0.0f, 50.0f, 200.0f, 100.0f);
    GraphInfo gi;
    gi.LineWidth(0.0f);
    gi.FillColor(Gradient(std::make_shared<dr::GradientAxialShading>(
        Color::FromArgb(255, 0, 0), Color::FromArgb(0, 0, 255))));
    rect->GraphInfo(gi);
    g.Shapes().push_back(std::move(rect));
    const auto out = MakeDoc("paint_axial", [&](Document&, Page& p) { p.Paragraphs().Add(g); });
    const std::string bytes = flatten_test::ReadFile(out);
    EXPECT_NE(bytes.find("/ShadingType 2"), std::string::npos);
    EXPECT_NE(bytes.find("/Pattern cs"), std::string::npos);
    Document doc{out};
    const Img img = flatten_test::Render(doc, 1);
    const auto left = Px(img, 3, 100), mid = Px(img, 100, 100), right = Px(img, 196, 100);
    EXPECT_GT(left[0], 230);
    EXPECT_LT(left[2], 25);
    EXPECT_GT(right[2], 230);
    EXPECT_LT(right[0], 25);
    EXPECT_TRUE(Near(mid, 128, 0, 128, 20)) << mid[0] << "," << mid[1] << "," << mid[2];
    EXPECT_TRUE(Near(Px(img, 100, 30), 255, 255, 255, 2)) << "outside the shape";
}

TEST(PaintFill, AxialGradientHonoursStartAndEnd) {
    dr::Graph g(200, 200);
    auto rect = std::make_unique<dr::Rectangle>(0.0f, 0.0f, 200.0f, 200.0f);
    auto grad = std::make_shared<dr::GradientAxialShading>(Color::White(), Color::Black());
    grad->Start(Point(0, 200));  // top ... bottom: a vertical gradient
    grad->End(Point(0, 0));
    GraphInfo gi;
    gi.LineWidth(0.0f);
    gi.FillColor(Gradient(grad));
    rect->GraphInfo(gi);
    g.Shapes().push_back(std::move(rect));
    const auto out = MakeDoc("paint_axial2", [&](Document&, Page& p) { p.Paragraphs().Add(g); });
    Document doc{out};
    const Img img = flatten_test::Render(doc, 1);
    EXPECT_GT(Px(img, 100, 195)[0], 235);
    EXPECT_LT(Px(img, 100, 5)[0], 20);
    EXPECT_EQ(Px(img, 20, 100)[0], Px(img, 180, 100)[0]) << "constant across a row";
}

TEST(PaintFill, RadialGradientCentreToCorners) {
    dr::Graph g(200, 200);
    auto rect = std::make_unique<dr::Rectangle>(0.0f, 0.0f, 200.0f, 200.0f);
    GraphInfo gi;
    gi.LineWidth(0.0f);
    gi.FillColor(Gradient(
        std::make_shared<dr::GradientRadialShading>(Color::White(), Color::FromArgb(0, 0, 255))));
    rect->GraphInfo(gi);
    g.Shapes().push_back(std::move(rect));
    const auto out = MakeDoc("paint_radial", [&](Document&, Page& p) { p.Paragraphs().Add(g); });
    EXPECT_NE(flatten_test::ReadFile(out).find("/ShadingType 3"), std::string::npos);
    Document doc{out};
    const Img img = flatten_test::Render(doc, 1);
    EXPECT_GT(Px(img, 100, 100)[0], 235);
    EXPECT_LT(Px(img, 2, 2)[0], 30);
}

TEST(PaintFill, TranslucentFillUsesConstantAlpha) {
    dr::Graph g(200, 200);
    auto rect = std::make_unique<dr::Rectangle>(0.0f, 0.0f, 100.0f, 100.0f);
    GraphInfo gi;
    gi.LineWidth(0.0f);
    gi.FillColor(Color::FromArgb(128, 0, 0, 0));
    rect->GraphInfo(gi);
    g.Shapes().push_back(std::move(rect));
    const auto out = MakeDoc("paint_alpha", [&](Document&, Page& p) { p.Paragraphs().Add(g); });
    EXPECT_NE(flatten_test::ReadFile(out).find("/ca 0.502"), std::string::npos);
    Document doc{out};
    const Img img = flatten_test::Render(doc, 1);
    EXPECT_TRUE(Near(Px(img, 50, 50), 127, 127, 127, 6));
}

TEST(PaintFill, PageBackgroundAndNegativeZIndexSitUnderText) {
    dr::Graph under(200, 200);
    under.ZIndex(-1);
    auto band = std::make_unique<dr::Rectangle>(0.0f, 90.0f, 200.0f, 40.0f);
    GraphInfo gi;
    gi.LineWidth(0.0f);
    gi.FillColor(Color::FromArgb(0, 0, 200));
    band->GraphInfo(gi);
    under.Shapes().push_back(std::move(band));

    const auto out = MakeDoc("paint_bg", [&](Document&, Page& p) {
        p.Background(Gradient(std::make_shared<dr::GradientAxialShading>(
            Color::FromArgb(255, 240, 240), Color::FromArgb(240, 255, 240))));
        Text::TextFragment frag("XXXXXXXX");
        frag.TextState().FontSize(30.0f);
        frag.TextState().ForegroundColor(Color::FromArgb(255, 255, 0));
        frag.Position(Text::Position(20, 100));
        Text::TextBuilder{p}.AppendText(frag);
        p.Paragraphs().Add(under);
    });
    Document doc{out};
    const Img img = flatten_test::Render(doc, 1);
    EXPECT_TRUE(Near(Px(img, 5, 10), 255, 240, 240, 6)) << "background, left";
    EXPECT_TRUE(Near(Px(img, 195, 10), 240, 255, 240, 6)) << "background, right";
    EXPECT_TRUE(Near(Px(img, 190, 110), 0, 0, 200, 6)) << "band";
    // The yellow text stays visible over the band (drawn after it).
    const int yellow = flatten_test::CountPixels(img, 20, 95, 180, 125, 0.0, [](int r, int g, int b) {
        return r > 200 && g > 200 && b < 80;
    });
    EXPECT_GT(yellow, 100);
}

TEST(PaintFill, TableBackgroundsAndCellTextStyle) {
    Table table;
    table.ColumnWidths("100 80");
    table.Left(10.0f);
    table.Top(190.0f);
    table.BackgroundColor(Color::FromArgb(230, 230, 250));
    Row& header = table.Rows().Add();
    header.BackgroundColor(Color::FromArgb(60, 60, 140));
    header.MinRowHeight(30.0);
    Text::TextState hs;
    hs.Font(Text::FontRepository::FindFont("Helvetica-Bold"));
    hs.FontSize(12.0f);
    hs.ForegroundColor(Color::White());
    header.DefaultCellTextState(hs);
    header.Cells().Add("Item");
    Cell& price = header.Cells().Add("Price");
    price.Alignment(HorizontalAlignment::Right);
    Row& body = table.Rows().Add();
    body.MinRowHeight(30.0);
    body.Cells().Add("Tea");
    Cell& hl = body.Cells().Add("$3");
    hl.BackgroundColor(Color::FromArgb(255, 240, 200));
    table.Border(BorderInfo(BorderSide::All, 1.0f, Color::FromArgb(200, 0, 0)));

    const auto out = MakeDoc("paint_table", [&](Document&, Page& p) { p.Paragraphs().Add(table); });
    const std::string bytes = flatten_test::ReadFile(out);
    EXPECT_NE(bytes.find("/BaseFont /Helvetica-Bold"), std::string::npos);
    EXPECT_NE(bytes.find("0.7843 0 0 RG"), std::string::npos);
    Document doc{out};
    const Img img = flatten_test::Render(doc, 1);
    EXPECT_TRUE(Near(Px(img, 105, 163), 60, 60, 140, 4)) << "header row background";
    EXPECT_TRUE(Near(Px(img, 15, 135), 230, 230, 250, 4)) << "table background";
    EXPECT_TRUE(Near(Px(img, 115, 135), 255, 240, 200, 4)) << "cell background";
    // White header text: a light pixel run inside the dark header row.
    const int white = flatten_test::CountPixels(img, 10, 160, 190, 190, 0.0, [](int r, int g, int b) {
        return r > 200 && g > 200 && b > 200;
    });
    EXPECT_GT(white, 20);
    // "Price" is right-aligned: its ink sits in the right half of the cell.
    const int left_half = flatten_test::CountPixels(img, 112, 162, 150, 188, 0.0,
                                                    [](int r, int, int) { return r > 200; });
    const int right_half = flatten_test::CountPixels(img, 150, 162, 188, 188, 0.0,
                                                     [](int r, int, int) { return r > 200; });
    EXPECT_GT(right_half, left_half);
}

TEST(PaintFill, FloatingBoxBackgroundWithRoundedBorder) {
    FloatingBox box(120, 60);
    MarginInfo m;
    m.Left(20);
    m.Top(20);
    box.Margin(m);
    box.BackgroundColor(Color::FromArgb(200, 230, 255));
    BorderInfo b(BorderSide::All, 2.0f, Color::FromArgb(0, 0, 150));
    b.RoundedBorderRadius(12.0);
    box.Border(b);
    const auto out = MakeDoc("paint_box", [&](Document&, Page& p) { p.Paragraphs().Add(box); });
    Document doc{out};
    const Img img = flatten_test::Render(doc, 1);
    EXPECT_TRUE(Near(Px(img, 80, 150), 200, 230, 255, 4));
    EXPECT_TRUE(Near(Px(img, 21, 179), 255, 255, 255, 40)) << "rounded corner";
}

// Not paint, but found while reviewing the showcase: a new FileAttachment
// annotation must carry its required /FS (with the file embedded) and icon.
#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/file_attachment_annotation.hpp>
#include <aspose/pdf/file_specification.hpp>

TEST(FileAttachmentSave, WritesFileSpecWithEmbeddedFileAndIcon) {
    const auto out = flatten_test::TmpPath("file_attach_fs");
    {
        Document doc{flatten_test::HelloWorldPdf()};
        Page page = doc.Pages()[1];
        FileSpecification fs(flatten_test::HelloWorldPdf(), "the source");
        Annotations::FileAttachmentAnnotation a(page, flatten_test::Rect(10, 10, 30, 30), fs);
        a.Icon(Annotations::FileIcon::Paperclip);
        page.Annotations().Add(a);
        doc.Save(out);
    }
    const auto parsed = flatten_test::ParsePage(flatten_test::ReadFile(out), 0);
    bool found = false;
    for (const auto& d : flatten_test::AnnotDicts(parsed)) {
        if (flatten_test::NameOf(flatten_test::Get(d, "Subtype")) != "FileAttachment") continue;
        found = true;
        EXPECT_EQ(flatten_test::NameOf(flatten_test::Get(d, "Name")), "Paperclip");
        const auto fsd = flatten_test::ResolveDict(parsed.dump, flatten_test::Get(d, "FS"));
        EXPECT_EQ(flatten_test::NameOf(flatten_test::Get(fsd, "Type")), "Filespec");
        const auto ef = flatten_test::ResolveDict(parsed.dump, flatten_test::Get(fsd, "EF"));
        const auto* f = flatten_test::Get(ef, "F");
        ASSERT_NE(f, nullptr);
        const auto* stream = flatten_test::Find(parsed.dump, std::get<foundation::objects::Ref>(f->v).id);
        ASSERT_NE(stream, nullptr);
        EXPECT_TRUE(std::holds_alternative<foundation::objects::Stream>(stream->value.v));
    }
    EXPECT_TRUE(found);
}

#include <aspose/pdf/artifact_collection.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/watermark_artifact.hpp>

TEST(PaintFill, BackgroundWatermarkStaysAbovePageBackground) {
    WatermarkArtifact wm;
    const auto out = MakeDoc("paint_bg_wm", [&](Document&, Page& p) {
        p.Background(Color::FromArgb(220, 240, 255));
        Text::TextState ts;
        ts.FontSize(60.0f);
        ts.ForegroundColor(Color::FromArgb(200, 0, 0));
        wm.SetTextAndState("WWW", ts);
        wm.Position(Point(20, 80));
        wm.IsBackground(true);
        p.Artifacts().Add(wm);
    });
    Document doc{out};
    const Img img = flatten_test::Render(doc, 1);
    EXPECT_TRUE(Near(Px(img, 5, 190), 220, 240, 255, 4)) << "background";
    EXPECT_GT(flatten_test::RedPixels(img, 0, 60, 200, 160), 100) << "watermark visible";
}

TEST(PaintFill, TwoAxialGradientsOnOnePage) {
    dr::Graph g(200, 200);
    g.ZIndex(-1);
    {
        auto r = std::make_unique<dr::Rectangle>(0.0f, 0.0f, 50.0f, 50.0f);
        GraphInfo t;
        t.LineWidth(0.0f);
        t.FillColor(Color::FromArgb(128, 0, 0, 255));
        r->GraphInfo(t);
        g.Shapes().push_back(std::move(r));
    }
    auto c = std::make_unique<dr::Circle>(100.0f, 100.0f, 40.0f);
    auto grad = std::make_shared<dr::GradientAxialShading>(Color::FromArgb(200, 0, 0),
                                                           Color::FromArgb(200, 0, 0));
    grad->Start(Point(0, 80));
    grad->End(Point(80, 0));
    GraphInfo gi;
    gi.LineWidth(0.0f);
    gi.FillColor(Gradient(grad));
    c->GraphInfo(gi);
    g.Shapes().push_back(std::move(c));
    const auto out = MakeDoc("paint_two_axial", [&](Document&, Page& p) {
        p.Background(Gradient(std::make_shared<dr::GradientAxialShading>(
            Color::FromArgb(230, 230, 255), Color::FromArgb(230, 255, 230))));
        p.Paragraphs().Add(g);
    });
    Document doc{out};
    const Img img = flatten_test::Render(doc, 1);
    EXPECT_TRUE(Near(Px(img, 100, 100), 200, 0, 0, 6))
        << Px(img, 100, 100)[0] << "," << Px(img, 100, 100)[1] << "," << Px(img, 100, 100)[2];
}

TEST(PaintFill, RowVerticalAlignmentCentresCellText) {
    Table table;
    table.ColumnWidths("180");
    table.Left(10.0f);
    table.Top(190.0f);
    Row& row = table.Rows().Add();
    row.MinRowHeight(60.0);
    row.VerticalAlignment(VerticalAlignment::Center);
    Text::TextState ts;
    ts.FontSize(20.0f);
    row.DefaultCellTextState(ts);
    row.Cells().Add("HHHH");
    const auto out = MakeDoc("paint_valign", [&](Document&, Page& p) { p.Paragraphs().Add(table); });
    Document doc{out};
    const Img img = flatten_test::Render(doc, 1);
    auto dark = [](int r, int g, int b) { return r + g + b < 200; };
    // Row spans y 130..190; centred text sits in the middle band.
    const int middle = flatten_test::CountPixels(img, 12, 150, 180, 170, 0.0, dark);
    const int top = flatten_test::CountPixels(img, 12, 175, 180, 188, 0.0, dark);
    EXPECT_GT(middle, 50);
    EXPECT_LT(top, middle / 4);
}
