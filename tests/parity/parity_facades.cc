// =============================================================================
// parity_facades.cc — PARITY measurement of pdflib's tests/test_facades.cpp
// (aspose.pdf.cpp.foss, 31 tests) re-expressed against the canonical
// Aspose.PDF FOSS for C++ public API.
//
// Each pdflib test maps to one TEST(ParityFacades, <Name>):
//   * a real ported test against the canonical surface, OR
//   * GTEST_SKIP() << "GAP: ..."   — capability absent from our lib, OR
//   * GTEST_SKIP() << "SHAPE: ..." — capability present but the contract
//                                    differs enough that a faithful port
//                                    is not possible (e.g. Stream cascade
//                                    dropped, different throw semantics).
//
// pdflib idioms that do NOT exist in our lib (and therefore drive the
// GAP/SHAPE verdicts):
//   * Document::create()/open() static factories          -> Document() ctor
//   * doc.pages().add(PageSize::A4)                        -> Pages().Add()
//     (our Add() is always US-Letter; no PageSize overload)
//   * page.text().add(...) / page.images().add(...)        -> not public
//   * doc.saveToMemory() / PdfExtractor::bindPdf(buffer)   -> Stream cascade
//   * Color public r/g/b fields + Color::fromHex(...)      -> dropped
//   * DocumentPrivilege public bool fields                 -> accessor methods
//   * PdfFileEditor::lastException() message getter        -> System.Exception
//                                                            cascade (dropped)
//   * PdfAnnotationEditor::exportAnnotationsToXfdf / import -> not in surface
//   * AnnotationSelector::selected()/select()              -> dropped (Selected)
//   * standalone MarkupAnnotation(AnnotationType)          -> abstract base
//   * makeHighlight()/makeUrlLink()/makeRedaction() factories + addStroke/
//     clearStrokes/setInteriorColor/setOverlayText free-method spellings ->
//     our concrete annotations bind to Document&/Page& and use PascalCase
//     property accessors.
//
// Fixture: tests/fixtures/text_extractor/hello_world.pdf (a one-page
// "Hello World" PDF) reached from this file via parent_path()/parent_path().
// =============================================================================

#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <aspose/pdf/color.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_privilege.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/point.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/text_absorber.hpp>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/highlight_annotation.hpp>
#include <aspose/pdf/annotations/link_annotation.hpp>
#include <aspose/pdf/annotations/annotation_flags.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/annotations/annotation_type.hpp>
#include <aspose/pdf/annotations/border.hpp>
#include <aspose/pdf/annotations/border_style.hpp>
#include <aspose/pdf/annotations/caret_annotation.hpp>
#include <aspose/pdf/annotations/caret_symbol.hpp>
#include <aspose/pdf/annotations/circle_annotation.hpp>
#include <aspose/pdf/annotations/ink_annotation.hpp>
#include <aspose/pdf/annotations/redaction_annotation.hpp>
#include <aspose/pdf/annotations/square_annotation.hpp>
#include <aspose/pdf/annotations/text_annotation.hpp>
#include <aspose/pdf/annotations/watermark_annotation.hpp>

#include <aspose/pdf/facades/bookmark.hpp>
#include <aspose/pdf/facades/key_size.hpp>
#include <aspose/pdf/facades/pdf_annotation_editor.hpp>
#include <aspose/pdf/facades/pdf_extractor.hpp>
#include <aspose/pdf/facades/pdf_file_editor.hpp>
#include <aspose/pdf/facades/pdf_file_security.hpp>

namespace {

namespace fs = std::filesystem;
using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;
using namespace Aspose::Pdf::Facades;

// One-page "Hello World" fixture (tests/fixtures/text_extractor/).
std::string HelloWorldPdf() {
    return (fs::path(__FILE__).parent_path().parent_path() / "fixtures" /
            "text_extractor" / "hello_world.pdf")
        .string();
}

// JPEG fixture (DCTDecode passthrough) for image-embed/extract parity.
std::string JpegFixture() {
    return (fs::path(__FILE__).parent_path().parent_path() / "fixtures" /
            "dctdecode" / "gradient_32.jpg")
        .string();
}

// Temp output path under the system temp dir.
std::string Tmp(const std::string& name) {
    return (fs::temp_directory_path() / ("parity_facades_" + name)).string();
}

// Build an N-page PDF at `path` by appending blank pages. Our Pages().Add()
// has no PageSize overload (always US-Letter) — page COUNT is what every
// pdflib editor test asserts, so this is a faithful substitute.
std::string MakePdf(const std::string& name, int pages) {
    const std::string path = Tmp(name);
    Document doc;
    for (int i = 0; i < pages; ++i) doc.Pages().Add();
    doc.Save(path);
    return path;
}

}  // namespace

