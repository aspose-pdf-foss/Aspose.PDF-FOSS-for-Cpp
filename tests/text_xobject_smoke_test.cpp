// =============================================================================
// text_xobject_smoke_test — TextAbsorber reads text drawn inside Form
// XObjects (`/Name Do`), places line breaks by the real page position
// (cm / form /Matrix), and survives self-referencing forms. Covers text
// burned in by Form::Flatten.
// =============================================================================

#include <map>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/text_absorber.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace flatten_test;

namespace {

// "<< extra /Length n >>" for a stream body.
std::string StreamDict(const std::string& extra, const std::string& body) {
    return "<< " + extra + " /Length " + std::to_string(body.size()) + " >>";
}

const char* kFont =
    "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica "
    "/Encoding /WinAnsiEncoding >>";

// A one-page PDF: page content `content` (object 4), page /Resources with
// font F1 (object 5) and XObjects named in `xobjects` (name -> object id);
// further objects (6+) from `more` with stream bodies in `bodies`.
void WritePage(const std::string& path, const std::string& content,
               const std::map<std::string, int>& xobjects,
               const std::vector<std::string>& more,
               std::map<int, std::string> bodies) {
    std::string xo;
    for (const auto& [name, id] : xobjects)
        xo += "/" + name + " " + std::to_string(id) + " 0 R ";
    std::vector<std::string> objs = {
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Contents 4 0 R "
        "/Resources << /Font << /F1 5 0 R >> /XObject << " + xo + ">> >> >>",
        StreamDict("", content),
        kFont};
    objs.insert(objs.end(), more.begin(), more.end());
    bodies[4] = content;
    WritePdf(path, objs, bodies);
}

std::string PageText(const std::string& path) {
    Document doc{path};
    Text::TextAbsorber abs;
    auto page = doc.Pages()[1];
    abs.Visit(page);
    return abs.Text();
}

int Count(const std::string& s, const std::string& needle) {
    int n = 0;
    for (std::size_t p = 0; (p = s.find(needle, p)) != std::string::npos;
         p += needle.size())
        ++n;
    return n;
}

}  // namespace

TEST(TextXObject, FormXObjectTextIsExtracted) {
    const std::string path = TmpPath("xobj_basic");
    const std::string form = "BT /F1 12 Tf 0 0 Td (Inside) Tj ET";
    WritePage(path, "BT /F1 12 Tf 100 750 Td (Page) Tj ET q 1 0 0 1 100 700 cm /X1 Do Q",
              {{"X1", 6}},
              {StreamDict("/Type /XObject /Subtype /Form /BBox [0 0 200 20] "
                          "/Resources << /Font << /F1 5 0 R >> >>",
                          form)},
              {{6, form}});
    const std::string text = PageText(path);
    EXPECT_NE(text.find("Page"), std::string::npos);
    EXPECT_NE(text.find("Inside"), std::string::npos);
    std::filesystem::remove(path);
}

TEST(TextXObject, PagePositionSeparatesLines) {
    // Both forms draw at local y = 0; cm puts them on different lines.
    const std::string path = TmpPath("xobj_lines");
    const std::string a = "BT /F1 12 Tf 0 0 Td (Alpha) Tj ET";
    const std::string b = "BT /F1 12 Tf 0 0 Td (Beta) Tj ET";
    WritePage(path,
              "q 1 0 0 1 100 700 cm /XA Do Q q 1 0 0 1 100 650 cm /XB Do Q",
              {{"XA", 6}, {"XB", 7}},
              {StreamDict("/Type /XObject /Subtype /Form /BBox [0 0 200 20]", a),
               StreamDict("/Type /XObject /Subtype /Form /BBox [0 0 200 20]", b)},
              {{6, a}, {7, b}});
    EXPECT_EQ(PageText(path), "Alpha\nBeta");
    std::filesystem::remove(path);
}

TEST(TextXObject, FormWithoutResourcesUsesCallers) {
    const std::string path = TmpPath("xobj_noresources");
    const std::string form = "BT /F1 12 Tf 0 0 Td (Borrowed) Tj ET";
    WritePage(path, "/X1 Do", {{"X1", 6}},
              {StreamDict("/Type /XObject /Subtype /Form /BBox [0 0 200 20]",
                          form)},
              {{6, form}});
    EXPECT_NE(PageText(path).find("Borrowed"), std::string::npos);
    std::filesystem::remove(path);
}

TEST(TextXObject, SelfReferencingFormTerminates) {
    const std::string path = TmpPath("xobj_cycle");
    const std::string form = "/X1 Do BT /F1 12 Tf 0 0 Td (Loop) Tj ET";
    WritePage(path, "/X1 Do", {{"X1", 6}},
              {StreamDict("/Type /XObject /Subtype /Form /BBox [0 0 200 20] "
                          "/Resources << /Font << /F1 5 0 R >> "
                          "/XObject << /X1 6 0 R >> >>",
                          form)},
              {{6, form}});
    std::string text;
    ASSERT_NO_THROW(text = PageText(path));
    EXPECT_EQ(Count(text, "Loop"), 1);
    std::filesystem::remove(path);
}

TEST(TextXObject, FlattenedFieldValueIsExtracted) {
    const std::string out = TmpPath("xobj_flatten");
    {
        Document doc{HelloWorldPdf()};
        Forms::TextBoxField f{doc, Rect(100, 400, 300, 420)};
        f.Value("Burned");
        doc.Form().Add(f, "f", 1);
        doc.Form().Flatten();
        doc.Save(out);
    }
    EXPECT_NE(PageText(out).find("Burned"), std::string::npos);
    std::filesystem::remove(out);
}
