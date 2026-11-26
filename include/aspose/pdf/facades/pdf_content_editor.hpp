#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PdfContentEditor — content / link / stamp / image /
// attachment / text editor. Mirrors canonical
// Aspose.Pdf.Facades.PdfContentEditor; extends SaveableFacade.
//
// Attachments, document JavaScript actions, viewer preferences (see
// ViewerPreference), image replacement / deletion, text replacement and
// stamp edits. Catalog edits (actions, viewer preferences) and attachments
// are written at Save; image, text and stamp edits rewrite the bound
// document at once.
//
// Stamps: the stamps PdfFileStamp draws (headers, footers, page numbers)
// carry their StampId; these methods find, delete, hide / show and move
// them. Stamps made by other tools are not recognised.
//
// The bulk of the canonical surface drops in cpp: every Create*/Draw*
// overload routes through System.Drawing.Rectangle / System.Drawing.Color
// (geometry/colour cascades), and the text-edit/replace options +
// Stamp/LineInfo/StampInfo types aren't in the cpp catalog. v1 ships the
// primitive-typed editor surface (attachments / viewer-preference /
// image-replace / text-replace(string) / stamp visibility + move).
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/facades/saveable_facade.hpp>

namespace Aspose::Pdf {
class Document;
}

// MSVC C4250: BindPdf/Close are inherited from Facade by dominance over
// the virtual IFacade base also reached via ISaveableFacade — intended.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

namespace Aspose::Pdf::Facades {

class PdfContentEditor : public SaveableFacade {
public:
    // ---- Document-action trigger names (canonical) ----
    inline static const std::string DocumentOpen = "DO";
    inline static const std::string DocumentClose = "WC";
    inline static const std::string DocumentWillSave = "WS";
    inline static const std::string DocumentSaved = "DS";
    inline static const std::string DocumentWillPrint = "WP";
    inline static const std::string DocumentPrinted = "DP";

    PdfContentEditor() noexcept = default;
    explicit PdfContentEditor(Aspose::Pdf::Document& document);

    // ---- Attachments ----
    // Embed the file at `path` (its file name is the attachment name).
    void AddDocumentAttachment(const std::string& path,
                               const std::string& description);
    // Remove every attachment.
    void DeleteAttachments();

    // ---- Document actions ----
    // JavaScript run on a document event: DocumentOpen ("DO") becomes the
    // /OpenAction; the others go into the catalog /AA.
    void AddDocumentAdditionalAction(const std::string& actionName,
                                     const std::string& javaScript);
    void RemoveDocumentOpenAction();

    // ---- Viewer preference (ViewerPreference flags) ----
    // Applies every flag set in `viewerAttribution`.
    void ChangeViewerPreference(int viewerAttribution);
    int GetViewerPreference();

    // ---- Image ----
    // Replace image `imageNum` (1-based) of page `pageNum` with the PNG /
    // JPEG file `fileName`. Throws std::out_of_range for a bad index.
    void ReplaceImage(int pageNum, int imageNum, const std::string& fileName);
    void DeleteImage(int pageNum, const std::vector<int>& imageNum);
    void DeleteImage();

    // ---- Text replace ----
    bool ReplaceText(const std::string& srcText, const std::string& destText);
    bool ReplaceText(const std::string& srcText, int pageNum,
                     const std::string& destText);
    bool ReplaceText(const std::string& srcText, const std::string& destText,
                     int pageNum);

    // ---- Stamps (by StampId, or by 0-based index on the page) ----
    // An id of 0 means "no id" and matches nothing. Move* puts the stamp's
    // lower-left corner at (x, y).
    void DeleteStamp(int pageNum, const std::vector<int>& stampIndex);
    void DeleteStampByIds(const std::vector<int>& stampIds);
    void DeleteStampByIds(int pageNum, const std::vector<int>& stampIds);
    void DeleteStampById(int pageNum, int stampId);
    void DeleteStampById(int stampId);
    void HideStampById(int pageNum, int stampId);
    void ShowStampById(int pageNum, int stampId);
    void MoveStampById(int pageNum, int stampId, double x, double y);
    // Throws std::out_of_range when the page has no stamp at `stampIndex`.
    void MoveStamp(int pageNum, int stampIndex, double x, double y);
};

}  // namespace Aspose::Pdf::Facades

#ifdef _MSC_VER
#pragma warning(pop)
#endif
