// =============================================================================
// text_paragraph_smoke_test — beat T-6 of the text cluster. TextParagraph lays
// out word-wrapped, aligned, multi-line text into a rectangle and is placed via
// TextBuilder::AppendParagraph; FloatingBox renders a bordered box through
// Page::Paragraphs.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <string>

#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/border_side.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/floating_box.hpp>
#include <aspose/pdf/font_repository.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/paragraphs.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/text_builder.hpp>
#include <aspose/pdf/text_fragment.hpp>
#include <aspose/pdf/text_fragment_absorber.hpp>
#include <aspose/pdf/text_fragment_collection.hpp>
#include <aspose/pdf/text_paragraph.hpp>
#include <aspose/pdf/text_state.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Text::TextBuilder;
using Aspose::Pdf::Text::TextFragmentAbsorber;
using Aspose::Pdf::Text::TextParagraph;

std::string HelloWorldPdf() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr)
        return (std::filesystem::path(env) / "hello_world.pdf").string();
    return (std::filesystem::path(__FILE__).parent_path().parent_path() /
            "pdfs" / "hello_world.pdf")
        .string();
}

int CountMatches(const std::string& pdf, const std::string& phrase) {
    Document doc{pdf};
    TextFragmentAbsorber a{phrase};
    a.Visit(doc);
    return a.TextFragments().Count();
}

// X of the first absorbed fragment matching `phrase`, or -1 if none.
double FirstX(const std::string& pdf, const std::string& phrase) {
    Document doc{pdf};
    TextFragmentAbsorber a{phrase};
    a.Visit(doc);
    if (a.TextFragments().Count() < 1) return -1.0;
    return a.TextFragments()[1].Rectangle().LLX();
}

// Render `word` in a fixed box (LLX 100, width 300) with the given alignment;
// return the X of the absorbed fragment, or -1 if not found.
double AlignedX(Aspose::Pdf::HorizontalAlignment align, const std::string& word) {
    using Aspose::Pdf::Text::TextState;
    const std::string out =
        (std::filesystem::temp_directory_path() /
         ("aspose_align_" + std::to_string(static_cast<int>(align)) + ".pdf"))
            .string();
    {
        Document doc{HelloWorldPdf()};
        TextState ts;
        ts.Font(Aspose::Pdf::Text::FontRepository::FindFont("Helvetica"));
        ts.FontSize(14.0f);
        TextParagraph para;
        para.Rectangle(Rectangle{100.0, 600.0, 400.0, 700.0, true});  // w=300
        para.HorizontalAlignment(align);
        para.AppendLine(word, ts);
        Page page = doc.Pages()[1];
        TextBuilder builder{page};
        builder.AppendParagraph(para);
        doc.Save(out);
    }
    const double x = FirstX(out, word);
    std::filesystem::remove(out);
    return x;
}

// X of the word "quick" minus X of "The" for the standard wrapping sentence
// rendered at the given alignment (both words are on the first wrapped line).
double FirstLineWordGap(Aspose::Pdf::HorizontalAlignment align) {
    const std::string out =
        (std::filesystem::temp_directory_path() /
         ("aspose_jgap_" + std::to_string(static_cast<int>(align)) + ".pdf"))
            .string();
    {
        Document doc{HelloWorldPdf()};
        TextParagraph para;
        para.Rectangle(Rectangle{72.0, 500.0, 320.0, 720.0, true});  // w=248
        para.HorizontalAlignment(align);
        para.AppendLine(
            "The quick brown fox jumps over the lazy dog again and again");
        Page page = doc.Pages()[1];
        TextBuilder builder{page};
        builder.AppendParagraph(para);
        doc.Save(out);
    }
    const double gap = FirstX(out, "quick") - FirstX(out, "The");
    std::filesystem::remove(out);
    return gap;
}

// X of the last word ("delta") of a two-line paragraph at the given alignment.
double LastLineLastWordX(Aspose::Pdf::HorizontalAlignment align) {
    const std::string out =
        (std::filesystem::temp_directory_path() /
         ("aspose_fj_" + std::to_string(static_cast<int>(align)) + ".pdf"))
            .string();
    {
        Document doc{HelloWorldPdf()};
        TextParagraph para;
        para.Rectangle(Rectangle{72.0, 500.0, 472.0, 700.0, true});  // w=400
        para.HorizontalAlignment(align);
        para.AppendLine("alpha beta");
        para.AppendLine("gamma delta");
        Page page = doc.Pages()[1];
        TextBuilder builder{page};
        builder.AppendParagraph(para);
        doc.Save(out);
    }
    const double x = FirstX(out, "delta");
    std::filesystem::remove(out);
    return x;
}

}  // namespace

// A wrapped, multi-line paragraph renders text that survives a round-trip.
TEST(TextParagraphSmoke, WrappedParagraphRenders) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_tpara.pdf").string();
    {
        Document doc{HelloWorldPdf()};
        TextParagraph para;
        para.Rectangle(Rectangle{72.0, 500.0, 300.0, 720.0, true});  // w=228
        para.HorizontalAlignment(HorizontalAlignment::Left);
        para.AppendLine(
            "The quick brown fox jumps over the lazy dog again and again");
        para.AppendLine("Second line of the paragraph block");

        Page page = doc.Pages()[1];
        TextBuilder builder{page};
        builder.AppendParagraph(para);
        doc.Save(out);
    }

    // Words from both logical lines are present after wrapping.
    EXPECT_GE(CountMatches(out, "quick"), 1);
    EXPECT_GE(CountMatches(out, "Second"), 1);
    std::filesystem::remove(out);
}

