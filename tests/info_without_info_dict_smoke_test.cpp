// =============================================================================
// info_without_info_dict_smoke_test — document information (/Info) set on a
// document created from scratch, or loaded from a file without an /Info
// dictionary, is saved: a new /Info object is referenced from the trailer.
// =============================================================================

#include <filesystem>
#include <string>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_info.hpp>
#include <aspose/pdf/facades/pdf_file_info.hpp>
#include <aspose/pdf/page_collection.hpp>

#include "flatten_test_support.hpp"

#include <gtest/gtest.h>

using namespace Aspose::Pdf;
using flatten_test::TmpPath;

TEST(InfoWithoutInfoDict, NewDocument) {
    const auto out = TmpPath("info_new_doc");
    {
        Document doc;
        doc.Pages().Add();
        doc.Info().Title("Fresh title");
        doc.Info().Author("Someone");
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(doc.Info().Title(), "Fresh title");
    EXPECT_EQ(doc.Info().Author(), "Someone");
}

TEST(InfoWithoutInfoDict, LoadedFileWithoutInfo) {
    const auto out = TmpPath("info_loaded_noinfo");
    {
        Document doc{flatten_test::HelloWorldPdf()};  // no /Info
        doc.Info().Title("Added later");
        doc.Save(out);
    }
    Document doc{out};
    EXPECT_EQ(doc.Info().Title(), "Added later");

    // A second revision keeps (and updates) the new /Info.
    const auto out2 = TmpPath("info_loaded_noinfo2");
    doc.Info().Subject("Second save");
    doc.Save(out2);
    Document again{out2};
    EXPECT_EQ(again.Info().Title(), "Added later");
    EXPECT_EQ(again.Info().Subject(), "Second save");
}

TEST(InfoWithoutInfoDict, PdfFileInfoSaveNewInfo) {
    const auto out = TmpPath("info_facade_noinfo");
    Facades::PdfFileInfo info{flatten_test::HelloWorldPdf()};
    info.Title("Via facade");
    EXPECT_TRUE(info.SaveNewInfo(out));
    Document doc{out};
    EXPECT_EQ(doc.Info().Title(), "Via facade");
}
