#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PdfPageEditor — page-level editor (move / rotate /
// zoom / align pages + page transitions). Mirrors canonical
// Aspose.Pdf.Facades.PdfPageEditor; extends SaveableFacade.
//
// v1: GetPages / GetPageSize / GetPageRotation read the real page tree;
// the transform operations (MovePosition / Rotation / Zoom / alignment /
// transitions) are staged into properties but ApplyChanges is a stub —
// the page content-transform wiring is deferred.
//
// Phased drops:
//   * GetPageBoxSize (System.Drawing.Rectangle return) — Drawing.Rectangle
//     cascade.
//   * Save(Stream) — Stream cascade.
//   * PageRotations (Dictionary<int,int>) — Dictionary cascade.
// =============================================================================

#include <vector>

#include <aspose/pdf/facades/alignment_type.hpp>
#include <aspose/pdf/facades/saveable_facade.hpp>
#include <aspose/pdf/facades/vertical_alignment_type.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/page_size.hpp>
#include <aspose/pdf/vertical_alignment.hpp>

namespace Aspose::Pdf {
class Document;
}

// MSVC C4250: BindPdf/Close are inherited from Facade by dominance over
// the virtual IFacade base also reached via ISaveableFacade — intended.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

namespace Aspose::Pdf::Facades {

class PdfPageEditor : public SaveableFacade {
public:
    // ---- Page-transition constants (canonical 1..16) ----
    static constexpr int SPLITVOUT = 1;
    static constexpr int SPLITHOUT = 2;
    static constexpr int SPLITVIN = 3;
    static constexpr int SPLITHIN = 4;
    static constexpr int BLINDV = 5;
    static constexpr int BLINDH = 6;
    static constexpr int INBOX = 7;
    static constexpr int OUTBOX = 8;
    static constexpr int LRWIPE = 9;
    static constexpr int RLWIPE = 10;
    static constexpr int BTWIPE = 11;
    static constexpr int TBWIPE = 12;
    static constexpr int DISSOLVE = 13;
    static constexpr int LRGLITTER = 14;
    static constexpr int TBGLITTER = 15;
    static constexpr int DGLITTER = 16;

    PdfPageEditor() noexcept = default;
    explicit PdfPageEditor(Aspose::Pdf::Document& document);

    void MovePosition(float offsetX, float offsetY);
    // Number of pages in the bound document (real).
    int GetPages();
    // Media-box size and rotation (degrees) of the 1-based page `pageId`.
    // Unbound → 0 x 0 / 0; out-of-range page → throws std::out_of_range.
    Aspose::Pdf::PageSize GetPageSize(int pageId);
    int GetPageRotation(int pageId);
    void ApplyChanges();

    int TransitionDuration() const noexcept;
    void TransitionDuration(int value) noexcept;
    int TransitionType() const noexcept;
    void TransitionType(int value) noexcept;
    int DisplayDuration() const noexcept;
    void DisplayDuration(int value) noexcept;
    const std::vector<int>& ProcessPages() const noexcept;
    void ProcessPages(std::vector<int> value);
    int Rotation() const noexcept;
    void Rotation(int value) noexcept;
    float Zoom() const noexcept;
    void Zoom(float value) noexcept;

    Aspose::Pdf::PageSize PageSize() const;
    void PageSize(Aspose::Pdf::PageSize value);

    const AlignmentType& Alignment() const noexcept;
    void Alignment(AlignmentType value);
    Aspose::Pdf::HorizontalAlignment HorizontalAlignment() const noexcept;
    void HorizontalAlignment(Aspose::Pdf::HorizontalAlignment value) noexcept;
    const Aspose::Pdf::Facades::VerticalAlignmentType&
        VerticalAlignment() const noexcept;
    void VerticalAlignment(Aspose::Pdf::Facades::VerticalAlignmentType value);
    Aspose::Pdf::VerticalAlignment VerticalAlignmentType() const noexcept;
    void VerticalAlignmentType(Aspose::Pdf::VerticalAlignment value) noexcept;

private:
    int transition_duration_ = 1;
    int transition_type_ = 0;
    int display_duration_ = 0;
    std::vector<int> process_pages_;
    int rotation_ = 0;
    float zoom_ = 1.0f;
    Aspose::Pdf::PageSize page_size_ = Aspose::Pdf::PageSize::PageLetter();
    AlignmentType alignment_ = AlignmentType::Left();
    Aspose::Pdf::HorizontalAlignment horizontal_alignment_ =
        Aspose::Pdf::HorizontalAlignment::None;
    Aspose::Pdf::Facades::VerticalAlignmentType vertical_alignment_ =
        Aspose::Pdf::Facades::VerticalAlignmentType::Top();
    Aspose::Pdf::VerticalAlignment vertical_alignment_type_ =
        Aspose::Pdf::VerticalAlignment::None;
};

}  // namespace Aspose::Pdf::Facades

#ifdef _MSC_VER
#pragma warning(pop)
#endif
