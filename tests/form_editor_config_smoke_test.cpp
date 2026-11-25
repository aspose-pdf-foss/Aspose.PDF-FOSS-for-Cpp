// =============================================================================
// form_editor_config_smoke_test — FormEditor field configuration: flags,
// limits, comb, move, rename, alignment, appearance flags, list items,
// copying fields (within the document and from another file), decoration
// from FormFieldFacade, and radio groups built from Items. Edits persist
// through Save for new fields and for fields loaded from a file.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_flags.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/field_type.hpp>
#include <aspose/pdf/facades/font_style.hpp>
#include <aspose/pdf/facades/form_editor.hpp>
#include <aspose/pdf/facades/form_field_facade.hpp>
#include <aspose/pdf/facades/property_flag.hpp>
#include <aspose/pdf/forms/choice_field.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;
using namespace flatten_test;

namespace {

std::string TwoPagesPdf() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "text_extractor" / "two_pages.pdf")
        .string();
}

Forms::Field* FieldByName(Document& doc, const std::string& name) {
    for (Forms::Field* f : doc.Form().Fields())
        if (f != nullptr && f->PartialName() == name) return f;
    return nullptr;
}

bool Has(const std::string& s, const std::string& needle) {
    return s.find(needle) != std::string::npos;
}

// `base` with a text field "f" = "Value" on page 1 at (100,400)-(300,420),
// saved to `path`.
void SaveTextField(const std::string& path,
                   const std::string& base = HelloWorldPdf()) {
    Document doc{base};
    FormEditor ed{doc};
    ASSERT_TRUE(ed.AddField(FieldType::Text, "f", 1, 100, 400, 300, 420));
    FieldByName(doc, "f")->Value("Value");
    doc.Save(path);
}

// Open `src` with a file-based FormEditor, run `edit`, save to `out`.
template <class Edit>
void EditFile(const std::string& src, const std::string& out, Edit edit) {
    FormEditor ed{src, out};
    edit(ed);
    ed.Save();
}

}  // namespace

TEST(FormEditorConfig, AttributesLimitAndMultilinePersist) {
    const std::string src = TmpPath("fe_attr_src");
    const std::string out = TmpPath("fe_attr_out");
    SaveTextField(src);
    EditFile(src, out, [](FormEditor& ed) {
        EXPECT_TRUE(ed.SetFieldAttribute("f", PropertyFlag::ReadOnly));
        EXPECT_TRUE(ed.SetFieldAttribute("f", PropertyFlag::Required));
        EXPECT_FALSE(ed.SetFieldAttribute("f", PropertyFlag::InvalidFlag));
        EXPECT_TRUE(ed.SetFieldLimit("f", 5));
        EXPECT_TRUE(ed.Single2Multiple("f"));
        EXPECT_FALSE(ed.SetFieldLimit("missing", 5));
    });
    Document re{out};
    auto* f = dynamic_cast<Forms::TextBoxField*>(FieldByName(re, "f"));
    ASSERT_NE(f, nullptr);
    EXPECT_TRUE(f->ReadOnly());
    EXPECT_TRUE(f->Required());
    EXPECT_EQ(f->MaxLen(), 5);
    EXPECT_TRUE(f->Multiline());
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormEditorConfig, CombNumberMoveAndRename) {
    const std::string src = TmpPath("fe_comb_src");
    const std::string out = TmpPath("fe_comb_out");
    SaveTextField(src);
    EditFile(src, out, [](FormEditor& ed) {
        EXPECT_TRUE(ed.SetFieldCombNumber("f", 4));
        EXPECT_TRUE(ed.MoveField("f", 50, 50, 250, 70));
        ed.RenameField("f", "g");
    });
    Document re{out};
    EXPECT_EQ(FieldByName(re, "f"), nullptr);
    auto* g = dynamic_cast<Forms::TextBoxField*>(FieldByName(re, "g"));
    ASSERT_NE(g, nullptr);
    EXPECT_TRUE(g->ForceCombs());
    EXPECT_EQ(g->MaxLen(), 4);
    EXPECT_DOUBLE_EQ(g->Rect().LLY(), 50.0);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormEditorConfig, HorizontalAndVerticalAlignment) {
    const std::string out = TmpPath("fe_align");
    {
        Document doc{HelloWorldPdf()};
        FormEditor ed{doc};
        ed.AddField(FieldType::Text, "top", 1, 100, 400, 300, 460);
        ed.AddField(FieldType::Text, "right", 1, 100, 300, 400, 320);
        FieldByName(doc, "top")->Value("Hi");
        FieldByName(doc, "right")->Value("Hi");
        EXPECT_TRUE(ed.SetFieldAlignmentV("top", FormFieldFacade::AlignTop));
        EXPECT_TRUE(ed.SetFieldAlignment("right", FormFieldFacade::AlignRight));
        doc.Save(out);
    }
    EXPECT_TRUE(Has(ReadFile(out), "/Q 2"));
    Document re{out};
    const auto img = Render(re, 1);
    EXPECT_GT(DarkPixels(img, 100, 435, 300, 460), 5);   // text at the top
    EXPECT_EQ(DarkPixels(img, 100, 400, 300, 425), 0);
    EXPECT_EQ(DarkPixels(img, 100, 300, 300, 320), 0);   // right-aligned
    EXPECT_GT(DarkPixels(img, 350, 300, 400, 320), 5);
    std::filesystem::remove(out);
}

