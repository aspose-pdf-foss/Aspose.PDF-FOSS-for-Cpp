#include "standard14_metrics.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string_view>

namespace foundation::standard14_metrics {

namespace {

// Internal record types — the .inc file embeds literal
// initialisers that reference these exact names.
struct GlyphWidth {
    const char* name;
    int width;
};

struct FontEntry {
    const char* name;
    FontDescriptor descriptor;
    const GlyphWidth* widths;
    std::size_t width_count;
};

#include "standard14_metrics_data.inc"

// Linear scan of the 14-entry kFonts table. Binary search would
// be overkill at n=14 and adds a sort-order invariant to the
// .inc generator that isn't worth its upkeep.
const FontEntry* FindFont(std::string_view font_name) noexcept {
    for (std::size_t i = 0; i < kFontCount; ++i) {
        if (font_name == kFonts[i].name) {
            return &kFonts[i];
        }
    }
    return nullptr;
}

// Binary search in the per-font width table. The generator emits
// entries sorted by Unicode codepoint of the key — which matches
// UTF-8 byte-lex order by construction — so std::lower_bound
// over std::string_view comparisons is correct.
int LookupCharWidth(const FontEntry& font,
                    std::string_view utf8_char) noexcept {
    auto it = std::lower_bound(
        font.widths, font.widths + font.width_count, utf8_char,
        [](const GlyphWidth& lhs, std::string_view rhs) {
            return std::string_view(lhs.name) < rhs;
        });
    if (it != font.widths + font.width_count
        && std::string_view(it->name) == utf8_char) {
        return it->width;
    }
    return 0;
}

}  // anonymous namespace

const FontDescriptor* GetDescriptor(
        std::string_view font_name) noexcept {
    const FontEntry* font = FindFont(font_name);
    return font ? &font->descriptor : nullptr;
}

int GetCharWidth(std::string_view font_name,
                 std::string_view utf8_char) noexcept {
    const FontEntry* font = FindFont(font_name);
    if (!font) return 0;
    return LookupCharWidth(*font, utf8_char);
}

std::span<const std::string_view> KnownFontNames() noexcept {
    // Built once, referenced forever. std::string_view over the
    // kFonts[].name C-strings; both are static-storage-duration
    // so the view is safe to return by span.
    static const std::array<std::string_view, 14> kNames = []() {
        std::array<std::string_view, 14> arr{};
        for (std::size_t i = 0; i < kFontCount; ++i) {
            arr[i] = kFonts[i].name;
        }
        return arr;
    }();
    return std::span<const std::string_view>(kNames.data(), kNames.size());
}

}  // namespace foundation::standard14_metrics
