#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::RichMediaAnnotation — rich-media
// annotation (PDF 1.7 ExtensionLevel 3 /Subtype /RichMedia).
// Mirrors canonical Aspose.Pdf.Annotations.RichMediaAnnotation;
// extends Annotation directly.
//
// The Stream-taking members take byte vectors here: SetContent,
// AddCustomData, SetPoster, CustomPlayer, Content.
//
// Save writes the annotation with its /RichMediaContent (the content, an
// optional custom player and custom data embedded as /Assets, one
// /Video or /Audio configuration with one instance) and
// /RichMediaSettings (activation on click / page open / page visible),
// plus an appearance: the poster image, or a frame with a "play" triangle.
// Without a custom player the instance plays the content asset directly
// (a Flash player is only used when given).
// =============================================================================

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/annotation_selector.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace Aspose::Pdf {
class Document;
class Page;
}

namespace Aspose::Pdf::Annotations {

class RichMediaAnnotation : public Annotation {
public:
    enum class ContentType : int {
        Audio = 0,
        Video = 1,
        Unknown = 2,
    };

    enum class ActivationEvent : int {
        Click = 0,
        PageOpen = 1,
        PageVisible = 2,
    };

    RichMediaAnnotation(Aspose::Pdf::Page& page,
                        Aspose::Pdf::Rectangle rect);

    void Accept(AnnotationSelector& visitor) override;

    // Re-emit the annotation's stream content from the configured
    // sources. v1 is a no-op; full update wiring lands with the
    // Document save-time edit-flow.
    // Checks the annotation is ready to save (content set); throws
    // std::invalid_argument otherwise. The properties are written at Save.
    void Update();

    // The media file (e.g. "clip.mp4" / "song.mp3") and its bytes.
    void SetContent(const std::string& fileName, std::vector<std::byte> data);
    const std::string& ContentName() const noexcept;
    const std::vector<std::byte>& Content() const noexcept;

    // An optional Flash player (.swf) the content is played with; it gets
    // FlashVars "src=<content>" (+ "&autoPlay=true" for video) unless
    // CustomFlashVariables is set.
    const std::vector<std::byte>& CustomPlayer() const noexcept;
    void CustomPlayer(std::vector<std::byte> value);

    // An extra embedded asset (subtitles, skins...).
    void AddCustomData(const std::string& name, std::vector<std::byte> data);

    // A PNG or JPEG image shown until the media is activated.
    void SetPoster(std::vector<std::byte> image);

    const std::string& CustomFlashVariables() const noexcept;
    void CustomFlashVariables(std::string value) noexcept;

    ContentType Type() const noexcept;
    void Type(ContentType value) noexcept;

    ActivationEvent ActivateOn() const noexcept;
    void ActivateOn(ActivationEvent value) noexcept;

protected:
    Annotations::AnnotationType TypeImpl() const noexcept override;

private:
    Aspose::Pdf::Page* page_ = nullptr;
    std::string custom_flash_variables_;
    ContentType type_ = ContentType::Unknown;
    ActivationEvent activate_on_ = ActivationEvent::Click;
    std::string content_name_;
    std::vector<std::byte> content_;
    std::vector<std::byte> custom_player_;
    std::vector<std::pair<std::string, std::vector<std::byte>>> custom_data_;
    std::vector<std::byte> poster_;
    friend class Aspose::Pdf::Document;
};

}  // namespace Aspose::Pdf::Annotations
