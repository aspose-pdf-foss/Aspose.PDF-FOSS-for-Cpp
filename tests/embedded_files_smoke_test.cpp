// =============================================================================
// embedded_files_smoke_test — Document.EmbeddedFiles wire-through.
// First gap-closer beat against the `aspose.pdf.cpp.foss` extension
// lib's `Attachments` feature: the LibForge cpp lib now ships the
// canonical `Aspose::Pdf::EmbeddedFileCollection` on Document and
// persists entries through Save via the Catalog's /Names
// /EmbeddedFiles name tree.
//
// v1 baseline:
//   * Each FileSpecification with IncludeContents=true and a
//     readable Name() path emits an /EmbeddedFile stream +
//     /Filespec dict.
//   * Catalog gets /Names /EmbeddedFiles → flat-leaf name tree.
//   * No /Filter (uncompressed bytes).
//   * No merge with existing /EmbeddedFiles — full replace.
//
// Deferred: compression, merge, unicode-aware key encoding,
// /CheckSum + /CreationDate + /ModDate /Params subdict entries.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/embedded_file_collection.hpp>
#include <aspose/pdf/file_specification.hpp>

#include "objects.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;

std::filesystem::path FixtureRoot() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path().parent_path() / "pdfs";
}

std::string HelloWorldPdf() {
    return (FixtureRoot() / "hello_world.pdf").string();
}

std::vector<std::byte> ReadAll(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    const auto end = in.tellg();
    std::vector<std::byte> bytes(static_cast<std::size_t>(end));
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(bytes.data()),
            static_cast<std::streamsize>(bytes.size()));
    return bytes;
}

std::string WriteTempPayload(const std::string& name,
                              const std::string& content) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream out(path, std::ios::binary);
    out.write(content.data(),
              static_cast<std::streamsize>(content.size()));
    return path.string();
}

}  // namespace

TEST(EmbeddedFilesSmoke, CollectionAddCountAndIndexing) {
    Document doc{HelloWorldPdf()};
    EXPECT_EQ(doc.EmbeddedFiles().Count(), 0);

    FileSpecification fs1{"/tmp/x.json", "schema"};
    FileSpecification fs2{"/tmp/y.txt"};
    doc.EmbeddedFiles().Add(fs1);
    doc.EmbeddedFiles().Add("custom_key", fs2);

    EXPECT_EQ(doc.EmbeddedFiles().Count(), 2);
    EXPECT_EQ(doc.EmbeddedFiles().Keys().size(), 2u);
    EXPECT_EQ(doc.EmbeddedFiles().Keys()[0], "/tmp/x.json");
    EXPECT_EQ(doc.EmbeddedFiles().Keys()[1], "custom_key");

    EXPECT_EQ(doc.EmbeddedFiles()[0].Description(), "schema");
    EXPECT_EQ(doc.EmbeddedFiles()["/tmp/x.json"].Description(),
              "schema");

    EXPECT_NE(doc.EmbeddedFiles().FindByName("/tmp/y.txt"),
              nullptr);
    EXPECT_EQ(doc.EmbeddedFiles().FindByName("missing"), nullptr);

    doc.EmbeddedFiles().DeleteByKey("custom_key");
    EXPECT_EQ(doc.EmbeddedFiles().Count(), 1);

    doc.EmbeddedFiles().Delete();
    EXPECT_EQ(doc.EmbeddedFiles().Count(), 0);
}

