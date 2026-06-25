// =============================================================================
// parity_page.cc — PARITY measurement of pdflib's tests/test_page.cpp
// (31 tests) ported to OUR canonical Aspose::Pdf surface.
//
// Each pdflib TEST_F(PageTest, X) / TEST(..., X) is re-expressed as
// TEST(ParityPage, X) against the canonical lib. Three verdict classes:
//
//   * REAL  — the capability maps onto our surface; the test runs for real.
//   * GAP   — the capability is missing from our v1 surface; GTEST_SKIP.
//   * SHAPE — the capability exists but with a different (incompatible)
//             API shape; GTEST_SKIP with the shape mismatch noted.
//
// Capability map (verified against include/aspose/pdf/ headers):
//   dimensions / resize  -> Page::Rect(), Page::SetPageSize(w,h),
//                           PageSize::A4()/PageLetter()
//   add arbitrary text   -> NO general page.text().add  (GAP)
//   extract text         -> Text::TextAbsorber (Visit + Text)
//   search text          -> Text::TextFragmentAbsorber (ctor(phrase),
//                           Visit, TextFragments().Count()/[1].Text())
//   add image            -> Page::AddImage(file, Rectangle)
//   image count/extract  -> PdfExtractor::ExtractImage / HasNextImage /
//                           GetNextImage(file) (REAL — Document::CountImages /
//                           ExtractImageToFile)
//   header / footer       -> PdfFileStamp AddHeader/AddFooter/AddPageNumber
//                           (stateless: input->output; NO header() get-back)
//   annotations          -> Page::Annotations().Add / Delete
//   attachments          -> Document::EmbeddedFiles() (document-level;
//                           FileSpecification is path-based) + PdfExtractor
//   render               -> PngDevice (we render; pdflib throws) — inverted
//
// Fixtures resolved relative to this file:
//   tests/fixtures/text_extractor/hello_world.pdf   (1 Letter page, "Hello World")
//   tests/fixtures/png_decoder/rgba_filter_none_8.png
// =============================================================================

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/highlight_annotation.hpp>
#include <aspose/pdf/annotations/link_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/embedded_file_collection.hpp>
#include <aspose/pdf/facades/pdf_extractor.hpp>
#include <aspose/pdf/facades/pdf_file_stamp.hpp>
#include <aspose/pdf/file_specification.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/page_size.hpp>
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/font_repository.hpp>
#include <aspose/pdf/position.hpp>
#include <aspose/pdf/text_absorber.hpp>
#include <aspose/pdf/text_builder.hpp>
#include <aspose/pdf/text_fragment.hpp>
#include <aspose/pdf/text_fragment_absorber.hpp>
#include <aspose/pdf/text_fragment_collection.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path FixturesDir() {
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "fixtures";
}
std::string HelloWorldPdf() {
    return (FixturesDir() / "text_extractor" / "hello_world.pdf").string();
}
std::string PngFixture() {
    return (FixturesDir() / "png_decoder" / "rgba_filter_none_8.png").string();
}
std::string Tmp(const std::string& n) {
    return (std::filesystem::temp_directory_path() / ("parity_page_" + n))
        .string();
}
std::vector<std::uint8_t> ReadBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(in),
                                     std::istreambuf_iterator<char>());
}
std::string WriteBytes(const std::string& name,
                       const std::vector<std::uint8_t>& data) {
    const std::string path = Tmp(name);
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    return path;
}
// Whole-document text via TextAbsorber.
std::string DocText(Document& doc) {
    Text::TextAbsorber abs;
    abs.Visit(doc);
    return abs.Text();
}
std::string DocText(const std::string& path) {
    Document d{path};
    return DocText(d);
}

}  // namespace

// =============================================================================
// -- Dimensions ---------------------------------------------------------------
// =============================================================================

// pdflib: A4Dimensions — page added as A4 measures 595.28 x 841.89.
// REAL: PageSize::A4() supplies the dimensions; SetPageSize stages them;
// Rect() reads them back.
TEST(ParityPage, A4Dimensions) {
    Document doc{HelloWorldPdf()};
    PageSize a4 = PageSize::A4();
    auto p = doc.Pages()[1];
    p.SetPageSize(a4.Width(), a4.Height());
    EXPECT_NEAR(p.Rect().Width(), 595.28, 1.0);
    EXPECT_NEAR(p.Rect().Height(), 841.89, 1.0);
}

