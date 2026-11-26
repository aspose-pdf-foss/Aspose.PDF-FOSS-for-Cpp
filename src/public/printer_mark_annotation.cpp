#include <aspose/pdf/annotations/printer_mark_annotation.hpp>

#include <algorithm>
#include <memory>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/bleed_mark_annotation.hpp>
#include <aspose/pdf/annotations/color_bar_annotation.hpp>
#include <aspose/pdf/annotations/page_information_annotation.hpp>
#include <aspose/pdf/annotations/registration_mark_annotation.hpp>
#include <aspose/pdf/annotations/trim_mark_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

namespace Aspose::Pdf::Annotations {

void PrinterMarkAnnotation::AddPrinterMarks(Aspose::Pdf::Document& document,
                                            PrinterMarksKind marksKind) {
    if (marksKind == PrinterMarksKind::None) return;
    for (int i = 1; i <= static_cast<int>(document.Pages().Count()); ++i) {
        auto page = document.Pages()[i];
        AddPrinterMarks(page, marksKind);
    }
}

void PrinterMarkAnnotation::AddPrinterMarks(Aspose::Pdf::Page& page,
                                            PrinterMarksKind marksKind) {
    const int kind = static_cast<int>(marksKind);
    auto& annots = page.Annotations();
    using C = PrinterMarkCornerPosition;
    using S = PrinterMarkSidePosition;
    if (kind & static_cast<int>(PrinterMarksKind::TrimMarks))
        for (C c : {C::TopLeft, C::TopRight, C::BottomLeft, C::BottomRight})
            annots.AddOwned(std::make_unique<TrimMarkAnnotation>(page, c));
    if (kind & static_cast<int>(PrinterMarksKind::BleedMarks))
        for (C c : {C::TopLeft, C::TopRight, C::BottomLeft, C::BottomRight})
            annots.AddOwned(std::make_unique<BleedMarkAnnotation>(page, c));
    if (kind & static_cast<int>(PrinterMarksKind::RegistrationMarks))
        for (S s : {S::Top, S::Bottom, S::Left, S::Right})
            annots.AddOwned(std::make_unique<RegistrationMarkAnnotation>(page, s));
    const auto trim = page.TrimBox();
    const auto media = page.MediaBox();
    if (kind & static_cast<int>(PrinterMarksKind::ColorBars)) {
        // Four CMYK bars along the top margin.
        const double top_margin = media.URY() - trim.URY();
        const double bar_w = std::min(top_margin * 7, (trim.URX() - trim.LLX()) / 4);
        const double bar_h = std::min(top_margin, bar_w / 7);
        if (bar_w > 0 && bar_h > 0)
            for (int i = 0; i < 4; ++i) {
                const double x = trim.LLX() + i * bar_w;
                annots.AddOwned(std::make_unique<ColorBarAnnotation>(
                    page, Aspose::Pdf::Rectangle(x, media.URY() - bar_h, x + bar_w, media.URY(), false),
                    static_cast<ColorsOfCMYK>(i)));
            }
    }
    if (kind & static_cast<int>(PrinterMarksKind::PageInformation)) {
        // File name, page number and date in the bottom margin.
        if (trim.LLY() - media.LLY() > 0)
            annots.AddOwned(std::make_unique<PageInformationAnnotation>(
                page, Aspose::Pdf::Rectangle(trim.LLX(), media.LLY(), trim.URX(), trim.LLY(), false)));
    }
}

}  // namespace Aspose::Pdf::Annotations
