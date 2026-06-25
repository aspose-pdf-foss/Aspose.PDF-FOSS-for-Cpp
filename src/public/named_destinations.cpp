// Bodies for the named-destination surface: NamedDestination (a name-keyed
// IAppointment) and NamedDestinationCollection (the document's /Names /Dests
// map). Mirrors canonical Aspose.Pdf.Annotations.NamedDestination +
// Aspose.Pdf.NamedDestinationCollection.

#include <aspose/pdf/named_destination_collection.hpp>

#include <aspose/pdf/annotations/i_appointment.hpp>
#include <aspose/pdf/annotations/named_destination.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Aspose::Pdf::Annotations {

// ---- NamedDestination ------------------------------------------------------

NamedDestination::NamedDestination(Aspose::Pdf::Document& doc, std::string name)
    : doc_(&doc), name_(std::move(name)) {}

const std::string& NamedDestination::Name() const noexcept { return name_; }

std::string NamedDestination::ToString() const { return name_; }

std::unique_ptr<IAppointment> NamedDestination::CloneAppointment() const {
    return std::unique_ptr<IAppointment>(new NamedDestination(*doc_, name_));
}

}  // namespace Aspose::Pdf::Annotations

namespace Aspose::Pdf {

// ---- NamedDestinationCollection --------------------------------------------

NamedDestinationCollection::NamedDestinationCollection() noexcept = default;
NamedDestinationCollection::~NamedDestinationCollection() = default;

void NamedDestinationCollection::Add(
    const std::string& name,
    const Aspose::Pdf::Annotations::IAppointment& appointment) {
    auto clone = appointment.CloneAppointment();
    for (auto& e : entries_) {
        if (e.name == name) {
            e.destination = std::move(clone);
            return;
        }
    }
    entries_.push_back(Entry{name, std::move(clone)});
}

void NamedDestinationCollection::Remove(const std::string& name) {
    for (auto it = entries_.begin(); it != entries_.end(); ++it) {
        if (it->name == name) {
            entries_.erase(it);
            return;
        }
    }
}

int NamedDestinationCollection::Count() const noexcept {
    return static_cast<int>(entries_.size());
}

std::vector<std::string> NamedDestinationCollection::Names() const {
    std::vector<std::string> names;
    names.reserve(entries_.size());
    for (const auto& e : entries_) names.push_back(e.name);
    return names;
}

Aspose::Pdf::Annotations::IAppointment* NamedDestinationCollection::operator[](
    const std::string& name) const {
    for (const auto& e : entries_) {
        if (e.name == name) return e.destination.get();
    }
    return nullptr;
}

}  // namespace Aspose::Pdf
