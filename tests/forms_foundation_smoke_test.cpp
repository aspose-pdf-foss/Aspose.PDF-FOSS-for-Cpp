// =============================================================================
// forms_foundation_smoke_test — beat F-1 of the Aspose::Pdf::Forms
// cluster. 8 leaf enums + 3 support classes (Option,
// OptionCollection, IconFit). Tier-0 with no Forms-internal deps —
// proves the namespace + builds the leaf surface that subsequent
// beats (Field abstract base, concrete field subclasses,
// signatures, Form) layer on top of.
// =============================================================================

#include <string>

#include <aspose/pdf/forms/box_style.hpp>
#include <aspose/pdf/forms/doc_mdp_access_permissions.hpp>
#include <aspose/pdf/forms/form_type.hpp>
#include <aspose/pdf/forms/icon_caption_position.hpp>
#include <aspose/pdf/forms/icon_fit.hpp>
#include <aspose/pdf/forms/option.hpp>
#include <aspose/pdf/forms/option_collection.hpp>
#include <aspose/pdf/forms/scaling_mode.hpp>
#include <aspose/pdf/forms/scaling_reason.hpp>
#include <aspose/pdf/forms/subject_name_elements.hpp>
#include <aspose/pdf/forms/symbology.hpp>

#include <gtest/gtest.h>

using namespace Aspose::Pdf::Forms;

TEST(FormsFoundationSmoke, EnumValuesPinned) {
    EXPECT_EQ(static_cast<int>(BoxStyle::Circle),  0);
    EXPECT_EQ(static_cast<int>(BoxStyle::Star),    5);

    EXPECT_EQ(static_cast<int>(DocMDPAccessPermissions::NoChanges),               1);
    EXPECT_EQ(static_cast<int>(DocMDPAccessPermissions::FillingInForms),          2);
    EXPECT_EQ(static_cast<int>(DocMDPAccessPermissions::AnnotationModification),  3);

    EXPECT_EQ(static_cast<int>(FormType::Standard), 0);
    EXPECT_EQ(static_cast<int>(FormType::Static),   1);
    EXPECT_EQ(static_cast<int>(FormType::Dynamic),  2);

    EXPECT_EQ(static_cast<int>(IconCaptionPosition::NoIcon),         0);
    EXPECT_EQ(static_cast<int>(IconCaptionPosition::CaptionOverlaid), 6);

    EXPECT_EQ(static_cast<int>(ScalingMode::Proportional), 0);
    EXPECT_EQ(static_cast<int>(ScalingMode::Anamorphic),   1);

    EXPECT_EQ(static_cast<int>(ScalingReason::Always),         0);
    EXPECT_EQ(static_cast<int>(ScalingReason::IconIsBigger),   1);
    EXPECT_EQ(static_cast<int>(ScalingReason::IconIsSmaller),  2);
    EXPECT_EQ(static_cast<int>(ScalingReason::Never),          3);

    EXPECT_EQ(static_cast<int>(SubjectNameElements::CN), 0);
    EXPECT_EQ(static_cast<int>(SubjectNameElements::E),  6);

    EXPECT_EQ(static_cast<int>(Symbology::PDF417),     0);
    EXPECT_EQ(static_cast<int>(Symbology::QRCode),     1);
    EXPECT_EQ(static_cast<int>(Symbology::DataMatrix), 2);
}

TEST(FormsFoundationSmoke, OptionAccessors) {
    Option o;
    EXPECT_TRUE(o.Value().empty());
    EXPECT_TRUE(o.Name().empty());
    EXPECT_FALSE(o.Selected());
    EXPECT_EQ(o.Index(), -1);

    o.Value("yes");
    o.Name("ApprovedOption");
    o.Selected(true);
    EXPECT_EQ(o.Value(), "yes");
    EXPECT_EQ(o.Name(), "ApprovedOption");
    EXPECT_TRUE(o.Selected());
}