TEST(FormEditorConfig, AppearanceFlagsPersist) {
    const std::string src = TmpPath("fe_flags_src");
    const std::string out = TmpPath("fe_flags_out");
    SaveTextField(src);
    EditFile(src, out, [](FormEditor& ed) {
        EXPECT_TRUE(ed.SetFieldAppearance(
            "f", Annotations::AnnotationFlags::Hidden));
        EXPECT_EQ(ed.GetFieldAppearance("f"),
                  Annotations::AnnotationFlags::Hidden);
    });
    Document re{out};
    EXPECT_EQ(FieldByName(re, "f")->Flags(), Annotations::AnnotationFlags::Hidden);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormEditorConfig, ListItemsFromItemsAndEdits) {
    const std::string src = TmpPath("fe_list_src");
    const std::string out = TmpPath("fe_list_out");
    {
        Document doc{HelloWorldPdf()};
        FormEditor ed{doc};
        ed.Items({"x", "y"});
        ASSERT_TRUE(ed.AddField(FieldType::ListBox, "l", 1, 100, 300, 200, 360));
        doc.Save(src);
    }
    EditFile(src, out, [](FormEditor& ed) {
        ed.AddListItem("l", "z");
        ed.DelListItem("l", "x");
    });
    Document re{out};
    auto* l = dynamic_cast<Forms::ChoiceField*>(FieldByName(re, "l"));
    ASSERT_NE(l, nullptr);
    ASSERT_EQ(l->Options().Count(), 2);
    EXPECT_EQ(l->Options()[0].Value(), "y");
    EXPECT_EQ(l->Options()[1].Value(), "z");
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormEditorConfig, CopyInnerField) {
    const std::string src = TmpPath("fe_inner_src");
    const std::string out = TmpPath("fe_inner_out");
    SaveTextField(src, TwoPagesPdf());
    EditFile(src, out, [](FormEditor& ed) {
        ed.CopyInnerField("f", "g", 2);
        ed.CopyInnerField("f", "h", 1, 50, 60);
    });
    Document re{out};
    auto* g = FieldByName(re, "g");
    auto* h = FieldByName(re, "h");
    ASSERT_NE(g, nullptr);
    ASSERT_NE(h, nullptr);
    EXPECT_EQ(g->Value(), "Value");
    EXPECT_DOUBLE_EQ(g->Rect().LLY(), 400.0);
    EXPECT_DOUBLE_EQ(h->Rect().LLX(), 50.0);
    EXPECT_DOUBLE_EQ(h->Rect().LLY(), 60.0);
    EXPECT_DOUBLE_EQ(h->Rect().URX() - h->Rect().LLX(), 200.0);
    EXPECT_EQ(CountSubtype(ParsePage(ReadFile(out), 1), "Widget"), 1);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormEditorConfig, CopyOuterField) {
    const std::string other = TmpPath("fe_outer_other");
    const std::string out = TmpPath("fe_outer_out");
    SaveTextField(other);
    {
        Document doc{HelloWorldPdf()};
        FormEditor ed{doc};
        ed.CopyOuterField(other, "f");
        ed.CopyOuterField(other, "f", 1, 20, 30);  // same name: placed again
        doc.Save(out);
    }
    Document re{out};
    ASSERT_GE(re.Form().Count(), 1);
    auto* f = FieldByName(re, "f");
    ASSERT_NE(f, nullptr);
    EXPECT_EQ(f->Value(), "Value");
    EXPECT_DOUBLE_EQ(f->Rect().LLY(), 400.0);
    std::filesystem::remove(other);
    std::filesystem::remove(out);
}

