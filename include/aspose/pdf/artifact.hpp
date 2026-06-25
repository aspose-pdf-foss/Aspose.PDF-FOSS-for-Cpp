#pragma once

// =============================================================================
// Aspose::Pdf::Artifact — a page artifact (PDF §14.8.2.2): a graphics object,
// such as a watermark, that is not part of the document's logical content.
// Mirrors canonical Aspose.Pdf.Artifact (Aspose.PDF 26.4.0) as the watermark
// subset: a text artifact (SetTextAndState + TextState) or an image artifact
// (SetImage from PNG / JPEG bytes), positioned with Position, rotated with
// Rotation, made translucent with Opacity, and drawn under or over the page
// content via IsBackground. Added to a page through Page::Artifacts().
//
// v1 subset: the (string, string) ctor ships; the (ArtifactType, ArtifactSubtype)
// enum ctor, the FormattedText / page-number / multi-line text setters, the
// custom-value dictionary and the laid-out Rectangle are deferred (see the cpp
// drops). WatermarkArtifact's default ctor is the usual entry point.
// =============================================================================

#include <cstddef>
#include <string>
#include <vector>

#include <aspose/pdf/point.hpp>
#include <aspose/pdf/text_state.hpp>

namespace Aspose::Pdf {

class Document;

class Artifact {
public:
    // Canonical Artifact(type, subType) — names the artifact's marked-content
    // type / subtype (e.g. "Watermark").
    Artifact(const std::string& type, const std::string& subType);
    virtual ~Artifact();

    // Canonical SetTextAndState — make this a text artifact: `text` rendered
    // with `textState` (font size / colour).
    void SetTextAndState(const std::string& text,
                         const Aspose::Pdf::Text::TextState& textState);

    // Canonical SetImage(Stream) — make this an image artifact from raw
    // PNG / JPEG file bytes.
    void SetImage(const std::vector<std::byte>& imageStream);

    // Canonical Position — the artifact's placement point in user space.
    Aspose::Pdf::Point Position() const noexcept;
    void Position(Aspose::Pdf::Point value) noexcept;

    // Canonical Rotation — clockwise rotation in degrees about Position.
    double Rotation() const noexcept;
    void Rotation(double value) noexcept;

    // Canonical Opacity — fill/stroke alpha in [0, 1].
    double Opacity() const noexcept;
    void Opacity(double value) noexcept;

    // Canonical Text — the text-artifact string.
    const std::string& Text() const noexcept;
    void Text(std::string value);

    // Canonical TextState — the text-artifact rendering state (font / size).
    Aspose::Pdf::Text::TextState& TextState() noexcept;
    void TextState(const Aspose::Pdf::Text::TextState& value);

    // Canonical IsBackground — when true the artifact is drawn beneath the
    // page content (a background watermark); otherwise as an overlay.
    bool IsBackground() const noexcept;
    void IsBackground(bool value) noexcept;

protected:
    // For WatermarkArtifact's default ctor (type/subtype = "Watermark").
    Artifact() noexcept;

private:
    friend class Document;

    std::string type_;
    std::string subtype_;
    std::string text_;
    std::vector<std::byte> image_;
    bool has_image_ = false;
    Aspose::Pdf::Point position_{0.0, 0.0};
    double rotation_ = 0.0;
    double opacity_ = 1.0;
    Aspose::Pdf::Text::TextState text_state_;
    bool is_background_ = false;
};

}  // namespace Aspose::Pdf