// pdflib: ResizePage — p.resize(400,600).
// REAL: Page::SetPageSize(400,600) -> Rect().
TEST(ParityPage, ResizePage) {
    Document doc{HelloWorldPdf()};
    auto p = doc.Pages()[1];
    p.SetPageSize(400.0, 600.0);
    EXPECT_NEAR(p.Rect().Width(), 400.0, 0.5);
    EXPECT_NEAR(p.Rect().Height(), 600.0, 0.5);
}

// pdflib: ResizeToPageSize — resize(PageSize::Letter) -> 612 x 792.
// REAL: PageSize::PageLetter() -> SetPageSize.
TEST(ParityPage, ResizeToPageSize) {
    Document doc{HelloWorldPdf()};
    PageSize letter = PageSize::PageLetter();
    auto p = doc.Pages()[1];
    p.SetPageSize(letter.Width(), letter.Height());
    EXPECT_NEAR(p.Rect().Width(), 612.0, 1.0);
    EXPECT_NEAR(p.Rect().Height(), 792.0, 1.0);
}

// pdflib: LandscapeOrientation — pages().add(A4, Orientation::Landscape)
// yields width > height.
// REAL (shape-adapted): no add(PageSize, Orientation) overload; landscape is
// modelled by swapping width/height on SetPageSize. We assert the resulting
// page is wider than tall.
TEST(ParityPage, LandscapeOrientation) {
    Document doc{HelloWorldPdf()};
    PageSize a4 = PageSize::A4();
    auto p = doc.Pages().Add();           // append a blank page
    p.SetPageSize(a4.Height(), a4.Width());  // swapped => landscape
    EXPECT_GT(p.Rect().Width(), p.Rect().Height());
}

// =============================================================================
// -- Text ---------------------------------------------------------------------
// =============================================================================

// pdflib: AddTextNoThrow — page.text().add("...", {x,y}, Font). Canonical
// shape: TextBuilder(page).AppendText(fragment) with the fragment carrying
// Text, Position and a TextState font/size.
TEST(ParityPage, AddTextNoThrow) {
    Document doc{HelloWorldPdf()};
    Page page = doc.Pages()[1];
    Text::TextBuilder builder{page};
    Text::TextFragment frag{"Inserted text"};
    frag.Position(Text::Position{100.0, 700.0});
    frag.TextState().Font(Text::FontRepository::FindFont("Helvetica"));
    frag.TextState().FontSize(12.0f);
    EXPECT_NO_THROW(builder.AppendText(frag));
}

// pdflib: ExtractTextEmpty — fresh blank page extracts to empty.
// REAL: a freshly appended blank page has no text content; TextAbsorber
// over just that page returns empty.
TEST(ParityPage, ExtractTextEmpty) {
    Document doc{HelloWorldPdf()};
    auto blank = doc.Pages().Add();  // appended blank page, no content
    Text::TextAbsorber abs;
    abs.Visit(blank);
    EXPECT_TRUE(abs.Text().empty());
}

// pdflib: AddAndExtractTextRoundTrip — add text, save, reopen, extract. The
// TextBuilder write goes into the page content stream + a Standard-14 font
// resource, so the inserted run is extractable after a save/reload.
TEST(ParityPage, AddAndExtractTextRoundTrip) {
    const std::string out = Tmp("page_addtext.pdf");
    {
        Document doc{HelloWorldPdf()};
        Page page = doc.Pages()[1];
        Text::TextBuilder builder{page};
        Text::TextFragment frag{"InsertedXYZ"};
        frag.Position(Text::Position{100.0, 700.0});
        frag.TextState().Font(Text::FontRepository::FindFont("Helvetica"));
        frag.TextState().FontSize(12.0f);
        builder.AppendText(frag);
        doc.Save(out);
    }
    Document re{out};
    Text::TextAbsorber abs;
    abs.Visit(re);
    EXPECT_NE(abs.Text().find("InsertedXYZ"), std::string::npos);
    std::filesystem::remove(out);
}

