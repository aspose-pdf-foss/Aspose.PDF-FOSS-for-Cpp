// Bodies for the watermark-artifact surface: Artifact, WatermarkArtifact,
// ArtifactCollection.

#include <aspose/pdf/artifact.hpp>
#include <aspose/pdf/artifact_collection.hpp>
#include <aspose/pdf/watermark_artifact.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace Aspose::Pdf {

// ---- Artifact --------------------------------------------------------------

Artifact::Artifact(const std::string& type, const std::string& subType)
    : type_(type), subtype_(subType) {}

Artifact::Artifact() noexcept = default;

Artifact::~Artifact() = default;

void Artifact::SetTextAndState(const std::string& text,
                               const Aspose::Pdf::Text::TextState& textState) {
    text_ = text;
    text_state_ = textState;
    has_image_ = false;
}

void Artifact::SetImage(const std::vector<std::byte>& imageStream) {
    image_ = imageStream;
    has_image_ = true;
}

Aspose::Pdf::Point Artifact::Position() const noexcept { return position_; }
void Artifact::Position(Aspose::Pdf::Point value) noexcept {
    position_ = value;
}

double Artifact::Rotation() const noexcept { return rotation_; }
void Artifact::Rotation(double value) noexcept { rotation_ = value; }

double Artifact::Opacity() const noexcept { return opacity_; }
void Artifact::Opacity(double value) noexcept { opacity_ = value; }

const std::string& Artifact::Text() const noexcept { return text_; }
void Artifact::Text(std::string value) { text_ = std::move(value); }

Aspose::Pdf::Text::TextState& Artifact::TextState() noexcept {
    return text_state_;
}
void Artifact::TextState(const Aspose::Pdf::Text::TextState& value) {
    text_state_ = value;
}

bool Artifact::IsBackground() const noexcept { return is_background_; }
void Artifact::IsBackground(bool value) noexcept { is_background_ = value; }

// ---- WatermarkArtifact -----------------------------------------------------

WatermarkArtifact::WatermarkArtifact() : Artifact() {}

// ---- ArtifactCollection ----------------------------------------------------

ArtifactCollection::ArtifactCollection() = default;
ArtifactCollection::~ArtifactCollection() = default;

void ArtifactCollection::Add(Artifact& artifact) {
    items_.push_back(&artifact);
}

void ArtifactCollection::Delete(int index) {
    if (index < 1 || static_cast<std::size_t>(index) > items_.size())
        throw std::out_of_range("ArtifactCollection::Delete: index");
    items_.erase(items_.begin() + (index - 1));
}

void ArtifactCollection::Delete(const Artifact& artifact) {
    auto it = std::find(items_.begin(), items_.end(), &artifact);
    if (it != items_.end()) items_.erase(it);
}

void ArtifactCollection::Update(Artifact& artifact) {
    // Reference-identity membership: artifacts render from their live state at
    // Save, so an in-place edit needs no action; a not-yet-added artifact is
    // appended.
    if (std::find(items_.begin(), items_.end(), &artifact) == items_.end())
        items_.push_back(&artifact);
}

int ArtifactCollection::Count() const noexcept {
    return static_cast<int>(items_.size());
}

Artifact& ArtifactCollection::operator[](int index) const {
    if (index < 1 || static_cast<std::size_t>(index) > items_.size())
        throw std::out_of_range("ArtifactCollection::operator[]: index");
    return *items_[static_cast<std::size_t>(index) - 1];
}

}  // namespace Aspose::Pdf
