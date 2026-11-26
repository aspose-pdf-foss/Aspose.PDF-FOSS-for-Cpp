#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PdfBookmarkEditor — outline (bookmark) editor
// for the bound PDF (create / extract / delete / modify / import /
// export). Mirrors canonical Aspose.Pdf.Facades.PdfBookmarkEditor;
// extends SaveableFacade.
//
// v1 baseline: every operation is a stub (Extract* returns an empty
// Bookmarks; the rest are no-ops). The document outline (/Outlines)
// read/write path is not yet in the foundation, so the actual bookmark
// tree manipulation is deferred to the beat that lands outline support.
// This beat ships the full public surface + the Bookmark / Bookmarks
// value types.
//
// Phased drops:
//   * CreateBookmarks(Color, bool, bool) — System.Drawing.Color cascade.
//   * Export/Import *(Stream) overloads — Stream cascade.
// =============================================================================

#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/bookmark.hpp>
#include <aspose/pdf/facades/saveable_facade.hpp>

// MSVC C4250: BindPdf/Close are inherited from Facade by dominance over
// the virtual IFacade base also reached via ISaveableFacade — intended.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

namespace Aspose::Pdf::Facades {

class PdfBookmarkEditor : public SaveableFacade {
public:
    PdfBookmarkEditor() noexcept = default;
    explicit PdfBookmarkEditor(Aspose::Pdf::Document& document);

    // ---- Create (v1 stubs) ----

    void CreateBookmarks();
    void CreateBookmarks(const Bookmark& bookmark);
    void CreateBookmarkOfPage(const std::string& title, int pageNumber);
    void CreateBookmarkOfPage(const std::vector<std::string>& title,
                              const std::vector<int>& pageNumber);

    // ---- Delete / Modify (v1 stubs) ----

    void DeleteBookmarks();
    void DeleteBookmarks(const std::string& title);
    void ModifyBookmarks(const std::string& oldTitle,
                         const std::string& newTitle);

    // ---- Extract: a flat depth-first list, Level() 1 = top ----

    Bookmarks ExtractBookmarks();
    // true: only the top-level bookmarks; false: all of them.
    Bookmarks ExtractBookmarks(bool upperLevel);
    // Every bookmark titled `title` with the bookmarks under it.
    Bookmarks ExtractBookmarks(const std::string& title);
    // The bookmarks under `parent` (matched by title, and by level when set).
    Bookmarks ExtractBookmarks(const Bookmark& parent);

    // ---- Import / Export (not implemented: throw std::logic_error) ----

    void ExtractBookmarksToHTML(const std::string& dataDir,
                                const std::string& outputFile);
    void ExportBookmarksToHtml(const std::string& dataDir,
                               const std::string& outputFile);
    void ExportBookmarksToXML(const std::string& outputFile);
    void ImportBookmarksWithXML(const std::string& xmlFile);

private:
    // Push the accumulated bookmarks onto the bound document's staged
    // /Outlines tree (flushed at Save).
    void Restage();
    static Aspose::Pdf::Document::OutlineNode ToNode(const Bookmark& bm);

    std::vector<Bookmark> staged_;
};

}  // namespace Aspose::Pdf::Facades

#ifdef _MSC_VER
#pragma warning(pop)
#endif
