#pragma once

// =============================================================================
// flatten_test_support — shared helpers for the flatten smoke tests: temp
// files, a hand-built PDF writer, page /Annots + /Resources inspection
// through the internal object parser, and page rendering to RGBA pixels.
// =============================================================================

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/png_device.hpp>
#include <aspose/pdf/rectangle.hpp>
#include <aspose/pdf/resolution.hpp>

#include "objects.hpp"
#include "pages_tree.hpp"
#include "png_decoder.hpp"
#include "trailer.hpp"

namespace flatten_test {

namespace fo = foundation::objects;

inline std::string HelloWorldPdf() {
    return (std::filesystem::path(__FILE__).parent_path().parent_path() /
            "pdfs" / "hello_world.pdf")
        .string();
}

inline std::string TmpPath(const std::string& tag) {
    return (std::filesystem::temp_directory_path() /
            ("asp_flatten_" + tag + ".pdf"))
        .string();
}

inline Aspose::Pdf::Rectangle Rect(double a, double b, double c, double d) {
    return Aspose::Pdf::Rectangle(a, b, c, d, false);
}

inline std::string ReadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

// A PDF from numbered object bodies (object i+1 = objs[i]); `streams` maps a
// 1-based object number to a stream body, its dict taken from objs.
inline void WritePdf(const std::string& path,
                     const std::vector<std::string>& objs,
                     const std::map<int, std::string>& streams = {}) {
    std::string pdf = "%PDF-1.7\n";
    std::vector<std::size_t> offsets;
    for (std::size_t i = 0; i < objs.size(); ++i) {
        offsets.push_back(pdf.size());
        pdf += std::to_string(i + 1) + " 0 obj\n" + objs[i];
        auto it = streams.find(static_cast<int>(i + 1));
        if (it != streams.end())
            pdf += "\nstream\n" + it->second + "\nendstream";
        pdf += "\nendobj\n";
    }
    const std::size_t xref = pdf.size();
    pdf += "xref\n0 " + std::to_string(objs.size() + 1) +
           "\n0000000000 65535 f \n";
    for (std::size_t off : offsets) {
        char line[21];
        std::snprintf(line, sizeof line, "%010zu 00000 n \n", off);
        pdf += line;
    }
    pdf += "trailer\n<< /Size " + std::to_string(objs.size() + 1) +
           " /Root 1 0 R >>\nstartxref\n" + std::to_string(xref) +
           "\n%%EOF\n";
    std::ofstream out(path, std::ios::binary);
    out.write(pdf.data(), static_cast<std::streamsize>(pdf.size()));
}

inline std::span<const std::byte> Bytes(const std::string& s) {
    return {reinterpret_cast<const std::byte*>(s.data()), s.size()};
}

inline const fo::Value* Get(const fo::Dict& d, const std::string& key) {
    for (const auto& kv : d.entries)
        if (kv.first == key) return &kv.second;
    return nullptr;
}

inline const fo::IndirectObject* Find(const fo::Dump& dump, std::uint32_t id) {
    for (const auto& o : dump.objects)
        if (o.id == id) return &o;
    return nullptr;
}

inline fo::Dict ResolveDict(const fo::Dump& dump, const fo::Value* v) {
    if (v == nullptr) return {};
    if (const auto* d = std::get_if<fo::Dict>(&v->v)) return *d;
    if (const auto* r = std::get_if<fo::Ref>(&v->v)) {
        if (const auto* o = Find(dump, r->id)) {
            if (const auto* d = std::get_if<fo::Dict>(&o->value.v)) return *d;
            if (const auto* s = std::get_if<fo::Stream>(&o->value.v))
                return s->header;
        }
    }
    return {};
}

inline std::string NameOf(const fo::Value* v) {
    if (v == nullptr) return {};
    if (const auto* n = std::get_if<std::string>(&v->v)) return *n;
    return {};
}

struct Parsed {
    fo::Dump dump;
    fo::Dict catalog;
    fo::Dict page;
};

inline Parsed ParsePage(const std::string& bytes, std::size_t page_index) {
    Parsed p;
    const auto span = Bytes(bytes);
    p.dump = fo::Parse(span);
    const auto trailer = foundation::trailer::Parse(span);
    if (const auto* c = Find(p.dump, trailer.root_id))
        if (const auto* d = std::get_if<fo::Dict>(&c->value.v)) p.catalog = *d;
    const auto tree = foundation::pages_tree::Parse(span);
    if (page_index < tree.leaves.size())
        if (const auto* o = Find(p.dump, tree.leaves[page_index].id))
            if (const auto* d = std::get_if<fo::Dict>(&o->value.v)) p.page = *d;
    return p;
}

inline std::vector<fo::Dict> AnnotDicts(const Parsed& p) {
    std::vector<fo::Dict> out;
    const fo::Value* a = Get(p.page, "Annots");
    if (a == nullptr) return out;
    const fo::Array* arr = std::get_if<fo::Array>(&a->v);
    if (arr == nullptr)
        if (const auto* r = std::get_if<fo::Ref>(&a->v))
            if (const auto* o = Find(p.dump, r->id))
                arr = std::get_if<fo::Array>(&o->value.v);
    if (arr == nullptr) return out;
    for (const auto& it : arr->items) out.push_back(ResolveDict(p.dump, &it));
    return out;
}

inline int CountSubtype(const Parsed& p, const std::string& subtype) {
    int n = 0;
    for (const auto& d : AnnotDicts(p))
        if (NameOf(Get(d, "Subtype")) == subtype) ++n;
    return n;
}

inline std::map<std::string, std::uint32_t> XObjects(const Parsed& p) {
    std::map<std::string, std::uint32_t> out;
    const fo::Dict res = ResolveDict(p.dump, Get(p.page, "Resources"));
    const fo::Dict xo = ResolveDict(p.dump, Get(res, "XObject"));
    for (const auto& kv : xo.entries)
        if (const auto* r = std::get_if<fo::Ref>(&kv.second.v))
            out[kv.first] = r->id;
    return out;
}

inline int FlattenedCount(const Parsed& p) {
    int n = 0;
    for (const auto& kv : XObjects(p))
        if (kv.first.rfind("AspFl", 0) == 0) ++n;
    return n;
}

// The /AP /N stream id of the first annotation of `subtype` (0 if none).
inline std::uint32_t FirstApId(const Parsed& p, const std::string& subtype) {
    for (const auto& d : AnnotDicts(p)) {
        if (NameOf(Get(d, "Subtype")) != subtype) continue;
        const fo::Dict ap = ResolveDict(p.dump, Get(d, "AP"));
        if (const auto* n = Get(ap, "N"))
            if (const auto* r = std::get_if<fo::Ref>(&n->v)) return r->id;
    }
    return 0;
}

// Render page `page_number` (1-based) at 72 dpi: 1 px == 1 pt.
inline foundation::png_decoder::DecodedImage Render(Aspose::Pdf::Document& doc,
                                                    int page_number) {
    Aspose::Pdf::Devices::PngDevice png(Aspose::Pdf::Devices::Resolution(72));
    std::stringstream sink(std::ios::binary | std::ios::out | std::ios::in);
    auto page = doc.Pages()[page_number];
    png.Process(page, sink);
    const std::string bytes = sink.str();
    return foundation::png_decoder::Decode(Bytes(bytes));
}

template <class Pred>
int CountPixels(const foundation::png_decoder::DecodedImage& img, double llx,
                double lly, double urx, double ury, double inset, Pred pred) {
    int n = 0;
    const int x0 = static_cast<int>(llx + inset);
    const int x1 = static_cast<int>(urx - inset);
    const int y0 = static_cast<int>(img.height - (ury - inset));
    const int y1 = static_cast<int>(img.height - (lly + inset));
    for (int y = std::max(0, y0); y < std::min<int>(img.height, y1); ++y) {
        for (int x = std::max(0, x0); x < std::min<int>(img.width, x1); ++x) {
            const std::size_t i =
                (static_cast<std::size_t>(y) * img.width + x) * 4;
            if (pred(std::to_integer<int>(img.pixels[i]),
                     std::to_integer<int>(img.pixels[i + 1]),
                     std::to_integer<int>(img.pixels[i + 2])))
                ++n;
        }
    }
    return n;
}

inline int DarkPixels(const foundation::png_decoder::DecodedImage& img,
                      double llx, double lly, double urx, double ury) {
    return CountPixels(img, llx, lly, urx, ury, 3.0,
                       [](int r, int g, int b) { return r + g + b < 384; });
}

inline int RedPixels(const foundation::png_decoder::DecodedImage& img,
                     double llx, double lly, double urx, double ury) {
    return CountPixels(img, llx, lly, urx, ury, 0.0, [](int r, int g, int b) {
        return r > 150 && g < 110 && b < 110;
    });
}

}  // namespace flatten_test
