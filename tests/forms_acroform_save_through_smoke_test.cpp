// =============================================================================
// forms_acroform_save_through_smoke_test — beat F8 (cluster-close)
// of the Forms cluster. AcroForm save-through wires
// Document.Form().Fields into the Catalog's /AcroForm dict via
// PdfWriterIncremental. Same pattern as annotation save-through
// phase 1 + embedded-files save-through, scaled to the AcroForm
// field tree.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/checkbox_field.hpp>
#include <aspose/pdf/forms/combo_box_field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/list_box_field.hpp>
#include <aspose/pdf/forms/password_box_field.hpp>
#include <aspose/pdf/forms/radio_button_field.hpp>
#include <aspose/pdf/forms/signature_field.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/rectangle.hpp>

#include "objects.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Forms;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::vector<std::byte> ReadAll(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    const auto end = in.tellg();
    std::vector<std::byte> bytes(static_cast<std::size_t>(end));
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    return bytes;
}

const foundation::objects::IndirectObject* FindCatalog(
        const foundation::objects::Dump& dump) {
    for (const auto& obj : dump.objects) {
        const auto* d = std::get_if<foundation::objects::Dict>(
            &obj.value.v);
        if (!d) continue;
        for (const auto& kv : d->entries) {
            if (kv.first == "Type") {
                if (auto* s = std::get_if<std::string>(&kv.second.v);
                        s && *s == "Catalog") {
                    return &obj;
                }
            }
        }
    }
    return nullptr;
}

const foundation::objects::Dict* FindAcroForm(
        const foundation::objects::IndirectObject& catalog) {
    const auto& cat_dict = std::get<foundation::objects::Dict>(
        catalog.value.v);
    for (const auto& kv : cat_dict.entries) {
        if (kv.first == "AcroForm") {
            return std::get_if<foundation::objects::Dict>(
                &kv.second.v);
        }
    }
    return nullptr;
}

const foundation::objects::Dict* ResolveDict(
        const foundation::objects::Dump& dump,
        std::uint32_t id) {
    for (const auto& obj : dump.objects) {
        if (obj.id == id) {
            return std::get_if<foundation::objects::Dict>(
                &obj.value.v);
        }
    }
    return nullptr;
}

}  // namespace

TEST(FormsAcroFormSaveThroughSmoke, NoFormBytesUntouched) {
    const auto src = ReadAll(HelloWorldPdf());
    Document doc{HelloWorldPdf()};
    const std::string out =
        (std::filesystem::temp_directory_path() /
         "aspose_acroform_passthrough.pdf").string();
    doc.Save(out);
    const auto saved = ReadAll(out);
    EXPECT_EQ(saved, src);
}

TEST(FormsAcroFormSaveThroughSmoke, TextBoxFieldEmitsAcroFormDict) {
    Document doc{HelloWorldPdf()};
    TextBoxField name{doc};
    name.PartialName("FullName");
    name.Value("Jane Doe");
    name.Rect(Rectangle{100, 600, 300, 620, false});
    doc.Form().Add(name);

    const std::string out =
        (std::filesystem::temp_directory_path() /
         "aspose_acroform_textbox.pdf").string();
    doc.Save(out);

    const auto saved = ReadAll(out);
    const auto dump = foundation::objects::Parse(
        std::span<const std::byte>(saved.data(), saved.size()));

    const auto* cat = FindCatalog(dump);
    ASSERT_NE(cat, nullptr);
    const auto* acroform = FindAcroForm(*cat);
    ASSERT_NE(acroform, nullptr);

    // Walk /Fields array → resolve to widget dict.
    const foundation::objects::Array* fields_arr = nullptr;
    for (const auto& kv : acroform->entries) {
        if (kv.first == "Fields") {
            fields_arr = std::get_if<foundation::objects::Array>(
                &kv.second.v);
        }
    }
    ASSERT_NE(fields_arr, nullptr);
    ASSERT_EQ(fields_arr->items.size(), 1u);
    const auto* ref = std::get_if<foundation::objects::Ref>(
        &fields_arr->items[0].v);
    ASSERT_NE(ref, nullptr);

    const auto* widget = ResolveDict(dump, ref->id);
    ASSERT_NE(widget, nullptr);

    std::string type, subtype, ft, t, v;
    const foundation::objects::Array* rect = nullptr;
    for (const auto& kv : widget->entries) {
        if (kv.first == "Type") {
            if (auto* s = std::get_if<std::string>(&kv.second.v); s) type = *s;
        } else if (kv.first == "Subtype") {
            if (auto* s = std::get_if<std::string>(&kv.second.v); s) subtype = *s;
        } else if (kv.first == "FT") {
            if (auto* s = std::get_if<std::string>(&kv.second.v); s) ft = *s;
        } else if (kv.first == "T") {
            if (auto* s = std::get_if<foundation::objects::String>(&kv.second.v);
                    s) t.assign(reinterpret_cast<const char*>(s->bytes.data()),
                                s->bytes.size());
        } else if (kv.first == "V") {
            if (auto* s = std::get_if<foundation::objects::String>(&kv.second.v);
                    s) v.assign(reinterpret_cast<const char*>(s->bytes.data()),
                                s->bytes.size());
        } else if (kv.first == "Rect") {
            rect = std::get_if<foundation::objects::Array>(&kv.second.v);
        }
    }

    EXPECT_EQ(type,    "Annot");
    EXPECT_EQ(subtype, "Widget");
    EXPECT_EQ(ft,      "Tx");
    EXPECT_EQ(t,       "FullName");
    EXPECT_EQ(v,       "Jane Doe");
    ASSERT_NE(rect, nullptr);
    ASSERT_EQ(rect->items.size(), 4u);
    EXPECT_DOUBLE_EQ(std::get<double>(rect->items[0].v), 100.0);
    EXPECT_DOUBLE_EQ(std::get<double>(rect->items[3].v), 620.0);
}