// Centered alignment also renders the text (placement differs; content same).
TEST(TextParagraphSmoke, CenteredParagraphRenders) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_tpara_c.pdf").string();
    {
        Document doc{HelloWorldPdf()};
        TextParagraph para;
        para.Rectangle(Rectangle{72.0, 400.0, 400.0, 700.0, true});
        para.HorizontalAlignment(HorizontalAlignment::Center);
        para.FirstLineIndent(10.0f);
        para.AppendLine("Centered heading text");

        Page page = doc.Pages()[1];
        TextBuilder builder{page};
        builder.AppendParagraph(para);
        doc.Save(out);
    }
    EXPECT_GE(CountMatches(out, "Centered"), 1);
    std::filesystem::remove(out);
}

// Center/Right alignment place the line by REAL glyph width, not a 0.5-em
// estimate. "MMMMMM" (M = 0.889 em) makes the two diverge sharply, so the
// expected center/right positions only hold when the layout uses real metrics.
TEST(TextParagraphSmoke, AlignmentShiftsByRealWidth) {
    using Aspose::Pdf::HorizontalAlignment;
    using Aspose::Pdf::Text::TextState;
    const std::string word = "MMMMMM";
    TextState ts;
    ts.Font(Aspose::Pdf::Text::FontRepository::FindFont("Helvetica"));
    ts.FontSize(14.0f);
    const double w = ts.MeasureString(word);  // real width in the box font/size

    const double left = AlignedX(HorizontalAlignment::Left, word);
    const double center = AlignedX(HorizontalAlignment::Center, word);
    const double right = AlignedX(HorizontalAlignment::Right, word);
    ASSERT_GT(left, 0.0);
    ASSERT_GT(center, 0.0);
    ASSERT_GT(right, 0.0);

    // Box is LLX=100, width=300.
    EXPECT_LT(left, center);
    EXPECT_LT(center, right);
    EXPECT_NEAR(left, 100.0, 3.0);
    EXPECT_NEAR(center, 100.0 + (300.0 - w) / 2.0, 3.0);
    EXPECT_NEAR(right, 100.0 + 300.0 - w, 3.0);
}

// Justified text wraps across the box and survives a round-trip.
TEST(TextParagraphSmoke, JustifiedParagraphRenders) {
    using Aspose::Pdf::HorizontalAlignment;
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_tpara_j.pdf").string();
    {
        Document doc{HelloWorldPdf()};
        TextParagraph para;
        para.Rectangle(Rectangle{72.0, 500.0, 320.0, 720.0, true});
        para.HorizontalAlignment(HorizontalAlignment::Justify);
        para.AppendLine(
            "The quick brown fox jumps over the lazy dog again and again");
        Page page = doc.Pages()[1];
        TextBuilder builder{page};
        builder.AppendParagraph(para);
        doc.Save(out);
    }
    EXPECT_GE(CountMatches(out, "quick"), 1);
    EXPECT_GE(CountMatches(out, "again"), 1);
    std::filesystem::remove(out);
}

// Justify widens inter-word spacing on non-final lines (vs Left-aligned).
TEST(TextParagraphSmoke, JustifyStretchesFirstLine) {
    using Aspose::Pdf::HorizontalAlignment;
    const double leftGap = FirstLineWordGap(HorizontalAlignment::Left);
    const double justGap = FirstLineWordGap(HorizontalAlignment::Justify);
    ASSERT_GT(leftGap, 0.0);
    ASSERT_GT(justGap, 0.0);
    EXPECT_GT(justGap, leftGap + 1.0);  // words spread apart under justify
}

// FullJustify stretches the FINAL line (delta pushed to the right edge);
// plain Justify leaves it ragged-left.
TEST(TextParagraphSmoke, FullJustifyStretchesLastLine) {
    using Aspose::Pdf::HorizontalAlignment;
    const double ragged = LastLineLastWordX(HorizontalAlignment::Justify);
    const double full = LastLineLastWordX(HorizontalAlignment::FullJustify);
    ASSERT_GT(ragged, 0.0);
    ASSERT_GT(full, 0.0);
    EXPECT_GT(full, ragged + 50.0);  // last line filled to the box under FullJustify
}

// A FloatingBox with a border renders a frame and round-trips cleanly.
TEST(TextParagraphSmoke, FloatingBoxFrameRenders) {
    const std::string out =
        (std::filesystem::temp_directory_path() / "aspose_fbox.pdf").string();
    {
        Document doc{HelloWorldPdf()};
        FloatingBox box{200.0f, 120.0f};
        box.Border(BorderInfo{BorderSide::All, 1.5f});
        EXPECT_DOUBLE_EQ(box.Width(), 200.0);
        EXPECT_DOUBLE_EQ(box.Height(), 120.0);

        Page page = doc.Pages()[1];
        page.Paragraphs().Add(box);  // box must outlive Save (non-owning)
        EXPECT_EQ(page.Paragraphs().Count(), 1);
        doc.Save(out);
    }
    // Reloads without error and the original page text survives.
    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), 1);
    std::filesystem::remove(out);
}

// FloatingBox property surface (Width/Height/IsNeedRepeating/Paragraphs).
TEST(TextParagraphSmoke, FloatingBoxProperties) {
    FloatingBox box;
    EXPECT_DOUBLE_EQ(box.Width(), 0.0);
    box.Width(150.0);
    box.Height(90.0);
    box.IsNeedRepeating(true);
    EXPECT_DOUBLE_EQ(box.Width(), 150.0);
    EXPECT_DOUBLE_EQ(box.Height(), 90.0);
    EXPECT_TRUE(box.IsNeedRepeating());
    EXPECT_EQ(box.Paragraphs().Count(), 0);
}