// -----------------------------------------------------------------------------
// DocumentPrivilege  (pdflib: public bool fields -> our accessor methods)
// -----------------------------------------------------------------------------

TEST(ParityFacades, ForbidAll) {
    auto p = DocumentPrivilege::ForbidAll();
    EXPECT_FALSE(p.AllowPrint());
    EXPECT_FALSE(p.AllowCopy());
    EXPECT_FALSE(p.AllowModifyContents());
}

TEST(ParityFacades, AllowAll) {
    auto p = DocumentPrivilege::AllowAll();
    EXPECT_TRUE(p.AllowPrint());
    EXPECT_TRUE(p.AllowCopy());
    EXPECT_TRUE(p.AllowModifyContents());
}

// -----------------------------------------------------------------------------
// Bookmark / Bookmarks
// (pdflib Bookmark is a heap node with ->title / ->pageNumber and a
//  shared_ptr<Bookmarks> childItems; ours is a value type deriving from
//  std::vector<Bookmark> with PascalCase Title()/PageNumber()/ChildItems().)
// -----------------------------------------------------------------------------

TEST(ParityFacades, BookmarksAddAndAccess) {
    Bookmarks bms;
    Bookmark a;
    a.Title("Introduction");
    a.PageNumber(1);
    Bookmark b;
    b.Title("Chapter 1");
    b.PageNumber(3);
    Bookmark c;
    c.Title("Conclusion");
    c.PageNumber(20);
    bms.push_back(a);
    bms.push_back(b);
    bms.push_back(c);

    EXPECT_EQ(bms.size(), 3u);
    EXPECT_EQ(bms[0].Title(), "Introduction");
    EXPECT_EQ(bms[1].PageNumber(), 3);
    EXPECT_EQ(bms[2].Title(), "Conclusion");
}

TEST(ParityFacades, BookmarksNestedChildren) {
    Bookmark chapter;
    chapter.Title("Chapter 1");
    chapter.PageNumber(3);

    Bookmarks kids;
    Bookmark s1;
    s1.Title("Section 1.1");
    s1.PageNumber(4);
    Bookmark s2;
    s2.Title("Section 1.2");
    s2.PageNumber(7);
    kids.push_back(s1);
    kids.push_back(s2);
    chapter.ChildItems(kids);

    Bookmarks readBack = chapter.ChildItems();
    EXPECT_EQ(readBack.size(), 2u);
    EXPECT_EQ(readBack[0].Title(), "Section 1.1");
}

TEST(ParityFacades, BookmarksIteration) {
    Bookmarks bms;
    Bookmark a;
    a.Title("A");
    a.PageNumber(1);
    Bookmark b;
    b.Title("B");
    b.PageNumber(2);
    bms.push_back(a);
    bms.push_back(b);

    int count = 0;
    for (auto& bm : bms) {
        (void)bm;
        ++count;
    }
    EXPECT_EQ(count, 2);
}

// -----------------------------------------------------------------------------
// PdfFileEditor — real file ops (Concatenate / Extract / Delete / Append).
// pdflib editor.concatenate(in1, in2, out) -> our Concatenate(in1, in2, out).
// -----------------------------------------------------------------------------

TEST(ParityFacades, ConcatenateTwoFiles) {
    auto p1 = MakePdf("c1.pdf", 2);
    auto p2 = MakePdf("c2.pdf", 3);
    auto out = Tmp("merged.pdf");

    PdfFileEditor editor;
    EXPECT_TRUE(editor.Concatenate(p1, p2, out));
    EXPECT_TRUE(fs::exists(out));

    Document merged{out};
    EXPECT_EQ(merged.Pages().Count(), 5u);

    fs::remove(p1);
    fs::remove(p2);
    fs::remove(out);
}

TEST(ParityFacades, ConcatenateList) {
    auto p1 = MakePdf("l1.pdf", 1);
    auto p2 = MakePdf("l2.pdf", 2);
    auto p3 = MakePdf("l3.pdf", 3);
    auto out = Tmp("list_merged.pdf");

    PdfFileEditor editor;
    EXPECT_TRUE(editor.Concatenate(std::vector<std::string>{p1, p2, p3}, out));

    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), 6u);

    fs::remove(p1);
    fs::remove(p2);
    fs::remove(p3);
    fs::remove(out);
}