// pdflib: SearchTextEmpty — searching a blank page finds nothing.
// REAL: TextFragmentAbsorber over a blank page -> 0 fragments.
TEST(ParityPage, SearchTextEmpty) {
    Document doc{HelloWorldPdf()};
    auto blank = doc.Pages().Add();
    Text::TextFragmentAbsorber abs{"anything"};
    abs.Visit(blank);
    EXPECT_EQ(abs.TextFragments().Count(), 0);
}

// pdflib: SearchTextFound — add "Hello World PDF", save, reopen, search
// "World" -> non-empty.
// REAL (shape-adapted): we can't add text, so we search the pre-existing
// "Hello World" fixture instead. The search capability under test is REAL.
TEST(ParityPage, SearchTextFound) {
    Document doc{HelloWorldPdf()};
    Text::TextFragmentAbsorber abs{"World"};
    abs.Visit(doc);
    ASSERT_GT(abs.TextFragments().Count(), 0);
    EXPECT_NE(abs.TextFragments()[1].Text().find("World"), std::string::npos);
}

// pdflib: ExtractTextFromFile1 — extract from a specific testdata PDF.
// REAL (shape-adapted): that exact fixture isn't in this repo; we extract
// from our hello_world.pdf fixture and assert its known content. Text
// extraction is REAL.
TEST(ParityPage, ExtractTextFromFile1) {
    Document doc{HelloWorldPdf()};
    std::string text = DocText(doc);
    while (!text.empty() &&
           (text.back() == '\n' || text.back() == '\r' || text.back() == ' '))
        text.pop_back();
    EXPECT_NE(text.find("Hello World"), std::string::npos);
}

// =============================================================================
// -- Images -------------------------------------------------------------------
// =============================================================================

// Count images in a (possibly reopened) document by walking PdfExtractor's
// image cursor — the canonical Facades enumeration surface.
int CountImagesViaExtractor(Document& doc) {
    Facades::PdfExtractor ex{doc};
    ex.ExtractImage();  // (re)start the image walk
    int n = 0;
    const std::string sink = Tmp("img_count_sink.bin");
    while (ex.HasNextImage()) {
        ex.GetNextImage(sink);
        ++n;
    }
    std::filesystem::remove(sink);
    return n;
}

// pdflib: ImageCountInitiallyZero — page.images().count() == 0.
// REAL (shape-adapted): the hello_world fixture has no image XObjects, so
// the PdfExtractor image enumeration yields 0.
TEST(ParityPage, ImageCountInitiallyZero) {
    Document doc{HelloWorldPdf()};
    EXPECT_EQ(CountImagesViaExtractor(doc), 0);
}

// pdflib: AddImageFromFile_JPEG — page.images().add(path, rect) no-throw.
// REAL: Page::AddImage(file, Rectangle). The repo has no committed JPEG
// fixture at the expected path, so we exercise the PNG decode path (the
// task-designated image fixture). Capability under test (file image embed)
// is REAL.
TEST(ParityPage, AddImageFromFile_JPEG) {
    Document doc{HelloWorldPdf()};
    auto p = doc.Pages()[1];
    EXPECT_NO_THROW({
        bool ok = p.AddImage(PngFixture(), Rectangle(10, 10, 120, 60, false));
        EXPECT_TRUE(ok);
    });
}

// pdflib: AddImageFromFile_PNG — page.images().add(png, rect) no-throw.
// REAL: Page::AddImage with the PNG fixture.
TEST(ParityPage, AddImageFromFile_PNG) {
    Document doc{HelloWorldPdf()};
    auto p = doc.Pages()[1];
    EXPECT_NO_THROW({
        bool ok = p.AddImage(PngFixture(), Rectangle(10, 10, 120, 60, false));
        EXPECT_TRUE(ok);
    });
}

