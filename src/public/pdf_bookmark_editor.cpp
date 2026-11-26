#include <aspose/pdf/facades/pdf_bookmark_editor.hpp>

#include "_pending.hpp"

#include <aspose/pdf/document.hpp>

namespace Aspose::Pdf::Facades {

PdfBookmarkEditor::PdfBookmarkEditor(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

// Outline create/extract are REAL — create stages an /Outlines tree on
// the bound document (flushed at Save); extract parses the existing
// /Outlines via foundation::outlines. HTML/XML import/export remain
// stubs.

Aspose::Pdf::Document::OutlineNode PdfBookmarkEditor::ToNode(
        const Bookmark& bm) {
    Aspose::Pdf::Document::OutlineNode n;
    n.title = bm.Title();
    n.page = bm.PageNumber();
    Bookmarks kids = bm.ChildItems();
    for (const auto& child : kids) n.children.push_back(ToNode(child));
    return n;
}

void PdfBookmarkEditor::Restage() {
    if (document_ == nullptr) return;
    std::vector<Aspose::Pdf::Document::OutlineNode> nodes;
    nodes.reserve(staged_.size());
    for (const auto& b : staged_) nodes.push_back(ToNode(b));
    document_->SetStagedOutlines(std::move(nodes));
}

void PdfBookmarkEditor::CreateBookmarks() { Restage(); }
void PdfBookmarkEditor::CreateBookmarks(const Bookmark& bookmark) {
    staged_.push_back(bookmark);
    Restage();
}
void PdfBookmarkEditor::CreateBookmarkOfPage(const std::string& title,
                                             int pageNumber) {
    Bookmark b;
    b.Title(title);
    b.PageNumber(pageNumber);
    staged_.push_back(std::move(b));
    Restage();
}
void PdfBookmarkEditor::CreateBookmarkOfPage(
        const std::vector<std::string>& title,
        const std::vector<int>& pageNumber) {
    for (std::size_t i = 0; i < title.size(); ++i) {
        Bookmark b;
        b.Title(title[i]);
        b.PageNumber(i < pageNumber.size() ? pageNumber[i] : 1);
        staged_.push_back(std::move(b));
    }
    Restage();
}

void PdfBookmarkEditor::DeleteBookmarks() {
    staged_.clear();
    if (document_ != nullptr) document_->ClearStagedOutlines();
}
void PdfBookmarkEditor::DeleteBookmarks(const std::string& title) {
    for (auto it = staged_.begin(); it != staged_.end();) {
        if (it->Title() == title)
            it = staged_.erase(it);
        else
            ++it;
    }
    Restage();
}
void PdfBookmarkEditor::ModifyBookmarks(const std::string& oldTitle,
                                        const std::string& newTitle) {
    for (auto& b : staged_)
        if (b.Title() == oldTitle) b.Title(newTitle);
    Restage();
}

Bookmarks PdfBookmarkEditor::ExtractBookmarks() {
    Bookmarks result;
    if (document_ == nullptr) return result;
    for (const auto& [depth, title] : document_->ParseOutlineItems()) {
        Bookmark b;
        b.Title(title);
        b.Level(depth + 1);
        result.push_back(std::move(b));
    }
    return result;
}
Bookmarks PdfBookmarkEditor::ExtractBookmarks(bool upperLevel) {
    Bookmarks all = ExtractBookmarks();
    if (!upperLevel) return all;
    Bookmarks top;
    for (auto& b : all)
        if (b.Level() == 1) top.push_back(std::move(b));
    return top;
}

namespace {

// The bookmarks (depth-first, with levels) under each one titled `title`;
// `with_root` keeps the titled bookmark itself.
Bookmarks Subtrees(const Bookmarks& all, const std::string& title, int level,
                   bool with_root) {
    Bookmarks out;
    for (std::size_t i = 0; i < all.size(); ++i) {
        if (all[i].Title() != title || (level > 0 && all[i].Level() != level))
            continue;
        if (with_root) out.push_back(all[i]);
        for (std::size_t j = i + 1; j < all.size() && all[j].Level() > all[i].Level(); ++j)
            out.push_back(all[j]);
    }
    return out;
}

}  // namespace

Bookmarks PdfBookmarkEditor::ExtractBookmarks(const std::string& title) {
    return Subtrees(ExtractBookmarks(), title, 0, true);
}
Bookmarks PdfBookmarkEditor::ExtractBookmarks(const Bookmark& parent) {
    return Subtrees(ExtractBookmarks(), parent.Title(), parent.Level(), false);
}

void PdfBookmarkEditor::ExtractBookmarksToHTML(const std::string&,
                                               const std::string&) {
    PENDING("bookmarks-html-export");
}
void PdfBookmarkEditor::ExportBookmarksToHtml(const std::string&,
                                              const std::string&) {
    PENDING("bookmarks-html-export");
}
void PdfBookmarkEditor::ExportBookmarksToXML(const std::string&) {
    PENDING("bookmarks-xml");
}
void PdfBookmarkEditor::ImportBookmarksWithXML(const std::string&) {
    PENDING("bookmarks-xml");
}

}  // namespace Aspose::Pdf::Facades
