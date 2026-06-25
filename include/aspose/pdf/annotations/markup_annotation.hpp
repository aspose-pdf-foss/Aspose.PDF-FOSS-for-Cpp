#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::MarkupAnnotation — abstract
// intermediate base for annotations carrying author / subject /
// reply metadata + review state. Mirrors canonical
// Aspose.Pdf.Annotations.MarkupAnnotation (Aspose.PDF 26.4.0);
// extends Annotation.
//
// Phased drops on this beat (drops.yaml):
//   * CreationDate — auto-drops via System.DateTime translations
//     cascade
//
// Popup landed at A12 when PopupAnnotation shipped.
//
// Concrete subclasses (TextAnnotation, FreeTextAnnotation,
// the Drawing family, etc.) land in cluster beats A6+.
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_state.hpp>
#include <aspose/pdf/annotations/annotation_state_model.hpp>
#include <aspose/pdf/annotations/reply_type.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Annotations {

class PopupAnnotation;

class MarkupAnnotation : public Annotation {
public:
    MarkupAnnotation() noexcept = default;
    explicit MarkupAnnotation(Aspose::Pdf::Document& document) noexcept;

    // Reset the review-state tracking on this annotation.
    void ClearState();

    // Set the annotation's review state and (optionally) the
    // reviewer's user name.
    void SetReviewState(AnnotationState state,
                        const std::string& userName);
    void SetReviewState(AnnotationState state);

    // Toggle the marked / unmarked marker-state.
    void SetMarkedState(bool marked);

    AnnotationState GetState() const noexcept;
    AnnotationStateModel GetStateModel() const noexcept;

    // ---- Properties ----

    const std::string& Title() const noexcept;
    void Title(std::string value) noexcept;

    const std::string& RichText() const noexcept;
    void RichText(std::string value) noexcept;

    const std::string& Subject() const noexcept;
    void Subject(std::string value) noexcept;

    double Opacity() const noexcept;
    void Opacity(double value) noexcept;

    // InReplyTo is held by shared_ptr to support polymorphic
    // Annotation subclasses without slicing. nullptr means no
    // reply relationship.
    const std::shared_ptr<Annotation>& InReplyTo() const noexcept;
    void InReplyTo(std::shared_ptr<Annotation> value) noexcept;

    // Popup is the optional floating-window companion that
    // displays this markup's contents. Held by shared_ptr; nullptr
    // when this markup has no popup attached. PopupAnnotation is
    // forward-declared at namespace scope to avoid a circular
    // header dependency.
    const std::shared_ptr<PopupAnnotation>& Popup() const noexcept;
    void Popup(std::shared_ptr<PopupAnnotation> value) noexcept;

    Annotations::ReplyType ReplyType() const noexcept;
    void ReplyType(Annotations::ReplyType value) noexcept;

private:
    Aspose::Pdf::Document* document_ = nullptr;
    std::string title_;
    std::string rich_text_;
    std::string subject_;
    double opacity_ = 1.0;
    std::shared_ptr<Annotation> in_reply_to_;
    std::shared_ptr<PopupAnnotation> popup_;
    Annotations::ReplyType reply_type_ = Annotations::ReplyType::Reply;
    AnnotationState state_ = AnnotationState::None;
    AnnotationStateModel state_model_ = AnnotationStateModel::None;
};

}  // namespace Aspose::Pdf::Annotations