TEST(ParityFacades, ExtractPageRange) {
    auto src = MakePdf("src.pdf", 5);
    auto out = Tmp("extracted.pdf");

    PdfFileEditor editor;
    EXPECT_TRUE(editor.Extract(src, 2, 4, out));

    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), 3u);

    fs::remove(src);
    fs::remove(out);
}

TEST(ParityFacades, ExtractSpecificPages) {
    auto src = MakePdf("spec_src.pdf", 5);
    auto out = Tmp("spec_extracted.pdf");

    PdfFileEditor editor;
    EXPECT_TRUE(editor.Extract(src, std::vector<int>{1, 3, 5}, out));

    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), 3u);

    fs::remove(src);
    fs::remove(out);
}

TEST(ParityFacades, DeletePages) {
    auto src = MakePdf("del_src.pdf", 5);
    auto out = Tmp("deleted.pdf");

    PdfFileEditor editor;
    EXPECT_TRUE(editor.Delete(src, std::vector<int>{2, 4}, out));

    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), 3u);

    fs::remove(src);
    fs::remove(out);
}

TEST(ParityFacades, AppendFile) {
    auto src = MakePdf("app_src.pdf", 3);
    auto port = MakePdf("app_port.pdf", 2);
    auto out = Tmp("appended.pdf");

    // pdflib append(src, port, startPage=1, endPage=2, out) appends pages
    // 1..2 of `port` onto `src`. Our Append(portFile, addFiles, start, end,
    // out) takes the base as the first arg and the add-list as a vector.
    PdfFileEditor editor;
    EXPECT_TRUE(editor.Append(src, std::vector<std::string>{port}, 1, 2, out));

    Document doc{out};
    EXPECT_EQ(doc.Pages().Count(), 5u);

    fs::remove(src);
    fs::remove(port);
    fs::remove(out);
}

TEST(ParityFacades, ConcatenateErrorReturnsMessage) {
    // The false-return-on-bad-input half ports against TryConcatenate
    // (the canonical non-throwing form). The pdflib assertion that an
    // error MESSAGE can be read back (lastException()) has no equivalent:
    // PdfFileEditor::LastException is a System.Exception getter and was
    // dropped via the System.Exception cascade — only a setter survives.
    PdfFileEditor editor;
    EXPECT_FALSE(editor.TryConcatenate("/no/such/a.pdf", "/no/such/b.pdf",
                                       "/no/out.pdf"));
    GTEST_SKIP() << "SHAPE: PdfFileEditor::LastException returns System.Exception (dropped cascade); only the false return is observable, not an exception message."
                    "LastException() getter dropped (System.Exception "
                    "cascade); only the false return is observable.";
}

// -----------------------------------------------------------------------------
// PdfExtractor
// -----------------------------------------------------------------------------

TEST(ParityFacades, ExtractTextFromFile) {
    // pdflib synthesises a text PDF via page.text().add(...) — our public
    // surface has no text-authoring API, so we bind the bundled
    // "Hello World" fixture instead. Text extraction is REAL (foundation
    // TextAbsorber), which is the capability under test.
    PdfExtractor ex;
    ex.BindPdf(HelloWorldPdf());
    ex.ExtractText();

    const std::string out = Tmp("extract_text.txt");
    ex.GetText(out);
    ASSERT_TRUE(fs::exists(out));

    std::string text;
    {
        // Scope the stream so its handle is closed before fs::remove — on
        // Windows an open handle blocks deletion ("being used by another
        // process"); POSIX tolerates unlinking an open file.
        std::ifstream f(out, std::ios::binary);
        text.assign(std::istreambuf_iterator<char>(f),
                    std::istreambuf_iterator<char>());
    }
    EXPECT_NE(text.find("Hello World"), std::string::npos);
    fs::remove(out);
}

TEST(ParityFacades, ExtractTextFromMemory) {
    GTEST_SKIP()
        << "SHAPE: memory binding dropped — both Document::saveToMemory() "
           "and PdfExtractor::BindPdf(buffer) fall to the Stream cascade; "
           "PdfExtractor binds via file path / Document& only.";
}

