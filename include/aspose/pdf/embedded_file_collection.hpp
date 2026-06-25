#pragma once

// =============================================================================
// Aspose::Pdf::EmbeddedFileCollection — document-level embedded
// file attachments (PDF §7.11.4 — Embedded File Streams via the
// Catalog's /Names /EmbeddedFiles name tree). Mirrors canonical
// Aspose.Pdf.EmbeddedFileCollection (Aspose.PDF 26.4.0).
//
// Storage model: each entry is a (key, FileSpecification) pair.
// The collection holds FileSpecifications by value (copy on Add).
// The `key` is the PDF name tree key (canonical /Names entry);
// when Add(file) is called without an explicit key, the key
// defaults to the FileSpecification's Name (typically the file
// path basename).
//
// Phased drops on this beat (drops.yaml):
//   * CopyTo(FileSpecification[], int) — abstract-array semantic
//   * IsSynchronized + SyncRoot — System.Collections legacy
//   * GetEnumerator — auto-drops via System.Collections.Generic
//     .IEnumerator translations cascade
// =============================================================================

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <aspose/pdf/file_specification.hpp>

namespace Aspose::Pdf {

class EmbeddedFileCollection {
public:
    EmbeddedFileCollection() noexcept = default;

    // ---- Mutation ----

    // Add a FileSpecification using its Name as the name-tree key.
    void Add(FileSpecification file);

    // Add a FileSpecification under an explicit name-tree key.
    void Add(const std::string& key, FileSpecification file);

    // Remove the entry whose name-tree key matches `key`. No-op
    // when the key is absent.
    void DeleteByKey(const std::string& key);

    // Remove the entry whose FileSpecification.Name() matches
    // `name`. No-op when absent.
    void Delete(const std::string& name);

    // Remove all entries.
    void Delete();

    // ---- Query ----

    // Find the FileSpecification whose Name() matches `name`.
    // Returns nullptr when none matches (canonical .NET returns
    // null; raw pointer is the closest cpp idiom for a nullable
    // reference).
    FileSpecification* FindByName(const std::string& name);

    int Count() const noexcept;

    // The list of name-tree keys, preserving Add order.
    std::vector<std::string> Keys() const;

    // Indexers — both throw std::out_of_range on miss.
    FileSpecification& operator[](int index);
    FileSpecification& operator[](const std::string& name);

    // Same indexers in const form (return const ref).
    const FileSpecification& operator[](int index) const;
    const FileSpecification& operator[](const std::string& name) const;

private:
    // Document populates entries_ on load-on-open (Document::EmbeddedFiles())
    // and tracks how many were loaded so Save can detect a delete-all
    // (Count drops to 0 while loaded_count_ > 0) and clear the name tree.
    friend class Aspose::Pdf::Document;
    std::size_t loaded_count_ = 0;

    std::vector<std::pair<std::string, FileSpecification>> entries_;
};

}  // namespace Aspose::Pdf
