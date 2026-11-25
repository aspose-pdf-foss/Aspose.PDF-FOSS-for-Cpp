// =============================================================================
// form_editor_actions_smoke_test — FormEditor actions: JavaScript on push
// buttons (SetFieldScript / AddFieldScript), form submission
// (SetSubmitUrl / SetSubmitFlag / AddSubmitBtn) and RemoveFieldAction, for
// new buttons and buttons loaded from a file.
// =============================================================================

#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/field_type.hpp>
#include <aspose/pdf/facades/form_editor.hpp>
#include <aspose/pdf/facades/submit_form_flag.hpp>
#include <aspose/pdf/forms/form.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;
using namespace flatten_test;

namespace {

// The /A dict of widget `name` (by /T) on page 1 of `path`.
fo::Dict WidgetAction(const std::string& path, const std::string& name) {
    const Parsed p = ParsePage(ReadFile(path), 0);
    for (const auto& d : AnnotDicts(p)) {
        if (NameOf(Get(d, "Subtype")) != "Widget") continue;
        const fo::Value* t = Get(d, "T");
        const auto* s = t ? std::get_if<fo::String>(&t->v) : nullptr;
        if (s == nullptr) continue;
        const std::string tn(reinterpret_cast<const char*>(s->bytes.data()),
                             s->bytes.size());
        if (tn == name) return ResolveDict(p.dump, Get(d, "A"));
    }
    return {};
}

std::string StringOf(const fo::Dict& d, const std::string& key) {
    const fo::Value* v = Get(d, key);
    const auto* s = v ? std::get_if<fo::String>(&v->v) : nullptr;
    return s ? std::string(reinterpret_cast<const char*>(s->bytes.data()),
                           s->bytes.size())
             : std::string();
}

std::int64_t IntOf(const fo::Dict& d, const std::string& key) {
    const fo::Value* v = Get(d, key);
    const auto* i = v ? std::get_if<std::int64_t>(&v->v) : nullptr;
    return i ? *i : -1;
}

// hello_world.pdf with push button "b" and text field "t" on page 1.
void SaveButtonForm(const std::string& path, const std::string& script = "") {
    Document doc{HelloWorldPdf()};
    FormEditor ed{doc};
    ed.AddField(FieldType::PushButton, "b", 1, 100, 500, 200, 530);
    ed.AddField(FieldType::Text, "t", 1, 100, 400, 300, 420);
    if (!script.empty()) ed.SetFieldScript("b", script);
    doc.Save(path);
}

}  // namespace

TEST(FormEditorActions, SetFieldScriptOnButton) {
    const std::string out = TmpPath("act_script");
    {
        Document doc{HelloWorldPdf()};
        FormEditor ed{doc};
        ed.AddField(FieldType::PushButton, "b", 1, 100, 500, 200, 530);
        ed.AddField(FieldType::Text, "t", 1, 100, 400, 300, 420);
        EXPECT_TRUE(ed.SetFieldScript("b", "app.alert('hi');"));
        EXPECT_FALSE(ed.SetFieldScript("t", "x();"));
        EXPECT_FALSE(ed.SetFieldScript("missing", "x();"));
        doc.Save(out);
    }
    const fo::Dict a = WidgetAction(out, "b");
    EXPECT_EQ(NameOf(Get(a, "S")), "JavaScript");
    EXPECT_EQ(StringOf(a, "JS"), "app.alert('hi');");
    EXPECT_TRUE(WidgetAction(out, "t").entries.empty());
    std::filesystem::remove(out);
}

TEST(FormEditorActions, AddFieldScriptAppendsToLoadedScript) {
    const std::string src = TmpPath("act_append_src");
    const std::string out = TmpPath("act_append_out");
    SaveButtonForm(src, "first();");
    {
        FormEditor ed{src, out};
        EXPECT_TRUE(ed.AddFieldScript("b", "second();"));
        ed.Save();
    }
    EXPECT_EQ(StringOf(WidgetAction(out, "b"), "JS"), "first();\nsecond();");
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormEditorActions, SetFieldScriptReplacesLoadedScript) {
    const std::string src = TmpPath("act_replace_src");
    const std::string out = TmpPath("act_replace_out");
    SaveButtonForm(src, "old();");
    {
        FormEditor ed{src, out};
        EXPECT_TRUE(ed.SetFieldScript("b", "fresh();"));
        ed.Save();
    }
    EXPECT_EQ(StringOf(WidgetAction(out, "b"), "JS"), "fresh();");
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormEditorActions, AddSubmitButton) {
    const std::string out = TmpPath("act_submit_btn");
    {
        Document doc{HelloWorldPdf()};
        FormEditor ed{doc};
        ed.SubmitFlag(SubmitFormFlag::Xfdf);
        ed.AddSubmitBtn("send", 1, "Send", "https://example.com/submit", 100,
                        100, 200, 130);
        doc.Save(out);
    }
    const fo::Dict a = WidgetAction(out, "send");
    EXPECT_EQ(NameOf(Get(a, "S")), "SubmitForm");
    EXPECT_EQ(IntOf(a, "Flags"), 32);
    const Parsed p = ParsePage(ReadFile(out), 0);
    const fo::Dict fs = ResolveDict(p.dump, Get(a, "F"));
    EXPECT_EQ(NameOf(Get(fs, "FS")), "URL");
    EXPECT_EQ(StringOf(fs, "F"), "https://example.com/submit");
    EXPECT_NE(ReadFile(out).find("(Send) Tj"), std::string::npos);
    std::filesystem::remove(out);
}

TEST(FormEditorActions, SetSubmitUrlAndFlagOnLoadedButton) {
    const std::string src = TmpPath("act_submit_src");
    const std::string out = TmpPath("act_submit_out");
    SaveButtonForm(src);
    {
        FormEditor ed{src, out};
        EXPECT_TRUE(ed.SetSubmitUrl("b", "https://example.com/in"));
        EXPECT_TRUE(ed.SetSubmitFlag("b", SubmitFormFlag::Pdf));
        EXPECT_FALSE(ed.SetSubmitUrl("t", "https://example.com/in"));
        ed.Save();
    }
    const fo::Dict a = WidgetAction(out, "b");
    EXPECT_EQ(NameOf(Get(a, "S")), "SubmitForm");
    EXPECT_EQ(IntOf(a, "Flags"), 256);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(FormEditorActions, RemoveFieldAction) {
    const std::string src = TmpPath("act_remove_src");
    const std::string out = TmpPath("act_remove_out");
    SaveButtonForm(src, "gone();");
    ASSERT_FALSE(WidgetAction(src, "b").entries.empty());
    {
        FormEditor ed{src, out};
        ed.RemoveFieldAction("b");
        ed.Save();
    }
    EXPECT_TRUE(WidgetAction(out, "b").entries.empty());
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}