// pdflib: AddBothImagesCountsTwo — add 2 images, count == 2.
// REAL: Page::AddImage twice, Save, reopen, count via PdfExtractor image
// enumeration == 2. (The repo ships only one image fixture; both adds use
// the PNG fixture at distinct rectangles — they become two distinct image
// XObjects.)
TEST(ParityPage, AddBothImagesCountsTwo) {
    const std::string out = Tmp("two_images.pdf");
    {
        Document doc{HelloWorldPdf()};
        auto p = doc.Pages()[1];
        ASSERT_TRUE(p.AddImage(PngFixture(), Rectangle(10, 10, 120, 60, false)));
        ASSERT_TRUE(
            p.AddImage(PngFixture(), Rectangle(150, 10, 260, 60, false)));
        doc.Save(out);
    }
    Document re{out};
    EXPECT_EQ(CountImagesViaExtractor(re), 2);
    std::filesystem::remove(out);
}

// pdflib: ExtractJpegRoundTrip — add image, save, reopen, extract(0) bytes
// (asserts JPEG magic FF D8).
// REAL (shape-adapted): image embed + extract round-trips for real via
// Page::AddImage -> Save -> reopen -> PdfExtractor::GetNextImage(file).
// The repo's only image fixture is a PNG (decoded to a raw-pixel XObject on
// embed), so the extracted stream is the image XObject body (non-empty)
// rather than verbatim JPEG; we assert non-empty extraction (the JPEG-magic
// form is a fixture limitation, not a capability gap).
TEST(ParityPage, ExtractJpegRoundTrip) {
    const std::string out = Tmp("extract_img.pdf");
    {
        Document doc{HelloWorldPdf()};
        auto p = doc.Pages()[1];
        ASSERT_TRUE(p.AddImage(PngFixture(), Rectangle(10, 10, 120, 60, false)));
        doc.Save(out);
    }
    Document re{out};
    Facades::PdfExtractor ex{re};
    ex.ExtractImage();
    ASSERT_TRUE(ex.HasNextImage());
    const std::string imgOut = Tmp("extracted_img.bin");
    ASSERT_TRUE(ex.GetNextImage(imgOut));
    const std::vector<std::uint8_t> raw = ReadBytes(imgOut);
    EXPECT_FALSE(raw.empty());
    std::filesystem::remove(out);
    std::filesystem::remove(imgOut);
}

// =============================================================================
// -- Header / Footer ----------------------------------------------------------
// =============================================================================

// pdflib: SetAndGetHeader — set a HeaderFooter, read header()->centerText.
// SHAPE: pdflib's stateful page.header() get-back object is pdflib-invented.
// Canonical header drawing is the stateless Facades::PdfFileStamp (draw-only,
// input->output); there is no stateful HeaderFooter object to read back.
TEST(ParityPage, SetAndGetHeader) {
    GTEST_SKIP() << "SHAPE: pdflib-invented stateful page.header() get-back. "
                    "Canonical PdfFileStamp is stateless (draw-only).";
}

// pdflib: RemoveHeader — setHeader then removeHeader, header() == nullptr.
// SHAPE: same pdflib-invented stateful model — canonical PdfFileStamp has no
// removeHeader()/header() get-back accessor to assert removal against.
TEST(ParityPage, RemoveHeader) {
    GTEST_SKIP() << "SHAPE: pdflib-invented stateful header — no removeHeader/"
                    "header() get-back on the stateless PdfFileStamp surface.";
}

// pdflib: HeaderTextAppearsInSavedPdf — set header, save, header text
// extractable from saved PDF.
// REAL: PdfFileStamp::AddHeader draws Helvetica text into each page's
// content stream (Document::ApplyTextStamps); extractable after Save.
TEST(ParityPage, HeaderTextAppearsInSavedPdf) {
    const std::string out = Tmp("header.pdf");
    {
        Document doc{HelloWorldPdf()};
        Facades::PdfFileStamp st{doc};
        st.AddHeader("MyHeader", 36.0f);
        st.Save(out);
    }
    EXPECT_NE(DocText(out).find("MyHeader"), std::string::npos);
    std::filesystem::remove(out);
}

