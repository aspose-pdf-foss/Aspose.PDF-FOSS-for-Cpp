#include <aspose/pdf/facades/pdf_page_editor.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

namespace Aspose::Pdf::Facades {

PdfPageEditor::PdfPageEditor(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

void PdfPageEditor::MovePosition(float offsetX, float offsetY) {
    move_set_ = true;
    move_x_ = offsetX;
    move_y_ = offsetY;
}

int PdfPageEditor::GetPages() {
    return document_ == nullptr
               ? 0
               : static_cast<int>(document_->Pages().Count());
}

Aspose::Pdf::PageSize PdfPageEditor::GetPageSize(int pageId) {
    if (document_ == nullptr) return Aspose::Pdf::PageSize(0.0f, 0.0f);
    const auto rect = document_->Pages()[pageId].Rect();
    return Aspose::Pdf::PageSize(static_cast<float>(rect.Width()),
                                 static_cast<float>(rect.Height()));
}

int PdfPageEditor::GetPageRotation(int pageId) {
    if (document_ == nullptr) return 0;
    return static_cast<int>(document_->Pages()[pageId].Rotate()) * 90;
}

void PdfPageEditor::ApplyChanges() {
    if (document_ == nullptr) return;
    if (rotation_ % 90 != 0)
        throw std::invalid_argument(
            "PdfPageEditor: Rotation must be a multiple of 90 degrees");

    // Effective alignment: the enum properties win; otherwise the
    // AlignmentType / VerticalAlignmentType ones; bottom-left by default.
    using HA = Aspose::Pdf::HorizontalAlignment;
    using VA = Aspose::Pdf::VerticalAlignment;
    HA halign = horizontal_alignment_;
    if (halign == HA::None)
        halign = alignment_ == AlignmentType::Center()  ? HA::Center
                 : alignment_ == AlignmentType::Right() ? HA::Right
                                                        : HA::Left;
    VA valign = vertical_alignment_type_;
    if (valign == VA::None)
        valign = !vertical_alignment_set_ ? VA::Bottom
                 : vertical_alignment_ == VerticalAlignmentType::Top()    ? VA::Top
                 : vertical_alignment_ == VerticalAlignmentType::Center() ? VA::Center
                                                                          : VA::Bottom;

    std::vector<int> pages = process_pages_;
    if (pages.empty())
        for (int i = 1; i <= document_->Pages().Count(); ++i) pages.push_back(i);

    const double pi = std::acos(-1.0);
    const double t = -pi * (((rotation_ % 360) + 360) % 360) / 180.0;
    const double ct = std::round(std::cos(t)), st = std::round(std::sin(t));
    const double z = zoom_;
    for (int page : pages) {
        if (page < 1 || page > document_->Pages().Count()) continue;
        const auto r = document_->Pages()[page].Rect();
        // Content transform: scale by Zoom, rotate by -Rotation, then put
        // the result's lower-left corner at the origin.
        std::array<double, 6> cm{z * ct, z * st, -z * st, z * ct, 0, 0};
        auto bbox = [&](const std::array<double, 6>& m) {
            double x0 = 1e300, y0 = 1e300, x1 = -1e300, y1 = -1e300;
            for (double x : {r.LLX(), r.URX()})
                for (double y : {r.LLY(), r.URY()}) {
                    const double px = m[0] * x + m[2] * y + m[4];
                    const double py = m[1] * x + m[3] * y + m[5];
                    x0 = std::min(x0, px); x1 = std::max(x1, px);
                    y0 = std::min(y0, py); y1 = std::max(y1, py);
                }
            return std::array<double, 4>{x0, y0, x1, y1};
        };
        const auto zoomed = bbox(cm);
        const auto rotated = bbox({ct, st, -st, ct, 0, 0});
        cm[4] = -zoomed[0];
        cm[5] = -zoomed[1];
        double dest_w = rotated[2] - rotated[0], dest_h = rotated[3] - rotated[1];
        if (page_size_set_) {
            dest_w = page_size_.Width();
            dest_h = page_size_.Height();
        }
        const double zw = zoomed[2] - zoomed[0], zh = zoomed[3] - zoomed[1];
        if (halign == HA::Right) cm[4] += dest_w - zw;
        else if (halign == HA::Center) cm[4] += (dest_w - zw) / 2;
        if (valign == VA::Top) cm[5] += dest_h - zh;
        else if (valign == VA::Center) cm[5] += (dest_h - zh) / 2;
        if (move_set_) {
            cm[4] += move_x_;
            cm[5] += move_y_;
        }
        const bool identity = cm[0] == 1 && cm[1] == 0 && cm[2] == 0 && cm[3] == 1 &&
                              std::abs(cm[4] + r.LLX()) < 1e-9 &&
                              std::abs(cm[5] + r.LLY()) < 1e-9 &&
                              std::abs(dest_w - r.Width()) < 1e-9 &&
                              std::abs(dest_h - r.Height()) < 1e-9 &&
                              r.LLX() == 0 && r.LLY() == 0;
        if (!identity)
            document_->TransformPageContentInternal(page, cm, dest_w, dest_h);
        document_->SetPageTransitionInternal(
            page, transition_type_set_ ? transition_type_ : 0,
            transition_duration_set_ ? transition_duration_ : -1,
            display_duration_set_ ? display_duration_ : -1);
    }
}

int PdfPageEditor::TransitionDuration() const noexcept {
    return transition_duration_;
}
void PdfPageEditor::TransitionDuration(int value) noexcept {
    transition_duration_ = value;
    transition_duration_set_ = true;
}
int PdfPageEditor::TransitionType() const noexcept { return transition_type_; }
void PdfPageEditor::TransitionType(int value) noexcept {
    transition_type_ = value;
    transition_type_set_ = true;
}
int PdfPageEditor::DisplayDuration() const noexcept {
    return display_duration_;
}
void PdfPageEditor::DisplayDuration(int value) noexcept {
    display_duration_ = value;
    display_duration_set_ = true;
}
const std::vector<int>& PdfPageEditor::ProcessPages() const noexcept {
    return process_pages_;
}
void PdfPageEditor::ProcessPages(std::vector<int> value) {
    process_pages_ = std::move(value);
}
int PdfPageEditor::Rotation() const noexcept { return rotation_; }
void PdfPageEditor::Rotation(int value) noexcept { rotation_ = value; }
float PdfPageEditor::Zoom() const noexcept { return zoom_; }
void PdfPageEditor::Zoom(float value) noexcept { zoom_ = value; }

Aspose::Pdf::PageSize PdfPageEditor::PageSize() const { return page_size_; }
void PdfPageEditor::PageSize(Aspose::Pdf::PageSize value) {
    page_size_ = value;
    page_size_set_ = true;
}

const AlignmentType& PdfPageEditor::Alignment() const noexcept {
    return alignment_;
}
void PdfPageEditor::Alignment(AlignmentType value) {
    alignment_ = std::move(value);
}
Aspose::Pdf::HorizontalAlignment PdfPageEditor::HorizontalAlignment()
    const noexcept {
    return horizontal_alignment_;
}
void PdfPageEditor::HorizontalAlignment(
    Aspose::Pdf::HorizontalAlignment value) noexcept {
    horizontal_alignment_ = value;
}
const Aspose::Pdf::Facades::VerticalAlignmentType&
PdfPageEditor::VerticalAlignment() const noexcept {
    return vertical_alignment_;
}
void PdfPageEditor::VerticalAlignment(
    Aspose::Pdf::Facades::VerticalAlignmentType value) {
    vertical_alignment_ = std::move(value);
    vertical_alignment_set_ = true;
}
Aspose::Pdf::VerticalAlignment PdfPageEditor::VerticalAlignmentType()
    const noexcept {
    return vertical_alignment_type_;
}
void PdfPageEditor::VerticalAlignmentType(
    Aspose::Pdf::VerticalAlignment value) noexcept {
    vertical_alignment_type_ = value;
}

}  // namespace Aspose::Pdf::Facades
