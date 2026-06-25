#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::AnnotationType — selector enum
// identifying the canonical Aspose.Pdf.Annotations subtype.
// Mirrors canonical Aspose.Pdf.Annotations.AnnotationType
// (Aspose.PDF 26.4.0). Each concrete annotation class
// (TextAnnotation, LinkAnnotation, etc.) shipped in cluster
// beats A6..A13 reports its kind via Annotation::AnnotationType().
// =============================================================================

namespace Aspose::Pdf::Annotations {

enum class AnnotationType : int {
    Text = 0,
    Circle = 1,
    Polygon = 2,
    PolyLine = 3,
    Line = 4,
    Square = 5,
    FreeText = 6,
    Highlight = 7,
    Underline = 8,
    Squiggly = 9,
    StrikeOut = 10,
    Caret = 11,
    Ink = 12,
    Link = 13,
    Popup = 14,
    FileAttachment = 15,
    Sound = 16,
    Movie = 17,
    Screen = 18,
    Widget = 19,
    Watermark = 20,
    TrapNet = 21,
    PrinterMark = 22,
    Redaction = 23,
    Stamp = 24,
    RichMedia = 25,
    Unknown = 26,
    PDF3D = 27,
    ColorBar = 28,
    TrimMark = 29,
    BleedMark = 30,
    RegistrationMark = 31,
    PageInformation = 32,
};

}  // namespace Aspose::Pdf::Annotations