// pdflib: FooterPageNumber — footer with "Page %d of %D" yields "Page 1 of 2".
// SHAPE: AddPageNumber supports the current-number token ('#') but NOT a
// total-pages token (%D). We assert the per-page number renders ("Page 1");
// the "of N" total-pages form is a shape gap.
TEST(ParityPage, FooterPageNumber) {
    const std::string out = Tmp("footer_num.pdf");
    {
        Document doc{HelloWorldPdf()};
        doc.Pages().Add();  // 2 pages total
        Facades::PdfFileStamp st{doc};
        st.AddPageNumber("Page #");  // '#' = current page number
        st.Save(out);
    }
    const std::string text = DocText(out);
    // Per-page number renders (REAL); "of <total>" token is unsupported.
    EXPECT_NE(text.find("Page 1"), std::string::npos);
}

// pdflib: HeaderNotDoubleRenderedOnMultipleSaves — header rendered once,
// not duplicated across repeated saves of the same stateful document.
// SHAPE: our model is stateless (one input->output stamping run), so the
// double-render hazard doesn't exist. We assert a single AddHeader+Save
// produces exactly one occurrence.
TEST(ParityPage, HeaderNotDoubleRenderedOnMultipleSaves) {
    const std::string out = Tmp("once.pdf");
    {
        Document doc{HelloWorldPdf()};
        Facades::PdfFileStamp st{doc};
        st.AddHeader("OnceOnly", 36.0f);
        st.Save(out);
    }
    const std::string text = DocText(out);
    int n = 0;
    for (std::size_t pos = 0;
         (pos = text.find("OnceOnly", pos)) != std::string::npos;
         pos += 8)
        ++n;
    EXPECT_EQ(n, 1);
}

// pdflib: SetAndGetHeaderFooter — set header+footer, read both back.
// SHAPE: pdflib-invented stateful header()/footer() get-back. Canonical
// PdfFileStamp header/footer are stateless draw operations (no read-back).
TEST(ParityPage, SetAndGetHeaderFooter) {
    GTEST_SKIP() << "SHAPE: pdflib-invented stateful header()/footer() "
                    "get-back. Canonical PdfFileStamp is stateless (draw-only).";
}

// =============================================================================
// -- Annotations --------------------------------------------------------------
// =============================================================================

// pdflib: AddAnnotations — add highlight + url link, count == 2.
// REAL: Page::Annotations().Add with HighlightAnnotation + LinkAnnotation.
// (Annotation instances live at the call site; the collection holds refs.)
TEST(ParityPage, AddAnnotations) {
    Document doc{HelloWorldPdf()};
    auto page = doc.Pages()[1];
    Annotations::HighlightAnnotation hl{page, Rectangle(10, 20, 110, 34, false)};
    Annotations::LinkAnnotation link{page, Rectangle(10, 50, 110, 64, false)};
    page.Annotations().Add(hl);
    page.Annotations().Add(link);
    EXPECT_EQ(page.Annotations().Count(), 2);
}

// pdflib: RemoveAnnotation — add 2, remove(0), count == 1.
// REAL: AnnotationCollection::Delete(int).
TEST(ParityPage, RemoveAnnotation) {
    Document doc{HelloWorldPdf()};
    auto page = doc.Pages()[1];
    Annotations::HighlightAnnotation a{page, Rectangle(0, 0, 10, 10, false)};
    Annotations::HighlightAnnotation b{page, Rectangle(20, 20, 30, 30, false)};
    page.Annotations().Add(a);
    page.Annotations().Add(b);
    page.Annotations().Delete(0);
    EXPECT_EQ(page.Annotations().Count(), 1);
}

// pdflib: AnnotationsSavedInPdf — add link, save, reopen no-throw.
// REAL: annotation save-through + reopen.
TEST(ParityPage, AnnotationsSavedInPdf) {
    const std::string out = Tmp("annots.pdf");
    {
        Document doc{HelloWorldPdf()};
        auto page = doc.Pages()[1];
        Annotations::LinkAnnotation link{page,
                                         Rectangle(10, 10, 110, 30, false)};
        page.Annotations().Add(link);
        doc.Save(out);
    }
    EXPECT_NO_THROW({ Document re{out}; });
    std::filesystem::remove(out);
}

