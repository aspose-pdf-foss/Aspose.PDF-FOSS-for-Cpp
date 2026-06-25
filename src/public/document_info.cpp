#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_info.hpp>

#include "objects.hpp"
#include "pages_tree.hpp"

#include <algorithm>
#include <cstdint>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Aspose::Pdf {

namespace {

const std::set<std::string>& predefined_keys() {
    static const std::set<std::string> keys = {
        "Title", "Creator", "Author", "Subject",
        "Keywords", "Producer", "Trapped",
        "CreationDate", "ModDate"
    };
    return keys;
}

std::string DecodePdfString(std::span<const std::byte> bytes) {
    if (bytes.size() >= 2 &&
        static_cast<unsigned char>(bytes[0]) == 0xFE &&
        static_cast<unsigned char>(bytes[1]) == 0xFF) {
        std::string out;
        out.reserve((bytes.size() - 2) / 2);
        for (std::size_t i = 2; i + 1 < bytes.size(); i += 2) {
            std::uint32_t cp =
                (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[i])) << 8) |
                 static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[i + 1]));
            if (cp < 0x80) {
                out.push_back(static_cast<char>(cp));
            } else if (cp < 0x800) {
                out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else {
                out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }
        return out;
    }
    std::string out;
    out.reserve(bytes.size());
    for (auto b : bytes) {
        unsigned char c = static_cast<unsigned char>(b);
        if (c < 0x80) {
            out.push_back(static_cast<char>(c));
        } else {
            out.push_back(static_cast<char>(0xC0 | (c >> 6)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }
    return out;
}

const std::string& EmptyString() {
    static const std::string empty;
    return empty;
}

}  // namespace

void DocumentInfo::EnsureLoaded() const {
    if (doc_->info_loaded_) return;
    doc_->info_loaded_ = true;
    if (doc_->info_id_ == 0) return;
    auto dump = foundation::objects::Parse(
        std::span<const std::byte>(doc_->bytes_.data(), doc_->bytes_.size()));
    for (const auto& obj : dump.objects) {
        if (obj.id != doc_->info_id_) continue;
        if (auto* d = std::get_if<foundation::objects::Dict>(&obj.value.v)) {
            for (const auto& [k, v] : d->entries) {
                if (auto* str = std::get_if<foundation::objects::String>(&v.v)) {
                    doc_->info_entries_.emplace_back(
                        k,
                        DecodePdfString(std::span<const std::byte>(
                            str->bytes.data(), str->bytes.size())));
                }
            }
        }
        break;
    }
}

const std::string& DocumentInfo::operator[](const std::string& key) const {
    EnsureLoaded();
    return Get(key);
}

const std::string& DocumentInfo::Get(const std::string& key) const noexcept {
    auto it = std::find_if(doc_->info_entries_.begin(), doc_->info_entries_.end(),
        [&](const auto& kv) { return kv.first == key; });
    return it == doc_->info_entries_.end() ? EmptyString() : it->second;
}

void DocumentInfo::Set(const std::string& key, std::string value) {
    auto it = std::find_if(doc_->info_entries_.begin(), doc_->info_entries_.end(),
        [&](const auto& kv) { return kv.first == key; });
    if (it == doc_->info_entries_.end()) {
        doc_->info_entries_.emplace_back(key, std::move(value));
    } else {
        it->second = std::move(value);
    }
    doc_->dirty_ = true;
}

DocumentInfo::DocumentInfo(Document& document) noexcept : doc_(&document) {
    EnsureLoaded();
}

void DocumentInfo::Clear() {
    doc_->info_entries_.clear();
    doc_->dirty_ = true;
}

void DocumentInfo::Add(const std::string& key, const std::string& value) {
    auto& entries = doc_->info_entries_;
    if (std::any_of(entries.begin(), entries.end(),
                    [&](const auto& kv) { return kv.first == key; })) {
        throw std::invalid_argument(
            "Aspose::Pdf::DocumentInfo::Add: key '" + key +
            "' already present");
    }
    entries.emplace_back(key, value);
    doc_->dirty_ = true;
}

void DocumentInfo::Remove(const std::string& key) {
    auto& entries = doc_->info_entries_;
    auto it = std::remove_if(entries.begin(), entries.end(),
        [&](const auto& kv) { return kv.first == key; });
    if (it != entries.end()) {
        entries.erase(it, entries.end());
        doc_->dirty_ = true;
    }
}

void DocumentInfo::ClearCustomData() {
    auto& entries = doc_->info_entries_;
    const auto& predef = predefined_keys();
    auto it = std::remove_if(entries.begin(), entries.end(),
        [&](const auto& kv) { return predef.count(kv.first) == 0; });
    if (it != entries.end()) {
        entries.erase(it, entries.end());
        doc_->dirty_ = true;
    }
}

bool DocumentInfo::IsPredefinedKey(const std::string& key) noexcept {
    return predefined_keys().count(key) != 0;
}

const std::string& DocumentInfo::Title()    const noexcept { return Get("Title"); }
void DocumentInfo::Title(std::string value)                 { Set("Title", std::move(value)); }

const std::string& DocumentInfo::Creator()  const noexcept { return Get("Creator"); }
void DocumentInfo::Creator(std::string value)               { Set("Creator", std::move(value)); }

const std::string& DocumentInfo::Author()   const noexcept { return Get("Author"); }
void DocumentInfo::Author(std::string value)                { Set("Author", std::move(value)); }

const std::string& DocumentInfo::Subject()  const noexcept { return Get("Subject"); }
void DocumentInfo::Subject(std::string value)               { Set("Subject", std::move(value)); }

const std::string& DocumentInfo::Keywords() const noexcept { return Get("Keywords"); }
void DocumentInfo::Keywords(std::string value)              { Set("Keywords", std::move(value)); }

const std::string& DocumentInfo::Producer() const noexcept { return Get("Producer"); }
void DocumentInfo::Producer(std::string value)              { Set("Producer", std::move(value)); }

const std::string& DocumentInfo::Trapped()  const noexcept { return Get("Trapped"); }
void DocumentInfo::Trapped(std::string value)               { Set("Trapped", std::move(value)); }

}  // namespace Aspose::Pdf
