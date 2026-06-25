// =============================================================================
// facades_pdf_file_editor_smoke_test — beat Fa2 of the Facades cluster.
// PdfFileEditor is the file-level editor (concatenate / split / extract /
// insert / delete / append / resize / booklet / n-up). v1 ships the full
// public surface: every file-manipulation method is a stub returning false
// (byte-level wiring deferred to a follow-on beat); properties have real
// storage; nested types (PageBreak / CorruptedItem / ContentsResizeValue /
// ContentsResizeParameters) are real.
// =============================================================================

#include <stdexcept>
#include <string>
#include <vector>

#include <aspose/pdf/facades/pdf_file_editor.hpp>
#include <aspose/pdf/pdf_format.hpp>

#include <gtest/gtest.h>

namespace {

using namespace Aspose::Pdf;
using namespace Aspose::Pdf::Facades;

}  // namespace

TEST(FacadesPdfFileEditorSmoke, EnumValues) {
    using Action = PdfFileEditor::ConcatenateCorruptedFileAction;
    EXPECT_EQ(static_cast<int>(Action::StopWithError),                       0);
    EXPECT_EQ(static_cast<int>(Action::ConcatenateIgnoringCorrupted),        1);
    EXPECT_EQ(static_cast<int>(Action::ConcatenateIgnoringCorruptedObjects), 2);

    EXPECT_EQ(static_cast<int>(PdfFormat::PDF_A_1A), 0);
    EXPECT_EQ(static_cast<int>(PdfFormat::v_1_7),    15);
    EXPECT_EQ(static_cast<int>(PdfFormat::v_2_0),    16);
    EXPECT_EQ(static_cast<int>(PdfFormat::PDF_E_1),  26);
}

TEST(FacadesPdfFileEditorSmoke, PropertyDefaults) {
    PdfFileEditor editor;
    EXPECT_FALSE(editor.AllowConcatenateExceptions());
    EXPECT_FALSE(editor.CloseConcatenatedStreams());
    EXPECT_EQ(editor.ConcatenationPacketSize(), 1000);
    EXPECT_TRUE(editor.ConversionLog().empty());
    EXPECT_FALSE(editor.CopyLogicalStructure());
    EXPECT_TRUE(editor.CopyOutlines());
    EXPECT_EQ(editor.CorruptedFileAction(),
              PdfFileEditor::ConcatenateCorruptedFileAction::StopWithError);
    EXPECT_TRUE(editor.CorruptedItems().empty());
    EXPECT_FALSE(editor.IncrementalUpdates());
    EXPECT_FALSE(editor.KeepActions());
    EXPECT_FALSE(editor.KeepFieldsUnique());
    EXPECT_FALSE(editor.MergeDuplicateLayers());
    EXPECT_TRUE(editor.MergeDuplicateOutlines());
    EXPECT_FALSE(editor.OptimizeSize());
    EXPECT_TRUE(editor.OwnerPassword().empty());
    EXPECT_FALSE(editor.PreserveUserRights());
    EXPECT_FALSE(editor.RemoveSignatures());
    EXPECT_TRUE(editor.UniqueSuffix().empty());
    EXPECT_FALSE(editor.UseDiskBuffer());
}

TEST(FacadesPdfFileEditorSmoke, PropertyRoundtrip) {
    PdfFileEditor editor;
    editor.AllowConcatenateExceptions(true);
    editor.CloseConcatenatedStreams(true);
    editor.ConcatenationPacketSize(500);
    editor.CopyLogicalStructure(true);
    editor.CopyOutlines(false);
    editor.CorruptedFileAction(
        PdfFileEditor::ConcatenateCorruptedFileAction::ConcatenateIgnoringCorrupted);
    editor.IncrementalUpdates(true);
    editor.KeepActions(true);
    editor.KeepFieldsUnique(true);
    editor.MergeDuplicateLayers(true);
    editor.MergeDuplicateOutlines(false);
    editor.OptimizeSize(true);
    editor.OwnerPassword("secret");
    editor.PreserveUserRights(true);
    editor.RemoveSignatures(true);
    editor.UniqueSuffix("_v2");
    editor.UseDiskBuffer(true);
    editor.ConvertTo(PdfFormat::PDF_A_2B);

    EXPECT_TRUE(editor.AllowConcatenateExceptions());
    EXPECT_TRUE(editor.CloseConcatenatedStreams());
    EXPECT_EQ(editor.ConcatenationPacketSize(), 500);
    EXPECT_TRUE(editor.CopyLogicalStructure());
    EXPECT_FALSE(editor.CopyOutlines());
    EXPECT_EQ(editor.CorruptedFileAction(),
              PdfFileEditor::ConcatenateCorruptedFileAction::ConcatenateIgnoringCorrupted);
    EXPECT_TRUE(editor.IncrementalUpdates());
    EXPECT_TRUE(editor.KeepActions());
    EXPECT_TRUE(editor.KeepFieldsUnique());
    EXPECT_TRUE(editor.MergeDuplicateLayers());
    EXPECT_FALSE(editor.MergeDuplicateOutlines());
    EXPECT_TRUE(editor.OptimizeSize());
    EXPECT_EQ(editor.OwnerPassword(), "secret");
    EXPECT_TRUE(editor.PreserveUserRights());
    EXPECT_TRUE(editor.RemoveSignatures());
    EXPECT_EQ(editor.UniqueSuffix(), "_v2");
    EXPECT_TRUE(editor.UseDiskBuffer());
}

