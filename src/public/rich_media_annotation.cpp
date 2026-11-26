#include <aspose/pdf/annotations/rich_media_annotation.hpp>

#include <stdexcept>
#include <utility>

namespace Aspose::Pdf::Annotations {

RichMediaAnnotation::RichMediaAnnotation(Aspose::Pdf::Page& page,
                                          Aspose::Pdf::Rectangle rect)
    : page_(&page) {
    Rect(std::move(rect));
}

void RichMediaAnnotation::Accept(AnnotationSelector& visitor) {
    visitor.Visit(*this);
}

void RichMediaAnnotation::Update() {
    if (content_name_.empty() || content_.empty())
        throw std::invalid_argument(
            "RichMediaAnnotation::Update: set the content first (SetContent)");
}

void RichMediaAnnotation::SetContent(const std::string& fileName,
                                     std::vector<std::byte> data) {
    content_name_ = fileName;
    content_ = std::move(data);
}
const std::string& RichMediaAnnotation::ContentName() const noexcept {
    return content_name_;
}
const std::vector<std::byte>& RichMediaAnnotation::Content() const noexcept {
    return content_;
}

const std::vector<std::byte>& RichMediaAnnotation::CustomPlayer() const noexcept {
    return custom_player_;
}
void RichMediaAnnotation::CustomPlayer(std::vector<std::byte> value) {
    custom_player_ = std::move(value);
}

void RichMediaAnnotation::AddCustomData(const std::string& name,
                                        std::vector<std::byte> data) {
    for (auto& [n, d] : custom_data_)
        if (n == name) {
            d = std::move(data);
            return;
        }
    custom_data_.emplace_back(name, std::move(data));
}

void RichMediaAnnotation::SetPoster(std::vector<std::byte> image) {
    poster_ = std::move(image);
}

const std::string&
RichMediaAnnotation::CustomFlashVariables() const noexcept {
    return custom_flash_variables_;
}
void RichMediaAnnotation::CustomFlashVariables(std::string v) noexcept {
    custom_flash_variables_ = std::move(v);
}

RichMediaAnnotation::ContentType
RichMediaAnnotation::Type() const noexcept { return type_; }
void RichMediaAnnotation::Type(ContentType v) noexcept { type_ = v; }

RichMediaAnnotation::ActivationEvent
RichMediaAnnotation::ActivateOn() const noexcept { return activate_on_; }
void RichMediaAnnotation::ActivateOn(ActivationEvent v) noexcept {
    activate_on_ = v;
}

AnnotationType RichMediaAnnotation::TypeImpl() const noexcept {
    return AnnotationType::RichMedia;
}

}  // namespace Aspose::Pdf::Annotations