TEST(EmbeddedFilesSmoke, SaveThroughCreatesCatalogNamesAndFilespec) {
    // Author a payload file on disk; create a FileSpec referencing
    // it; add to Document.EmbeddedFiles; save; re-parse saved bytes
    // and verify the Catalog → /Names → /EmbeddedFiles → /Names
    // array references a /Filespec dict whose /EF /F stream
    // matches the payload bytes.
    const std::string payload = "embedded-file-payload-bytes-2026";
    const std::string payload_path = WriteTempPayload(
        "aspose_emb_payload.txt", payload);

    Document doc{HelloWorldPdf()};
    FileSpecification fs{payload_path, "test payload"};
    doc.EmbeddedFiles().Add(fs);

    const std::string out =
        (std::filesystem::temp_directory_path() /
         "aspose_emb_save.pdf").string();
    doc.Save(out);

    const auto saved = ReadAll(out);
    const auto dump = foundation::objects::Parse(
        std::span<const std::byte>(saved.data(), saved.size()));

    // Locate the Catalog (Type=Catalog).
    const foundation::objects::IndirectObject* catalog = nullptr;
    for (const auto& obj : dump.objects) {
        const auto* d = std::get_if<foundation::objects::Dict>(
            &obj.value.v);
        if (!d) continue;
        for (const auto& kv : d->entries) {
            if (kv.first == "Type") {
                if (auto* s = std::get_if<std::string>(&kv.second.v);
                        s && *s == "Catalog") catalog = &obj;
            }
        }
        if (catalog) break;
    }
    ASSERT_NE(catalog, nullptr);

    // Catalog → /Names (inline dict for v1 baseline).
    const auto& cat_dict = std::get<foundation::objects::Dict>(
        catalog->value.v);
    const foundation::objects::Dict* names_dict = nullptr;
    for (const auto& kv : cat_dict.entries) {
        if (kv.first == "Names") {
            names_dict = std::get_if<foundation::objects::Dict>(
                &kv.second.v);
        }
    }
    ASSERT_NE(names_dict, nullptr);

    // /Names → /EmbeddedFiles → name tree.
    const foundation::objects::Dict* emb_dict = nullptr;
    for (const auto& kv : names_dict->entries) {
        if (kv.first == "EmbeddedFiles") {
            emb_dict = std::get_if<foundation::objects::Dict>(
                &kv.second.v);
        }
    }
    ASSERT_NE(emb_dict, nullptr);

    // /Names array: [(key) <ref>] pairs.
    const foundation::objects::Array* names_arr = nullptr;
    for (const auto& kv : emb_dict->entries) {
        if (kv.first == "Names") {
            names_arr = std::get_if<foundation::objects::Array>(
                &kv.second.v);
        }
    }
    ASSERT_NE(names_arr, nullptr);
    ASSERT_EQ(names_arr->items.size(), 2u);

    // First item: key string. Second: Filespec ref.
    const auto* key_str = std::get_if<foundation::objects::String>(
        &names_arr->items[0].v);
    ASSERT_NE(key_str, nullptr);
    const std::string key_bytes(
        reinterpret_cast<const char*>(key_str->bytes.data()),
        key_str->bytes.size());
    EXPECT_EQ(key_bytes, payload_path);

    const auto* filespec_ref = std::get_if<foundation::objects::Ref>(
        &names_arr->items[1].v);
    ASSERT_NE(filespec_ref, nullptr);

    // Resolve Filespec object.
    const foundation::objects::IndirectObject* filespec_obj = nullptr;
    for (const auto& obj : dump.objects) {
        if (obj.id == filespec_ref->id) filespec_obj = &obj;
    }
    ASSERT_NE(filespec_obj, nullptr);
    const auto* filespec_dict = std::get_if<foundation::objects::Dict>(
        &filespec_obj->value.v);
    ASSERT_NE(filespec_dict, nullptr);

    // Walk Filespec entries.
    std::string fs_type, fs_f, fs_desc;
    const foundation::objects::Ref* ef_stream_ref = nullptr;
    for (const auto& kv : filespec_dict->entries) {
        if (kv.first == "Type") {
            if (auto* s = std::get_if<std::string>(&kv.second.v); s)
                fs_type = *s;
        } else if (kv.first == "F") {
            if (auto* s = std::get_if<foundation::objects::String>(&kv.second.v); s)
                fs_f.assign(reinterpret_cast<const char*>(s->bytes.data()),
                            s->bytes.size());
        } else if (kv.first == "Desc") {
            if (auto* s = std::get_if<foundation::objects::String>(&kv.second.v); s)
                fs_desc.assign(reinterpret_cast<const char*>(s->bytes.data()),
                               s->bytes.size());
        } else if (kv.first == "EF") {
            if (auto* d = std::get_if<foundation::objects::Dict>(&kv.second.v)) {
                for (const auto& efkv : d->entries) {
                    if (efkv.first == "F") {
                        ef_stream_ref = std::get_if<foundation::objects::Ref>(
                            &efkv.second.v);
                    }
                }
            }
        }
    }

    EXPECT_EQ(fs_type, "Filespec");
    EXPECT_EQ(fs_f, payload_path);
    EXPECT_EQ(fs_desc, "test payload");
    ASSERT_NE(ef_stream_ref, nullptr);

    // Resolve the /EmbeddedFile stream and check its bytes.
    const foundation::objects::IndirectObject* stream_obj = nullptr;
    for (const auto& obj : dump.objects) {
        if (obj.id == ef_stream_ref->id) stream_obj = &obj;
    }
    ASSERT_NE(stream_obj, nullptr);
    const auto* stream_val = std::get_if<foundation::objects::Stream>(
        &stream_obj->value.v);
    ASSERT_NE(stream_val, nullptr);
    std::string body_bytes(
        reinterpret_cast<const char*>(stream_val->body.data()),
        stream_val->body.size());
    EXPECT_EQ(body_bytes, payload);
}

TEST(EmbeddedFilesSmoke, NoEmbeddedFilesLeavesBytesUntouched) {
    const auto src = ReadAll(HelloWorldPdf());
    Document doc{HelloWorldPdf()};
    const std::string out =
        (std::filesystem::temp_directory_path() /
         "aspose_emb_passthrough.pdf").string();
    doc.Save(out);
    const auto saved = ReadAll(out);
    EXPECT_EQ(saved, src);
}
