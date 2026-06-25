#include <aspose/pdf/facades/bookmark.hpp>

#include <utility>

namespace Aspose::Pdf::Facades {

const std::string& Bookmark::Action() const { return action_; }
void Bookmark::Action(std::string value) { action_ = std::move(value); }

bool Bookmark::BoldFlag() const noexcept { return bold_; }
void Bookmark::BoldFlag(bool value) noexcept { bold_ = value; }

Bookmarks Bookmark::ChildItem() const {
    Bookmarks result;
    result.assign(child_items_.begin(), child_items_.end());
    return result;
}
void Bookmark::ChildItem(const Bookmarks& value) {
    child_items_.assign(value.begin(), value.end());
}
Bookmarks Bookmark::ChildItems() const { return ChildItem(); }
void Bookmark::ChildItems(const Bookmarks& value) { ChildItem(value); }

const std::string& Bookmark::Destination() const { return destination_; }
void Bookmark::Destination(std::string value) {
    destination_ = std::move(value);
}

bool Bookmark::ItalicFlag() const noexcept { return italic_; }
void Bookmark::ItalicFlag(bool value) noexcept { italic_ = value; }

int Bookmark::Level() const noexcept { return level_; }
void Bookmark::Level(int value) noexcept { level_ = value; }

const std::string& Bookmark::PageDisplay() const { return page_display_; }
void Bookmark::PageDisplay(std::string value) {
    page_display_ = std::move(value);
}

int Bookmark::PageDisplay_Bottom() const noexcept { return pd_bottom_; }
void Bookmark::PageDisplay_Bottom(int value) noexcept { pd_bottom_ = value; }
int Bookmark::PageDisplay_Left() const noexcept { return pd_left_; }
void Bookmark::PageDisplay_Left(int value) noexcept { pd_left_ = value; }
int Bookmark::PageDisplay_Right() const noexcept { return pd_right_; }
void Bookmark::PageDisplay_Right(int value) noexcept { pd_right_ = value; }
int Bookmark::PageDisplay_Top() const noexcept { return pd_top_; }
void Bookmark::PageDisplay_Top(int value) noexcept { pd_top_ = value; }
int Bookmark::PageDisplay_Zoom() const noexcept { return pd_zoom_; }
void Bookmark::PageDisplay_Zoom(int value) noexcept { pd_zoom_ = value; }

int Bookmark::PageNumber() const noexcept { return page_number_; }
void Bookmark::PageNumber(int value) noexcept { page_number_ = value; }

const std::string& Bookmark::RemoteFile() const { return remote_file_; }
void Bookmark::RemoteFile(std::string value) {
    remote_file_ = std::move(value);
}

const std::string& Bookmark::Title() const { return title_; }
void Bookmark::Title(std::string value) { title_ = std::move(value); }

bool Bookmark::Open() const noexcept { return open_; }
void Bookmark::Open(bool value) noexcept { open_ = value; }

}  // namespace Aspose::Pdf::Facades
