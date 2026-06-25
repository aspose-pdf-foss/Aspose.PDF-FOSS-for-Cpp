#include <aspose/pdf/document_privilege.hpp>

namespace Aspose::Pdf::Facades {

// =============================================================================
// Property accessors — straight field getters/setters; default-
// constructed instances are equivalent to ForbidAll() (all flags
// false, all levels 0).
// =============================================================================

bool DocumentPrivilege::AllowPrint() const noexcept {
    return allow_print_;
}
void DocumentPrivilege::AllowPrint(bool v) noexcept {
    allow_print_ = v;
}

bool DocumentPrivilege::AllowDegradedPrinting() const noexcept {
    return allow_degraded_printing_;
}
void DocumentPrivilege::AllowDegradedPrinting(bool v) noexcept {
    allow_degraded_printing_ = v;
}

bool DocumentPrivilege::AllowModifyContents() const noexcept {
    return allow_modify_contents_;
}
void DocumentPrivilege::AllowModifyContents(bool v) noexcept {
    allow_modify_contents_ = v;
}

bool DocumentPrivilege::AllowCopy() const noexcept {
    return allow_copy_;
}
void DocumentPrivilege::AllowCopy(bool v) noexcept {
    allow_copy_ = v;
}

bool DocumentPrivilege::AllowModifyAnnotations() const noexcept {
    return allow_modify_annotations_;
}
void DocumentPrivilege::AllowModifyAnnotations(bool v) noexcept {
    allow_modify_annotations_ = v;
}

bool DocumentPrivilege::AllowFillIn() const noexcept {
    return allow_fill_in_;
}
void DocumentPrivilege::AllowFillIn(bool v) noexcept {
    allow_fill_in_ = v;
}

bool DocumentPrivilege::AllowScreenReaders() const noexcept {
    return allow_screen_readers_;
}
void DocumentPrivilege::AllowScreenReaders(bool v) noexcept {
    allow_screen_readers_ = v;
}

bool DocumentPrivilege::AllowAssembly() const noexcept {
    return allow_assembly_;
}
void DocumentPrivilege::AllowAssembly(bool v) noexcept {
    allow_assembly_ = v;
}

int DocumentPrivilege::PrintAllowLevel() const noexcept {
    return print_allow_level_;
}
void DocumentPrivilege::PrintAllowLevel(int v) noexcept {
    print_allow_level_ = v;
}

int DocumentPrivilege::ChangeAllowLevel() const noexcept {
    return change_allow_level_;
}
void DocumentPrivilege::ChangeAllowLevel(int v) noexcept {
    change_allow_level_ = v;
}

int DocumentPrivilege::CopyAllowLevel() const noexcept {
    return copy_allow_level_;
}
void DocumentPrivilege::CopyAllowLevel(int v) noexcept {
    copy_allow_level_ = v;
}

// =============================================================================
// Static preset accessors.
//
// Each preset returns a fresh DocumentPrivilege with exactly one
// permission category enabled. AllowAll grants every flag (and
// sets every level to 2 — the canonical "full" tier in the
// legacy 0/1/2 model); ForbidAll is a default-constructed
// instance.
// =============================================================================

DocumentPrivilege DocumentPrivilege::DegradedPrinting() {
    DocumentPrivilege p;
    p.allow_degraded_printing_ = true;
    p.print_allow_level_ = 1;   // degraded tier
    return p;
}

DocumentPrivilege DocumentPrivilege::Print() {
    DocumentPrivilege p;
    p.allow_print_ = true;
    p.print_allow_level_ = 2;   // full tier
    return p;
}

DocumentPrivilege DocumentPrivilege::ModifyContents() {
    DocumentPrivilege p;
    p.allow_modify_contents_ = true;
    p.change_allow_level_ = 2;
    return p;
}

DocumentPrivilege DocumentPrivilege::Copy() {
    DocumentPrivilege p;
    p.allow_copy_ = true;
    p.copy_allow_level_ = 2;
    return p;
}

DocumentPrivilege DocumentPrivilege::ModifyAnnotations() {
    DocumentPrivilege p;
    p.allow_modify_annotations_ = true;
    return p;
}

DocumentPrivilege DocumentPrivilege::FillIn() {
    DocumentPrivilege p;
    p.allow_fill_in_ = true;
    return p;
}

DocumentPrivilege DocumentPrivilege::ScreenReaders() {
    DocumentPrivilege p;
    p.allow_screen_readers_ = true;
    return p;
}

DocumentPrivilege DocumentPrivilege::Assembly() {
    DocumentPrivilege p;
    p.allow_assembly_ = true;
    return p;
}

DocumentPrivilege DocumentPrivilege::AllowAll() {
    DocumentPrivilege p;
    p.allow_print_ = true;
    p.allow_degraded_printing_ = true;
    p.allow_modify_contents_ = true;
    p.allow_copy_ = true;
    p.allow_modify_annotations_ = true;
    p.allow_fill_in_ = true;
    p.allow_screen_readers_ = true;
    p.allow_assembly_ = true;
    p.print_allow_level_ = 2;
    p.change_allow_level_ = 2;
    p.copy_allow_level_ = 2;
    return p;
}

DocumentPrivilege DocumentPrivilege::ForbidAll() {
    return DocumentPrivilege{};
}

// =============================================================================
// IComparable<Object> — lexicographic ordering across the eleven
// backing fields (8 bools then 3 ints). Returns -1, 0, or +1 per
// the .NET IComparable contract.
// =============================================================================

namespace {
    template <typename T>
    int LexStep(int acc, const T& a, const T& b) noexcept {
        if (acc != 0) return acc;
        if (a < b)   return -1;
        if (b < a)   return +1;
        return 0;
    }
}

int DocumentPrivilege::CompareTo(
        const DocumentPrivilege& other) const noexcept {
    int r = 0;
    r = LexStep(r, allow_print_,              other.allow_print_);
    r = LexStep(r, allow_degraded_printing_,  other.allow_degraded_printing_);
    r = LexStep(r, allow_modify_contents_,    other.allow_modify_contents_);
    r = LexStep(r, allow_copy_,               other.allow_copy_);
    r = LexStep(r, allow_modify_annotations_, other.allow_modify_annotations_);
    r = LexStep(r, allow_fill_in_,            other.allow_fill_in_);
    r = LexStep(r, allow_screen_readers_,     other.allow_screen_readers_);
    r = LexStep(r, allow_assembly_,           other.allow_assembly_);
    r = LexStep(r, print_allow_level_,        other.print_allow_level_);
    r = LexStep(r, change_allow_level_,       other.change_allow_level_);
    r = LexStep(r, copy_allow_level_,         other.copy_allow_level_);
    return r;
}

}  // namespace Aspose::Pdf::Facades
