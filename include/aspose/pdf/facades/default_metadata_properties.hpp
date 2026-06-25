#pragma once

// =============================================================================
// Aspose::Pdf::Facades::DefaultMetadataProperties — XMP property
// selector for PdfXmpMetadata's read-only convenience getters
// (PDF/XMP §14.3). Mirrors canonical
// Aspose.Pdf.Facades.DefaultMetadataProperties.
// =============================================================================

namespace Aspose::Pdf::Facades {

enum class DefaultMetadataProperties : int {
    Advisory = 0,
    BaseURL = 1,
    CreateDate = 2,
    CreatorTool = 3,
    Identifier = 4,
    MetadataDate = 5,
    ModifyDate = 6,
    Nickname = 7,
    Thumbnails = 8,
};

}  // namespace Aspose::Pdf::Facades
