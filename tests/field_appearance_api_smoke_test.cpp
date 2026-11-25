// =============================================================================
// field_appearance_api_smoke_test — Form::AddFieldAppearance /
// RemoveFieldAppearance: one field shown at several places (widgets that
// share the field's value), for new fields and fields loaded from a file.
// =============================================================================

#include <stdexcept>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/field.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/radio_button_field.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace flatten_test;

namespace {

std::string TwoPagesPdf() {
    return (std::filesystem::path(__FILE__).parent_path() / "fixtures" /
            "text_extractor" / "two_pages.pdf")
        .string();
}

int Occurrences(const std::string& s, const std::string& needle) {
    int n = 0;
    for (std::size_t pos = 0; (pos = s.find(needle, pos)) != std::string::npos;
         pos += needle.size())
        ++n;
    return n;
}

Forms::Field* FieldByName(Document& doc, const std::string& name) {
    for (Forms::Field* f : doc.Form().Fields())
        if (f != nullptr && f->PartialName() == name) return f;
    return nullptr;
}

}  // namespace

TEST(FieldAppearanceApi, NewFieldTwoPagesHasWidgetOnBoth) {
    const std::string out = TmpPath("fa_two_pages");
    {
        Document doc{TwoPagesPdf()};
        Forms::TextBoxField f{doc, Rect(100, 400, 300, 420)};
        f.Value("Shared");
        doc.Form().Add(f, "name", 1);
        doc.Form().AddFieldAppearance(f, 2, Rect(100, 500, 300, 520));
        doc.Save(out);
    }
    const std::string bytes = ReadFile(out);
    EXPECT_EQ(CountSubtype(ParsePage(bytes, 0), "Widget"), 1);
    EXPECT_EQ(CountSubtype(ParsePage(bytes, 1), "Widget"), 1);
    EXPECT_GE(Occurrences(bytes, "(Shared) Tj"), 2);
    Document re{out};
    EXPECT_EQ(re.Form().Count(), 1);
    EXPECT_GT(DarkPixels(Render(re, 2), 100, 500, 300, 520), 20);
    std::filesystem::remove(out);
}

TEST(FieldAppearanceApi, NewFieldRemoveOriginalKeepsAdded) {
    const std::string out = TmpPath("fa_remove_orig");
    {
        Document doc{TwoPagesPdf()};
        Forms::TextBoxField f{doc, Rect(100, 400, 300, 420)};
        f.Value("Moved");
        doc.Form().Add(f, "name", 1);
        doc.Form().AddFieldAppearance(f, 2, Rect(100, 500, 300, 520));
        doc.Form().RemoveFieldAppearance(f, 1);
        doc.Save(out);
    }
    const std::string bytes = ReadFile(out);
    EXPECT_EQ(CountSubtype(ParsePage(bytes, 0), "Widget"), 0);
    EXPECT_EQ(CountSubtype(ParsePage(bytes, 1), "Widget"), 1);
    std::filesystem::remove(out);
}

TEST(FieldAppearanceApi, RemoveLastAppearanceThrows) {
    Document doc{HelloWorldPdf()};
    Forms::TextBoxField f{doc, Rect(100, 400, 300, 420)};
    doc.Form().Add(f, "name", 1);
    EXPECT_THROW(doc.Form().RemoveFieldAppearance(f, 1), std::invalid_argument);
}

TEST(FieldAppearanceApi, BadIndexThrows) {
    Document doc{HelloWorldPdf()};
    Forms::TextBoxField f{doc, Rect(100, 400, 300, 420)};
    doc.Form().Add(f, "name", 1);
    doc.Form().AddFieldAppearance(f, 1, Rect(100, 300, 300, 320));
    EXPECT_THROW(doc.Form().RemoveFieldAppearance(f, 0), std::out_of_range);
    EXPECT_THROW(doc.Form().RemoveFieldAppearance(f, 3), std::out_of_range);
}

TEST(FieldAppearanceApi, RadioAddAppearanceThrows) {
    Document doc{HelloWorldPdf()};
    Forms::RadioButtonField rb{doc};
    rb.AddOption("a", Rect(100, 300, 120, 320));
    doc.Form().Add(rb, "color", 1);
    EXPECT_THROW(doc.Form().AddFieldAppearance(rb, 1, Rect(200, 300, 220, 320)),
                 std::invalid_argument);
}

TEST(FieldAppearanceApi, FieldNotInFormThrows) {
    Document doc{HelloWorldPdf()};
    Forms::TextBoxField f{doc, Rect(100, 400, 300, 420)};
    EXPECT_THROW(doc.Form().AddFieldAppearance(f, 1, Rect(100, 300, 300, 320)),
                 std::invalid_argument);
}

TEST(FieldAppearanceApi, FlattenBurnsAllAppearances) {
    const std::string out = TmpPath("fa_flatten");
    {
        Document doc{TwoPagesPdf()};
        Forms::TextBoxField f{doc, Rect(100, 400, 300, 420)};
        f.Value("Both");
        doc.Form().Add(f, "name", 1);
        doc.Form().AddFieldAppearance(f, 2, Rect(100, 500, 300, 520));
        doc.Form().Flatten();
        doc.Save(out);
    }
    const std::string bytes = ReadFile(out);
    EXPECT_EQ(FlattenedCount(ParsePage(bytes, 0)), 1);
    EXPECT_EQ(FlattenedCount(ParsePage(bytes, 1)), 1);
    EXPECT_EQ(CountSubtype(ParsePage(bytes, 1), "Widget"), 0);
    std::filesystem::remove(out);
}
