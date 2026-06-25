#pragma once

// =============================================================================
// Aspose::Pdf::Annotations::Characteristics — annotation
// presentation-state carrier (background + border colors +
// rotation). Mirrors canonical Aspose.Pdf.Annotations.
// Characteristics (Aspose.PDF 26.4.0).
//
// Canonical Background and Border properties take System.Drawing
// .Color values — both drop automatically via the cpp
// translations.yaml dropped: cascade. Only Rotate (an
// Aspose.Pdf.Rotation enum getter/setter) ships.
// =============================================================================

#include <aspose/pdf/rotation.hpp>

namespace Aspose::Pdf::Annotations {

class Characteristics {
public:
    Characteristics() noexcept = default;

    Aspose::Pdf::Rotation Rotate() const noexcept;
    void Rotate(Aspose::Pdf::Rotation value) noexcept;

private:
    Aspose::Pdf::Rotation rotate_ = Aspose::Pdf::Rotation::None;
};

}  // namespace Aspose::Pdf::Annotations
