#pragma once

// =============================================================================
// Aspose::Pdf::Facades::DocumentPrivilege — preset-based privilege
// container consumed by Document::Encrypt and the upcoming
// PdfFileSecurity facade. Mirrors canonical Aspose.Pdf.Facades.
// DocumentPrivilege (Aspose.PDF 26.4.0).
//
// Each instance carries eight Boolean Allow* flags + three Int32
// AllowLevel fields (the canonical 0/1/2 tiered-permission model)
// + ten static preset accessors that return a DocumentPrivilege
// pre-configured for a single permission category.
//
// Use the static presets for the common cases:
//
//     auto p = DocumentPrivilege::AllowAll();        // grant everything
//     auto q = DocumentPrivilege::ForbidAll();       // forbid everything
//     auto r = DocumentPrivilege::Print();           // print only
//
// or fluently mutate an instance via the property accessors:
//
//     auto p = DocumentPrivilege::ForbidAll();
//     p.AllowFillIn(true);
//     p.AllowPrint(true);
//
// CompareTo provides a deterministic ordering across the
// IComparable<Object> contract — instances are ordered
// lexicographically over (AllowPrint, AllowDegradedPrinting,
// AllowModifyContents, AllowCopy, AllowModifyAnnotations,
// AllowFillIn, AllowScreenReaders, AllowAssembly, PrintAllowLevel,
// ChangeAllowLevel, CopyAllowLevel). Callers comparing
// "how restrictive" should NOT rely on the ordering — use the
// preset accessors or the explicit field accessors instead.
// =============================================================================

namespace Aspose::Pdf::Facades {

class DocumentPrivilege {
public:
    // Default-constructed instance has every flag false and every
    // level 0 — equivalent to ForbidAll(). Provided so callers can
    // build a privilege incrementally via the property setters.
    DocumentPrivilege() noexcept = default;

    // Boolean Allow* properties — eight independent flags
    // matching the canonical .NET surface.

    bool AllowPrint() const noexcept;
    void AllowPrint(bool value) noexcept;

    bool AllowDegradedPrinting() const noexcept;
    void AllowDegradedPrinting(bool value) noexcept;

    bool AllowModifyContents() const noexcept;
    void AllowModifyContents(bool value) noexcept;

    bool AllowCopy() const noexcept;
    void AllowCopy(bool value) noexcept;

    bool AllowModifyAnnotations() const noexcept;
    void AllowModifyAnnotations(bool value) noexcept;

    bool AllowFillIn() const noexcept;
    void AllowFillIn(bool value) noexcept;

    bool AllowScreenReaders() const noexcept;
    void AllowScreenReaders(bool value) noexcept;

    bool AllowAssembly() const noexcept;
    void AllowAssembly(bool value) noexcept;

    // Int32 *AllowLevel properties — three independent fields for
    // the legacy tiered-permission model (canonical values: 0 =
    // none, 1 = degraded / limited, 2 = full).

    int PrintAllowLevel() const noexcept;
    void PrintAllowLevel(int value) noexcept;

    int ChangeAllowLevel() const noexcept;
    void ChangeAllowLevel(int value) noexcept;

    int CopyAllowLevel() const noexcept;
    void CopyAllowLevel(int value) noexcept;

    // Static preset accessors — each returns a DocumentPrivilege
    // configured with only the named permission category granted.

    static DocumentPrivilege DegradedPrinting();
    static DocumentPrivilege Print();
    static DocumentPrivilege ModifyContents();
    static DocumentPrivilege Copy();
    static DocumentPrivilege ModifyAnnotations();
    static DocumentPrivilege FillIn();
    static DocumentPrivilege ScreenReaders();
    static DocumentPrivilege Assembly();
    static DocumentPrivilege AllowAll();
    static DocumentPrivilege ForbidAll();

    // IComparable<Object> — lexicographic ordering over the eleven
    // backing fields (8 bools then 3 ints). Returns -1 / 0 / +1
    // matching the canonical .NET IComparable contract.
    int CompareTo(const DocumentPrivilege& other) const noexcept;

private:
    bool allow_print_ = false;
    bool allow_degraded_printing_ = false;
    bool allow_modify_contents_ = false;
    bool allow_copy_ = false;
    bool allow_modify_annotations_ = false;
    bool allow_fill_in_ = false;
    bool allow_screen_readers_ = false;
    bool allow_assembly_ = false;
    int print_allow_level_ = 0;
    int change_allow_level_ = 0;
    int copy_allow_level_ = 0;
};

}  // namespace Aspose::Pdf::Facades
