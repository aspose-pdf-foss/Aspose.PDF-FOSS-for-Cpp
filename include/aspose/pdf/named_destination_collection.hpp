#pragma once

// =============================================================================
// Aspose::Pdf::NamedDestinationCollection — the document's named destinations
// (PDF §12.3.2.3 — the Catalog's /Names /Dests name tree). Mirrors canonical
// Aspose.Pdf.NamedDestinationCollection (Aspose.PDF 26.4.0): a name-keyed map
// of IAppointment navigation targets (typically ExplicitDestination subtypes).
//
// Storage model: ordered (key, IAppointment) entries. The collection OWNS its
// destinations — Add() stores a polymorphic copy of the supplied appointment
// (via IAppointment's internal clone hook), so callers keep ownership of the
// object they pass in. Insertion order is preserved for Names().
//
// Persistence: load-on-open reads the Catalog's /Names /Dests name tree;
// Save() writes it back through the same tree (incremental update).
// =============================================================================

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Aspose::Pdf::Annotations {
class IAppointment;
}  // namespace Aspose::Pdf::Annotations

namespace Aspose::Pdf {

class NamedDestinationCollection {
public:
    // unique_ptr<IAppointment> members hold an incomplete type in this header,
    // so the special members are emitted out-of-line in the .cpp.
    NamedDestinationCollection() noexcept;
    ~NamedDestinationCollection();

    NamedDestinationCollection(const NamedDestinationCollection&) = delete;
    NamedDestinationCollection& operator=(const NamedDestinationCollection&) =
        delete;

    // Canonical NamedDestinationCollection.Add — register `appointment` under
    // `name`. Stores a polymorphic copy; replaces any existing entry for the
    // same name.
    void Add(const std::string& name,
             const Aspose::Pdf::Annotations::IAppointment& appointment);

    // Canonical NamedDestinationCollection.Remove — drop the entry for `name`.
    // No-op when the name is absent.
    void Remove(const std::string& name);

    // Canonical NamedDestinationCollection.Count.
    int Count() const noexcept;

    // Canonical NamedDestinationCollection.Names — the registered names in
    // insertion order.
    std::vector<std::string> Names() const;

    // Canonical NamedDestinationCollection indexer — the destination
    // registered under `name`, or nullptr when absent (non-owning).
    Aspose::Pdf::Annotations::IAppointment* operator[](
        const std::string& name) const;

private:
    struct Entry {
        std::string name;
        std::unique_ptr<Aspose::Pdf::Annotations::IAppointment> destination;
    };

    std::vector<Entry> entries_;

    // Count of entries populated by load-on-open. Lets Save() tell "nothing to
    // persist" from "loaded entries were all removed → clear the tree".
    std::size_t loaded_count_ = 0;

    friend class Document;
};

}  // namespace Aspose::Pdf
