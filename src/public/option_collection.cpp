#include <aspose/pdf/forms/option_collection.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace Aspose::Pdf::Forms {

void OptionCollection::Add(Option item) {
    const int new_index = static_cast<int>(items_.size());
    items_.emplace_back(std::move(item));
    items_.back().IndexInternal(new_index);
}

void OptionCollection::Clear() {
    items_.clear();
}

bool OptionCollection::Remove(const Option& item) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
        if (it->Name() == item.Name() && it->Value() == item.Value()) {
            items_.erase(it);
            // Reindex the trailing entries to keep Index() consistent.
            for (std::size_t i = 0; i < items_.size(); ++i) {
                items_[i].IndexInternal(static_cast<int>(i));
            }
            return true;
        }
    }
    return false;
}

bool OptionCollection::Contains(const Option& item) const noexcept {
    for (const auto& e : items_) {
        if (e.Name() == item.Name() && e.Value() == item.Value()) {
            return true;
        }
    }
    return false;
}

int OptionCollection::Count() const noexcept {
    return static_cast<int>(items_.size());
}

bool OptionCollection::IsReadOnly() const noexcept { return false; }

Option& OptionCollection::Get(int index) {
    if (index < 0 ||
        static_cast<std::size_t>(index) >= items_.size()) {
        throw std::out_of_range(
            "Aspose::Pdf::Forms::OptionCollection::Get(int): "
            "index " + std::to_string(index) + " out of range [0.." +
            std::to_string(items_.size()) + ")");
    }
    return items_[static_cast<std::size_t>(index)];
}

Option& OptionCollection::Get(const std::string& name) {
    for (auto& e : items_) {
        if (e.Name() == name) return e;
    }
    throw std::out_of_range(
        "Aspose::Pdf::Forms::OptionCollection::Get(name): "
        "no entry with Name '" + name + "'");
}

Option& OptionCollection::operator[](int index) {
    return Get(index);
}

Option& OptionCollection::operator[](const std::string& name) {
    return Get(name);
}

const Option& OptionCollection::operator[](int index) const {
    return const_cast<OptionCollection*>(this)->Get(index);
}

const Option& OptionCollection::operator[](const std::string& name) const {
    return const_cast<OptionCollection*>(this)->Get(name);
}

}  // namespace Aspose::Pdf::Forms
