// Bodies for the image-resource surface: XImage, XImageCollection, Resources.

#include <aspose/pdf/resources.hpp>
#include <aspose/pdf/x_image.hpp>
#include <aspose/pdf/x_image_collection.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Aspose::Pdf {

// ---- XImage ----------------------------------------------------------------

int XImage::Width() const noexcept { return width_; }
int XImage::Height() const noexcept { return height_; }
const std::string& XImage::Name() const noexcept { return name_; }
void XImage::Name(std::string value) { name_ = std::move(value); }
bool XImage::ContainsTransparency() const noexcept {
    return contains_transparency_;
}
bool XImage::ImageMask() const noexcept { return image_mask_; }
std::vector<std::byte> XImage::GetRawImageData() const { return data_; }

// ---- XImageCollection ------------------------------------------------------

XImageCollection::XImageCollection() = default;
XImageCollection::~XImageCollection() = default;

std::string XImageCollection::Add(const std::vector<std::byte>& image) {
    // Pick a resource name not already taken on the page (loaded names + the
    // names already staged in this collection). The XObject itself is written
    // at Document::Save (Document::AppendImagesUpdate consumes is_new_ + data_).
    auto taken = [&](const std::string& n) {
        for (const auto& nm : loaded_names_)
            if (nm == n) return true;
        for (const auto& im : images_)
            if (im->Name() == n) return true;
        return false;
    };
    std::string name;
    for (int i = static_cast<int>(images_.size()) + 1;; ++i) {
        name = "XImg" + std::to_string(i);
        if (!taken(name)) break;
    }

    auto img = std::make_unique<XImage>();
    img->name_ = name;
    img->data_ = image;
    img->is_new_ = true;
    images_.push_back(std::move(img));
    return name;
}

void XImageCollection::Replace(int index, const std::vector<std::byte>& image) {
    if (index < 1 || static_cast<std::size_t>(index) > images_.size())
        throw std::out_of_range("XImageCollection::Replace: index");
    XImage& im = *images_[static_cast<std::size_t>(index) - 1];
    im.data_ = image;
    if (!im.is_new_)
        im.is_replaced_ = true;  // loaded image: re-embed under the same name
    // (a not-yet-saved Add just gets its pending source bytes updated)
}

int XImageCollection::Count() const noexcept {
    return static_cast<int>(images_.size());
}

std::vector<std::string> XImageCollection::Names() const {
    std::vector<std::string> names;
    names.reserve(images_.size());
    for (const auto& im : images_) names.push_back(im->Name());
    return names;
}

XImage& XImageCollection::operator[](int index) const {
    if (index < 1 || static_cast<std::size_t>(index) > images_.size())
        throw std::out_of_range("XImageCollection::operator[]: index");
    return *images_[static_cast<std::size_t>(index) - 1];
}

XImage& XImageCollection::operator[](const std::string& name) const {
    for (const auto& im : images_)
        if (im->Name() == name) return *im;
    throw std::out_of_range("XImageCollection::operator[]: name not found");
}

void XImageCollection::Delete(int index) {
    if (index < 1 || static_cast<std::size_t>(index) > images_.size())
        throw std::out_of_range("XImageCollection::Delete: index");
    images_.erase(images_.begin() + (index - 1));
}

void XImageCollection::Delete(const std::string& name) {
    for (auto it = images_.begin(); it != images_.end(); ++it) {
        if ((*it)->Name() == name) {
            images_.erase(it);
            return;
        }
    }
}

void XImageCollection::Delete() { images_.clear(); }
void XImageCollection::Clear() noexcept { images_.clear(); }

bool XImageCollection::Contains(const XImage& image) const noexcept {
    for (const auto& im : images_)
        if (im.get() == &image) return true;
    return false;
}

bool XImageCollection::Remove(const XImage& image) {
    for (auto it = images_.begin(); it != images_.end(); ++it) {
        if (it->get() == &image) {
            images_.erase(it);
            return true;
        }
    }
    return false;
}

std::string XImageCollection::GetImageName(const XImage& image) const {
    for (const auto& im : images_)
        if (im.get() == &image) return im->Name();
    return std::string();
}

// ---- Resources -------------------------------------------------------------

Resources::Resources() = default;
Resources::~Resources() = default;

XImageCollection& Resources::Images() noexcept { return images_; }

}  // namespace Aspose::Pdf
