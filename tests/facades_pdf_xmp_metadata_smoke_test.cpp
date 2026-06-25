// =============================================================================
// facades_pdf_xmp_metadata_smoke_test — beat Fa7 of the Facades cluster.
// PdfXmpMetadata is a dictionary-style facade over the bound document's
// XMP metadata, wired through the REAL foundation Metadata
// (Document::Metadata()). Mutations land on the document and persist
// through SaveableFacade::Save.
// =============================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/pdf_xmp_metadata.hpp>
#include <aspose/pdf/metadata.hpp>
#include <aspose/pdf/xmp_value.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

std::filesystem::path MetadataFixtureDir() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__).parent_path() / "fixtures" /
           "metadata";
}

std::string WithInfoPdf() {
    return (MetadataFixtureDir() / "with_info.pdf").string();
}

std::string TempPath(const char* name) {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    char suffix[40];
    std::snprintf(suffix, sizeof(suffix), "aspose_xmpfacade_%lld_%s",
                  static_cast<long long>(stamp), name);
    return (std::filesystem::temp_directory_path() / suffix).string();
}

}  // namespace

TEST(FacadesPdfXmpMetadataSmoke, AddContainsCountThroughBoundDoc) {
    Document doc{WithInfoPdf()};
    PdfXmpMetadata xmp{doc};

    xmp.Add("dc:creator", XmpValue(std::string("LibForge")));
    EXPECT_TRUE(xmp.Contains("dc:creator"));
    EXPECT_TRUE(xmp.ContainsKey("dc:creator"));
    EXPECT_GE(xmp.Count(), 1);
    EXPECT_FALSE(xmp.IsReadOnly());
    EXPECT_FALSE(xmp.IsFixedSize());

    // The mutation is visible on the underlying document Metadata.
    EXPECT_TRUE(doc.Metadata().Contains("dc:creator"));
}

TEST(FacadesPdfXmpMetadataSmoke, TryGetValueAndIndexer) {
    Document doc{WithInfoPdf()};
    PdfXmpMetadata xmp{doc};
    xmp.Add("dc:title", XmpValue(std::string("Hello XMP")));

    XmpValue got{std::string()};
    ASSERT_TRUE(xmp.TryGetValue("dc:title", got));
    EXPECT_EQ(got.ToStringValue(), "Hello XMP");
    EXPECT_EQ(xmp["dc:title"].ToStringValue(), "Hello XMP");

    // Indexer assignment routes to the document Metadata.
    xmp["dc:title"] = XmpValue(std::string("Updated"));
    EXPECT_EQ(xmp["dc:title"].ToStringValue(), "Updated");
}

TEST(FacadesPdfXmpMetadataSmoke, KeysValuesRemoveClear) {
    Document doc{WithInfoPdf()};
    PdfXmpMetadata xmp{doc};
    xmp.Add("dc:a", XmpValue(std::string("1")));
    xmp.Add("dc:b", XmpValue(std::string("2")));

    EXPECT_GE(xmp.Keys().size(), 2u);
    EXPECT_GE(xmp.Values().size(), 2u);

    EXPECT_TRUE(xmp.Remove("dc:a"));
    EXPECT_FALSE(xmp.Contains("dc:a"));

    xmp.Clear();
    EXPECT_EQ(xmp.Count(), 0);
}

TEST(FacadesPdfXmpMetadataSmoke, NamespaceRegistry) {
    Document doc{WithInfoPdf()};
    PdfXmpMetadata xmp{doc};
    const std::string uri = "http://purl.org/dc/elements/1.1/";
    xmp.RegisterNamespaceURI("dc", uri);
    EXPECT_EQ(xmp.GetNamespaceURIByPrefix("dc"), uri);
    EXPECT_EQ(xmp.GetPrefixByNamespaceURI(uri), "dc");
}

TEST(FacadesPdfXmpMetadataSmoke, SaveThenReloadPersists) {
    const std::string out = TempPath("roundtrip.pdf");
    {
        Document doc{WithInfoPdf()};
        PdfXmpMetadata xmp{doc};
        xmp.RegisterNamespaceURI("dc", "http://purl.org/dc/elements/1.1/");
        xmp.Add("dc:title", XmpValue(std::string("Facade Title")));
        xmp.Save(out);  // SaveableFacade::Save -> Document::Save
    }
    ASSERT_TRUE(std::filesystem::exists(out));

    Document reloaded{out};
    EXPECT_EQ(reloaded.Metadata().at("dc:title").ToStringValue(),
              "Facade Title");
    std::filesystem::remove(out);
}

TEST(FacadesPdfXmpMetadataSmoke, UnboundReadsAreSafeMutationsThrow) {
    PdfXmpMetadata xmp;  // nothing bound
    EXPECT_EQ(xmp.Count(), 0);
    EXPECT_FALSE(xmp.Contains("k"));
    EXPECT_TRUE(xmp.Keys().empty());
    EXPECT_THROW(xmp.Add("k", XmpValue(std::string("v"))),
                 std::runtime_error);
}
