#pragma once

// =============================================================================
// Aspose::Pdf::Facades::ViewerPreference — the viewer-preference flags taken
// by PdfContentEditor::ChangeViewerPreference and returned (OR-ed) by
// GetViewerPreference. Values match Aspose.PDF.
// =============================================================================

namespace Aspose::Pdf::Facades {

class ViewerPreference {
public:
    static constexpr int PageModeUseOC = 1048576;
    static constexpr int DisplayDocTitle = 262144;
    static constexpr int NonFullScreenPageModeUseOC = 524288;
    static constexpr int PageLayoutSinglePage = 1;
    static constexpr int PageLayoutOneColumn = 2;
    static constexpr int PageLayoutTwoColumnLeft = 4;
    static constexpr int PageLayoutTwoColumnRight = 8;
    static constexpr int PageModeUseNone = 16;
    static constexpr int PageModeUseOutlines = 32;
    static constexpr int PageModeUseThumbs = 64;
    static constexpr int PageModeFullScreen = 128;
    static constexpr int PageModeUseAttachment = 2097152;
    static constexpr int HideToolbar = 256;
    static constexpr int HideMenubar = 512;
    static constexpr int HideWindowUI = 1024;
    static constexpr int FitWindow = 2048;
    static constexpr int CenterWindow = 4096;
    static constexpr int NonFullScreenPageModeUseNone = 8192;
    static constexpr int NonFullScreenPageModeUseOutlines = 16384;
    static constexpr int NonFullScreenPageModeUseThumbs = 32768;
    static constexpr int DirectionL2R = 65536;
    static constexpr int DirectionR2L = 131072;
    static constexpr int Simplex = 4194304;
    static constexpr int DuplexFlipShortEdge = 8388608;
    static constexpr int DuplexFlipLongEdge = 16777216;
    static constexpr int PrintScalingAppDefault = 1 << 25;
    static constexpr int PrintScalingNone = 1 << 26;
    static constexpr int PickTrayByPDFSize = 1 << 27;
};

}  // namespace Aspose::Pdf::Facades
