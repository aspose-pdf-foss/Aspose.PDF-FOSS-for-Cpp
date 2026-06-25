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

    // ---- Extract (v1 stubs — return empty Bookmarks) ----

    Bookmarks ExtractBookmarks();
    Bookmarks ExtractBookmarks(bool keepLevels);
    Bookmarks ExtractBookmarks(const std::string& title);
    Bookmarks ExtractBookmarks(const Bookmark& parent);

    // ---- Import / Export (v1 stubs) ----

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
