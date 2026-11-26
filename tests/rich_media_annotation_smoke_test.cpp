// =============================================================================
// rich_media_annotation_smoke_test — RichMediaAnnotation with SetContent:
// saved with /RichMediaContent (assets, configuration, instance) and
// /RichMediaSettings, a poster or default appearance; a Flash player gets
// FlashVars; no content is an error. qpdf --check validates the file where
// qpdf is installed.
// =============================================================================

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/rich_media_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Annotations;
using flatten_test::ReadFile;
using flatten_test::TmpPath;

std::vector<std::byte> Bytes(const std::string& s) {
    return {reinterpret_cast<const std::byte*>(s.data()),
            reinterpret_cast<const std::byte*>(s.data()) + s.size()};
}

std::vector<std::byte> FileBytes(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    const std::string s((std::istreambuf_iterator<char>(in)), {});
    return Bytes(s);
}

bool Has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

}  // namespace

TEST(RichMediaAnnotation, VideoContentSaved) {
    const auto out = TmpPath("richmedia_video");
    {
        Document doc{flatten_test::HelloWorldPdf()};
        auto page = doc.Pages()[1];
        RichMediaAnnotation rm{page, Rectangle(100, 400, 400, 600, false)};
        rm.SetContent("clip.mp4", Bytes("FAKE-MP4-PAYLOAD"));
        rm.Type(RichMediaAnnotation::ContentType::Video);
        rm.ActivateOn(RichMediaAnnotation::ActivationEvent::PageOpen);
        rm.Update();
        page.Annotations().Add(rm);
        doc.Save(out);
    }
    const std::string bytes = ReadFile(out);
    EXPECT_TRUE(Has(bytes, "/Subtype /RichMedia"));
    EXPECT_TRUE(Has(bytes, "/RichMediaContent"));
    EXPECT_TRUE(Has(bytes, "(clip.mp4)"));
    EXPECT_TRUE(Has(bytes, "FAKE-MP4-PAYLOAD"));
    EXPECT_TRUE(Has(bytes, "/Condition /PO"));
    EXPECT_TRUE(Has(bytes, "/RichMediaConfiguration"));
    EXPECT_FALSE(Has(bytes, "player.swf"));  // the content is played directly
    // Default appearance: a frame with a grey play triangle.
    Document doc{out};
    const auto img = flatten_test::Render(doc, 1);
    EXPECT_GT(flatten_test::DarkPixels(img, 100, 400, 400, 600), 0);
}

TEST(RichMediaAnnotation, FlashPlayerGetsFlashVars) {
    const auto out = TmpPath("richmedia_flash");
    {
        Document doc{flatten_test::HelloWorldPdf()};
        auto page = doc.Pages()[1];
        RichMediaAnnotation rm{page, Rectangle(100, 400, 400, 600, false)};
        rm.SetContent("song.mp3", Bytes("FAKE-MP3"));
        rm.Type(RichMediaAnnotation::ContentType::Audio);
        rm.CustomPlayer(Bytes("FAKE-SWF"));
        rm.AddCustomData("skin.swf", Bytes("FAKE-SKIN"));
        page.Annotations().Add(rm);
        doc.Save(out);
    }
    const std::string bytes = ReadFile(out);
    EXPECT_TRUE(Has(bytes, "(player.swf)"));
    EXPECT_TRUE(Has(bytes, "(skin.swf)"));
    EXPECT_TRUE(Has(bytes, "/Subtype /Flash"));
    EXPECT_TRUE(Has(bytes, "(src=song.mp3)"));
    EXPECT_TRUE(Has(bytes, "/Condition /XA"));
}

TEST(RichMediaAnnotation, PosterAppearance) {
    const auto out = TmpPath("richmedia_poster");
    {
        Document doc{flatten_test::HelloWorldPdf()};
        auto page = doc.Pages()[1];
        RichMediaAnnotation rm{page, Rectangle(100, 100, 300, 300, false)};
        rm.SetContent("clip.mp4", Bytes("FAKE"));
        rm.SetPoster(FileBytes(std::filesystem::path(__FILE__).parent_path() / "fixtures" /
                               "png_decoder" / "rgb_checker_16.png"));
        page.Annotations().Add(rm);
        doc.Save(out);
    }
    EXPECT_TRUE(Has(ReadFile(out), "/Im0 Do"));
    Document doc{out};
    const auto img = flatten_test::Render(doc, 1);
    EXPECT_GT(flatten_test::CountPixels(img, 100, 100, 300, 300, 5,
                                        [](int r, int g, int b) { return r + g + b < 700; }),
              1000);
}

TEST(RichMediaAnnotation, ContentIsRequired) {
    Document doc{flatten_test::HelloWorldPdf()};
    auto page = doc.Pages()[1];
    RichMediaAnnotation rm{page, Rectangle(100, 100, 300, 300, false)};
    EXPECT_THROW(rm.Update(), std::invalid_argument);
    page.Annotations().Add(rm);
    EXPECT_THROW(doc.Save(TmpPath("richmedia_empty")), std::runtime_error);
}

TEST(RichMediaAnnotation, QpdfAcceptsTheFile) {
    if (std::system("qpdf --version >/dev/null 2>&1") != 0)
        GTEST_SKIP() << "qpdf not available";
    const auto out = TmpPath("richmedia_qpdf");
    {
        Document doc{flatten_test::HelloWorldPdf()};
        auto page = doc.Pages()[1];
        RichMediaAnnotation rm{page, Rectangle(100, 400, 400, 600, false)};
        rm.SetContent("clip.mp4", Bytes("FAKE-MP4"));
        rm.CustomPlayer(Bytes("FAKE-SWF"));
        page.Annotations().Add(rm);
        doc.Save(out);
    }
    EXPECT_EQ(std::system(("qpdf --check '" + out + "' >/dev/null 2>&1").c_str()), 0);
}
