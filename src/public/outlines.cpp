// Bodies for the document-outline tree: Outlines (abstract base owning the
// child nodes), OutlineCollection (the /Outlines root) and
// OutlineItemCollection (a node + collection of children). Mirrors canonical
// Aspose.Pdf.Outlines / OutlineCollection / OutlineItemCollection.

#include <aspose/pdf/outline_collection.hpp>
#include <aspose/pdf/outline_item_collection.hpp>
#include <aspose/pdf/outlines.hpp>

#include <aspose/pdf/annotations/i_appointment.hpp>
#include <aspose/pdf/annotations/pdf_action.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Aspose::Pdf {

// ---- Outlines (base) -------------------------------------------------------

Outlines::Outlines() = default;
Outlines::~Outlines() = default;

int Outlines::Count() const noexcept {
    return static_cast<int>(children_.size());
}

bool Outlines::IsReadOnly() const noexcept { return false; }

int Outlines::VisibleCount() const noexcept {
    int total = 0;
    for (const auto& c : children_) {
        ++total;
        if (c->Open()) total += c->VisibleCount();
    }
    return total;
}

void Outlines::Adopt(OutlineItemCollection& child) { child.parent_ = this; }

void Outlines::Add(OutlineItemCollection& item) {
    auto node = OutlineItemCollection::DeepClone(item);
    node->parent_ = this;
    children_.push_back(std::move(node));
}

void Outlines::Clear() noexcept { children_.clear(); }

bool Outlines::Contains(const OutlineItemCollection& item) const noexcept {
    for (const auto& c : children_) {
        if (c->Title() == item.Title()) return true;
    }
    return false;
}

bool Outlines::Remove(OutlineItemCollection& item) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if ((*it)->Title() == item.Title()) {
            children_.erase(it);
            return true;
        }
    }
    return false;
}

void Outlines::Remove(int index) {
    if (index < 1 || static_cast<std::size_t>(index) > children_.size()) {
        throw std::out_of_range("Outlines::Remove: index out of range");
    }
    children_.erase(children_.begin() + (index - 1));
}

void Outlines::Delete() noexcept { children_.clear(); }

void Outlines::Delete(const std::string& name) {
    for (auto it = children_.begin(); it != children_.end(); ++it) {
        if ((*it)->Title() == name) {
            children_.erase(it);
            return;
        }
    }
}

OutlineItemCollection* Outlines::First() const noexcept {
    return children_.empty() ? nullptr : children_.front().get();
}

OutlineItemCollection* Outlines::Last() const noexcept {
    return children_.empty() ? nullptr : children_.back().get();
}

OutlineItemCollection& Outlines::operator[](int index) const {
    if (index < 1 || static_cast<std::size_t>(index) > children_.size()) {
        throw std::out_of_range("Outlines::operator[]: index out of range");
    }
    return *children_[static_cast<std::size_t>(index) - 1];
}

// ---- OutlineCollection -----------------------------------------------------

OutlineCollection::OutlineCollection() = default;
OutlineCollection::~OutlineCollection() = default;

// ---- OutlineItemCollection -------------------------------------------------

OutlineItemCollection::OutlineItemCollection(OutlineCollection& /*outlines*/) {}
OutlineItemCollection::OutlineItemCollection() = default;
OutlineItemCollection::~OutlineItemCollection() = default;

std::unique_ptr<OutlineItemCollection> OutlineItemCollection::DeepClone(
        const OutlineItemCollection& src) {
    auto n = std::unique_ptr<OutlineItemCollection>(new OutlineItemCollection());
    n->title_ = src.title_;
    n->bold_ = src.bold_;
    n->italic_ = src.italic_;
    n->open_ = src.open_;
    if (src.destination_) n->destination_ = src.destination_->CloneAppointment();
    if (src.action_) n->action_ = src.action_->CloneAction();
    for (const auto& c : src.children_) {
        auto cc = DeepClone(*c);
        cc->parent_ = n.get();
        n->children_.push_back(std::move(cc));
    }
    return n;
}

const std::string& OutlineItemCollection::Title() const noexcept {
    return title_;
}
void OutlineItemCollection::Title(std::string value) {
    title_ = std::move(value);
}

bool OutlineItemCollection::Bold() const noexcept { return bold_; }
void OutlineItemCollection::Bold(bool value) { bold_ = value; }

bool OutlineItemCollection::Italic() const noexcept { return italic_; }
void OutlineItemCollection::Italic(bool value) { italic_ = value; }

bool OutlineItemCollection::Open() const noexcept { return open_; }
void OutlineItemCollection::Open(bool value) { open_ = value; }

Aspose::Pdf::Annotations::IAppointment* OutlineItemCollection::Destination()
        const noexcept {
    return destination_.get();
}

void OutlineItemCollection::Destination(
        const Aspose::Pdf::Annotations::IAppointment& value) {
    destination_ = value.CloneAppointment();
    action_.reset();  // a node targets a Destination or an Action, not both
}

Aspose::Pdf::Annotations::PdfAction* OutlineItemCollection::Action()
        const noexcept {
    return action_.get();
}

void OutlineItemCollection::Action(
        const Aspose::Pdf::Annotations::PdfAction& value) {
    action_ = value.CloneAction();
    destination_.reset();
}

void OutlineItemCollection::Insert(int index, OutlineItemCollection& item) {
    if (index < 1 || static_cast<std::size_t>(index) > children_.size() + 1) {
        throw std::out_of_range("OutlineItemCollection::Insert: index");
    }
    auto node = DeepClone(item);
    node->parent_ = this;
    children_.insert(children_.begin() + (index - 1), std::move(node));
}

OutlineItemCollection* OutlineItemCollection::Next() const noexcept {
    if (parent_ == nullptr) return nullptr;
    const auto& sib = parent_->children_;
    for (std::size_t i = 0; i < sib.size(); ++i) {
        if (sib[i].get() == this) {
            return (i + 1 < sib.size()) ? sib[i + 1].get() : nullptr;
        }
    }
    return nullptr;
}

OutlineItemCollection* OutlineItemCollection::Prev() const noexcept {
    if (parent_ == nullptr) return nullptr;
    const auto& sib = parent_->children_;
    for (std::size_t i = 0; i < sib.size(); ++i) {
        if (sib[i].get() == this) {
            return (i > 0) ? sib[i - 1].get() : nullptr;
        }
    }
    return nullptr;
}

bool OutlineItemCollection::HasNext() const noexcept {
    return Next() != nullptr;
}

Outlines* OutlineItemCollection::Parent() const noexcept { return parent_; }

int OutlineItemCollection::Level() const noexcept {
    int level = 0;
    const Outlines* p = parent_;
    while (p != nullptr) {
        ++level;
        p = dynamic_cast<const OutlineItemCollection*>(p) != nullptr
                ? static_cast<const OutlineItemCollection*>(p)->parent_
                : nullptr;
    }
    return level;
}

}  // namespace Aspose::Pdf
