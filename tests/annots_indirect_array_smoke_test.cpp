// =============================================================================
// annots_indirect_array_smoke_test — a page whose /Annots is an indirect
// reference to an array (`/Annots 4 0 R`, allowed by PDF §7.7.3.3) must be
// read, and kept, exactly like an inline /Annots array by every path that
// reads or rewrites it: annotation load / save, form widget binding and
// signing.
// =============================================================================

#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/square_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_file_signature.hpp>
#include <aspose/pdf/forms/form.hpp>
#include <aspose/pdf/forms/pkcs7.hpp>
#include <aspose/pdf/forms/text_box_field.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>

#include "objects.hpp"
#include "pages_tree.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
namespace fo = foundation::objects;

std::string TmpPath(const std::string& tag) {
    return (std::filesystem::temp_directory_path() /
            ("asp_annots_ref_" + tag + ".pdf"))
        .string();
}

std::string ReadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

// A one-page PDF whose page carries `/Annots 4 0 R` -> [5 0 R], object 5
// being a Square annotation.
void WriteIndirectAnnotsPdf(const std::string& path) {
    const std::vector<std::string> objs = {
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
        "/Annots 4 0 R >>",
        "[5 0 R]",
        "<< /Type /Annot /Subtype /Square /Rect [50 50 90 90] "
        "/C [1 0 0] >>",
    };
    std::string pdf = "%PDF-1.7\n";
    std::vector<std::size_t> offsets;
    for (std::size_t i = 0; i < objs.size(); ++i) {
        offsets.push_back(pdf.size());
        pdf += std::to_string(i + 1) + " 0 obj\n" + objs[i] + "\nendobj\n";
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

// Count the annotations of `subtype` on page 1 of the file at `path`,
// resolving an indirect /Annots array and indirect entries.
int CountSubtype(const std::string& path, const std::string& subtype) {
    const std::string bytes = ReadFile(path);
    const std::span<const std::byte> span(
        reinterpret_cast<const std::byte*>(bytes.data()), bytes.size());
    const fo::Dump dump = fo::Parse(span);
    const auto tree = foundation::pages_tree::Parse(span);
    auto find = [&](std::uint32_t id) -> const fo::Value* {
        for (const auto& o : dump.objects)
            if (o.id == id) return &o.value;
        return nullptr;
    };
    auto resolve = [&](const fo::Value* v) -> const fo::Value* {
        if (v == nullptr) return nullptr;
        if (const auto* r = std::get_if<fo::Ref>(&v->v)) return find(r->id);
        return v;
    };
    auto get = [](const fo::Dict& d, const std::string& k) -> const fo::Value* {
        for (const auto& kv : d.entries)
            if (kv.first == k) return &kv.second;
        return nullptr;
    };
    const auto* page = std::get_if<fo::Dict>(&find(tree.leaves[0].id)->v);
    const auto* av = resolve(get(*page, "Annots"));
    const auto* arr = av ? std::get_if<fo::Array>(&av->v) : nullptr;
    if (arr == nullptr) return 0;
    int n = 0;
    for (const auto& item : arr->items) {
        const auto* dv = resolve(&item);
        const auto* d = dv ? std::get_if<fo::Dict>(&dv->v) : nullptr;
        if (d == nullptr) continue;
        const auto* st = get(*d, "Subtype");
        const auto* name = st ? std::get_if<std::string>(&st->v) : nullptr;
        if (name != nullptr && *name == subtype) ++n;
    }
    return n;
}

}  // namespace

TEST(AnnotsIndirectArray, LoadReadsIndirectAnnots) {
    const std::string src = TmpPath("load");
    WriteIndirectAnnotsPdf(src);
    ASSERT_EQ(CountSubtype(src, "Square"), 1);
    Document doc{src};
    EXPECT_EQ(doc.Pages()[1].Annotations().Count(), 1);
    std::filesystem::remove(src);
}

TEST(AnnotsIndirectArray, AddingAnnotationKeepsExisting) {
    const std::string src = TmpPath("add_src");
    const std::string out = TmpPath("add_out");
    WriteIndirectAnnotsPdf(src);
    {
        Document doc{src};
        auto page = doc.Pages()[1];
        Annotations::SquareAnnotation sq{
            page, Rectangle(200, 200, 260, 260, false)};
        page.Annotations().Add(sq);
        doc.Save(out);
    }
    EXPECT_EQ(CountSubtype(out, "Square"), 2);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(AnnotsIndirectArray, AddingFormFieldKeepsExisting) {
    const std::string src = TmpPath("field_src");
    const std::string out = TmpPath("field_out");
    WriteIndirectAnnotsPdf(src);
    {
        Document doc{src};
        Forms::TextBoxField tb{doc, Rectangle(100, 400, 300, 420, false)};
        tb.Value("x");
        doc.Form().Add(tb, "name", 1);
        doc.Save(out);
    }
    EXPECT_EQ(CountSubtype(out, "Square"), 1);
    EXPECT_EQ(CountSubtype(out, "Widget"), 1);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}

TEST(AnnotsIndirectArray, SigningKeepsExisting) {
    const std::string src = TmpPath("sign_src");
    const std::string out = TmpPath("sign_out");
    WriteIndirectAnnotsPdf(src);
    {
        Document doc{src};
        Facades::PdfFileSignature sig{doc, out};
        Forms::PKCS7 cert;
        sig.Sign("Signature1", cert);
        sig.Save();
    }
    EXPECT_EQ(CountSubtype(out, "Square"), 1);
    EXPECT_EQ(CountSubtype(out, "Widget"), 1);
    std::filesystem::remove(src);
    std::filesystem::remove(out);
}
