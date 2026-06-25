#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "objects.hpp"

namespace foundation::pdf_writer_incremental {

// One indirect object to replace (or add) in the incremental
// section. (id, generation) identifies the slot; `value` is
// the new body.
struct DirtyObject {
    std::uint32_t id;
    std::uint16_t generation;
    foundation::objects::Value value;
};

// v2 extension — trailer overlay for encryption-aware writes.
//
// The caller pre-encrypts strings + stream bodies in the dirty
// list (via foundation::crypt_filter) before passing them to
// the writer; this options struct only injects the trailer-side
// surface required to make the saved bytes parse as a valid
// encrypted PDF:
//
//   * has_encrypt_ref — when true, the new trailer dict gains
//                       `/Encrypt <encrypt_id> <encrypt_gen> R`
//                       pointing at the /Encrypt indirect
//                       object. The caller is responsible for
//                       including the /Encrypt dict itself in
//                       the `dirty` list AND for leaving its
//                       contents PLAINTEXT (PDF §7.6.2 forbids
//                       encrypting the /Encrypt dictionary
//                       itself).
//
//   * has_file_id     — when true, the new trailer dict's /ID
//                       array is replaced with two byte strings
//                       file_id_first and file_id_second.
//                       Encryption requires a stable /ID; the
//                       same 16 bytes that were passed to
//                       encrypt_writer as `file_id` must appear
//                       here as file_id_first. When false, /ID
//                       is copied verbatim from the original
//                       trailer.
struct AppendOptions {
    bool has_encrypt_ref = false;
    std::uint32_t encrypt_id = 0;
    std::uint16_t encrypt_gen = 0;

    bool has_file_id = false;
    std::vector<std::byte> file_id_first;
    std::vector<std::byte> file_id_second;
};

// Append an incremental update section to `original`. See
// spec yaml `intent` for the full algorithm.
//
// v1:   non-stream Values only.
// v1.1: Stream variant allowed as top-level DirtyObject.value.
// v2 (this overload): when `options.has_encrypt_ref` and/or
// `options.has_file_id` are set, the new trailer dict gains
// the corresponding entries (see `AppendOptions` above for
// the contract).
std::vector<std::byte> AppendIncremental(
    std::span<const std::byte> original,
    std::span<const DirtyObject> dirty,
    const AppendOptions& options);

// v1.1 compatibility overload — equivalent to passing a
// default-constructed AppendOptions (no trailer overlay).
std::vector<std::byte> AppendIncremental(
    std::span<const std::byte> original,
    std::span<const DirtyObject> dirty);

}  // namespace foundation::pdf_writer_incremental
