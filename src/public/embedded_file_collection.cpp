#include <aspose/pdf/embedded_file_collection.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace Aspose::Pdf {

void EmbeddedFileCollection::Add(FileSpecification file) {
    std::string key = file.Name();
    entries_.emplace_back(std::move(key), std::move(file));
}

void EmbeddedFileCollection::Add(const std::string& key,
                                  FileSpecification file) {
    entries_.emplace_back(key, std::move(file));
}

void EmbeddedFileCollection::DeleteByKey(const std::string& key) {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->first == key) {
            entries_.erase(it);
            return;
        }
    }
}

void EmbeddedFileCollection::Delete(const std::string& name) {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->second.Name() == name) {
            entries_.erase(it);
            return;
        }
    }
}

void EmbeddedFileCollection::Delete() {
    entries_.clear();
}

FileSpecification*
EmbeddedFileCollection::FindByName(const std::string& name) {
    for (auto& e : entries_) {
        if (e.second.Name() == name) {
            return &e.second;
        }
    }
    return nullptr;
}

int EmbeddedFileCollection::Count() const noexcept {
    return static_cast<int>(entries_.size());
}

std::vector<std::string> EmbeddedFileCollection::Keys() const {
    std::vector<std::string> out;
    out.reserve(entries_.size());
    for (const auto& e : entries_) out.push_back(e.first);
    return out;
}

FileSpecification& EmbeddedFileCollection::operator[](int index) {
    if (index < 0 ||
        static_cast<std::size_t>(index) >= entries_.size()) {
        throw std::out_of_range(
            "Aspose::Pdf::EmbeddedFileCollection::operator[]: "
            "index " + std::to_string(index) +
            " out of range [0.." +
            std::to_string(entries_.size()) + ")");
    }
    return entries_[static_cast<std::size_t>(index)].second;
}

FileSpecification&
EmbeddedFileCollection::operator[](const std::string& name) {
    auto* found = FindByName(name);
    if (found == nullptr) {
        throw std::out_of_range(
            "Aspose::Pdf::EmbeddedFileCollection::operator[]: "
            "no entry with Name '" + name + "'");
    }
    return *found;
}

const FileSpecification&
EmbeddedFileCollection::operator[](int index) const {
    return const_cast<EmbeddedFileCollection*>(this)
        ->operator[](index);
}

const FileSpecification&
EmbeddedFileCollection::operator[](const std::string& name) const {
    return const_cast<EmbeddedFileCollection*>(this)
        ->operator[](name);
}

}  // namespace Aspose::Pdf
