#pragma once

// Printer-mark placement (as Aspose.PDF): corner marks sit in the margin
// outside a page box's corner, registration targets centred on a side of the
// trim box, both clipped to the media box.

#include <algorithm>

#include <aspose/pdf/annotations/printer_mark_corner_position.hpp>
#include <aspose/pdf/annotations/printer_mark_side_position.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace foundation::printer_marks {

inline Aspose::Pdf::Rectangle MoveToCorner(const Aspose::Pdf::Rectangle& box,
                                           const Aspose::Pdf::Rectangle& media,
                                           Aspose::Pdf::Annotations::PrinterMarkCornerPosition corner,
                                           double size) {
    using C = Aspose::Pdf::Annotations::PrinterMarkCornerPosition;
    const double left = std::min(size, box.LLX() - media.LLX());
    const double right = std::min(size, media.URX() - box.URX());
    const double top = std::min(size, media.URY() - box.URY());
    const double bottom = std::min(size, box.LLY() - media.LLY());
    switch (corner) {
        case C::TopLeft: return {box.LLX() - left, box.URY(), box.LLX(), box.URY() + top, false};
        case C::TopRight: return {box.URX(), box.URY(), box.URX() + right, box.URY() + top, false};
        case C::BottomLeft: return {box.LLX() - left, box.LLY() - bottom, box.LLX(), box.LLY(), false};
        default: return {box.URX(), box.LLY() - bottom, box.URX() + right, box.LLY(), false};
    }
}

constexpr double kRegistrationMarkSize = 10 + 12 + 10;

inline Aspose::Pdf::Rectangle MoveToPageMargin(const Aspose::Pdf::Rectangle& trim,
                                               const Aspose::Pdf::Rectangle& media,
                                               Aspose::Pdf::Annotations::PrinterMarkSidePosition side) {
    using S = Aspose::Pdf::Annotations::PrinterMarkSidePosition;
    const double m = kRegistrationMarkSize;
    double x = 0, y = 0, w = m, h = m;
    switch (side) {
        case S::Top:
            h = std::min(m, media.URY() - trim.URY());
            x = trim.LLX() + (trim.URX() - trim.LLX()) / 2 - w / 2;
            y = trim.URY();
            break;
        case S::Bottom:
            h = std::min(m, trim.LLY() - media.LLY());
            x = trim.LLX() + (trim.URX() - trim.LLX()) / 2 - w / 2;
            y = trim.LLY() - h;
            break;
        case S::Left:
            w = std::min(m, trim.LLX() - media.LLX());
            x = trim.LLX() - w;
            y = trim.LLY() + (trim.URY() - trim.LLY()) / 2 - h / 2;
            break;
        default:
            w = std::min(m, media.URX() - trim.URX());
            x = trim.URX();
            y = trim.LLY() + (trim.URY() - trim.LLY()) / 2 - h / 2;
            break;
    }
    return {x, y, x + w, y + h, false};
}

}  // namespace foundation::printer_marks