TEST(FormsFoundationSmoke, OptionCollectionAddCountIndex) {
    OptionCollection c;
    EXPECT_EQ(c.Count(), 0);
    EXPECT_FALSE(c.IsReadOnly());

    Option a; a.Name("alpha"); a.Value("v0");
    Option b; b.Name("beta");  b.Value("v1");
    c.Add(a);
    c.Add(b);

    EXPECT_EQ(c.Count(), 2);
    EXPECT_EQ(c[0].Name(),       "alpha");
    EXPECT_EQ(c["beta"].Value(), "v1");
    EXPECT_EQ(c.Get(0).Index(),  0);
    EXPECT_EQ(c.Get("beta").Index(), 1);

    EXPECT_TRUE(c.Contains(a));
    Option missing; missing.Name("gamma"); missing.Value("v2");
    EXPECT_FALSE(c.Contains(missing));

    EXPECT_TRUE(c.Remove(a));
    EXPECT_EQ(c.Count(), 1);
    EXPECT_EQ(c[0].Name(), "beta");
    EXPECT_EQ(c[0].Index(), 0);  // reindexed after remove

    c.Clear();
    EXPECT_EQ(c.Count(), 0);
}

TEST(FormsFoundationSmoke, OptionCollectionThrowsOnMiss) {
    OptionCollection c;
    EXPECT_THROW((void)c[0],         std::out_of_range);
    EXPECT_THROW((void)c["missing"], std::out_of_range);
}

TEST(FormsFoundationSmoke, IconFitDefaultsAndAccessors) {
    IconFit f;
    EXPECT_EQ(f.ScalingReason(), ScalingReason::Always);
    EXPECT_EQ(f.ScalingMode(),   ScalingMode::Proportional);
    EXPECT_DOUBLE_EQ(f.LeftoverLeft(),   0.5);
    EXPECT_DOUBLE_EQ(f.LeftoverBottom(), 0.5);
    EXPECT_FALSE(f.SpreadOnBorder());

    f.ScalingReason(ScalingReason::IconIsSmaller);
    f.ScalingMode(ScalingMode::Anamorphic);
    f.LeftoverLeft(0.25);
    f.LeftoverBottom(0.75);
    f.SpreadOnBorder(true);

    EXPECT_EQ(f.ScalingReason(), ScalingReason::IconIsSmaller);
    EXPECT_EQ(f.ScalingMode(),   ScalingMode::Anamorphic);
    EXPECT_DOUBLE_EQ(f.LeftoverLeft(),   0.25);
    EXPECT_DOUBLE_EQ(f.LeftoverBottom(), 0.75);
    EXPECT_TRUE(f.SpreadOnBorder());
}

TEST(FormsFoundationSmoke, IconFitNameRoundtrips) {
    EXPECT_EQ(IconFit::ScalingReasonToName(ScalingReason::Always),         "A");
    EXPECT_EQ(IconFit::ScalingReasonToName(ScalingReason::IconIsBigger),   "B");
    EXPECT_EQ(IconFit::ScalingReasonToName(ScalingReason::IconIsSmaller),  "S");
    EXPECT_EQ(IconFit::ScalingReasonToName(ScalingReason::Never),          "N");
    EXPECT_EQ(IconFit::NameToScalingReason("A"), ScalingReason::Always);
    EXPECT_EQ(IconFit::NameToScalingReason("N"), ScalingReason::Never);
    EXPECT_THROW((void)IconFit::NameToScalingReason("Z"), std::out_of_range);

    EXPECT_EQ(IconFit::ScalingModeToName(ScalingMode::Proportional), "P");
    EXPECT_EQ(IconFit::ScalingModeToName(ScalingMode::Anamorphic),   "A");
    EXPECT_EQ(IconFit::NameToScalingMode("P"), ScalingMode::Proportional);
    EXPECT_THROW((void)IconFit::NameToScalingMode("Z"), std::out_of_range);
}
