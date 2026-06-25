#pragma once

// =============================================================================
// Aspose::Pdf::Facades::Bookmark + Bookmarks — outline-entry value type
// and its collection. Mirror canonical Aspose.Pdf.Facades.Bookmark and
// Aspose.Pdf.Facades.Bookmarks (the latter extends List<Bookmark>).
//
// Bookmark is a pure data carrier consumed by PdfBookmarkEditor: title,
// target page + page-display geometry, nested children, and the
// bold/italic/open display flags.
//
// Phased drops:
//   * TitleColor (System.Drawing.Color) — Color cascade.
//   * CustomAcorbatViewerMenuActionName (System.Enum[]) — untyped enum
//     array, non-idiomatic (drops.yaml).
// =============================================================================

#include <string>
#include <vector>

namespace Aspose::Pdf::Facades {

class Bookmarks;  // defined below, after Bookmark

class Bookmark {
public:
    Bookmark() = default;

    const std::string& Action() const;
    void Action(std::string value);

    bool BoldFlag() const noexcept;
    void BoldFlag(bool value) noexcept;

    // Nested child outline entries. ChildItem / ChildItems are
    // canonical aliases over the same backing collection.
    Bookmarks ChildItem() const;
    void ChildItem(const Bookmarks& value);
    Bookmarks ChildItems() const;
    void ChildItems(const Bookmarks& value);

    const std::string& Destination() const;
    void Destination(std::string value);

    bool ItalicFlag() const noexcept;
    void ItalicFlag(bool value) noexcept;

    int Level() const noexcept;
    void Level(int value) noexcept;

    const std::string& PageDisplay() const;
    void PageDisplay(std::string value);

    int PageDisplay_Bottom() const noexcept;
    void PageDisplay_Bottom(int value) noexcept;
    int PageDisplay_Left() const noexcept;
    void PageDisplay_Left(int value) noexcept;
    int PageDisplay_Right() const noexcept;
    void PageDisplay_Right(int value) noexcept;
    int PageDisplay_Top() const noexcept;
    void PageDisplay_Top(int value) noexcept;
    int PageDisplay_Zoom() const noexcept;
    void PageDisplay_Zoom(int value) noexcept;

    int PageNumber() const noexcept;
    void PageNumber(int value) noexcept;

    const std::string& RemoteFile() const;
    void RemoteFile(std::string value);

    const std::string& Title() const;
    void Title(std::string value);

    bool Open() const noexcept;
    void Open(bool value) noexcept;

private:
    std::string action_;
    std::string destination_;
    std::string page_display_;
    std::string remote_file_;
    std::string title_;
    bool bold_ = false;
    bool italic_ = false;
    bool open_ = false;
    int level_ = 1;
    int page_number_ = 1;
    int pd_bottom_ = 0;
    int pd_left_ = 0;
    int pd_right_ = 0;
    int pd_top_ = 0;
    int pd_zoom_ = 0;
    std::vector<Bookmark> child_items_;
};

// Bookmark collection — canonical extends List<Bookmark>; the C++
// idiom derives from std::vector<Bookmark>.
class Bookmarks : public std::vector<Bookmark> {
public:
    Bookmarks() = default;
};

}  // namespace Aspose::Pdf::Facades
