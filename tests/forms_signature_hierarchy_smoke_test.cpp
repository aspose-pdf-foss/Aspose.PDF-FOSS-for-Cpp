// =============================================================================
// forms_signature_hierarchy_smoke_test — beat F6 of the Forms
// cluster. Signature abstract base + 3 PKCS subclasses (PKCS1 /
// PKCS7 / PKCS7Detached) + ExternalSignature + DocMDPSignature
// wrapper + SignatureCustomAppearance.
//
// XFA + SignHash deferred to F7 (XFA pairs with Form; SignHash is
// a .NET delegate that translates to 0 shipped members in cpp).
//
// Smoke also exercises the F5.5 SignatureField forward-decl
// link: a real Signature subclass can now be passed to
// SignatureField::Sign.
// =============================================================================

#include <string>

#include <aspose/pdf/color.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/doc_mdp_access_permissions.hpp>
#include <aspose/pdf/forms/doc_mdp_signature.hpp>
#include <aspose/pdf/forms/external_signature.hpp>
#include <aspose/pdf/forms/pkcs1.hpp>
#include <aspose/pdf/forms/pkcs7.hpp>
#include <aspose/pdf/forms/pkcs7_detached.hpp>
#include <aspose/pdf/forms/signature.hpp>
#include <aspose/pdf/forms/signature_custom_appearance.hpp>
#include <aspose/pdf/forms/signature_field.hpp>
#include <aspose/pdf/forms/subject_name_elements.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/rotation.hpp>

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Forms;

TEST(FormsSignatureHierarchySmoke, SignatureAccessorRoundtrip) {
    PKCS7Detached sig{"/path/to/cert.pfx", "secret"};
    sig.Authority("ACME CA");
    sig.Location("Berlin");
    sig.Reason("Approval");
    sig.ContactInfo("ops@acme.invalid");
    sig.UseLtv(true);
    sig.AvoidEstimatingSignatureLength(true);
    sig.DefaultSignatureLength(16384);
    sig.ShowProperties(true);

    EXPECT_EQ(sig.Authority(),    "ACME CA");
    EXPECT_EQ(sig.Location(),     "Berlin");
    EXPECT_EQ(sig.Reason(),       "Approval");
    EXPECT_EQ(sig.ContactInfo(),  "ops@acme.invalid");
    EXPECT_TRUE(sig.UseLtv());
    EXPECT_TRUE(sig.AvoidEstimatingSignatureLength());
    EXPECT_EQ(sig.DefaultSignatureLength(), 16384);
    EXPECT_TRUE(sig.ShowProperties());

    EXPECT_TRUE(sig.ByteRange().empty());
    // Verify() v1 stub returns true.
    EXPECT_TRUE(sig.Verify());
}

TEST(FormsSignatureHierarchySmoke, PKCS1AndPKCS7CtorsCompile) {
    PKCS1 p1;
    PKCS1 p1b{"/etc/cert.pfx", "p1pass"};
    PKCS7 p7;
    PKCS7 p7b{"/etc/cert.pfx", "p7pass"};
    (void)p1; (void)p1b; (void)p7; (void)p7b;
    SUCCEED();
}

TEST(FormsSignatureHierarchySmoke, ExternalSignatureCtor) {
    ExternalSignature ext{"base64payload...", true};
    // ExternalSignature is a Signature — confirm inherited
    // accessors work.
    Signature& base = ext;
    base.Reason("Imported certificate");
    EXPECT_EQ(base.Reason(), "Imported certificate");
}

TEST(FormsSignatureHierarchySmoke, DocMDPSignatureWrapsSignature) {
    PKCS7Detached inner;
    DocMDPSignature mdp{inner,
        DocMDPAccessPermissions::FillingInForms};
    EXPECT_EQ(mdp.AccessPermissions(),
              DocMDPAccessPermissions::FillingInForms);
}

TEST(FormsSignatureHierarchySmoke, CustomAppearanceDefaults) {
    SignatureCustomAppearance a;
    EXPECT_FALSE(a.IsForegroundImage());
    EXPECT_DOUBLE_EQ(a.FontSize(), 10.0);
    EXPECT_FALSE(a.ShowContactInfo());
    EXPECT_TRUE(a.ShowReason());
    EXPECT_TRUE(a.ShowLocation());
    EXPECT_EQ(a.ContactInfoLabel(),     "Contact:");
    EXPECT_EQ(a.ReasonLabel(),          "Reason:");
    EXPECT_EQ(a.LocationLabel(),        "Location:");
    EXPECT_EQ(a.DigitalSignedLabel(),   "Digitally signed by");
    EXPECT_FALSE(a.UseDigitalSubjectFormat());
    EXPECT_EQ(a.Rotation(),             Aspose::Pdf::Rotation::None);
    EXPECT_NEAR(a.ForegroundColor().A(), 1.0, 1e-9);
    EXPECT_NEAR(a.BackgroundColor().A(), 1.0, 1e-9);
}

TEST(FormsSignatureHierarchySmoke, CustomAppearanceRoundtrip) {
    SignatureCustomAppearance a;
    a.FontFamilyName("Liberation Sans");
    a.FontSize(14.0);
    a.ForegroundColor(Color::Blue());
    a.BackgroundColor(Color::White());
    a.UseDigitalSubjectFormat(true);
    a.DigitalSubjectFormat({SubjectNameElements::CN,
                            SubjectNameElements::O,
                            SubjectNameElements::C});
    a.Rotation(Aspose::Pdf::Rotation::on90);

    EXPECT_EQ(a.FontFamilyName(),       "Liberation Sans");
    EXPECT_DOUBLE_EQ(a.FontSize(),      14.0);
    EXPECT_TRUE(a.UseDigitalSubjectFormat());
    ASSERT_EQ(a.DigitalSubjectFormat().size(), 3u);
    EXPECT_EQ(a.DigitalSubjectFormat()[0], SubjectNameElements::CN);
    EXPECT_EQ(a.Rotation(),             Aspose::Pdf::Rotation::on90);
}

TEST(FormsSignatureHierarchySmoke, SignatureCustomAppearanceOnSignature) {
    PKCS7Detached sig;
    sig.CustomAppearance().FontSize(16.0);
    sig.CustomAppearance().ShowLocation(false);
    EXPECT_DOUBLE_EQ(sig.CustomAppearance().FontSize(), 16.0);
    EXPECT_FALSE(sig.CustomAppearance().ShowLocation());
}

TEST(FormsSignatureHierarchySmoke, SignatureFieldSignWiresSignature) {
    // F5.5 left SignatureField::Sign(Signature&) accepting a
    // forward-declared type. With the Signature class now real,
    // we can exercise the wire-through end-to-end.
    Document doc;
    SignatureField sf{doc, Rectangle{0, 0, 100, 40, false}};
    PKCS7Detached sig;
    sig.Reason("Smoke test");

    sf.Sign(sig);
    ASSERT_NE(sf.Signature(), nullptr);
    EXPECT_EQ(sf.Signature()->Reason(), "Smoke test");
}
