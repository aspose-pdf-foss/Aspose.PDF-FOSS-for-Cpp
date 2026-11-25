// =============================================================================
// forms_flatten_smoke_test — Form::Flatten() + Save() burn every visible
// widget's appearance into its page content (no values lost), and value-edited
// loaded fields get a fresh /AP on a plain Save.
// =============================================================================

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/square_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/checkbox_field.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/list_box_field.hpp>
#include <aspose/pdf/forms/radio_button_field.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/resolution.hpp>

#include "objects.hpp"
#include "pages_tree.hpp"
#include "png_decoder.hpp"
#include "trailer.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using Aspose::Pdf::Forms::CheckboxField;
using Aspose::Pdf::Forms::Field;
using Aspose::Pdf::Forms::TextBoxField;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::string TmpPath(const std::string& tag) {
    return (std::filesystem::temp_directory_path() /
            ("asp_form_flatten_" + tag + ".pdf"))
        .string();
}

Rectangle Rect(double a, double b, double c, double d) {
    return Rectangle(a, b, c, d, false);
}

std::string ReadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

bool Has(const std::string& s, const std::string& needle) {
    return s.find(needle) != std::string::npos;
}

void WriteFile(const std::string& path, const std::string& bytes) {
    std::ofstream out(path, std::ios::binary);
    out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
}

