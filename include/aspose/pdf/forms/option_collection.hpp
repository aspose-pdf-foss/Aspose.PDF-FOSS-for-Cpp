#pragma once

// =============================================================================
// Aspose::Pdf::Forms::OptionCollection — list of Option entries
// owned by a ChoiceField (ListBoxField / ComboBoxField). PDF
// §12.7.4.4 /Opt array. Mirrors canonical
// Aspose.Pdf.Forms.OptionCollection.
//
// Storage model: the collection holds Option values directly.
// Add(item) copies the supplied Option into the collection and
// sets its Index() property to its position. Indexer access is
// 0-based.
//
// Phased drops (matches AnnotationCollection / EmbeddedFileCollection):
//   * CopyTo(Option[], int) — abstract-array semantic
//   * GetEnumerator — IEnumerator translations cascade
//   * IsSynchronized + SyncRoot — System.Collections legacy
// =============================================================================

#include <cstddef>
#include <string>
#include <vector>

#include <aspose/pdf/forms/option.hpp>

namespace Aspose::Pdf::Forms {

class OptionCollection {
public:
    OptionCollection() noexcept = default;

    // ---- Mutation ----

    void Add(Option item);

    void Clear();

    // Removes the option matching by Name + Value identity.
    // Returns true iff a match was found and removed.
    bool Remove(const Option& item);

    // ---- Query ----

    bool Contains(const Option& item) const noexcept;

    int Count() const noexcept;

    bool IsReadOnly() const noexcept;

    // Get accessors — same semantics as the indexers. Both
    // throw std::out_of_range on miss.
    Option& Get(int index);
    Option& Get(const std::string& name);

    Option& operator[](int index);
    Option& operator[](const std::string& name);

    const Option& operator[](int index) const;
    const Option& operator[](const std::string& name) const;

private:
    std::vector<Option> items_;
};

}  // namespace Aspose::Pdf::Forms
