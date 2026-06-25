#pragma once

// =============================================================================
// PENDING("<capability-slug>") — marks a method body whose real
// implementation is blocked on a capability that hasn't shipped yet.
//
// Behaviour:
//   * Throws std::logic_error with a self-describing message naming the
//     blocking capability slug, so callers see exactly what's missing.
//   * Greppable — a simple scan of src/public/ for occurrences of
//     PENDING("...") produces the prioritised backlog.
//
// Convention:
//   The slug argument names a capability. When that capability is
//   implemented, callers of this method can flip
//   the body from PENDING(...) to a real implementation.
//
// This header is private to the lib's public-API translation units —
// users of the lib never see it. The user-facing exception they catch
// is std::logic_error.
// =============================================================================

#include <stdexcept>
#include <string>

#define PENDING(slug)                                                       \
    do {                                                                    \
        throw std::logic_error(                                             \
            std::string(                                                    \
                "Aspose::Pdf: not implemented — blocked on capability '")   \
            + (slug) + "'");                                                \
    } while (false)
