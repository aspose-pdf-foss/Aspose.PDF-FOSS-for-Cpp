// =============================================================================
// loaded_field_edit_smoke_test — edits to a form field loaded from a file
// (name, flags, rectangle, text-box options, list options, annotation
// flags) persist through Save; untouched fields are not rewritten.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_flags.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/list_box_field.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace flatten_test;

namespace {

Forms::Field* FieldByName(Document& doc, const std::string& name) {
    for (Forms::Field* f : doc.Form().Fields())
        if (f != nullptr && f->PartialName() == name) return f;
    return nullptr;
}

// hello_world.pdf with text field "t" (multi-line) at (100,400)-(300,420)
// and list box "l" with options a, b at (100,300)-(200,360).
void SaveForm(const std::string& path) {
    Document doc{HelloWorldPdf()};
    Forms::TextBoxField t{doc, Rect(100, 400, 300, 420)};
    t.Value("text");
    t.Multiline(true);
    Forms::ListBoxField l{doc, Rect(100, 300, 200, 360)};
    l.AddOption("a");
    l.AddOption("b");
    doc.Form().Add(t, "t", 1);
    doc.Form().Add(l, "l", 1);
    doc.Save(path);
}

// Open `src`, apply `edit` to the reopened document, save to `out` and
// reopen it for `check`.
template <class Edit, class Check>
void EditAndCheck(const std::string& tag, Edit edit, Check check) {
    const std::string src = TmpPath("edit_" + tag + "_src");
    const std::string out = TmpPath("edit_" + tag + "_out");
    SaveForm(src);
    {
        Document doc{src};
        edit(doc);
        doc.Save(out);
    }
    {
        Document re{out};
        check(re, ReadFile(out));
    }
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

}  // namespace

TEST(LoadedFieldEdit, RenamePersists) {
    EditAndCheck("rename",
        [](Document& d) { FieldByName(d, "t")->PartialName("renamed"); },
        [](Document& re, const std::string&) {
            EXPECT_NE(FieldByName(re, "renamed"), nullptr);
            EXPECT_EQ(FieldByName(re, "t"), nullptr);
            EXPECT_EQ(re.Form().Count(), 2);
        });
}

TEST(LoadedFieldEdit, ReadOnlyAndRequiredPersistKeepingOtherFlags) {
    EditAndCheck("flags",
        [](Document& d) {
            auto* t = FieldByName(d, "t");
            t->ReadOnly(true);
            t->Required(true);
        },
        [](Document& re, const std::string&) {
            auto* t = dynamic_cast<Forms::TextBoxField*>(FieldByName(re, "t"));
            ASSERT_NE(t, nullptr);
            EXPECT_TRUE(t->ReadOnly());
            EXPECT_TRUE(t->Required());
            EXPECT_TRUE(t->Multiline());  // untouched bit kept
        });
}

TEST(LoadedFieldEdit, MovePersistsAndRedraws) {
    EditAndCheck("move",
        [](Document& d) {
            FieldByName(d, "t")->Rect(Rect(100, 200, 300, 220));
        },
        [](Document& re, const std::string&) {
            const auto r = FieldByName(re, "t")->Rect();
            EXPECT_DOUBLE_EQ(r.LLY(), 200.0);
            EXPECT_DOUBLE_EQ(r.URY(), 220.0);
            EXPECT_GT(DarkPixels(Render(re, 1), 100, 200, 300, 220), 20);
        });
}

TEST(LoadedFieldEdit, TextBoxOptionsPersist) {
    EditAndCheck("textopts",
        [](Document& d) {
            auto* t = dynamic_cast<Forms::TextBoxField*>(FieldByName(d, "t"));
            t->Multiline(false);
            t->MaxLen(5);
            t->ForceCombs(true);
            t->TextHorizontalAlignment(HorizontalAlignment::Center);
        },
        [](Document& re, const std::string& bytes) {
            auto* t = dynamic_cast<Forms::TextBoxField*>(FieldByName(re, "t"));
            ASSERT_NE(t, nullptr);
            EXPECT_FALSE(t->Multiline());
            EXPECT_EQ(t->MaxLen(), 5);
            EXPECT_TRUE(t->ForceCombs());
            EXPECT_EQ(t->TextHorizontalAlignment(), HorizontalAlignment::Center);
            EXPECT_NE(bytes.find("/Q 1"), std::string::npos);
        });
}

TEST(LoadedFieldEdit, ListOptionsPersist) {
    EditAndCheck("listopts",
        [](Document& d) {
            auto* l = dynamic_cast<Forms::ListBoxField*>(FieldByName(d, "l"));
            l->AddOption("c");
            l->DeleteOption("a");
        },
        [](Document& re, const std::string&) {
            auto* l = dynamic_cast<Forms::ListBoxField*>(FieldByName(re, "l"));
            ASSERT_NE(l, nullptr);
            auto& opts = l->Options();
            ASSERT_EQ(opts.Count(), 2);
            EXPECT_EQ(opts[0].Value(), "b");
            EXPECT_EQ(opts[1].Value(), "c");
        });
}

TEST(LoadedFieldEdit, AnnotationFlagsPersist) {
    EditAndCheck("annotflags",
        [](Document& d) {
            FieldByName(d, "t")->Flags(Annotations::AnnotationFlags::Hidden);
        },
        [](Document& re, const std::string&) {
            EXPECT_EQ(FieldByName(re, "t")->Flags(),
                      Annotations::AnnotationFlags::Hidden);
        });
}

TEST(LoadedFieldEdit, UntouchedFieldsAreNotRewritten) {
    const std::string src = TmpPath("edit_untouched_src");
    const std::string out = TmpPath("edit_untouched_out");
    SaveForm(src);
    {
        Document doc{src};
        FieldByName(doc, "t")->PartialName("only_t");
        doc.Save(out);
    }
    // The list box object keeps a single definition (not re-emitted).
    const std::string bytes = ReadFile(out);
    const Parsed before = ParsePage(ReadFile(src), 0);
    const Parsed after = ParsePage(bytes, 0);
    int list_defs = 0;
    for (std::size_t p = 0; (p = bytes.find("(l)", p)) != std::string::npos; ++p)
        ++list_defs;
    EXPECT_EQ(list_defs, 1);
    (void)before;
    (void)after;
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}
