#pragma once

// =============================================================================
// Aspose::Pdf::WatermarkArtifact — a watermark page artifact. Mirrors canonical
// Aspose.Pdf.WatermarkArtifact (Aspose.PDF 26.4.0): an Artifact whose default
// ctor pre-sets the watermark type. Configure it via the inherited Artifact
// surface (SetTextAndState / SetImage / Position / Rotation / Opacity /
// IsBackground) and add it through Page::Artifacts().
// =============================================================================

#include <aspose/pdf/artifact.hpp>

namespace Aspose::Pdf {

class WatermarkArtifact : public Artifact {
public:
    WatermarkArtifact();
};

}  // namespace Aspose::Pdf
