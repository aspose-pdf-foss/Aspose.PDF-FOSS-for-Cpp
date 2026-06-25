#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PdfFileEditor — file-level PDF editor
// (concatenate / split / extract / insert / delete / append /
// resize / booklet / n-up / convert-to-format). Mirrors canonical
// Aspose.Pdf.Facades.PdfFileEditor.
//
// Concatenate / Split methods come in two flavours canonical:
//   * Throwing form (Concatenate, Append, Insert, Delete,
//     Extract, MakeBooklet, MakeNUp, SplitFromFirst, SplitToEnd,
//     ResizeContents) — returns bool but throws on
//     PdfException.
//   * Try* form (TryConcatenate, TryAppend, …) — returns bool
//     and surfaces the exception through LastException.
//
// v1 baseline: every file-manipulation method is a stub that
// returns false. The actual byte manipulation needs substantial
// PdfWriter wiring that's deferred to a follow-on beat. Properties
// + nested types are real storage.
//
// Phased drops:
//   * All Stream-based overloads (canonical pattern: every
//     concat/split/extract method has a Stream variant). Drops
//     via Stream cascade.
//   * Other deps: ContentsResizeParameters/Value, PageBreak, and
//     CorruptedItem are NESTED on PdfFileEditor per canonical and
//     declared inline below.
// =============================================================================

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <aspose/pdf/page_size.hpp>
#include <aspose/pdf/pdf_format.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Facades {

class PdfFileEditor {
public:
    // ---- Nested enum ----

    enum class ConcatenateCorruptedFileAction : int {
        StopWithError = 0,
        ConcatenateIgnoringCorrupted = 1,
        ConcatenateIgnoringCorruptedObjects = 2,
    };

    // ---- Nested classes ----

    class PageBreak {
    public:
        PageBreak() noexcept = default;
        PageBreak(int pageNumber, double position) noexcept;

        int PageNumber() const noexcept;
        void PageNumber(int value) noexcept;
        double Position() const noexcept;
        void Position(double value) noexcept;

    private:
        int page_number_ = 1;
        double position_ = 0.0;
    };

    class CorruptedItem {
    public:
        CorruptedItem() noexcept = default;
        explicit CorruptedItem(int index) noexcept;

        int Index() const noexcept;
        // Exception (System.Exception) property drops via
        // translations cascade.

    private:
        int index_ = -1;
    };

    class ContentsResizeValue {
    public:
        ContentsResizeValue() noexcept = default;

        // Static factory methods — canonical "type-safe enum"
        // construction returning new instances.
        static ContentsResizeValue Percents(double value) noexcept;
        static ContentsResizeValue Units(double value) noexcept;
        static ContentsResizeValue Auto() noexcept;

        // Setter-only canonical (kept in cpp as write-shadow
        // through the factory methods above).
        void PercentValue(double value) noexcept;
        void UnitValue(double value) noexcept;

        double Value() const noexcept;
        bool IsPercent() const noexcept;

    private:
        double value_ = 0.0;
        bool is_percent_ = false;
        bool is_auto_ = true;
    };

    class ContentsResizeParameters {
    public:
        ContentsResizeParameters() noexcept = default;

        // Convenience helpers from canonical (3 args: PageSizeUnits,
        // MarginsUnits, ContentsResize-Units factory choices).
        // Full canonical surface has ~30 members — v1 ships the
        // 14 backing accessors per its seed.

        ContentsResizeValue NewPageWidth() const noexcept;
        void NewPageWidth(ContentsResizeValue value) noexcept;
        ContentsResizeValue NewPageHeight() const noexcept;
        void NewPageHeight(ContentsResizeValue value) noexcept;

        ContentsResizeValue LeftMargin() const noexcept;
        void LeftMargin(ContentsResizeValue value) noexcept;
        ContentsResizeValue RightMargin() const noexcept;
        void RightMargin(ContentsResizeValue value) noexcept;
        ContentsResizeValue TopMargin() const noexcept;
        void TopMargin(ContentsResizeValue value) noexcept;
        ContentsResizeValue BottomMargin() const noexcept;
        void BottomMargin(ContentsResizeValue value) noexcept;

        ContentsResizeValue ContentsWidth() const noexcept;
        void ContentsWidth(ContentsResizeValue value) noexcept;
        ContentsResizeValue ContentsHeight() const noexcept;
        void ContentsHeight(ContentsResizeValue value) noexcept;