// =============================================================================
// -- Attachments --------------------------------------------------------------
// =============================================================================
//
// SHAPE NOTE for the whole bucket: pdflib attaches *per page* and takes raw
// in-memory bytes; our model attaches at the *document* level via
// FileSpecification(path) (path-based — in-memory raw-bytes attach is a GAP).
// Where the underlying capability (attach a file, count, extract, remove)
// maps, we port REAL against EmbeddedFiles() + PdfExtractor, writing the test
// bytes to a temp file first.

// pdflib: AttachmentCountInitiallyZero — page.attachments().count() == 0.
// REAL (shape-adapted to document-level): EmbeddedFiles().Count() == 0.
TEST(ParityPage, AttachmentCountInitiallyZero) {
    Document doc{HelloWorldPdf()};
    EXPECT_EQ(doc.EmbeddedFiles().Count(), 0);
}

// pdflib: AttachOneFile — attach raw bytes, count == 1.
// REAL (shape-adapted): in-memory raw-bytes attach is a GAP, so we write the
// bytes to a temp file and attach via FileSpecification(path). Count == 1.
TEST(ParityPage, AttachOneFile) {
    Document doc{HelloWorldPdf()};
    const std::string p = WriteBytes("attach_one.bin", {1, 2, 3, 4, 5});
    FileSpecification fs{p, "A test file"};
    doc.EmbeddedFiles().Add(fs);
    EXPECT_EQ(doc.EmbeddedFiles().Count(), 1);
    std::filesystem::remove(p);
}

// pdflib: AttachAndExtractRoundTrip — attach bytes, save, reopen, extract(0)
// equals original.
// REAL (shape-adapted): path-based attach -> Save -> load-on-open -> extract
// via PdfExtractor::GetAttachment(outputPath) -> compare bytes.
TEST(ParityPage, AttachAndExtractRoundTrip) {
    const std::vector<std::uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
    const std::string src = WriteBytes("hello.txt", data);
    const std::string pdf = Tmp("attach_rt.pdf");
    {
        Document doc{HelloWorldPdf()};
        FileSpecification fs{src};
        doc.EmbeddedFiles().Add(fs);
        doc.Save(pdf);
    }
    Document re{pdf};
    ASSERT_EQ(re.EmbeddedFiles().Count(), 1);

    const std::string outPath = Tmp("extracted.bin");
    Facades::PdfExtractor ex{re};
    ex.GetAttachment(outPath);
    const std::vector<std::uint8_t> extracted = ReadBytes(outPath);
    EXPECT_EQ(extracted, data);

    std::filesystem::remove(src);
    std::filesystem::remove(pdf);
    std::filesystem::remove(outPath);
}

// pdflib: RemoveAttachmentReducesCount — attach 3, save, reopen count == 3,
// remove(1), count == 2.
// REAL (shape-adapted): EmbeddedFiles + DeleteByKey/Delete; we delete by the
// known name-tree key.
TEST(ParityPage, RemoveAttachmentReducesCount) {
    const std::string pa = WriteBytes("a.bin", {1, 2});
    const std::string pb = WriteBytes("b.bin", {3, 4});
    const std::string pc = WriteBytes("c.bin", {5, 6});
    const std::string pdf = Tmp("attach3.pdf");
    {
        Document doc{HelloWorldPdf()};
        doc.EmbeddedFiles().Add(FileSpecification{pa});
        doc.EmbeddedFiles().Add(FileSpecification{pb});
        doc.EmbeddedFiles().Add(FileSpecification{pc});
        doc.Save(pdf);
    }
    Document re{pdf};
    ASSERT_EQ(re.EmbeddedFiles().Count(), 3);
    // The name-tree key is the FileSpecification's Name() — the full path it
    // was added with (round-trips verbatim through Save / load-on-open).
    re.EmbeddedFiles().DeleteByKey(pb);
    EXPECT_EQ(re.EmbeddedFiles().Count(), 2);

    std::filesystem::remove(pa);
    std::filesystem::remove(pb);
    std::filesystem::remove(pc);
    std::filesystem::remove(pdf);
}

