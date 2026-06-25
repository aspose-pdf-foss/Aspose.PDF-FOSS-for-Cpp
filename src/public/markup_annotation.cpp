#include <aspose/pdf/annotations/markup_annotation.hpp>

#include <memory>
#include <utility>

#include <aspose/pdf/annotations/popup_annotation.hpp>

namespace Aspose::Pdf::Annotations {

MarkupAnnotation::MarkupAnnotation(
        Aspose::Pdf::Document& document) noexcept
    : document_(&document) {}

void MarkupAnnotation::ClearState() {
    state_ = AnnotationState::None;
    state_model_ = AnnotationStateModel::None;
}

void MarkupAnnotation::SetReviewState(
        AnnotationState state, const std::string& userName) {
    state_ = state;
    state_model_ = AnnotationStateModel::Review;
    if (!userName.empty()) {
        // The reviewer's name is persisted via the standard
        // Title (canonical /T) entry — same convention as
        // Aspose.Pdf .NET when an explicit user name is supplied.
        title_ = userName;
    }
}

void MarkupAnnotation::SetReviewState(AnnotationState state) {
    state_ = state;
    state_model_ = AnnotationStateModel::Review;
}

void MarkupAnnotation::SetMarkedState(bool marked) {
    state_ = marked ? AnnotationState::Marked
                    : AnnotationState::Unmarked;
    state_model_ = AnnotationStateModel::Marked;
}

AnnotationState MarkupAnnotation::GetState() const noexcept {
    return state_;
}

AnnotationStateModel MarkupAnnotation::GetStateModel() const noexcept {
    return state_model_;
}

const std::string& MarkupAnnotation::Title() const noexcept {
    return title_;
}
void MarkupAnnotation::Title(std::string v) noexcept {
    title_ = std::move(v);
}

const std::string& MarkupAnnotation::RichText() const noexcept {
    return rich_text_;
}
void MarkupAnnotation::RichText(std::string v) noexcept {
    rich_text_ = std::move(v);
}

const std::string& MarkupAnnotation::Subject() const noexcept {
    return subject_;
}
void MarkupAnnotation::Subject(std::string v) noexcept {
    subject_ = std::move(v);
}

double MarkupAnnotation::Opacity() const noexcept {
    return opacity_;
}
void MarkupAnnotation::Opacity(double v) noexcept {
    opacity_ = v;
}

const std::shared_ptr<Annotation>&
        MarkupAnnotation::InReplyTo() const noexcept {
    return in_reply_to_;
}
void MarkupAnnotation::InReplyTo(
        std::shared_ptr<Annotation> v) noexcept {
    in_reply_to_ = std::move(v);
}

const std::shared_ptr<PopupAnnotation>&
        MarkupAnnotation::Popup() const noexcept {
    return popup_;
}
void MarkupAnnotation::Popup(std::shared_ptr<PopupAnnotation> v) noexcept {
    popup_ = std::move(v);
}

ReplyType MarkupAnnotation::ReplyType() const noexcept {
    return reply_type_;
}
void MarkupAnnotation::ReplyType(Annotations::ReplyType v) noexcept {
    reply_type_ = v;
}

}  // namespace Aspose::Pdf::Annotations