std::string ReplaceAll(std::string s, const std::string& from,
                       const std::string& to) {
    for (std::size_t pos = 0;
         (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
        s.replace(pos, from.size(), to);
    return s;
}

Field* FieldByName(Document& doc, const std::string& name) {
    for (Field* f : doc.Form().Fields()) {
        if (f != nullptr && f->PartialName() == name) return f;
    }
    return nullptr;
}

}  // namespace

TEST(FormsFlatten, FlattenEmptiesFormForUser) {
    Document doc{HelloWorldPdf()};
    TextBoxField tb{doc, Rect(100, 400, 300, 420)};
    tb.Value("Hello");
    doc.Form().Add(tb, 1);
    ASSERT_EQ(doc.Form().Count(), 1);
    doc.Form().Flatten();
    EXPECT_EQ(doc.Form().Count(), 0);
}

TEST(FormsFlatten, FlattenedNewFieldIsWrittenBeforeRemoval) {
    const std::string out = TmpPath("newfield");
    {
        Document doc{HelloWorldPdf()};
        TextBoxField tb{doc, Rect(100, 400, 300, 420)};
        tb.Value("Hello");
        doc.Form().Add(tb, 1);
        doc.Form().Flatten();
        doc.Save(out);
    }
    EXPECT_TRUE(Has(ReadFile(out), "(Hello) Tj"));
    Document re{out};
    EXPECT_EQ(re.Form().Count(), 0);
    std::filesystem::remove(out);
}

TEST(FormsFlatten, EditedLoadedTextFieldGetsNewAppearance) {
    const std::string out = TmpPath("edit1");
    const std::string out2 = TmpPath("edit2");
    {
        Document doc{HelloWorldPdf()};
        TextBoxField tb{doc, Rect(100, 400, 300, 420)};
        tb.Value("Old");
        doc.Form().Add(tb, "name", 1);
        doc.Save(out);
    }
    {
        Document re{out};
        Field* f = FieldByName(re, "name");
        ASSERT_NE(f, nullptr);
        f->Value("New");
        re.Save(out2);
    }
    EXPECT_TRUE(Has(ReadFile(out2), "(New) Tj"));
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}

TEST(FormsFlatten, EditedLoadedCheckboxKeepsItsOnStateName) {
    const std::string out = TmpPath("chk1");
    const std::string out2 = TmpPath("chk2");
    {
        Document doc{HelloWorldPdf()};
        CheckboxField cb{doc, Rect(100, 560, 120, 580)};
        doc.Form().Add(cb, "agree", 1);
        doc.Save(out);
    }
    // Rename the on-state from /Yes to /Chk (same length keeps the xref
    // offsets valid), as a file authored by another tool might.
    const std::string original = ReadFile(out);
    ASSERT_TRUE(Has(original, "/Yes"));
    WriteFile(out, ReplaceAll(original, "/Yes", "/Chk"));
    {
        Document re{out};
        auto* cb = dynamic_cast<CheckboxField*>(FieldByName(re, "agree"));
        ASSERT_NE(cb, nullptr);
        cb->Checked(true);
        re.Save(out2);
    }
    const std::string saved = ReadFile(out2);
    EXPECT_TRUE(Has(saved, "/AS /Chk"));
    EXPECT_FALSE(Has(saved, "/AS /Yes"));
    std::filesystem::remove(out);
    std::filesystem::remove(out2);
}

// ---- Flatten pass: inspection helpers ---------------------------------------

namespace {

namespace fo = foundation::objects;

std::span<const std::byte> Bytes(const std::string& s) {
    return {reinterpret_cast<const std::byte*>(s.data()), s.size()};
}

const fo::Value* Get(const fo::Dict& d, const std::string& key) {
    for (const auto& kv : d.entries)
        if (kv.first == key) return &kv.second;
    return nullptr;
}

const fo::IndirectObject* Find(const fo::Dump& dump, std::uint32_t id) {
    for (const auto& o : dump.objects)
        if (o.id == id) return &o;
    return nullptr;
}

// Follow a reference to its dict (or return an inline dict); empty if none.
fo::Dict ResolveDict(const fo::Dump& dump, const fo::Value* v) {
    if (v == nullptr) return {};
    if (const auto* d = std::get_if<fo::Dict>(&v->v)) return *d;
    if (const auto* r = std::get_if<fo::Ref>(&v->v)) {
        if (const auto* o = Find(dump, r->id)) {
            if (const auto* d = std::get_if<fo::Dict>(&o->value.v)) return *d;
            if (const auto* s = std::get_if<fo::Stream>(&o->value.v))
                return s->header;
        }
    }
    return {};
}

struct Parsed {
    fo::Dump dump;
    fo::Dict catalog;
    fo::Dict page;  // the requested page's dict
};

Parsed ParsePage(const std::string& bytes, std::size_t page_index) {
    Parsed p;
    const auto span = Bytes(bytes);
    p.dump = fo::Parse(span);
    const auto trailer = foundation::trailer::Parse(span);
    if (const auto* c = Find(p.dump, trailer.root_id))
        if (const auto* d = std::get_if<fo::Dict>(&c->value.v)) p.catalog = *d;
    const auto tree = foundation::pages_tree::Parse(span);
    if (page_index < tree.leaves.size())
        if (const auto* o = Find(p.dump, tree.leaves[page_index].id))
            if (const auto* d = std::get_if<fo::Dict>(&o->value.v)) p.page = *d;
    return p;
}

std::vector<fo::Dict> AnnotDicts(const Parsed& p) {
    std::vector<fo::Dict> out;
    const fo::Value* a = Get(p.page, "Annots");
    const fo::Array* arr = a ? std::get_if<fo::Array>(&a->v) : nullptr;
    fo::Array resolved;
    if (a != nullptr && arr == nullptr) {
        if (const auto* r = std::get_if<fo::Ref>(&a->v))
            if (const auto* o = Find(p.dump, r->id))
                if (const auto* ra = std::get_if<fo::Array>(&o->value.v))
                    resolved = *ra;
        arr = &resolved;
    }
    if (arr == nullptr) return out;
    for (const auto& it : arr->items) out.push_back(ResolveDict(p.dump, &it));
    return out;
}

std::string NameOf(const fo::Value* v) {
    if (v == nullptr) return {};
    if (const auto* n = std::get_if<std::string>(&v->v)) return *n;
    return {};
}

int CountSubtype(const Parsed& p, const std::string& subtype) {
    int n = 0;
    for (const auto& d : AnnotDicts(p))
        if (NameOf(Get(d, "Subtype")) == subtype) ++n;
    return n;
}

// The page's /Resources /XObject entries: name -> referenced object id.
std::map<std::string, std::uint32_t> XObjects(const Parsed& p) {
    std::map<std::string, std::uint32_t> out;
    const fo::Dict res = ResolveDict(p.dump, Get(p.page, "Resources"));
    const fo::Dict xo = ResolveDict(p.dump, Get(res, "XObject"));
    for (const auto& kv : xo.entries)
        if (const auto* r = std::get_if<fo::Ref>(&kv.second.v))
            out[kv.first] = r->id;
    return out;
}

int FlattenedCount(const Parsed& p) {
    int n = 0;
    for (const auto& kv : XObjects(p))
        if (kv.first.rfind("AspFl", 0) == 0) ++n;
    return n;
}

// Render page `page_number` (1-based) at 72 dpi: 1 px == 1 pt.
foundation::png_decoder::DecodedImage Render(Document& doc, int page_number) {
    Aspose::Pdf::Devices::PngDevice png(Aspose::Pdf::Devices::Resolution(72));
    std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
    auto page = doc.Pages()[page_number];
    png.Process(page, sink);
    const std::string bytes = sink.str();
    return foundation::png_decoder::Decode(Bytes(bytes));
}

// Count pixels inside the PDF-space rect (shrunk by `inset` pt) matching
// `pred(r, g, b)`.
template <class Pred>
int CountPixels(const foundation::png_decoder::DecodedImage& img,
                double llx, double lly, double urx, double ury,
                double inset, Pred pred) {
    int n = 0;
    const int x0 = static_cast<int>(llx + inset);
    const int x1 = static_cast<int>(urx - inset);
    const int y0 = static_cast<int>(img.height - (ury - inset));
    const int y1 = static_cast<int>(img.height - (lly + inset));
    for (int y = std::max(0, y0); y < std::min<int>(img.height, y1); ++y) {
        for (int x = std::max(0, x0); x < std::min<int>(img.width, x1); ++x) {
            const std::size_t i = (static_cast<std::size_t>(y) * img.width + x) * 4;
            const int r = std::to_integer<int>(img.pixels[i]);
            const int g = std::to_integer<int>(img.pixels[i + 1]);
            const int b = std::to_integer<int>(img.pixels[i + 2]);
            if (pred(r, g, b)) ++n;
        }
    }
    return n;
}

int DarkPixels(const foundation::png_decoder::DecodedImage& img, double llx,
               double lly, double urx, double ury) {
    return CountPixels(img, llx, lly, urx, ury, 3.0,
                       [](int r, int g, int b) { return r + g + b < 384; });
}

int GreenPixels(const foundation::png_decoder::DecodedImage& img, double llx,
                double lly, double urx, double ury) {
    return CountPixels(img, llx, lly, urx, ury, 1.0, [](int r, int g, int b) {
        return g > r + 40 && g > b + 40;
    });
}

// The /AP /N stream id of the first widget on the page (0 if none).
std::uint32_t FirstWidgetApId(const Parsed& p) {
    for (const auto& d : AnnotDicts(p)) {
        if (NameOf(Get(d, "Subtype")) != "Widget") continue;
        const fo::Dict ap = ResolveDict(p.dump, Get(d, "AP"));
        if (const auto* n = Get(ap, "N"))
            if (const auto* r = std::get_if<fo::Ref>(&n->v)) return r->id;
    }
    return 0;
}

// Save `doc` with one text field "name" = `value` on page 1 to `path`.
void SaveTextForm(const std::string& path, const std::string& value) {
    Document doc{HelloWorldPdf()};
    TextBoxField tb{doc, Rect(100, 400, 300, 420)};
    tb.Value(value);
    doc.Form().Add(tb, "name", 1);
    doc.Save(path);
}

}  // namespace

TEST(FormsFlatten, FlattenBurnsNewTextValue) {
    const std::string out = TmpPath("burn_new");
    int before = 0;
    {
        Document plain{HelloWorldPdf()};
        before = DarkPixels(Render(plain, 1), 100, 400, 300, 420);
    }
    {
        Document doc{HelloWorldPdf()};
        TextBoxField tb{doc, Rect(100, 400, 300, 420)};
        tb.Value("Hello");
        doc.Form().Add(tb, 1);
        doc.Form().Flatten();
        doc.Save(out);
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(Get(p.catalog, "AcroForm"), nullptr);
    EXPECT_EQ(CountSubtype(p, "Widget"), 0);
    EXPECT_EQ(FlattenedCount(p), 1);
    Document re{out};
    EXPECT_GT(DarkPixels(Render(re, 1), 100, 400, 300, 420), before + 20);
    std::filesystem::remove(out);
}

TEST(FormsFlatten, FlattenBurnsEditedLoadedValue) {
    const std::string src = TmpPath("edited_src");
    const std::string plain_out = TmpPath("edited_plain");
    const std::string edited_out = TmpPath("edited_out");
    SaveTextForm(src, "i");
    {
        Document re{src};
        re.Form().Flatten();
        re.Save(plain_out);
    }
    {
        Document re{src};
        Field* f = FieldByName(re, "name");
        ASSERT_NE(f, nullptr);
        f->Value("WWWWWWWW");
        re.Form().Flatten();
        re.Save(edited_out);
    }
    EXPECT_EQ(FlattenedCount(ParsePage(ReadFile(edited_out), 0)), 1);
    EXPECT_EQ(CountSubtype(ParsePage(ReadFile(edited_out), 0), "Widget"), 0);
    Document plain{plain_out};
    Document edited{edited_out};
    EXPECT_GT(DarkPixels(Render(edited, 1), 100, 400, 300, 420),
              DarkPixels(Render(plain, 1), 100, 400, 300, 420) + 20);
    std::filesystem::remove(src);
    std::filesystem::remove(plain_out);
    std::filesystem::remove(edited_out);
}

TEST(FormsFlatten, FlattenReusesUntouchedLoadedAppearance) {
    const std::string src = TmpPath("reuse_src");
    const std::string out = TmpPath("reuse_out");
    SaveTextForm(src, "Keep");
    const std::uint32_t ap_id = FirstWidgetApId(ParsePage(ReadFile(src), 0));
    ASSERT_NE(ap_id, 0u);
    {
        Document re{src};
        re.Form().Flatten();
        re.Save(out);
    }
    const auto xo = XObjects(ParsePage(ReadFile(out), 0));
    ASSERT_EQ(xo.count("AspFl1"), 1u);
    EXPECT_EQ(xo.at("AspFl1"), ap_id);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormsFlatten, FlattenCheckboxStates) {
    const std::string out = TmpPath("checks");
    {
        Document doc{HelloWorldPdf()};
        CheckboxField on{doc, Rect(100, 560, 130, 590)};
        on.Checked(true);
        CheckboxField off{doc, Rect(200, 560, 230, 590)};
        doc.Form().Add(on, "on", 1);
        doc.Form().Add(off, "off", 1);
        doc.Form().Flatten();
        doc.Save(out);
    }
    Document re{out};
    const auto img = Render(re, 1);
    EXPECT_GT(GreenPixels(img, 100, 560, 130, 590), 5);
    EXPECT_EQ(GreenPixels(img, 200, 560, 230, 590), 0);
    EXPECT_EQ(FlattenedCount(ParsePage(ReadFile(out), 0)), 2);
    std::filesystem::remove(out);
}

TEST(FormsFlatten, FlattenSkipsHiddenWidgetKeepsOtherAnnots) {
    const std::string src = TmpPath("hidden_src");
    const std::string out = TmpPath("hidden_out");
    SaveTextForm(src, "Secret");
    // /F 4 (Print) -> /F 2 (Hidden); same length keeps the xref valid.
    const std::string original = ReadFile(src);
    ASSERT_TRUE(Has(original, "/F 4"));
    WriteFile(src, ReplaceAll(original, "/F 4", "/F 2"));
    {
        Document re{src};
        auto page = re.Pages()[1];
        Aspose::Pdf::Annotations::SquareAnnotation sq{page,
                                                      Rect(50, 50, 90, 90)};
        page.Annotations().Add(sq);
        re.Form().Flatten();
        re.Save(out);
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(FlattenedCount(p), 0);
    EXPECT_EQ(CountSubtype(p, "Widget"), 0);
    EXPECT_EQ(CountSubtype(p, "Square"), 1);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormsFlatten, FlattenFieldWithoutPage) {
    const std::string out = TmpPath("nopage");
    {
        Document doc{HelloWorldPdf()};
        TextBoxField tb{doc, Rect(100, 400, 300, 420)};
        tb.Value("Floating");
        doc.Form().Add(tb);
        doc.Form().Flatten();
        EXPECT_NO_THROW(doc.Save(out));
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(Get(p.catalog, "AcroForm"), nullptr);
    Document re{out};
    EXPECT_EQ(re.Pages().Count(), 1u);
    std::filesystem::remove(out);
}

TEST(FormsFlatten, FlattenOnBlankNewPage) {
    const std::string out = TmpPath("blank");
    {
        Document doc;
        doc.Pages().Add();
        TextBoxField tb{doc, Rect(100, 400, 300, 420)};
        tb.Value("Blank page");
        doc.Form().Add(tb, 1);
        doc.Form().Flatten();
        doc.Save(out);
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(FlattenedCount(p), 1);
    EXPECT_EQ(CountSubtype(p, "Widget"), 0);
    Document re{out};
    EXPECT_GT(DarkPixels(Render(re, 1), 100, 400, 300, 420), 20);
    std::filesystem::remove(out);
}

TEST(FormsFlatten, FlattenStateDictWithoutMatchingAS) {
    const std::string src = TmpPath("noas_src");
    const std::string out = TmpPath("noas_out");
    {
        Document doc{HelloWorldPdf()};
        CheckboxField cb{doc, Rect(100, 560, 130, 590)};
        doc.Form().Add(cb, "agree", 1);
        doc.Save(src);
    }
    const std::string original = ReadFile(src);
    ASSERT_TRUE(Has(original, "/AS /Off"));
    WriteFile(src, ReplaceAll(original, "/AS /Off", "/AS /Zzz"));
    {
        Document re{src};
        re.Form().Flatten();
        EXPECT_NO_THROW(re.Save(out));
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(FlattenedCount(p), 0);
    EXPECT_EQ(CountSubtype(p, "Widget"), 0);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormsFlatten, FlattenReversedRect) {
    const std::string out = TmpPath("reversed");
    int before = 0;
    {
        Document plain{HelloWorldPdf()};
        before = DarkPixels(Render(plain, 1), 100, 400, 300, 420);
    }
    {
        Document doc{HelloWorldPdf()};
        TextBoxField tb{doc, Rect(300, 420, 100, 400)};
        tb.Value("Reversed");
        doc.Form().Add(tb, 1);
        doc.Form().Flatten();
        doc.Save(out);
    }
    Document re{out};
    EXPECT_GT(DarkPixels(Render(re, 1), 100, 400, 300, 420), before + 20);
    std::filesystem::remove(out);
}

TEST(FormsFlatten, FlattenNoFields) {
    const std::string out = TmpPath("nofields");
    {
        Document doc{HelloWorldPdf()};
        doc.Form().Flatten();
        EXPECT_NO_THROW(doc.Save(out));
    }
    Document re{out};
    EXPECT_EQ(re.Pages().Count(), 1u);
    EXPECT_EQ(re.Form().Count(), 0);
    std::filesystem::remove(out);
}

TEST(FormsFlatten, FlattenRadioGroupShowsSelectedOption) {
    const std::string out = TmpPath("radio");
    {
        Document doc{HelloWorldPdf()};
        Aspose::Pdf::Forms::RadioButtonField rb{doc};
        rb.AddOption("a", Rect(100, 300, 120, 320));
        rb.AddOption("b", Rect(200, 300, 220, 320));
        rb.Value("b");
        doc.Form().Add(rb, "color", 1);
        doc.Form().Flatten();
        doc.Save(out);
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(FlattenedCount(p), 2);
    EXPECT_EQ(CountSubtype(p, "Widget"), 0);
    Document re{out};
    const auto img = Render(re, 1);
    EXPECT_GT(DarkPixels(img, 200, 300, 220, 320),
              DarkPixels(img, 100, 300, 120, 320) + 20);
    std::filesystem::remove(out);
}

TEST(FormsFlatten, FlattenListBoxShowsSelection) {
    const std::string out = TmpPath("list");
    {
        Document doc{HelloWorldPdf()};
        Aspose::Pdf::Forms::ListBoxField lb{doc, Rect(100, 200, 200, 260)};
        lb.AddOption("one");
        lb.AddOption("two");
        lb.Selected(1);
        doc.Form().Add(lb, "pick", 1);
        doc.Form().Flatten();
        doc.Save(out);
    }
    EXPECT_EQ(FlattenedCount(ParsePage(ReadFile(out), 0)), 1);
    Document re{out};
    const int highlight =
        CountPixels(Render(re, 1), 100, 200, 200, 260, 2.0,
                    [](int r, int, int b) { return b > r + 50; });
    EXPECT_GT(highlight, 20);
    std::filesystem::remove(out);
}

// A page /Annots rewrite (an annotation added or removed) must keep the
// entries the annotation collection does not model, such as form widgets.
TEST(FormsAnnotations, AddingAnnotationKeepsLoadedWidgets) {
    const std::string src = TmpPath("annadd_src");
    const std::string out = TmpPath("annadd_out");
    SaveTextForm(src, "Keep");
    {
        Document re{src};
        auto page = re.Pages()[1];
        Aspose::Pdf::Annotations::SquareAnnotation sq{page,
                                                      Rect(50, 50, 90, 90)};
        page.Annotations().Add(sq);
        re.Save(out);
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(CountSubtype(p, "Widget"), 1);
    EXPECT_EQ(CountSubtype(p, "Square"), 1);
    Document re2{out};
    EXPECT_EQ(re2.Form().Count(), 1);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormsAnnotations, DeletingAnnotationKeepsLoadedWidgets) {
    const std::string src = TmpPath("anndel_src");
    const std::string out = TmpPath("anndel_out");
    {
        Document doc{HelloWorldPdf()};
        TextBoxField tb{doc, Rect(100, 400, 300, 420)};
        tb.Value("Keep");
        doc.Form().Add(tb, "name", 1);
        auto page = doc.Pages()[1];
        Aspose::Pdf::Annotations::SquareAnnotation sq{page,
                                                      Rect(50, 50, 90, 90)};
        page.Annotations().Add(sq);
        doc.Save(src);
    }
    ASSERT_EQ(CountSubtype(ParsePage(ReadFile(src), 0), "Widget"), 1);
    {
        Document re{src};
        auto page = re.Pages()[1];
        ASSERT_EQ(page.Annotations().Count(), 1);
        page.Annotations().Delete(0);  // 0-based
        re.Save(out);
    }
    const Parsed p = ParsePage(ReadFile(out), 0);
    EXPECT_EQ(CountSubtype(p, "Widget"), 1);
    EXPECT_EQ(CountSubtype(p, "Square"), 0);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}
