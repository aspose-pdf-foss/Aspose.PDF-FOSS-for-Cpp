// =============================================================================
// forms_barcode_signature_smoke_test — beat F5.5 of the Forms
// cluster. BarcodeField (extends TextBoxField; 6 read-only props)
// + SignatureField (extends Field; Sign + Signature accessor with
// forward-declared Signature class — the F6 signature hierarchy
// fills in the type).
// =============================================================================

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/forms/barcode_field.hpp>
#include <aspose/pdf/forms/signature_field.hpp>
#include <aspose/pdf/forms/symbology.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Forms;

TEST(FormsBarcodeSignatureSmoke, BarcodeFieldDefaults) {
    Document doc;
    BarcodeField b{doc, Rectangle{0, 0, 200, 80, false}};
    EXPECT_EQ(b.Resolution(),    72);
    EXPECT_TRUE(b.Caption().empty());
    EXPECT_EQ(b.Symbology(),     Symbology::PDF417);
    EXPECT_EQ(b.XSymWidth(),     0);
    EXPECT_EQ(b.XSymHeight(),    0);
    EXPECT_EQ(b.ECC(),           5);

    // Inherited from TextBoxField — Field surface still works.
    b.PartialName("BarcodeAttachment");
    EXPECT_EQ(b.PartialName(), "BarcodeAttachment");
    b.AddBarcode("978-0-13-235088-4");
    SUCCEED();
}

TEST(FormsBarcodeSignatureSmoke, SignatureFieldDefaultsAndStorage) {
    Document doc;
    SignatureField s{doc, Rectangle{0, 0, 100, 40, false}};
    EXPECT_EQ(s.Signature(), nullptr);

    // Sign / Signature surface compiles against the forward
    // declaration. Exercising the wire-through needs a concrete
    // Signature subclass — that lands at F6.
    void (SignatureField::* sign_fn)(Forms::Signature&)
        = &SignatureField::Sign;
    (void)sign_fn;
    SUCCEED();
}
