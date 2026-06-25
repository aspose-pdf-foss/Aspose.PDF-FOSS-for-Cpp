#pragma once

// =============================================================================
// foundation::standard14_outlines — Standard14 PostScript font name
// to TrueType outline bytes resolver.
//
// PDF 32000-1:2008 §9.6.2.2 mandates that conforming readers MUST
// always have outlines for the 14 standard PostScript fonts —
// these are NEVER embedded in the PDF; the reader is expected to
// supply them. This primitive is that supply.
//
// Resolution order:
//   1. Probe each directory in `dirs` for filename matches against
//      a per-name candidate list (e.g. `/Helvetica` matches
//      `LiberationSans-Regular.ttf`, `Arial.ttf`, `Helvetica.ttf`,
//      `DejaVuSans.ttf`).
//   2. Fall back to the bundled Liberation TTF compiled into the
//      static lib (Liberation 2.1.5, OFL 1.1).
//   3. Empty bytes if the name isn't a known Standard14 name or
//      has no bundle (Symbol, ZapfDingbats — see anchor doc).
//
// See the project spec for the full
// anchor.
// =============================================================================

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace foundation::standard14_outlines {

// Per-OS default font-search directories. Compiled in via
// platform #ifdef. Non-existent paths in the returned list are
// tolerated and silently skipped at probe time.
//
//   * macOS  : /System/Library/Fonts, /Library/Fonts,
//              ~/Library/Fonts
//   * Linux  : /usr/share/fonts, /usr/local/share/fonts,
//              ~/.fonts, ~/.local/share/fonts
//   * Windows: %WINDIR%\Fonts (resolved at runtime)
std::vector<std::string> DefaultSearchDirs();

// Resolve a Standard14 base-font name to TrueType bytes.
// Probes `dirs` first; falls back to the bundled Liberation
// substitute. Returns empty vector if the name isn't known or has
// no bundle entry.
std::vector<std::byte> Resolve(std::string_view base_font_name,
                               std::span<const std::string> dirs);

// Convenience overload using DefaultSearchDirs().
std::vector<std::byte> Resolve(std::string_view base_font_name);

// Returns just the bundled Liberation bytes for a Standard14
// name, or empty span if the name isn't bundled. Doesn't probe
// the filesystem.
std::span<const std::uint8_t> Bundled(std::string_view base_font_name);

// Map an arbitrary PDF /BaseFont name to the nearest of the 12
// resolvable Standard14 names, so a non-embedded font that is not
// itself a Standard14 (e.g. Arial, TrebuchetMS, "ABCDEF+Arial,Bold",
// TimesNewRoman, Calibri) still resolves to a metric-class substitute
// instead of rendering nothing. Strips a subset prefix and style
// suffix, classifies the family (sans / serif / monospace) and the
// bold / italic style from the remaining name tokens, and composes the
// matching Standard14 name (Helvetica / Times-Roman / Courier family).
// A name with no recognised family classifies as serif when
// `prefer_serif` is set (e.g. the /FontDescriptor /Flags serif bit),
// else as the sans-serif default (Helvetica). Always returns one of the
// 12 names that Resolve / Bundled can satisfy.
std::string Canonicalize(std::string_view base_font_name,
                         bool prefer_serif = false);

}  // namespace foundation::standard14_outlines