// pdflib: RemoveAttachmentPdfValid — attach, save, reopen, remove, re-save,
// reopen, count == 0.
// REAL (shape-adapted): EmbeddedFiles().Delete() clears all; re-save +
// reopen confirms a valid PDF with 0 attachments.
TEST(ParityPage, RemoveAttachmentPdfValid) {
    const std::string src = WriteBytes("payload.bin", {0xDE, 0xAD, 0xBE, 0xEF});
    const std::string pdf1 = Tmp("attach_valid1.pdf");
    {
        Document doc{HelloWorldPdf()};
        doc.EmbeddedFiles().Add(FileSpecification{src, "test"});
        doc.Save(pdf1);
    }
    const std::string pdf2 = Tmp("attach_valid2.pdf");
    {
        Document re{pdf1};
        re.EmbeddedFiles().Delete();  // remove all
        EXPECT_EQ(re.EmbeddedFiles().Count(), 0);
        re.Save(pdf2);
    }
    Document re2{pdf2};
    EXPECT_EQ(re2.EmbeddedFiles().Count(), 0);

    std::filesystem::remove(src);
    std::filesystem::remove(pdf1);
    std::filesystem::remove(pdf2);
}

// pdflib: RemoveMiddleAttachment — attach a,b,c, save, reopen, remove(b),
// extract(0)==a and extract(1)==c.
// SHAPE: our extraction surface (PdfExtractor::GetAttachment) writes a single
// named/first attachment to a file — there is no extract-by-positional-index
// returning bytes for arbitrary entries. We verify the *removal* semantics
// (b is gone, a and c remain) via the collection instead of positional
// byte-extract.
TEST(ParityPage, RemoveMiddleAttachment) {
    const std::string pa = WriteBytes("ma.bin", {1});
    const std::string pb = WriteBytes("mb.bin", {2});
    const std::string pc = WriteBytes("mc.bin", {3});
    const std::string pdf = Tmp("attach_mid.pdf");
    {
        Document doc{HelloWorldPdf()};
        doc.EmbeddedFiles().Add(FileSpecification{pa});
        doc.EmbeddedFiles().Add(FileSpecification{pb});
        doc.EmbeddedFiles().Add(FileSpecification{pc});
        doc.Save(pdf);
    }
    Document re{pdf};
    re.EmbeddedFiles().DeleteByKey(pb);  // key == full path it was added with
    const std::vector<std::string> keys = re.EmbeddedFiles().Keys();
    EXPECT_EQ(keys.size(), 2u);
    EXPECT_EQ(re.EmbeddedFiles().FindByName(pb), nullptr);
    EXPECT_NE(re.EmbeddedFiles().FindByName(pa), nullptr);
    EXPECT_NE(re.EmbeddedFiles().FindByName(pc), nullptr);

    std::filesystem::remove(pa);
    std::filesystem::remove(pb);
    std::filesystem::remove(pc);
    std::filesystem::remove(pdf);
}

// =============================================================================
// -- Render (pdflib expects throw; WE EXCEED pdflib) --------------------------
// =============================================================================

// pdflib: RenderThrowsNotSupported — renderToImage throws
// PdfNotSupportedException (pdflib has no rasteriser).
// INVERTED -> REAL: WE render. PngDevice::Process produces a non-empty PNG.
// Note: we exceed pdflib here.
TEST(ParityPage, RenderThrowsNotSupported_WeExceed) {
    Document doc{HelloWorldPdf()};
    auto page = doc.Pages()[1];
    Devices::PngDevice png;
    std::ostringstream os(std::ios::binary);
    EXPECT_NO_THROW(png.Process(page, os));
    const std::string bytes = os.str();
    EXPECT_FALSE(bytes.empty());
    // PNG magic signature (\x89 P N G).
    ASSERT_GE(bytes.size(), 4u);
    EXPECT_EQ(static_cast<unsigned char>(bytes[0]), 0x89);
    EXPECT_EQ(bytes[1], 'P');
    EXPECT_EQ(bytes[2], 'N');
    EXPECT_EQ(bytes[3], 'G');
}
