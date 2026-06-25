#pragma once

// =============================================================================
// Aspose::Pdf::FloatingBox — a positioned box paragraph that hosts its own
// child paragraphs. Mirrors canonical Aspose.Pdf.FloatingBox (Aspose.PDF
// 26.4.0): a BaseParagraph with a Width / Height, a Border and BackgroundColor,
// and a nested Paragraphs collection. Added to a page via Page::Paragraphs().
//
// v1 subset: the box frame (Border) renders at Save; the nested Paragraphs are
// stored and exposed but their column flow / BackgroundColor fill is deferred
// (the lib renders strokes in black; see GraphInfo / DrawEdge). ColumnInfo and
// Clone are dropped (see the cpp drops).
// =============================================================================

#include <aspose/pdf/base_paragraph.hpp>
#include <aspose/pdf/border_info.hpp>
#include <aspose/pdf/color.hpp>
#include <aspose/pdf/paragraphs.hpp>

namespace Aspose::Pdf {

class FloatingBox : public BaseParagraph {
public:
    FloatingBox();
    FloatingBox(float width, float height);

    // Canonical Width / Height — the box dimensions in points.
    double Width() const noexcept;
    void Width(double value) noexcept;
    double Height() const noexcept;
    void Height(double value) noexcept;

    // Canonical IsNeedRepeating — repeat the box across pages on overflow.
    bool IsNeedRepeating() const noexcept;
    void IsNeedRepeating(bool value) noexcept;

    // Canonical Paragraphs — the box's child paragraphs.
    Aspose::Pdf::Paragraphs& Paragraphs() noexcept;
    void Paragraphs(Aspose::Pdf::Paragraphs value);

    // Canonical Border — the box frame (rendered at Save).
    const Aspose::Pdf::BorderInfo& Border() const noexcept;
    void Border(Aspose::Pdf::BorderInfo value);

    // Canonical BackgroundColor — the box fill colour (stored; fill rendering
    // deferred while colour emission is black-only).
    const Aspose::Pdf::Color& BackgroundColor() const noexcept;
    void BackgroundColor(Aspose::Pdf::Color value);

private:
    friend class Document;

    double width_ = 0.0;
    double height_ = 0.0;
    bool is_need_repeating_ = false;
    Aspose::Pdf::Paragraphs paragraphs_;
    Aspose::Pdf::BorderInfo border_;
    Aspose::Pdf::Color background_color_;
};

}  // namespace Aspose::Pdf