    private:
        ContentsResizeValue new_page_width_;
        ContentsResizeValue new_page_height_;
        ContentsResizeValue left_margin_;
        ContentsResizeValue right_margin_;
        ContentsResizeValue top_margin_;
        ContentsResizeValue bottom_margin_;
        ContentsResizeValue contents_width_;
        ContentsResizeValue contents_height_;
    };

    PdfFileEditor() noexcept = default;

    // ---- Concatenate ----

    // Concatenate(firstFile, secondFile, outputFile) — most-used
    // overload. v1 stub returns false.
    bool Concatenate(const std::string& firstInputFile,
                     const std::string& secondInputFile,
                     const std::string& outputFile);
    bool Concatenate(const std::vector<std::string>& inputFiles,
                     const std::string& outputFile);
    bool Concatenate(const std::vector<Aspose::Pdf::Document*>& src,
                     Aspose::Pdf::Document& dest);

    bool TryConcatenate(const std::string& firstInputFile,
                        const std::string& secondInputFile,
                        const std::string& outputFile);
    bool TryConcatenate(const std::vector<std::string>& inputFiles,
                        const std::string& outputFile);

    // ---- Append ----

    bool Append(const std::string& portFile,
                const std::vector<std::string>& addFiles,
                int startPage, int endPage,
                const std::string& outputFile);
    bool TryAppend(const std::string& portFile,
                   const std::vector<std::string>& addFiles,
                   int startPage, int endPage,
                   const std::string& outputFile);

    // ---- Insert ----

    bool Insert(const std::string& inputFile, int insertLocation,
                const std::string& portFile,
                int startPage, int endPage,
                const std::string& outputFile);
    bool TryInsert(const std::string& inputFile, int insertLocation,
                   const std::string& portFile,
                   int startPage, int endPage,
                   const std::string& outputFile);

    // ---- Delete ----

    bool Delete(const std::string& inputFile,
                const std::vector<int>& pageNumber,
                const std::string& outputFile);
    bool TryDelete(const std::string& inputFile,
                   const std::vector<int>& pageNumber,
                   const std::string& outputFile);

    // ---- Extract ----

    bool Extract(const std::string& inputFile,
                 int startPage, int endPage,
                 const std::string& outputFile);
    bool Extract(const std::string& inputFile,
                 const std::vector<int>& pageNumber,
                 const std::string& outputFile);
    bool TryExtract(const std::string& inputFile,
                    int startPage, int endPage,
                    const std::string& outputFile);
    bool TryExtract(const std::string& inputFile,
                    const std::vector<int>& pageNumber,
                    const std::string& outputFile);

    // ---- Split ----

    bool SplitFromFirst(const std::string& inputFile, int location,
                        const std::string& outputFile);
    bool SplitToEnd(const std::string& inputFile, int location,
                    const std::string& outputFile);
    std::vector<std::string> SplitToPages(
        const std::string& inputFile);
    std::vector<std::string> SplitToBulks(
        const std::string& inputFile,
        const std::vector<std::vector<int>>& numberOfPage);

    bool TrySplitFromFirst(const std::string& inputFile, int location,
                           const std::string& outputFile);
    bool TrySplitToEnd(const std::string& inputFile, int location,
                       const std::string& outputFile);

    // ---- Booklet / N-Up ----

    bool MakeBooklet(const std::string& inputFile,
                     const std::string& outputFile);
    bool MakeBooklet(const std::string& inputFile,
                     const std::string& outputFile,
                     Aspose::Pdf::PageSize pageSize);
    bool TryMakeBooklet(const std::string& inputFile,
                        const std::string& outputFile);

    bool MakeNUp(const std::string& firstInputFile,
                 const std::string& secondInputFile,
                 const std::string& outputFile);
    bool MakeNUp(const std::vector<std::string>& inputFiles,
                 const std::string& outputFile, bool isSidewise);
    bool TryMakeNUp(const std::string& firstInputFile,
                    const std::string& secondInputFile,
                    const std::string& outputFile);

    // ---- Resize ----

    bool ResizeContents(const std::string& inputFile,
                        const std::string& outputFile,
                        const std::vector<int>& pages,
                        ContentsResizeParameters parameters);
    bool ResizeContentsPct(const std::string& inputFile,
                           const std::string& outputFile,
                           double xPercent, double yPercent);
    bool TryResizeContents(const std::string& inputFile,
                           const std::string& outputFile,
                           const std::vector<int>& pages,
                           ContentsResizeParameters parameters);

    // ---- Page margins / breaks ----

