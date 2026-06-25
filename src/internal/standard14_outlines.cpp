#include "standard14_outlines.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#  include <windows.h>
#endif

#include "standard14_outlines_bundle.inc"

namespace foundation::standard14_outlines {

namespace {

// Per-Standard14 candidate filename list. Probed against each
// search directory; first hit wins. Liberation filenames come
// first because they're the metric-equivalent reference; OS-
// native names follow for convenience on systems where the OS
// already ships them.
struct CandidateList {
    std::string_view base_font_name;
    std::array<std::string_view, 4> filenames;
};

constexpr std::array<CandidateList, 12> kCandidates = {{
    // Helvetica family
    {"Helvetica",             {{"LiberationSans-Regular.ttf",
                                "Arial.ttf",
                                "ArialMT.ttf",
                                "DejaVuSans.ttf"}}},
    {"Helvetica-Bold",        {{"LiberationSans-Bold.ttf",
                                "Arial Bold.ttf",
                                "Arial-Bold.ttf",
                                "DejaVuSans-Bold.ttf"}}},
    {"Helvetica-Oblique",     {{"LiberationSans-Italic.ttf",
                                "Arial Italic.ttf",
                                "Arial-Italic.ttf",
                                "DejaVuSans-Oblique.ttf"}}},
    {"Helvetica-BoldOblique", {{"LiberationSans-BoldItalic.ttf",
                                "Arial Bold Italic.ttf",
                                "Arial-BoldItalic.ttf",
                                "DejaVuSans-BoldOblique.ttf"}}},

    // Times-Roman family
    {"Times-Roman",           {{"LiberationSerif-Regular.ttf",
                                "Times New Roman.ttf",
                                "TimesNewRomanPSMT.ttf",
                                "DejaVuSerif.ttf"}}},
    {"Times-Bold",            {{"LiberationSerif-Bold.ttf",
                                "Times New Roman Bold.ttf",
                                "TimesNewRomanPS-BoldMT.ttf",
                                "DejaVuSerif-Bold.ttf"}}},
    {"Times-Italic",          {{"LiberationSerif-Italic.ttf",
                                "Times New Roman Italic.ttf",
                                "TimesNewRomanPS-ItalicMT.ttf",
                                "DejaVuSerif-Italic.ttf"}}},
    {"Times-BoldItalic",      {{"LiberationSerif-BoldItalic.ttf",
                                "Times New Roman Bold Italic.ttf",
                                "TimesNewRomanPS-BoldItalicMT.ttf",
                                "DejaVuSerif-BoldItalic.ttf"}}},

    // Courier family
    {"Courier",               {{"LiberationMono-Regular.ttf",
                                "Courier New.ttf",
                                "CourierNewPSMT.ttf",
                                "DejaVuSansMono.ttf"}}},
    {"Courier-Bold",          {{"LiberationMono-Bold.ttf",
                                "Courier New Bold.ttf",
                                "CourierNewPS-BoldMT.ttf",
                                "DejaVuSansMono-Bold.ttf"}}},
    {"Courier-Oblique",       {{"LiberationMono-Italic.ttf",
                                "Courier New Italic.ttf",
                                "CourierNewPS-ItalicMT.ttf",
                                "DejaVuSansMono-Oblique.ttf"}}},
    {"Courier-BoldOblique",   {{"LiberationMono-BoldItalic.ttf",
                                "Courier New Bold Italic.ttf",
                                "CourierNewPS-BoldItalicMT.ttf",
                                "DejaVuSansMono-BoldOblique.ttf"}}},
}};

// Read a regular file fully into a byte vector. Returns empty
// on any error (missing, unreadable, locked) — caller falls
// through to the next candidate or the bundle.
std::vector<std::byte> ReadFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    const auto end = f.tellg();
    if (end <= 0) return {};
    std::vector<std::byte> buf(static_cast<std::size_t>(end));
    f.seekg(0, std::ios::beg);
    if (!f.read(reinterpret_cast<char*>(buf.data()),
                static_cast<std::streamsize>(buf.size()))) {
        return {};
    }
    return buf;
}

const CandidateList* LookupCandidates(std::string_view name) {
    for (const auto& c : kCandidates) {
        if (c.base_font_name == name) return &c;
    }
    return nullptr;
}

const BundleEntry* LookupBundle(std::string_view name) {
    for (const auto& e : kBundle) {
        if (e.name == name) return &e;
    }
    return nullptr;
}

#if defined(__APPLE__)
const std::vector<std::string>& BuiltinDefaults() {
    static const std::vector<std::string> dirs = [] {
        std::vector<std::string> out = {
            "/System/Library/Fonts",
            "/Library/Fonts",
        };
        if (const char* home = std::getenv("HOME"); home && *home) {
            out.emplace_back(std::string(home) + "/Library/Fonts");
        }
        return out;
    }();
    return dirs;
}
#elif defined(_WIN32)
const std::vector<std::string>& BuiltinDefaults() {
    static const std::vector<std::string> dirs = [] {
        std::vector<std::string> out;
        char buf[MAX_PATH];
        DWORD n = GetEnvironmentVariableA("WINDIR", buf, MAX_PATH);
        if (n > 0 && n < MAX_PATH) {
            out.emplace_back(std::string(buf) + "\\Fonts");
        } else {
            out.emplace_back("C:\\Windows\\Fonts");
        }
        return out;
    }();
    return dirs;
}
#else  // assume Linux / *BSD
const std::vector<std::string>& BuiltinDefaults() {
    static const std::vector<std::string> dirs = [] {
        std::vector<std::string> out = {
            "/usr/share/fonts",
            "/usr/local/share/fonts",
        };
        if (const char* home = std::getenv("HOME"); home && *home) {
            const std::string h(home);
            out.emplace_back(h + "/.fonts");
            out.emplace_back(h + "/.local/share/fonts");
        }
        return out;
    }();
    return dirs;
}
#endif

}  // namespace