TEST(ParityFacades, ExtractImageCount) {
    // pdflib seeds an image via page.images().add(jpg, rect) then walks
    // extractImage()/hasNextImage()/getNextImage(). Our public seeding
    // path is Page::AddImage(file, Rectangle); image extraction is REAL
    // now (PdfExtractor walks image XObjects via Document::CountImages /
    // ExtractImageToFile), so both halves of the pdflib test port.
    const std::string src = Tmp("img_src.pdf");
    {
        Document doc;
        doc.Pages().Add();
        Page pg = doc.Pages()[1];
        ASSERT_TRUE(pg.AddImage(JpegFixture(), Rectangle{10, 10, 120, 60, false}));
        doc.Save(src);
    }

    Document re{src};
    PdfExtractor ex{re};
    ex.ExtractImage();
    EXPECT_TRUE(ex.HasNextImage());

    const std::string imgOut = Tmp("extracted_img.jpg");
    EXPECT_TRUE(ex.GetNextImage(imgOut));
    ASSERT_TRUE(fs::exists(imgOut));
    EXPECT_GT(fs::file_size(imgOut), 0u);   // pdflib: img not empty
    EXPECT_FALSE(ex.HasNextImage());        // exactly one image

    fs::remove(src);
    fs::remove(imgOut);
}

// -----------------------------------------------------------------------------
// PdfConverter
// -----------------------------------------------------------------------------

TEST(ParityFacades, ConverterThrowsNotSupported) {
    GTEST_SKIP()
        << "SHAPE: opposite contract — pdflib's PdfConverter::getNextImage() "
           "throws PdfNotSupportedException (a deliberate stub). Our "
           "PdfConverter is a REAL rasteriser whose GetNextImage(file) "
           "renders a page to PNG and returns bool; there is no "
           "PdfNotSupportedException to assert.";
}

// -----------------------------------------------------------------------------
// PdfAnnotationEditor
// -----------------------------------------------------------------------------

TEST(ParityFacades, DeleteAllAnnotations) {
    // Build a doc with two real annotations on page 1, save, reopen, and
    // delete every annotation through the editor; verify the saved result
    // re-loads with an empty collection. (Uses our concrete annotation
    // ctors + load-on-open round-trip instead of make*() factories.)
    auto path = Tmp("ann_src.pdf");
    {
        Document doc;
        doc.Pages().Add();
        Page pg = doc.Pages()[1];
        SquareAnnotation sq{pg, Rectangle{10, 20, 110, 34, true}};
        sq.Contents("note");
        CircleAnnotation ci{pg, Rectangle{10, 50, 110, 64, true}};
        pg.Annotations().Add(sq);
        pg.Annotations().Add(ci);
        ASSERT_EQ(pg.Annotations().Count(), 2);
        doc.Save(path);
    }

    auto out = Tmp("ann_del.pdf");
    PdfAnnotationEditor editor;
    editor.BindPdf(path);
    editor.DeleteAnnotations();  // clears every page's collection (real)
    editor.Save(out);

    Document doc{out};
    EXPECT_EQ(doc.Pages()[1].Annotations().Count(), 0);

    fs::remove(path);
    fs::remove(out);
}

TEST(ParityFacades, DeleteAnnotationsByType) {
    // pdflib: add a Highlight + a Link, deleteAnnotations(Highlight) -> 1 left.
    // Ours: DeleteAnnotations(typeName) deletes by canonical type name.
    Document doc{HelloWorldPdf()};
    Page page = doc.Pages()[1];
    Aspose::Pdf::Annotations::HighlightAnnotation hl{
        page, Rectangle{10, 20, 110, 34, false}};
    Aspose::Pdf::Annotations::LinkAnnotation ln{
        page, Rectangle{10, 50, 110, 64, false}};
    page.Annotations().Add(hl);
    page.Annotations().Add(ln);
    ASSERT_EQ(page.Annotations().Count(), 2);

    PdfAnnotationEditor editor{doc};
    editor.DeleteAnnotations(std::string("Highlight"));

    auto& annots = doc.Pages()[1].Annotations();
    ASSERT_EQ(annots.Count(), 1);
    EXPECT_EQ(annots[0].AnnotationType(),
              Aspose::Pdf::Annotations::AnnotationType::Link);
}

TEST(ParityFacades, ExportAnnotationsXfdf) {
    GTEST_SKIP()
        << "SHAPE: canonical ExportAnnotationsToXfdf takes only a "
           "System.IO.Stream (no file/string overload) — the Stream cascade "
           "is dropped for the cpp target. pdflib's string-returning "
           "exportAnnotationsToXfdf() is its own shape.";
}

TEST(ParityFacades, ImportXfdfThrowsNotSupported) {
    GTEST_SKIP()
        << "SHAPE: import contract differs — our "
           "ImportAnnotationsFromXfdf(file) is a deferred no-op stub that "
           "neither throws PdfNotSupportedException nor imports; there is no "
           "PdfNotSupportedException type to assert against.";
}

