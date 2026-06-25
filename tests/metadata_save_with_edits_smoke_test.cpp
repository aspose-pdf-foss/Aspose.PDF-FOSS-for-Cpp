// End-to-end smoke for Metadata save-with-edits (v1.1).
//
// Mirrors the python and csharp lib arcs: mutate doc.Metadata,
// Save, reload — the persisted XMP stream is recoverable via
// Document::Metadata. foundation::pdf_writer_incremental emits
// a /Metadata stream object (v1.1 stream value variant);
// Document::AppendMetadataUpdate wires the Catalog's /Metadata
// Ref when the source had no existing /Metadata.

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/metadata.hpp>
#include <aspose/pdf/xmp_value.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

std::filesystem::path FixtureDir() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR");
            env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path() / "fixtures" / "metadata";
}

// Build a unique temp-path suffix from the steady_clock count.
// Portable across MSVC + clang + gcc — avoids <unistd.h>'s
// POSIX-only getpid which broke the Windows CI build.
std::filesystem::path TempPath(const char* name) {
    auto tmp = std::filesystem::temp_directory_path();
    const auto stamp = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    char suffix[32];
    std::snprintf(suffix, sizeof(suffix), "%lld",
        static_cast<long long>(stamp));
    return tmp / (std::string("aspose_") + suffix + "_" + name);
}

std::vector<std::byte> ReadAllBytes(
        const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    f.seekg(0, std::ios::end);
    auto size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<std::byte> out(static_cast<std::size_t>(size));
    f.read(reinterpret_cast<char*>(out.data()), size);
    return out;
}

bool ContainsBytes(const std::vector<std::byte>& haystack,
                   const std::string& needle) {
    if (needle.empty()) return true;
    if (haystack.size() < needle.size()) return false;
    for (std::size_t i = 0;
            i + needle.size() <= haystack.size(); ++i) {
        bool ok = true;
        for (std::size_t j = 0; j < needle.size(); ++j) {
            if (static_cast<char>(haystack[i + j]) != needle[j]) {
                ok = false; break;
            }
        }
        if (ok) return true;
    }
    return false;
}

}  // namespace

TEST(MetadataSaveWithEditsSmoke, SaveEmitsMetadataStream) {
    const auto src = FixtureDir() / "with_info.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;

    Aspose::Pdf::Document doc(src.string());
    doc.Metadata()["dc:title"] =
        Aspose::Pdf::XmpValue(std::string("Saved Title"));

    const auto out = TempPath("meta_save_smoke.pdf");
    doc.Save(out.string());
    auto bytes = ReadAllBytes(out);
    EXPECT_TRUE(ContainsBytes(bytes,
        "<dc:title>Saved Title</dc:title>"));
    EXPECT_TRUE(ContainsBytes(bytes,
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\""));
    std::filesystem::remove(out);
}

TEST(MetadataSaveWithEditsSmoke, SaveThenReloadRoundTripsMetadata) {
    const auto src = FixtureDir() / "with_info.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;

    Aspose::Pdf::Document doc(src.string());
    doc.Metadata()["dc:title"] =
        Aspose::Pdf::XmpValue(std::string("Persisted Title"));
    doc.Metadata()["pdf:Producer"] =
        Aspose::Pdf::XmpValue(std::string("Aspose.PDF FOSS"));

    const auto out = TempPath("meta_roundtrip.pdf");
    doc.Save(out.string());

    Aspose::Pdf::Document reloaded(out.string());
    EXPECT_EQ(reloaded.Metadata().at("dc:title").ToStringValue(),
              "Persisted Title");
    EXPECT_EQ(reloaded.Metadata().at("pdf:Producer").ToStringValue(),
              "Aspose.PDF FOSS");
    std::filesystem::remove(out);
}

TEST(MetadataSaveWithEditsSmoke, SaveCleanWhenNothingDirty) {
    const auto src = FixtureDir() / "with_info.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;

    const auto out = TempPath("meta_clean_save.pdf");
    Aspose::Pdf::Document(src.string()).Save(out.string());
    auto src_bytes = ReadAllBytes(src);
    auto out_bytes = ReadAllBytes(out);
    EXPECT_EQ(src_bytes, out_bytes);
    std::filesystem::remove(out);
}

TEST(MetadataSaveWithEditsSmoke, CustomNamespaceUriRoundTrips) {
    // v1.1: source-population captures xmlns:prefix → uri
    // declarations into the Metadata registry. A custom
    // prefix saved + reloaded preserves its URI binding
    // (closes the xmp::Parse URI capture parity gap).
    const auto src = FixtureDir() / "with_info.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;

    Aspose::Pdf::Document doc(src.string());
    doc.Metadata().RegisterNamespaceUri(
        "custom", "http://example.com/v1/");
    doc.Metadata()["custom:tag"] =
        Aspose::Pdf::XmpValue(std::string("value"));

    const auto out = TempPath("meta_ns_roundtrip.pdf");
    doc.Save(out.string());
    Aspose::Pdf::Document reloaded(out.string());
    EXPECT_EQ(reloaded.Metadata().GetNamespaceUriByPrefix("custom"),
              "http://example.com/v1/");
    EXPECT_EQ(reloaded.Metadata().at("custom:tag").ToStringValue(),
              "value");
    std::filesystem::remove(out);
}

TEST(MetadataSaveWithEditsSmoke, XmlSpecialCharsEscaped) {
    const auto src = FixtureDir() / "with_info.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;

    Aspose::Pdf::Document doc(src.string());
    doc.Metadata()["dc:title"] =
        Aspose::Pdf::XmpValue(std::string("A & B <test>"));
    const auto out = TempPath("meta_escape.pdf");
    doc.Save(out.string());
    auto bytes = ReadAllBytes(out);
    EXPECT_TRUE(ContainsBytes(bytes,
        "<dc:title>A &amp; B &lt;test&gt;</dc:title>"));
    std::filesystem::remove(out);
}