    bool AddMargins(const std::string& inputFile,
                    const std::string& outputFile,
                    const std::vector<int>& pages,
                    double leftMargin, double rightMargin,
                    double topMargin, double bottomMargin);
    bool AddMarginsPct(const std::string& inputFile,
                       const std::string& outputFile,
                       const std::vector<int>& pages,
                       double leftMarginPct, double rightMarginPct,
                       double topMarginPct, double bottomMarginPct);
    bool AddPageBreak(const std::string& inputFile,
                      const std::string& outputFile,
                      const std::vector<PageBreak>& pageBreaks);

    // ---- Properties — concat behavior ----

    bool AllowConcatenateExceptions() const noexcept;
    void AllowConcatenateExceptions(bool value) noexcept;

    bool CloseConcatenatedStreams() const noexcept;
    void CloseConcatenatedStreams(bool value) noexcept;

    int ConcatenationPacketSize() const noexcept;
    void ConcatenationPacketSize(int value) noexcept;

    const std::string& ConversionLog() const noexcept;

    // ConvertTo is canonical setter-only on the property side; v1
    // stores the value but the actual format conversion is
    // deferred.
    void ConvertTo(Aspose::Pdf::PdfFormat value) noexcept;

    bool CopyLogicalStructure() const noexcept;
    void CopyLogicalStructure(bool value) noexcept;

    bool CopyOutlines() const noexcept;
    void CopyOutlines(bool value) noexcept;

    ConcatenateCorruptedFileAction CorruptedFileAction() const noexcept;
    void CorruptedFileAction(ConcatenateCorruptedFileAction value) noexcept;

    const std::vector<CorruptedItem>& CorruptedItems() const noexcept;

    bool IncrementalUpdates() const noexcept;
    void IncrementalUpdates(bool value) noexcept;

    bool KeepActions() const noexcept;
    void KeepActions(bool value) noexcept;

    bool KeepFieldsUnique() const noexcept;
    void KeepFieldsUnique(bool value) noexcept;

    // LastException is canonical setter-only (writes the most
    // recent exception from a Try* call). v1 stores a string —
    // System.Exception drops via translations.
    void LastException(const std::string& value);

    bool MergeDuplicateLayers() const noexcept;
    void MergeDuplicateLayers(bool value) noexcept;

    bool MergeDuplicateOutlines() const noexcept;
    void MergeDuplicateOutlines(bool value) noexcept;

    bool OptimizeSize() const noexcept;
    void OptimizeSize(bool value) noexcept;

    const std::string& OwnerPassword() const noexcept;
    void OwnerPassword(std::string value);

    bool PreserveUserRights() const noexcept;
    void PreserveUserRights(bool value) noexcept;

    bool RemoveSignatures() const noexcept;
    void RemoveSignatures(bool value) noexcept;

    const std::string& UniqueSuffix() const noexcept;
    void UniqueSuffix(std::string value);

    bool UseDiskBuffer() const noexcept;
    void UseDiskBuffer(bool value) noexcept;

private:
    // Throwing / Try* shared cores (isTry suppresses rethrow).
    bool ConcatenateTwo(const std::string& firstInputFile,
                        const std::string& secondInputFile,
                        const std::string& outputFile, bool isTry);
    bool ConcatenateMany(const std::vector<std::string>& inputFiles,
                         const std::string& outputFile, bool isTry);
    bool AppendImpl(const std::string& portFile,
                    const std::vector<std::string>& addFiles, int startPage,
                    int endPage, const std::string& outputFile, bool isTry);
    bool InsertImpl(const std::string& inputFile, int insertLocation,
                    const std::string& portFile, int startPage, int endPage,
                    const std::string& outputFile, bool isTry);

    bool allow_concatenate_exceptions_ = false;
    bool close_concatenated_streams_ = false;
    int concatenation_packet_size_ = 1000;
    std::string conversion_log_;
    Aspose::Pdf::PdfFormat convert_to_ = Aspose::Pdf::PdfFormat::v_1_7;
    bool copy_logical_structure_ = false;
    bool copy_outlines_ = true;
    ConcatenateCorruptedFileAction corrupted_file_action_ =
        ConcatenateCorruptedFileAction::StopWithError;
    std::vector<CorruptedItem> corrupted_items_;
    bool incremental_updates_ = false;
    bool keep_actions_ = false;
    bool keep_fields_unique_ = false;
    bool merge_duplicate_layers_ = false;
    bool merge_duplicate_outlines_ = true;
    bool optimize_size_ = false;
    std::string owner_password_;
    bool preserve_user_rights_ = false;
    bool remove_signatures_ = false;
    std::string unique_suffix_;
    bool use_disk_buffer_ = false;
};

}  // namespace Aspose::Pdf::Facades