TEST(FormEditorConfig, DecorateFieldFromFacade) {
    const std::string src = TmpPath("fe_decorate_src");
    const std::string out = TmpPath("fe_decorate_out");
    SaveTextField(src);
    EditFile(src, out, [](FormEditor& ed) {
        FormFieldFacade facade;
        facade.Font(FontStyle::HelveticaBold);
        facade.FontSize(16);
        facade.Alignment(FormFieldFacade::AlignRight);
        ed.Facade(facade);
        ed.DecorateField("f");
    });
    const std::string bytes = ReadFile(out);
    EXPECT_TRUE(Has(bytes, "/HeBo 16 Tf"));
    EXPECT_TRUE(Has(bytes, "/Q 2"));
    EXPECT_TRUE(Has(bytes, "/BaseFont /Helvetica-Bold"));
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormEditorConfig, DecorateAllFieldsOfAType) {
    const std::string out = TmpPath("fe_decorate_type");
    {
        Document doc{HelloWorldPdf()};
        FormEditor ed{doc};
        ed.AddField(FieldType::Text, "a", 1, 100, 400, 300, 420);
        ed.AddField(FieldType::Text, "b", 1, 100, 350, 300, 370);
        ed.AddField(FieldType::CheckBox, "c", 1, 100, 300, 120, 320);
        FormFieldFacade facade;
        facade.Font(FontStyle::TimesRoman);
        facade.FontSize(9);
        ed.Facade(facade);
        ed.DecorateField(FieldType::Text);
        doc.Save(out);
    }
    const std::string bytes = ReadFile(out);
    int count = 0;
    for (std::size_t p = 0; (p = bytes.find("/DA (/TiRo 9 Tf", p)) != std::string::npos;
         ++p)
        ++count;
    EXPECT_EQ(count, 2);
    std::filesystem::remove(out);
}

TEST(FormEditorConfig, RadioGroupFromItems) {
    const std::string out = TmpPath("fe_radio");
    {
        Document doc{HelloWorldPdf()};
        FormEditor ed{doc};
        ed.Items({"a", "b", "c"});
        ed.RadioHoriz(true);
        ed.RadioGap(10);
        ed.RadioButtonItemSize(20);
        ASSERT_TRUE(ed.AddField(FieldType::Radio, "r", 1, 100, 500, 120, 520));
        doc.Save(out);
    }
    EXPECT_EQ(CountSubtype(ParsePage(ReadFile(out), 0), "Widget"), 3);
    std::filesystem::remove(out);
}

TEST(FormEditorConfig, MultiLineTextFieldType) {
    Document doc{HelloWorldPdf()};
    FormEditor ed{doc};
    ASSERT_TRUE(ed.AddField(FieldType::MultiLineText, "m", 1, 100, 300, 300, 400));
    auto* m = dynamic_cast<Forms::TextBoxField*>(FieldByName(doc, "m"));
    ASSERT_NE(m, nullptr);
    EXPECT_TRUE(m->Multiline());
}