// -----------------------------------------------------------------------------
// PdfFileSecurity — real encryption round-trip.
// -----------------------------------------------------------------------------

TEST(ParityFacades, SecurityEncryptRoundTrip) {
    auto path = MakePdf("sec.pdf", 1);
    auto out = Tmp("enc.pdf");

    // pdflib: sec.bindPdf(path); sec.encryptFile(out, "user", "owner", priv).
    // Ours: input + output are properties; EncryptFile takes the passwords +
    // privilege + key size (output via OutputFile).
    PdfFileSecurity sec;
    sec.InputFile(path);
    sec.OutputFile(out);
    bool ok = sec.EncryptFile("user", "owner", DocumentPrivilege::AllowAll(),
                              KeySize::x128);
    EXPECT_TRUE(ok);

    Document doc{out, "user"};
    EXPECT_TRUE(doc.IsEncrypted());
    EXPECT_EQ(doc.Pages().Count(), 1u);

    fs::remove(path);
    fs::remove(out);
}

// -----------------------------------------------------------------------------
// New annotation types
// (pdflib free-method spellings: addStroke / clearStrokes / setInteriorColor /
//  hasInteriorColor / setOverlayText / fromHex / r-component reads. Our
//  concrete annotations bind to Document&/Page& and use PascalCase property
//  accessors; Color has no fromHex and no public r/g/b.)
// -----------------------------------------------------------------------------

TEST(ParityFacades, InkAnnotation) {
    Document doc;
    doc.Pages().Add();
    Page pg = doc.Pages()[1];

    InkAnnotation::StrokeList strokes;
    strokes.push_back({Point{10, 10}, Point{20, 20}, Point{30, 10}});
    strokes.push_back({Point{40, 40}, Point{50, 50}});

    InkAnnotation ann{pg, Rectangle{0, 0, 200, 100, true}, strokes};
    EXPECT_EQ(ann.AnnotationType(), AnnotationType::Ink);
    EXPECT_EQ(ann.InkList().size(), 2u);
    EXPECT_EQ(ann.InkList()[0].size(), 3u);

    ann.InkList(InkAnnotation::StrokeList{});  // clearStrokes() analog
    EXPECT_TRUE(ann.InkList().empty());
}

TEST(ParityFacades, SquareAnnotationInteriorColor) {
    Document doc;
    SquareAnnotation ann{doc};

    // pdflib: setInteriorColor(Color::fromHex(0xFF0000FF)) then reads .r.
    // No fromHex and no public r/g/b on our Color — use FromArgb and assert
    // round-trip on alpha only (A() is the sole public component accessor).
    ann.InteriorColor(Color::FromArgb(255, 255, 0, 0));
    EXPECT_NEAR(ann.InteriorColor().A(), 1.0, 0.01);
    EXPECT_EQ(ann.AnnotationType(), AnnotationType::Square);
}

TEST(ParityFacades, CircleAnnotation) {
    Document doc;
    CircleAnnotation ann{doc};
    ann.InteriorColor(Color::White());
    EXPECT_EQ(ann.AnnotationType(), AnnotationType::Circle);
    EXPECT_NEAR(ann.InteriorColor().A(), 1.0, 0.01);  // White is opaque
}

TEST(ParityFacades, RedactionAnnotation) {
    Document doc;
    doc.Pages().Add();
    Page pg = doc.Pages()[1];
    RedactionAnnotation ann{pg, Rectangle{10, 20, 110, 50, true}};
    ann.FillColor(Color::Black());
    ann.OverlayText("REDACTED");
    ann.Repeat(true);  // setRepeatOverlayText analog

    EXPECT_EQ(ann.AnnotationType(), AnnotationType::Redaction);
    EXPECT_EQ(ann.OverlayText(), "REDACTED");
    EXPECT_TRUE(ann.Repeat());
    // pdflib asserts fillColor().r == 0 — no public r on our Color; assert
    // the FillColor round-trips by alpha (Black is opaque) instead.
    EXPECT_NEAR(ann.FillColor().A(), 1.0, 0.01);
}

