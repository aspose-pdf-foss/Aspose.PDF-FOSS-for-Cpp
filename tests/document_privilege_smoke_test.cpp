// =============================================================================
// document_privilege_smoke_test — preset accessors, property
// round-trip, CompareTo lexicographic ordering. Exercises the
// E2 public-surface contract.
// =============================================================================

#include <aspose/pdf/document_privilege.hpp>

#include <gtest/gtest.h>

using Aspose::Pdf::Facades::DocumentPrivilege;

TEST(DocumentPrivilegeSmoke, DefaultConstructedEqualsForbidAll) {
    DocumentPrivilege d;
    EXPECT_FALSE(d.AllowPrint());
    EXPECT_FALSE(d.AllowDegradedPrinting());
    EXPECT_FALSE(d.AllowModifyContents());
    EXPECT_FALSE(d.AllowCopy());
    EXPECT_FALSE(d.AllowModifyAnnotations());
    EXPECT_FALSE(d.AllowFillIn());
    EXPECT_FALSE(d.AllowScreenReaders());
    EXPECT_FALSE(d.AllowAssembly());
    EXPECT_EQ(d.PrintAllowLevel(),  0);
    EXPECT_EQ(d.ChangeAllowLevel(), 0);
    EXPECT_EQ(d.CopyAllowLevel(),   0);

    auto forbid = DocumentPrivilege::ForbidAll();
    EXPECT_EQ(d.CompareTo(forbid), 0);
}

TEST(DocumentPrivilegeSmoke, AllowAllSetsEveryFlag) {
    auto a = DocumentPrivilege::AllowAll();
    EXPECT_TRUE(a.AllowPrint());
    EXPECT_TRUE(a.AllowDegradedPrinting());
    EXPECT_TRUE(a.AllowModifyContents());
    EXPECT_TRUE(a.AllowCopy());
    EXPECT_TRUE(a.AllowModifyAnnotations());
    EXPECT_TRUE(a.AllowFillIn());
    EXPECT_TRUE(a.AllowScreenReaders());
    EXPECT_TRUE(a.AllowAssembly());
    EXPECT_EQ(a.PrintAllowLevel(),  2);
    EXPECT_EQ(a.ChangeAllowLevel(), 2);
    EXPECT_EQ(a.CopyAllowLevel(),   2);
}

TEST(DocumentPrivilegeSmoke, PresetIsolation) {
    // Each preset enables ONLY its named permission, with the
    // associated AllowLevel set to the canonical "full" or
    // "degraded" tier.
    auto p = DocumentPrivilege::Print();
    EXPECT_TRUE(p.AllowPrint());
    EXPECT_EQ(p.PrintAllowLevel(), 2);
    EXPECT_FALSE(p.AllowDegradedPrinting());
    EXPECT_FALSE(p.AllowModifyContents());
    EXPECT_FALSE(p.AllowCopy());

    auto dp = DocumentPrivilege::DegradedPrinting();
    EXPECT_TRUE(dp.AllowDegradedPrinting());
    EXPECT_EQ(dp.PrintAllowLevel(), 1);
    EXPECT_FALSE(dp.AllowPrint());

    auto c = DocumentPrivilege::Copy();
    EXPECT_TRUE(c.AllowCopy());
    EXPECT_EQ(c.CopyAllowLevel(), 2);
    EXPECT_FALSE(c.AllowPrint());

    auto mc = DocumentPrivilege::ModifyContents();
    EXPECT_TRUE(mc.AllowModifyContents());
    EXPECT_EQ(mc.ChangeAllowLevel(), 2);

    auto ma = DocumentPrivilege::ModifyAnnotations();
    EXPECT_TRUE(ma.AllowModifyAnnotations());
    EXPECT_EQ(ma.ChangeAllowLevel(), 0);  // no associated level

    auto f = DocumentPrivilege::FillIn();
    EXPECT_TRUE(f.AllowFillIn());

    auto sr = DocumentPrivilege::ScreenReaders();
    EXPECT_TRUE(sr.AllowScreenReaders());

    auto as = DocumentPrivilege::Assembly();
    EXPECT_TRUE(as.AllowAssembly());
}

TEST(DocumentPrivilegeSmoke, PropertyRoundtrip) {
    DocumentPrivilege d;
    d.AllowPrint(true);
    d.AllowFillIn(true);
    d.AllowAssembly(true);
    d.PrintAllowLevel(2);
    d.CopyAllowLevel(1);
    EXPECT_TRUE(d.AllowPrint());
    EXPECT_TRUE(d.AllowFillIn());
    EXPECT_TRUE(d.AllowAssembly());
    EXPECT_FALSE(d.AllowCopy());
    EXPECT_EQ(d.PrintAllowLevel(), 2);
    EXPECT_EQ(d.CopyAllowLevel(),  1);
    EXPECT_EQ(d.ChangeAllowLevel(), 0);

    d.AllowPrint(false);
    EXPECT_FALSE(d.AllowPrint());
}

TEST(DocumentPrivilegeSmoke, CompareToOrders) {
    auto forbid = DocumentPrivilege::ForbidAll();
    auto allow  = DocumentPrivilege::AllowAll();
    auto print  = DocumentPrivilege::Print();

    EXPECT_EQ(forbid.CompareTo(forbid), 0);
    EXPECT_EQ(allow.CompareTo(allow),   0);

    // Forbid < Print (AllowPrint differs; forbid is false, print is true).
    EXPECT_LT(forbid.CompareTo(print), 0);
    EXPECT_GT(print.CompareTo(forbid), 0);

    // Print < AllowAll (Print has AllowDegradedPrinting=false, AllowAll=true).
    EXPECT_LT(print.CompareTo(allow), 0);
}
