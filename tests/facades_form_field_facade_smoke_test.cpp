// =============================================================================
// facades_form_field_facade_smoke_test — beat Fa8 of the Facades
// cluster. FormFieldFacade is the appearance descriptor for a form
// field (a pure value type) — 26 canonical static presets + the
// border/font/alignment/geometry/item properties. Consumed by the
// upcoming FormEditor.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/facades/encoding_type.hpp>
#include <aspose/pdf/facades/font_style.hpp>
#include <aspose/pdf/facades/form_field_facade.hpp>

#include <gtest/gtest.h>

namespace {
using namespace Aspose::Pdf::Facades;
}  // namespace

TEST(FacadesFormFieldFacadeSmoke, CanonicalConstants) {
    EXPECT_FLOAT_EQ(FormFieldFacade::BorderWidthUndefined, -1.0f);
    EXPECT_FLOAT_EQ(FormFieldFacade::BorderWidthUndified, -1.0f);
    EXPECT_FLOAT_EQ(FormFieldFacade::BorderWidthThin, 1.0f);
    EXPECT_FLOAT_EQ(FormFieldFacade::BorderWidthMedium, 2.0f);
    EXPECT_FLOAT_EQ(FormFieldFacade::BorderWidthThick, 3.0f);

    EXPECT_EQ(FormFieldFacade::BorderStyleSolid, 0);
    EXPECT_EQ(FormFieldFacade::BorderStyleUnderline, 4);
    EXPECT_EQ(FormFieldFacade::BorderStyleUndefined, 5);

    EXPECT_EQ(FormFieldFacade::AlignLeft, 0);
    EXPECT_EQ(FormFieldFacade::AlignJustified, 4);
    EXPECT_EQ(FormFieldFacade::AlignTop, 0);
    EXPECT_EQ(FormFieldFacade::AlignBottom, 2);

    EXPECT_EQ(FormFieldFacade::CheckBoxStyleCircle, 108);
    EXPECT_EQ(FormFieldFacade::CheckBoxStyleCheck, 52);
    EXPECT_EQ(FormFieldFacade::CheckBoxStyleCross, 56);
    EXPECT_EQ(FormFieldFacade::CheckBoxStyleDiamond, 117);
    EXPECT_EQ(FormFieldFacade::CheckBoxStyleStar, 72);
    EXPECT_EQ(FormFieldFacade::CheckBoxStyleSquare, 110);
    EXPECT_EQ(FormFieldFacade::CheckBoxStyleUndefined, 32);
}

TEST(FacadesFormFieldFacadeSmoke, Defaults) {
    FormFieldFacade f;
    EXPECT_EQ(f.BorderStyle(), FormFieldFacade::BorderStyleUndefined);
    EXPECT_FLOAT_EQ(f.BorderWidth(), FormFieldFacade::BorderWidthUndefined);
    EXPECT_EQ(f.Font(), FontStyle::Courier);
    EXPECT_TRUE(f.CustomFont().empty());
    EXPECT_FLOAT_EQ(f.FontSize(), 0.0f);
    EXPECT_EQ(f.TextEncoding(), EncodingType::Identity_h);
    EXPECT_EQ(f.Alignment(), FormFieldFacade::AlignUndefined);
    EXPECT_EQ(f.Rotation(), 0);
    EXPECT_TRUE(f.Caption().empty());
    EXPECT_EQ(f.ButtonStyle(), FormFieldFacade::CheckBoxStyleUndefined);
    EXPECT_TRUE(f.Position().empty());
    EXPECT_EQ(f.PageNumber(), 1);
    EXPECT_TRUE(f.Items().empty());
    EXPECT_TRUE(f.ExportItems().empty());
}

TEST(FacadesFormFieldFacadeSmoke, PropertyRoundtrip) {
    FormFieldFacade f;
    f.BorderStyle(FormFieldFacade::BorderStyleBeveled);
    f.BorderWidth(FormFieldFacade::BorderWidthThick);
    f.Font(FontStyle::CourierBold);
    f.CustomFont("MyFont");
    f.FontSize(12.5f);
    f.TextEncoding(EncodingType::Cp1250);
    f.Alignment(FormFieldFacade::AlignCenter);
    f.Rotation(90);
    f.Caption("Submit");
    f.ButtonStyle(FormFieldFacade::CheckBoxStyleStar);
    f.Position(std::vector<float>{1.0f, 2.0f, 3.0f, 4.0f});
    f.PageNumber(3);
    f.Items(std::vector<std::string>{"A", "B"});
    f.ExportItems(std::vector<std::vector<std::string>>{{"x", "1"}, {"y", "2"}});

    EXPECT_EQ(f.BorderStyle(), FormFieldFacade::BorderStyleBeveled);
    EXPECT_FLOAT_EQ(f.BorderWidth(), 3.0f);
    EXPECT_EQ(f.Font(), FontStyle::CourierBold);
    EXPECT_EQ(f.CustomFont(), "MyFont");
    EXPECT_FLOAT_EQ(f.FontSize(), 12.5f);
    EXPECT_EQ(f.TextEncoding(), EncodingType::Cp1250);
    EXPECT_EQ(f.Alignment(), FormFieldFacade::AlignCenter);
    EXPECT_EQ(f.Rotation(), 90);
    EXPECT_EQ(f.Caption(), "Submit");
    EXPECT_EQ(f.ButtonStyle(), FormFieldFacade::CheckBoxStyleStar);
    ASSERT_EQ(f.Position().size(), 4u);
    EXPECT_FLOAT_EQ(f.Position()[2], 3.0f);
    EXPECT_EQ(f.PageNumber(), 3);
    ASSERT_EQ(f.Items().size(), 2u);
    EXPECT_EQ(f.Items()[1], "B");
    ASSERT_EQ(f.ExportItems().size(), 2u);
    EXPECT_EQ(f.ExportItems()[0][0], "x");
}

TEST(FacadesFormFieldFacadeSmoke, ResetRestoresDefaults) {
    FormFieldFacade f;
    f.FontSize(20.0f);
    f.Caption("temp");
    f.PageNumber(9);
    f.Items(std::vector<std::string>{"z"});

    f.Reset();
    EXPECT_FLOAT_EQ(f.FontSize(), 0.0f);
    EXPECT_TRUE(f.Caption().empty());
    EXPECT_EQ(f.PageNumber(), 1);
    EXPECT_TRUE(f.Items().empty());
}