TEST(ParityFacades, WatermarkAnnotation) {
    Document doc;
    doc.Pages().Add();
    Page pg = doc.Pages()[1];
    WatermarkAnnotation ann{pg, Rectangle{0, 0, 200, 100, true}};
    ann.Contents("DRAFT");
    ann.Opacity(0.5);

    EXPECT_EQ(ann.AnnotationType(), AnnotationType::Watermark);
    EXPECT_NEAR(ann.Opacity(), 0.5, 0.001);
    // pdflib also asserts ann.rotation() == 45 — our WatermarkAnnotation has
    // no Rotation property (FixedPrint/rotation deferred), so only Opacity
    // is checked here.
}

TEST(ParityFacades, CaretAnnotation) {
    Document doc;
    CaretAnnotation ann{doc};
    ann.Symbol(CaretSymbol::Paragraph);
    EXPECT_EQ(ann.AnnotationType(), AnnotationType::Caret);
    EXPECT_EQ(ann.Symbol(), CaretSymbol::Paragraph);
}

TEST(ParityFacades, BorderStyle) {
    // pdflib treats Border as a POD with public width(double)/style/dashArray
    // and ann.setBorder(b)/ann.border(). Our Border is a class owned by the
    // annotation (Border& Border()), Width is an int, Style is settable, and
    // dashArray (Dash) was dropped. Port the width+style half; dashArray is a
    // SHAPE gap noted inline.
    Document doc;
    TextAnnotation ann{doc};
    ann.Border().Width(2);
    ann.Border().Style(BorderStyle::Dashed);

    EXPECT_EQ(ann.Border().Width(), 2);
    EXPECT_EQ(ann.Border().Style(), BorderStyle::Dashed);
    // SHAPE: Border::dashArray has no equivalent — Dash dropped (reserved
    // 'true'/'false' member names per the drops note); not asserted.
}

TEST(ParityFacades, AnnotationFlags) {
    Document doc;
    TextAnnotation ann{doc};
    ann.Flags(AnnotationFlags::Print | AnnotationFlags::ReadOnly);

    auto has = [](AnnotationFlags v, AnnotationFlags f) {
        return (v & f) == f;  // pdflib hasFlag() analog via our operator&
    };
    EXPECT_TRUE(has(ann.Flags(), AnnotationFlags::Print));
    EXPECT_TRUE(has(ann.Flags(), AnnotationFlags::ReadOnly));
    EXPECT_FALSE(has(ann.Flags(), AnnotationFlags::Hidden));
}

TEST(ParityFacades, AnnotationSelectorVisitor) {
    // pdflib subclasses AnnotationSelector, overrides visit(MarkupAnnotation&),
    // and calls select(&a) / selected(). Our AnnotationSelector dropped the
    // Selected list + select(); it exposes per-concrete-type Visit() overloads
    // for double dispatch. Port the dispatch by accumulating in the derived
    // selector itself.
    struct HighlightCounter : public AnnotationSelector {
        int squares = 0;
        int circles = 0;
        void Visit(SquareAnnotation& a) override {
            (void)a;
            ++squares;
        }
        void Visit(CircleAnnotation& a) override {
            (void)a;
            ++circles;
        }
    };

    Document doc;
    SquareAnnotation sq{doc};
    CircleAnnotation ci{doc};

    HighlightCounter sel;
    sq.Accept(sel);
    ci.Accept(sel);

    EXPECT_EQ(sel.squares, 1);
    EXPECT_EQ(sel.circles, 1);
}

TEST(ParityFacades, NewAnnotationsSavedInPdf) {
    // pdflib adds Ink + Square + Redaction then saveToMemory()/open(buffer).
    // We save to a file (saveToMemory is a Stream-cascade drop) and reopen by
    // path — the round-trip-validity capability under test is preserved.
    Document doc;
    doc.Pages().Add();
    Page pg = doc.Pages()[1];

    InkAnnotation::StrokeList strokes;
    strokes.push_back({Point{10, 10}, Point{50, 50}, Point{100, 10}});
    InkAnnotation ink{pg, Rectangle{10, 10, 210, 110, true}, strokes};

    SquareAnnotation sq{pg, Rectangle{50, 200, 150, 280, true}};
    sq.InteriorColor(Color::FromArgb(255, 0, 255, 0));

    RedactionAnnotation rd{pg, Rectangle{100, 300, 300, 330, true}};
    rd.OverlayText("CONFIDENTIAL");

    pg.Annotations().Add(ink);
    pg.Annotations().Add(sq);
    pg.Annotations().Add(rd);

    const std::string out = Tmp("new_annots.pdf");
    EXPECT_NO_THROW({
        doc.Save(out);
        Document doc2{out};
        EXPECT_EQ(doc2.Pages().Count(), 1u);
    });
    fs::remove(out);
}
