#pragma once

// =============================================================================
// Aspose::Pdf::BaseParagraph — abstract base for paragraph-like
// page elements (annotations, generated headers/footers, etc.).
// Mirrors canonical Aspose.Pdf.BaseParagraph (Aspose.PDF 26.4.0).
//
// The members fall into two groups:
//   * Layout-engine geometry (VerticalAlignment, HorizontalAlignment,
//     Margin, IsFirstParagraphInColumn, IsKeptWithNext,
//     IsInNewPage, IsInLineParagraph, Hyperlink, ZIndex) — load
//     and persist verbatim; they're inert for annotations (which
//     are placed via their own Rect) but appear on the canonical
//     surface and so we ship them.
//   * System.ICloneable.Clone() — dropped per cpp drops.yaml
//     (C++ idiom uses copy-construction for value-shaped clones).
//
// BaseParagraph is abstract — instantiated only through derived
// classes (Annotation in beat A2 + future Paragraph-family
// types).
// =============================================================================

#include <memory>

#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/hyperlink.hpp>
#include <aspose/pdf/margin_info.hpp>
#include <aspose/pdf/vertical_alignment.hpp>

namespace Aspose::Pdf {

class BaseParagraph {
public:
    virtual ~BaseParagraph() = default;

    BaseParagraph(const BaseParagraph&) = default;
    BaseParagraph& operator=(const BaseParagraph&) = default;
    BaseParagraph(BaseParagraph&&) noexcept = default;
    BaseParagraph& operator=(BaseParagraph&&) noexcept = default;

    Aspose::Pdf::VerticalAlignment VerticalAlignment() const noexcept;
    void VerticalAlignment(Aspose::Pdf::VerticalAlignment value) noexcept;

    Aspose::Pdf::HorizontalAlignment HorizontalAlignment() const noexcept;
    void HorizontalAlignment(Aspose::Pdf::HorizontalAlignment value) noexcept;

    const Aspose::Pdf::MarginInfo& Margin() const noexcept;
    void Margin(Aspose::Pdf::MarginInfo value) noexcept;

    bool IsFirstParagraphInColumn() const noexcept;
    void IsFirstParagraphInColumn(bool value) noexcept;

    bool IsKeptWithNext() const noexcept;
    void IsKeptWithNext(bool value) noexcept;

    bool IsInNewPage() const noexcept;
    void IsInNewPage(bool value) noexcept;

    bool IsInLineParagraph() const noexcept;
    void IsInLineParagraph(bool value) noexcept;

    // Hyperlink is held by shared_ptr to support polymorphic
    // subclasses (WebHyperlink, LocalHyperlink). nullptr means
    // no hyperlink set.
    const std::shared_ptr<Aspose::Pdf::Hyperlink>& Hyperlink() const noexcept;
    void Hyperlink(std::shared_ptr<Aspose::Pdf::Hyperlink> value) noexcept;

    int ZIndex() const noexcept;
    void ZIndex(int value) noexcept;

protected:
    // Protected ctor — derived classes only.
    BaseParagraph() noexcept = default;

private:
    Aspose::Pdf::VerticalAlignment vertical_alignment_
        = Aspose::Pdf::VerticalAlignment::None;
    Aspose::Pdf::HorizontalAlignment horizontal_alignment_
        = Aspose::Pdf::HorizontalAlignment::None;
    Aspose::Pdf::MarginInfo margin_;
    bool is_first_paragraph_in_column_ = false;
    bool is_kept_with_next_ = false;
    bool is_in_new_page_ = false;
    bool is_in_line_paragraph_ = false;
    std::shared_ptr<Aspose::Pdf::Hyperlink> hyperlink_;
    int z_index_ = 0;
};

}  // namespace Aspose::Pdf