TEST(FormsAcroFormSaveThroughSmoke, FieldSubtypeDispatchedFt) {
    Document doc{HelloWorldPdf()};

    PasswordBoxField pwd;     pwd.PartialName("pwd");        doc.Form().Add(pwd);
    CheckboxField cb{doc};    cb.PartialName("subscribe");   doc.Form().Add(cb);
    RadioButtonField rb{doc}; rb.PartialName("color");       doc.Form().Add(rb);
    ComboBoxField combo;      combo.PartialName("country");  doc.Form().Add(combo);
    ListBoxField lb;          lb.PartialName("languages");   doc.Form().Add(lb);
    SignatureField sf{doc, Rectangle{0,0,10,10,false}};
    sf.PartialName("sig");                                   doc.Form().Add(sf);

    const std::string out =
        (std::filesystem::temp_directory_path() /
         "aspose_acroform_subtypes.pdf").string();
    doc.Save(out);

    const auto saved = ReadAll(out);
    const auto dump = foundation::objects::Parse(
        std::span<const std::byte>(saved.data(), saved.size()));

    // For each widget object in the dump, collect (T, FT).
    std::map<std::string, std::string> t_to_ft;
    for (const auto& obj : dump.objects) {
        const auto* d = std::get_if<foundation::objects::Dict>(
            &obj.value.v);
        if (!d) continue;
        std::string subtype, ft, t;
        for (const auto& kv : d->entries) {
            if (kv.first == "Subtype") {
                if (auto* s = std::get_if<std::string>(&kv.second.v);
                        s) subtype = *s;
            } else if (kv.first == "FT") {
                if (auto* s = std::get_if<std::string>(&kv.second.v);
                        s) ft = *s;
            } else if (kv.first == "T") {
                if (auto* s = std::get_if<foundation::objects::String>(
                        &kv.second.v); s)
                    t.assign(reinterpret_cast<const char*>(s->bytes.data()),
                             s->bytes.size());
            }
        }
        if (subtype == "Widget" && !t.empty()) {
            t_to_ft[t] = ft;
        }
    }

    EXPECT_EQ(t_to_ft["pwd"],        "Tx");
    EXPECT_EQ(t_to_ft["subscribe"],  "Btn");
    EXPECT_EQ(t_to_ft["color"],      "Btn");
    EXPECT_EQ(t_to_ft["country"],    "Ch");
    EXPECT_EQ(t_to_ft["languages"],  "Ch");
    EXPECT_EQ(t_to_ft["sig"],        "Sig");
}

TEST(FormsAcroFormSaveThroughSmoke, FieldFlagsEmitFf) {
    Document doc{HelloWorldPdf()};
    TextBoxField multi{doc};
    multi.PartialName("comments");
    multi.Multiline(true);
    multi.ReadOnly(true);
    multi.Required(true);
    doc.Form().Add(multi);

    const std::string out =
        (std::filesystem::temp_directory_path() /
         "aspose_acroform_ff.pdf").string();
    doc.Save(out);

    const auto saved = ReadAll(out);
    const auto dump = foundation::objects::Parse(
        std::span<const std::byte>(saved.data(), saved.size()));

    const auto* cat = FindCatalog(dump);
    ASSERT_NE(cat, nullptr);
    const auto* acroform = FindAcroForm(*cat);
    ASSERT_NE(acroform, nullptr);

    const foundation::objects::Array* fields_arr = nullptr;
    for (const auto& kv : acroform->entries) {
        if (kv.first == "Fields") {
            fields_arr = std::get_if<foundation::objects::Array>(
                &kv.second.v);
        }
    }
    ASSERT_NE(fields_arr, nullptr);
    const auto* ref = std::get_if<foundation::objects::Ref>(
        &fields_arr->items[0].v);
    ASSERT_NE(ref, nullptr);
    const auto* widget = ResolveDict(dump, ref->id);
    ASSERT_NE(widget, nullptr);

    std::int64_t ff = 0;
    for (const auto& kv : widget->entries) {
        if (kv.first == "Ff") {
            if (auto* n = std::get_if<std::int64_t>(&kv.second.v); n)
                ff = *n;
        }
    }
    // ReadOnly=1, Required=2, Multiline=4096 → 4099
    EXPECT_EQ(ff, 1 + 2 + 4096);
}