std::vector<std::string> DefaultSearchDirs() {
    return BuiltinDefaults();
}

std::vector<std::byte> Resolve(std::string_view base_font_name,
                               std::span<const std::string> dirs) {
    const auto* cands = LookupCandidates(base_font_name);
    if (cands) {
        // Probe each (dir, candidate-filename) pair. First hit wins.
        // We don't recurse — only the dir's top level is scanned —
        // because Windows / macOS tend to flatten font dirs and
        // Linux dirs often contain a deep tree where exhaustive
        // scan would be slow on every Resolve call.
        for (const auto& dir : dirs) {
            for (const auto& fn : cands->filenames) {
                if (fn.empty()) continue;
                const std::string path = dir + "/" + std::string(fn);
                auto bytes = ReadFile(path);
                if (!bytes.empty()) return bytes;
            }
        }
    }
    // Fall through: bundled Liberation.
    const auto* bundle = LookupBundle(base_font_name);
    if (!bundle) return {};
    std::vector<std::byte> out(bundle->data.size());
    for (std::size_t i = 0; i < bundle->data.size(); ++i) {
        out[i] = static_cast<std::byte>(bundle->data[i]);
    }
    return out;
}

std::vector<std::byte> Resolve(std::string_view base_font_name) {
    return Resolve(base_font_name, DefaultSearchDirs());
}

std::span<const std::uint8_t> Bundled(std::string_view base_font_name) {
    const auto* bundle = LookupBundle(base_font_name);
    if (!bundle) return {};
    return bundle->data;
}

std::string Canonicalize(std::string_view base_font_name,
                         bool prefer_serif) {
    // Drop a subset prefix ("ABCDEF+Arial" → "Arial"): six tag chars
    // plus '+' per PDF §9.6.4, but tolerate any short prefix.
    std::string_view nm = base_font_name;
    if (const auto plus = nm.find('+');
        plus != std::string_view::npos && plus <= 8) {
        nm = nm.substr(plus + 1);
    }
    // Lowercase copy for substring classification.
    std::string low;
    low.reserve(nm.size());
    for (char c : nm) {
        low.push_back((c >= 'A' && c <= 'Z')
                          ? static_cast<char>(c - 'A' + 'a')
                          : c);
    }
    const auto has = [&](std::string_view k) {
        return low.find(k) != std::string::npos;
    };

    const bool bold = has("bold") || has("black") || has("heavy")
                      || has("semibold");
    const bool ital = has("italic") || has("oblique");

    // Family: monospace, then sans (checked before serif so that
    // "sans-serif" classifies as sans), then serif; otherwise the
    // caller-supplied default.
    enum Family { kSans, kSerif, kMono } fam;
    if (has("courier") || has("mono") || has("consol")) {
        fam = kMono;
    } else if (has("arial") || has("helvetica") || has("grotesque")
               || has("sans") || has("verdana") || has("tahoma")
               || has("calibri") || has("segoe") || has("trebuchet")
               || has("gothic") || has("gill") || has("frutiger")
               || has("optima") || has("futura")) {
        fam = kSans;
    } else if (has("times") || has("serif") || has("roman")
               || has("georgia") || has("garamond") || has("minion")
               || has("palatino") || has("century") || has("cambria")
               || has("book")) {
        fam = kSerif;
    } else {
        fam = prefer_serif ? kSerif : kSans;
    }

    switch (fam) {
        case kMono:
            if (bold && ital) return "Courier-BoldOblique";
            if (bold)         return "Courier-Bold";
            if (ital)         return "Courier-Oblique";
            return "Courier";
        case kSerif:
            if (bold && ital) return "Times-BoldItalic";
            if (bold)         return "Times-Bold";
            if (ital)         return "Times-Italic";
            return "Times-Roman";
        default:  // kSans
            if (bold && ital) return "Helvetica-BoldOblique";
            if (bold)         return "Helvetica-Bold";
            if (ital)         return "Helvetica-Oblique";
            return "Helvetica";
    }
}

}  // namespace foundation::standard14_outlines
