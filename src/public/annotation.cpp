#include <aspose/pdf/annotations/annotation.hpp>

#include <utility>

namespace Aspose::Pdf::Annotations {

Annotation::Annotation() noexcept
    : border_(this) {}

Annotation::Annotation(Aspose::Pdf::Rectangle rect) noexcept
    : rect_(std::move(rect)), border_(this) {}

// ---- Methods ----

Aspose::Pdf::Rectangle Annotation::GetRectangle(
        bool considerRotation) const noexcept {
    (void)considerRotation;  // page-rotation aware variant
                              // wires through at A14
                              // (Page.Annotations binding).
    return rect_;
}

void Annotation::Flatten() {
    // Flattening burns the annotation's appearance into the page
    // content stream. v1 stub — surfaces are stored, but
    // appearance generation lands with the Forms cluster's
    // appearance-stream generator.
}

void Annotation::Accept(AnnotationSelector& visitor) {
    // Default base-class behavior: no dispatch. Concrete
    // annotation subclasses (A6..A13) override this to call
    // `visitor.Visit(*this)` — the canonical visitor
    // double-dispatch idiom.
    (void)visitor;
}

// ---- Properties ----

bool Annotation::UpdateAppearanceOnConvert() const noexcept {
    return update_appearance_on_convert_;
}
void Annotation::UpdateAppearanceOnConvert(bool v) noexcept {
    update_appearance_on_convert_ = v;
}

bool Annotation::UseFontSubset() const noexcept {
    return use_font_subset_;
}
void Annotation::UseFontSubset(bool v) noexcept {
    use_font_subset_ = v;
}

AnnotationFlags Annotation::Flags() const noexcept { return flags_; }
void Annotation::Flags(AnnotationFlags v) noexcept { flags_ = v; }

AnnotationType Annotation::AnnotationType() const noexcept {
    return TypeImpl();
}

double Annotation::Width() const noexcept {
    return rect_.Width();
}
void Annotation::Width(double v) noexcept {
    rect_ = Aspose::Pdf::Rectangle{
        rect_.LLX(), rect_.LLY(),
        rect_.LLX() + v, rect_.URY(),
        /*normalizeCoordinates=*/false};
}

double Annotation::Height() const noexcept {
    return rect_.Height();
}
void Annotation::Height(double v) noexcept {
    rect_ = Aspose::Pdf::Rectangle{
        rect_.LLX(), rect_.LLY(),
        rect_.URX(), rect_.LLY() + v,
        /*normalizeCoordinates=*/false};
}

const Aspose::Pdf::Rectangle& Annotation::Rect() const noexcept {
    return rect_;
}
void Annotation::Rect(Aspose::Pdf::Rectangle v) noexcept {
    rect_ = std::move(v);
}

const std::string& Annotation::Contents() const noexcept {
    return contents_;
}
void Annotation::Contents(std::string v) noexcept {
    contents_ = std::move(v);
}

const std::string& Annotation::Name() const noexcept { return name_; }
void Annotation::Name(std::string v) noexcept {
    name_ = std::move(v);
}

Aspose::Pdf::Color Annotation::Color() const noexcept {
    return color_;
}
void Annotation::Color(Aspose::Pdf::Color v) noexcept {
    color_ = std::move(v);
}

Border& Annotation::Border() noexcept { return border_; }
const Border& Annotation::Border() const noexcept { return border_; }
void Annotation::Border(Annotations::Border v) noexcept {
    border_ = std::move(v);
}

const std::string& Annotation::ActiveState() const noexcept {
    return active_state_;
}
void Annotation::ActiveState(std::string v) noexcept {
    active_state_ = std::move(v);
}

const Characteristics& Annotation::Characteristics() const noexcept {
    return characteristics_;
}

TextAlignment Annotation::Alignment() const noexcept {
    return alignment_;
}
void Annotation::Alignment(TextAlignment v) noexcept {
    alignment_ = v;
}

// Note: the BaseParagraph::HorizontalAlignment accessor is
// inherited; Annotation overrides it (canonical surface has
// the property explicitly on Annotation too).
Aspose::Pdf::HorizontalAlignment
        Annotation::HorizontalAlignment() const noexcept {
    return BaseParagraph::HorizontalAlignment();
}
void Annotation::HorizontalAlignment(
        Aspose::Pdf::HorizontalAlignment v) noexcept {
    BaseParagraph::HorizontalAlignment(v);
}

Aspose::Pdf::HorizontalAlignment
        Annotation::TextHorizontalAlignment() const noexcept {
    return text_horizontal_alignment_;
}
void Annotation::TextHorizontalAlignment(
        Aspose::Pdf::HorizontalAlignment v) noexcept {
    text_horizontal_alignment_ = v;
}

const std::string& Annotation::FullName() const noexcept {
    return full_name_;
}
int Annotation::PageIndex() const noexcept { return page_index_; }

void Annotation::SetFullName(std::string v) noexcept {
    full_name_ = std::move(v);
}
void Annotation::SetPageIndex(int v) noexcept {
    page_index_ = v;
}

}  // namespace Aspose::Pdf::Annotations
