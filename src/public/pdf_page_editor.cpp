#include <aspose/pdf/facades/pdf_page_editor.hpp>

#include <utility>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>

namespace Aspose::Pdf::Facades {

PdfPageEditor::PdfPageEditor(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

void PdfPageEditor::MovePosition(float, float) {}

int PdfPageEditor::GetPages() {
    return document_ == nullptr
               ? 0
               : static_cast<int>(document_->Pages().Count());
}

Aspose::Pdf::PageSize PdfPageEditor::GetPageSize(int) {
    // v1 stub — canonical US-Letter default until Page exposes Rect/CropBox.
    return Aspose::Pdf::PageSize::PageLetter();
}

int PdfPageEditor::GetPageRotation(int) { return 0; }

void PdfPageEditor::ApplyChanges() {}

int PdfPageEditor::TransitionDuration() const noexcept {
    return transition_duration_;
}
void PdfPageEditor::TransitionDuration(int value) noexcept {
    transition_duration_ = value;
}
int PdfPageEditor::TransitionType() const noexcept { return transition_type_; }
void PdfPageEditor::TransitionType(int value) noexcept {
    transition_type_ = value;
}
int PdfPageEditor::DisplayDuration() const noexcept {
    return display_duration_;
}
void PdfPageEditor::DisplayDuration(int value) noexcept {
    display_duration_ = value;
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
