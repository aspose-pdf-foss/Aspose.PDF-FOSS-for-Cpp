#include <aspose/pdf/document.hpp>
#include <aspose/pdf/metadata.hpp>
#include <aspose/pdf/xmp_value.hpp>

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace {

std::filesystem::path FixtureDir() {
    if (const char* env = std::getenv("SMOKE_FIXTURE_DIR"); env != nullptr) {
        return std::filesystem::path(env);
    }
    return std::filesystem::path(__FILE__)
        .parent_path()
        / "fixtures" / "metadata";
}

}  // namespace

TEST(MetadataSmoke, ReadsXmpFromWithXmpFixture) {
    const auto src = FixtureDir() / "with_xmp.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;

    Aspose::Pdf::Document doc(src.string());
    auto& m = doc.Metadata();

    EXPECT_GT(m.Count(), 0);
    EXPECT_TRUE(m.ContainsKey("dc:title"));
    EXPECT_EQ(m.at("dc:title").ToStringValue(), "LibForge Test Document");
}

TEST(MetadataSmoke, EmptyMetadataForWithoutXmpFixture) {
    const auto src = FixtureDir() / "without_xmp.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;

    Aspose::Pdf::Document doc(src.string());
    EXPECT_EQ(doc.Metadata().Count(), 0);
}

TEST(MetadataSmoke, AddRemoveContainsRoundtrip) {
    const auto src = FixtureDir() / "without_xmp.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;
    Aspose::Pdf::Document doc(src.string());
    auto& m = doc.Metadata();
    EXPECT_EQ(m.Count(), 0);

    m.Add("dc:format", Aspose::Pdf::XmpValue(std::string("application/pdf")));
    EXPECT_TRUE(m.ContainsKey("dc:format"));
    EXPECT_TRUE(m.Contains("dc:format"));
    EXPECT_EQ(m.Count(), 1);
    EXPECT_EQ(m.at("dc:format").ToStringValue(), "application/pdf");

    Aspose::Pdf::XmpValue out{std::string{}};
    EXPECT_TRUE(m.TryGetValue("dc:format", out));
    EXPECT_EQ(out.ToStringValue(), "application/pdf");

    EXPECT_TRUE(m.Remove("dc:format"));
    EXPECT_FALSE(m.ContainsKey("dc:format"));
    EXPECT_EQ(m.Count(), 0);
    EXPECT_FALSE(m.Remove("dc:format"));
}

TEST(MetadataSmoke, KeysAndValuesReflectEntries) {
    const auto src = FixtureDir() / "without_xmp.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;
    Aspose::Pdf::Document doc(src.string());
    auto& m = doc.Metadata();
    m.Add("xmp:CreateDate", Aspose::Pdf::XmpValue(std::string("2026-05-04")));
    m.Add("pdf:Producer", Aspose::Pdf::XmpValue(std::string("FOSS")));

    auto keys = m.Keys();
    auto vals = m.Values();
    EXPECT_EQ(keys.size(), 2u);
    EXPECT_EQ(vals.size(), 2u);
}

TEST(MetadataSmoke, NamespaceRegistrationRoundtrip) {
    const auto src = FixtureDir() / "without_xmp.pdf";
    ASSERT_TRUE(std::filesystem::exists(src)) << src;
    Aspose::Pdf::Document doc(src.string());
    auto& m = doc.Metadata();
    m.RegisterNamespaceUri("dc", "http://purl.org/dc/elements/1.1/");
    EXPECT_EQ(m.GetNamespaceUriByPrefix("dc"),
              "http://purl.org/dc/elements/1.1/");
    EXPECT_EQ(m.GetPrefixByNamespaceUri(
                  "http://purl.org/dc/elements/1.1/"), "dc");
    EXPECT_EQ(m.GetNamespaceUriByPrefix("missing"), "");
}

TEST(XmpValueSmoke, StringValueShape) {
    Aspose::Pdf::XmpValue v(std::string("hello"));
    EXPECT_TRUE(v.IsString());
    EXPECT_FALSE(v.IsInteger());
    EXPECT_EQ(v.ToStringValue(), "hello");
    EXPECT_EQ(v.ToString(), "hello");
}

TEST(XmpValueSmoke, IntegerValueShape) {
    Aspose::Pdf::XmpValue v(42);
    EXPECT_TRUE(v.IsInteger());
    EXPECT_EQ(v.ToInteger(), 42);
    EXPECT_DOUBLE_EQ(v.ToDouble(), 42.0);
    EXPECT_EQ(v.ToStringValue(), "42");
}

TEST(XmpValueSmoke, DoubleValueShape) {
    Aspose::Pdf::XmpValue v(3.5);
    EXPECT_TRUE(v.IsDouble());
    EXPECT_DOUBLE_EQ(v.ToDouble(), 3.5);
    EXPECT_EQ(v.ToInteger(), 3);
}

TEST(XmpValueSmoke, ArrayValueShape) {
    std::vector<Aspose::Pdf::XmpValue> arr;
    arr.emplace_back(std::string("Alice"));
    arr.emplace_back(std::string("Bob"));
    Aspose::Pdf::XmpValue v(std::move(arr));
    EXPECT_TRUE(v.IsArray());
    auto out = v.ToArray();
    EXPECT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0].ToStringValue(), "Alice");
    EXPECT_EQ(out[1].ToStringValue(), "Bob");
    EXPECT_EQ(v.ToStringValue(), "Alice\tBob");
}

TEST(XmpValueSmoke, StringToIntegerParse) {
    Aspose::Pdf::XmpValue v(std::string("123"));
    EXPECT_EQ(v.ToInteger(), 123);
}

TEST(XmpValueSmoke, OutOfScopeIsPredicatesAreFalse) {
    Aspose::Pdf::XmpValue v(std::string("x"));
    EXPECT_FALSE(v.IsDateTime());
    EXPECT_FALSE(v.IsField());
    EXPECT_FALSE(v.IsNamedValue());
    EXPECT_FALSE(v.IsRaw());
    EXPECT_FALSE(v.IsNamedValues());
    EXPECT_FALSE(v.IsStructure());
}
