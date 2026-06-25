// =============================================================================
// facades_foundation_smoke_test — beat Fa-1 of the Facades cluster.
// 15 leaf enums + 2 type-safe enum classes (AlignmentType,
// VerticalAlignmentType) + 2 interfaces (IFacade, ISaveableFacade)
// + 2 abstract bases (Facade, SaveableFacade). Tier-0 with no
// Facades-internal deps.
// =============================================================================

#include <filesystem>
#include <fstream>
#include <span>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/facades/algorithm.hpp>
#include <aspose/pdf/facades/alignment_type.hpp>
#include <aspose/pdf/facades/auto_rotate_mode.hpp>
#include <aspose/pdf/facades/blending_color_space.hpp>
#include <aspose/pdf/facades/data_type.hpp>
#include <aspose/pdf/facades/default_metadata_properties.hpp>
#include <aspose/pdf/facades/encoding_type.hpp>
#include <aspose/pdf/facades/facade.hpp>
#include <aspose/pdf/facades/field_type.hpp>
#include <aspose/pdf/facades/font_style.hpp>
#include <aspose/pdf/facades/i_facade.hpp>
#include <aspose/pdf/facades/i_saveable_facade.hpp>
#include <aspose/pdf/facades/image_merge_mode.hpp>
#include <aspose/pdf/facades/key_size.hpp>
#include <aspose/pdf/facades/positioning_mode.hpp>
#include <aspose/pdf/facades/property_flag.hpp>
#include <aspose/pdf/facades/saveable_facade.hpp>
#include <aspose/pdf/facades/stamp_type.hpp>
#include <aspose/pdf/facades/submit_form_flag.hpp>
#include <aspose/pdf/facades/vertical_alignment_type.hpp>
#include <aspose/pdf/facades/word_wrap_mode.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

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

}  // namespace

TEST(FacadesFoundationSmoke, EnumValuesPinned) {
    EXPECT_EQ(static_cast<int>(Algorithm::RC4), 0);
    EXPECT_EQ(static_cast<int>(Algorithm::AES), 1);

    EXPECT_EQ(static_cast<int>(AutoRotateMode::None),           0);
    EXPECT_EQ(static_cast<int>(AutoRotateMode::AntiClockWise),  2);

    EXPECT_EQ(static_cast<int>(BlendingColorSpace::DontChange),  0);
    EXPECT_EQ(static_cast<int>(BlendingColorSpace::DeviceCMYK),  3);

    EXPECT_EQ(static_cast<int>(DataType::FDF),    0);
    EXPECT_EQ(static_cast<int>(DataType::ODBC),   5);

    EXPECT_EQ(static_cast<int>(DefaultMetadataProperties::Advisory),    0);
    EXPECT_EQ(static_cast<int>(DefaultMetadataProperties::Thumbnails),  8);

    EXPECT_EQ(static_cast<int>(EncodingType::Identity_h), 0);
    EXPECT_EQ(static_cast<int>(EncodingType::Macroman),   6);

    EXPECT_EQ(static_cast<int>(FieldType::Text),     0);
    EXPECT_EQ(static_cast<int>(FieldType::DateTime), 12);

    EXPECT_EQ(static_cast<int>(FontStyle::Courier),     0);
    EXPECT_EQ(static_cast<int>(FontStyle::CjkFont),     15);

    // ImageMergeMode starts at 1 — not the usual 0-based default.
    EXPECT_EQ(static_cast<int>(ImageMergeMode::Vertical),   1);
    EXPECT_EQ(static_cast<int>(ImageMergeMode::Horizontal), 2);
    EXPECT_EQ(static_cast<int>(ImageMergeMode::Center),     3);

    EXPECT_EQ(static_cast<int>(KeySize::x40),  0);
    EXPECT_EQ(static_cast<int>(KeySize::x256), 2);

    EXPECT_EQ(static_cast<int>(PositioningMode::Legacy),  0);
    EXPECT_EQ(static_cast<int>(PositioningMode::Current), 2);

    EXPECT_EQ(static_cast<int>(PropertyFlag::ReadOnly),    0);
    EXPECT_EQ(static_cast<int>(PropertyFlag::InvalidFlag), 3);

    EXPECT_EQ(static_cast<int>(StampType::Form),  0);
    EXPECT_EQ(static_cast<int>(StampType::Image), 1);

    EXPECT_EQ(static_cast<int>(SubmitFormFlag::Fdf),  0);
    EXPECT_EQ(static_cast<int>(SubmitFormFlag::Pdf),  5);

    EXPECT_EQ(static_cast<int>(WordWrapMode::Default), 0);
    EXPECT_EQ(static_cast<int>(WordWrapMode::ByWords), 1);
}

TEST(FacadesFoundationSmoke, AlignmentTypeNamedInstances) {
    EXPECT_EQ(AlignmentType::Left().ToString(),   "Left");
    EXPECT_EQ(AlignmentType::Center().ToString(), "Center");
    EXPECT_EQ(AlignmentType::Right().ToString(),  "Right");

    EXPECT_EQ(AlignmentType::Left(),  AlignmentType::Left());
    EXPECT_NE(AlignmentType::Left(),  AlignmentType::Center());

    AlignmentType custom{"Justify"};
    EXPECT_EQ(custom.ToString(), "Justify");
    EXPECT_NE(custom, AlignmentType::Left());
}

TEST(FacadesFoundationSmoke, VerticalAlignmentTypeNamedInstances) {
    EXPECT_EQ(VerticalAlignmentType::Top().ToString(),    "Top");
    EXPECT_EQ(VerticalAlignmentType::Center().ToString(), "Center");
    EXPECT_EQ(VerticalAlignmentType::Bottom().ToString(), "Bottom");

    EXPECT_NE(VerticalAlignmentType::Top(),
              VerticalAlignmentType::Bottom());
}

TEST(FacadesFoundationSmoke, FacadeBindByDocument) {
    Document doc;
    Facade f;
    EXPECT_EQ(f.Document(), nullptr);
    f.BindPdf(doc);
    EXPECT_EQ(f.Document(), &doc);
    f.Close();
    EXPECT_EQ(f.Document(), nullptr);
}

TEST(FacadesFoundationSmoke, FacadeBindByFile) {
    Facade f;
    f.BindPdf(HelloWorldPdf());
    ASSERT_NE(f.Document(), nullptr);
    EXPECT_GT(f.Document()->Pages().Count(), 0u);
    f.Dispose();  // alias for Close
    EXPECT_EQ(f.Document(), nullptr);
}

TEST(FacadesFoundationSmoke, SaveableFacadeRoundtrip) {
    SaveableFacade sf;
    sf.BindPdf(HelloWorldPdf());

    const std::string out =
        (std::filesystem::temp_directory_path() /
         "aspose_facade_save.pdf").string();
    sf.Save(out);

    // Reload the saved file through a fresh Document to confirm
    // it parses.
    Document reloaded{out};
    EXPECT_GT(reloaded.Pages().Count(), 0u);
}

TEST(FacadesFoundationSmoke, SaveableFacadeWithoutBindThrows) {
    SaveableFacade sf;
    EXPECT_THROW(sf.Save("/tmp/never.pdf"), std::runtime_error);
}

TEST(FacadesFoundationSmoke, InterfacePolymorphism) {
    // Facade implements IFacade; SaveableFacade implements both.
    SaveableFacade sf;
    IFacade& as_ifacade = sf;
    ISaveableFacade& as_isaveable = sf;
    (void)as_ifacade;
    (void)as_isaveable;
    SUCCEED();
}