TEST(FacadesPdfFileEditorSmoke, NestedPageBreak) {
    PdfFileEditor::PageBreak pb;
    EXPECT_EQ(pb.PageNumber(), 1);
    EXPECT_DOUBLE_EQ(pb.Position(), 0.0);

    PdfFileEditor::PageBreak pb2{3, 100.5};
    EXPECT_EQ(pb2.PageNumber(), 3);
    EXPECT_DOUBLE_EQ(pb2.Position(), 100.5);

    pb2.PageNumber(7);
    pb2.Position(250.0);
    EXPECT_EQ(pb2.PageNumber(), 7);
    EXPECT_DOUBLE_EQ(pb2.Position(), 250.0);
}

TEST(FacadesPdfFileEditorSmoke, NestedCorruptedItem) {
    PdfFileEditor::CorruptedItem def;
    EXPECT_EQ(def.Index(), -1);

    PdfFileEditor::CorruptedItem item{4};
    EXPECT_EQ(item.Index(), 4);
}

TEST(FacadesPdfFileEditorSmoke, NestedContentsResizeValue) {
    auto pct = PdfFileEditor::ContentsResizeValue::Percents(80.0);
    EXPECT_DOUBLE_EQ(pct.Value(), 80.0);
    EXPECT_TRUE(pct.IsPercent());

    auto units = PdfFileEditor::ContentsResizeValue::Units(200.0);
    EXPECT_DOUBLE_EQ(units.Value(), 200.0);
    EXPECT_FALSE(units.IsPercent());

    auto auto_v = PdfFileEditor::ContentsResizeValue::Auto();
    EXPECT_DOUBLE_EQ(auto_v.Value(), 0.0);
    EXPECT_FALSE(auto_v.IsPercent());
}

TEST(FacadesPdfFileEditorSmoke, NestedContentsResizeParameters) {
    PdfFileEditor::ContentsResizeParameters params;
    params.NewPageWidth(PdfFileEditor::ContentsResizeValue::Units(595.0));
    params.LeftMargin(PdfFileEditor::ContentsResizeValue::Percents(10.0));

    EXPECT_DOUBLE_EQ(params.NewPageWidth().Value(), 595.0);
    EXPECT_FALSE(params.NewPageWidth().IsPercent());
    EXPECT_DOUBLE_EQ(params.LeftMargin().Value(), 10.0);
    EXPECT_TRUE(params.LeftMargin().IsPercent());
}

TEST(FacadesPdfFileEditorSmoke, BadInputAndRemainingStubs) {
    // The real file ops (Concatenate/Append/Insert/Delete/Extract/Split)
    // return false on a non-existent input (the load throws and the
    // throwing form swallows it, AllowConcatenateExceptions off);
    // SplitToPages/SplitToBulks surface the load failure. The layout ops
    // (booklet / n-up / resize / margins) are still stubs returning
    // false. Real behaviour is covered by the *_ops_smoke_test.
    PdfFileEditor editor;
    const std::vector<std::string> files{"a.pdf", "b.pdf"};
    const std::vector<int> pages{1, 2};

    EXPECT_FALSE(editor.Concatenate("a.pdf", "b.pdf", "out.pdf"));
    EXPECT_FALSE(editor.Concatenate(files, "out.pdf"));
    EXPECT_FALSE(editor.TryConcatenate("a.pdf", "b.pdf", "out.pdf"));
    EXPECT_FALSE(editor.Append("a.pdf", files, 1, 2, "out.pdf"));
    EXPECT_FALSE(editor.Insert("a.pdf", 1, "b.pdf", 1, 2, "out.pdf"));
    EXPECT_FALSE(editor.Delete("a.pdf", pages, "out.pdf"));
    EXPECT_FALSE(editor.Extract("a.pdf", 1, 2, "out.pdf"));
    EXPECT_FALSE(editor.Extract("a.pdf", pages, "out.pdf"));
    EXPECT_FALSE(editor.SplitFromFirst("a.pdf", 2, "out.pdf"));
    EXPECT_FALSE(editor.SplitToEnd("a.pdf", 2, "out.pdf"));
    EXPECT_THROW((void)editor.SplitToPages("a.pdf"), std::exception);
    EXPECT_THROW((void)editor.SplitToBulks("a.pdf", {{1, 2}}),
                 std::exception);

    // Still stubs (layout features deferred):
    EXPECT_FALSE(editor.MakeBooklet("a.pdf", "out.pdf"));
    EXPECT_FALSE(editor.MakeNUp("a.pdf", "b.pdf", "out.pdf"));
    EXPECT_FALSE(editor.ResizeContentsPct("a.pdf", "out.pdf", 50.0, 50.0));
    EXPECT_FALSE(editor.AddMargins("a.pdf", "out.pdf", pages, 10, 10, 10, 10));
}
