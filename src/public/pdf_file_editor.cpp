#include <aspose/pdf/facades/pdf_file_editor.hpp>

#include <filesystem>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page_collection.hpp>

namespace Aspose::Pdf::Facades {

namespace {

// Exception policy shared by the throwing / Try* facade forms: the
// throwing form rethrows only when AllowConcatenateExceptions is set;
// the Try* form always swallows and returns false.
template <class F>
bool RunEditor(F&& op, bool isTry, bool allowExceptions) {
    try {
        op();
        return true;
    } catch (const std::exception&) {
        if (!isTry && allowExceptions) throw;
        return false;
    }
}

std::vector<int> AllPages(const Aspose::Pdf::Document& doc) {
    std::vector<int> v;
    const int n = static_cast<int>(
        const_cast<Aspose::Pdf::Document&>(doc).Pages().Count());
    for (int i = 1; i <= n; ++i) v.push_back(i);
    return v;
}

// Clamp a [start..end] 1-based inclusive range to [1..count]; end<=0
// means "to last".
std::vector<int> PageRange(int count, int start, int end) {
    if (start < 1) start = 1;
    if (end <= 0 || end > count) end = count;
    std::vector<int> v;
    for (int i = start; i <= end; ++i) v.push_back(i);
    return v;
}

// Pages of [1..count] that are NOT in `keep`.
std::vector<int> Complement(int count, const std::vector<int>& keep) {
    std::set<int> k(keep.begin(), keep.end());
    std::vector<int> v;
    for (int i = 1; i <= count; ++i)
        if (k.find(i) == k.end()) v.push_back(i);
    return v;
}

std::string SplitName(const std::string& input, const std::string& tag) {
    std::filesystem::path p(input);
    std::filesystem::path out =
        p.parent_path() / (p.stem().string() + "_" + tag +
                           p.extension().string());
    return out.string();
}

}  // namespace

// ===== PageBreak =============================================================

PdfFileEditor::PageBreak::PageBreak(int pageNumber,
                                      double position) noexcept
    : page_number_(pageNumber), position_(position) {}

int PdfFileEditor::PageBreak::PageNumber() const noexcept {
    return page_number_;
}
void PdfFileEditor::PageBreak::PageNumber(int v) noexcept {
    page_number_ = v;
}
double PdfFileEditor::PageBreak::Position() const noexcept {
    return position_;
}
void PdfFileEditor::PageBreak::Position(double v) noexcept {
    position_ = v;
}

// ===== CorruptedItem =========================================================

PdfFileEditor::CorruptedItem::CorruptedItem(int index) noexcept
    : index_(index) {}

int PdfFileEditor::CorruptedItem::Index() const noexcept {
    return index_;
}

// ===== ContentsResizeValue ===================================================

PdfFileEditor::ContentsResizeValue
PdfFileEditor::ContentsResizeValue::Percents(double value) noexcept {
    ContentsResizeValue v;
    v.value_ = value;
    v.is_percent_ = true;
    v.is_auto_ = false;
    return v;
}

PdfFileEditor::ContentsResizeValue
PdfFileEditor::ContentsResizeValue::Units(double value) noexcept {
    ContentsResizeValue v;
    v.value_ = value;
    v.is_percent_ = false;
    v.is_auto_ = false;
    return v;
}

PdfFileEditor::ContentsResizeValue
PdfFileEditor::ContentsResizeValue::Auto() noexcept {
    ContentsResizeValue v;
    v.value_ = 0.0;
    v.is_auto_ = true;
    return v;
}

void PdfFileEditor::ContentsResizeValue::PercentValue(
        double v) noexcept {
    value_ = v;
    is_percent_ = true;
    is_auto_ = false;
}
void PdfFileEditor::ContentsResizeValue::UnitValue(
        double v) noexcept {
    value_ = v;
    is_percent_ = false;
    is_auto_ = false;
}

double PdfFileEditor::ContentsResizeValue::Value() const noexcept {
    return value_;
}
bool PdfFileEditor::ContentsResizeValue::IsPercent() const noexcept {
    return is_percent_;
}

// ===== ContentsResizeParameters ==============================================

#define CRP_ACCESSOR(NAME, FIELD)                                  \
    PdfFileEditor::ContentsResizeValue                              \
    PdfFileEditor::ContentsResizeParameters::NAME() const noexcept { \
        return FIELD;                                              \
    }                                                              \
    void PdfFileEditor::ContentsResizeParameters::NAME(            \
            ContentsResizeValue v) noexcept {                      \
        FIELD = std::move(v);                                      \
    }

CRP_ACCESSOR(NewPageWidth,   new_page_width_)
CRP_ACCESSOR(NewPageHeight,  new_page_height_)
CRP_ACCESSOR(LeftMargin,     left_margin_)
CRP_ACCESSOR(RightMargin,    right_margin_)
CRP_ACCESSOR(TopMargin,      top_margin_)
CRP_ACCESSOR(BottomMargin,   bottom_margin_)
CRP_ACCESSOR(ContentsWidth,  contents_width_)
CRP_ACCESSOR(ContentsHeight, contents_height_)

#undef CRP_ACCESSOR

// ===== PdfFileEditor — real file operations ==================================
// Concatenate / Append / Insert deep-import the source pages' object
// graph into the destination (Document::ImportPagesFrom); Extract /
// Delete / Split reduce to PageCollection page removal. Each writes the
// result via incremental update. Layout ops (booklet / n-up / resize /
// margins) remain stubs below.

namespace {

bool DoDelete(const std::string& inputFile, const std::vector<int>& pages,
              const std::string& out, bool isTry, bool allowEx) {
    return RunEditor([&] {
        Aspose::Pdf::Document d(inputFile);
        d.Pages().Delete(pages);
        d.Save(out);
    }, isTry, allowEx);
}

bool DoExtractKeep(const std::string& inputFile,
                   const std::vector<int>& keep, const std::string& out,
                   bool isTry, bool allowEx) {
    return RunEditor([&] {
        Aspose::Pdf::Document d(inputFile);
        const int count = static_cast<int>(d.Pages().Count());
        const std::vector<int> del = Complement(count, keep);
        if (!del.empty()) d.Pages().Delete(del);
        d.Save(out);
    }, isTry, allowEx);
}

}  // namespace

bool PdfFileEditor::ConcatenateTwo(const std::string& a, const std::string& b,
                                   const std::string& out, bool isTry) {
    return RunEditor([&] {
        Aspose::Pdf::Document d1(a);
        Aspose::Pdf::Document d2(b);
        d1.ImportPagesFrom(d2, AllPages(d2), 0);
        d1.Save(out);
    }, isTry, allow_concatenate_exceptions_);
}
bool PdfFileEditor::ConcatenateMany(const std::vector<std::string>& files,
                                    const std::string& out, bool isTry) {
    return RunEditor([&] {
        if (files.empty())
            throw std::runtime_error("PdfFileEditor: no input files");
        Aspose::Pdf::Document d(files.front());
        for (std::size_t i = 1; i < files.size(); ++i) {
            Aspose::Pdf::Document s(files[i]);
            d.ImportPagesFrom(s, AllPages(s), 0);
        }
        d.Save(out);
    }, isTry, allow_concatenate_exceptions_);
}
bool PdfFileEditor::AppendImpl(const std::string& portFile,
                               const std::vector<std::string>& addFiles,
                               int startPage, int endPage,
                               const std::string& out, bool isTry) {
    return RunEditor([&] {
        Aspose::Pdf::Document d(portFile);
        for (const auto& add : addFiles) {
            Aspose::Pdf::Document s(add);
            const int count = static_cast<int>(s.Pages().Count());
            d.ImportPagesFrom(s, PageRange(count, startPage, endPage), 0);
        }
        d.Save(out);
    }, isTry, allow_concatenate_exceptions_);
}
bool PdfFileEditor::InsertImpl(const std::string& inputFile,
                               int insertLocation,
                               const std::string& portFile, int startPage,
                               int endPage, const std::string& out,
                               bool isTry) {
    return RunEditor([&] {
        Aspose::Pdf::Document d(inputFile);
        Aspose::Pdf::Document s(portFile);
        const int count = static_cast<int>(s.Pages().Count());
        d.ImportPagesFrom(s, PageRange(count, startPage, endPage),
                          insertLocation);
        d.Save(out);
    }, isTry, allow_concatenate_exceptions_);
}

bool PdfFileEditor::Concatenate(const std::string& a, const std::string& b,
                                const std::string& out) {
    return ConcatenateTwo(a, b, out, false);
}
bool PdfFileEditor::Concatenate(const std::vector<std::string>& files,
                                const std::string& out) {
    return ConcatenateMany(files, out, false);
}
bool PdfFileEditor::Concatenate(
        const std::vector<Aspose::Pdf::Document*>& src,
        Aspose::Pdf::Document& dest) {
    return RunEditor([&] {
        for (auto* s : src) {
            if (s != nullptr)
                dest.ImportPagesFrom(*s, AllPages(*s), 0);
        }
    }, false, allow_concatenate_exceptions_);
}

bool PdfFileEditor::TryConcatenate(const std::string& a, const std::string& b,
                                   const std::string& out) {
    return ConcatenateTwo(a, b, out, true);
}
bool PdfFileEditor::TryConcatenate(const std::vector<std::string>& files,
                                   const std::string& out) {
    return ConcatenateMany(files, out, true);
}

bool PdfFileEditor::Append(const std::string& portFile,
                           const std::vector<std::string>& addFiles,
                           int startPage, int endPage,
                           const std::string& out) {
    return AppendImpl(portFile, addFiles, startPage, endPage, out, false);
}
bool PdfFileEditor::TryAppend(const std::string& portFile,
                              const std::vector<std::string>& addFiles,
                              int startPage, int endPage,
                              const std::string& out) {
    return AppendImpl(portFile, addFiles, startPage, endPage, out, true);
}

bool PdfFileEditor::Insert(const std::string& inputFile, int insertLocation,
                           const std::string& portFile, int startPage,
                           int endPage, const std::string& out) {
    return InsertImpl(inputFile, insertLocation, portFile, startPage,
                      endPage, out, false);
}
bool PdfFileEditor::TryInsert(const std::string& inputFile,
                              int insertLocation, const std::string& portFile,
                              int startPage, int endPage,
                              const std::string& out) {
    return InsertImpl(inputFile, insertLocation, portFile, startPage,
                      endPage, out, true);
}

bool PdfFileEditor::Delete(const std::string& inputFile,
                           const std::vector<int>& pageNumber,
                           const std::string& out) {
    return DoDelete(inputFile, pageNumber, out, false,
                    allow_concatenate_exceptions_);
}
bool PdfFileEditor::TryDelete(const std::string& inputFile,
                              const std::vector<int>& pageNumber,
                              const std::string& out) {
    return DoDelete(inputFile, pageNumber, out, true,
                    allow_concatenate_exceptions_);
}

bool PdfFileEditor::Extract(const std::string& inputFile, int startPage,
                            int endPage, const std::string& out) {
    return RunEditor([&] {
        Aspose::Pdf::Document d(inputFile);
        const int count = static_cast<int>(d.Pages().Count());
        const std::vector<int> keep = PageRange(count, startPage, endPage);
        const std::vector<int> del = Complement(count, keep);
        if (!del.empty()) d.Pages().Delete(del);
        d.Save(out);
    }, false, allow_concatenate_exceptions_);
}
bool PdfFileEditor::Extract(const std::string& inputFile,
                            const std::vector<int>& pageNumber,
                            const std::string& out) {
    return DoExtractKeep(inputFile, pageNumber, out, false,
                         allow_concatenate_exceptions_);
}
bool PdfFileEditor::TryExtract(const std::string& inputFile, int startPage,
                               int endPage, const std::string& out) {
    return RunEditor([&] {
        Aspose::Pdf::Document d(inputFile);
        const int count = static_cast<int>(d.Pages().Count());
        const std::vector<int> keep = PageRange(count, startPage, endPage);
        const std::vector<int> del = Complement(count, keep);
        if (!del.empty()) d.Pages().Delete(del);
        d.Save(out);
    }, true, allow_concatenate_exceptions_);
}
bool PdfFileEditor::TryExtract(const std::string& inputFile,
                               const std::vector<int>& pageNumber,
                               const std::string& out) {
    return DoExtractKeep(inputFile, pageNumber, out, true,
                         allow_concatenate_exceptions_);
}

bool PdfFileEditor::SplitFromFirst(const std::string& inputFile, int location,
                                   const std::string& out) {
    return RunEditor([&] {
        Aspose::Pdf::Document d(inputFile);
        const int count = static_cast<int>(d.Pages().Count());
        const std::vector<int> del = Complement(count,
                                                 PageRange(count, 1, location));
        if (!del.empty()) d.Pages().Delete(del);
        d.Save(out);
    }, false, allow_concatenate_exceptions_);
}
bool PdfFileEditor::SplitToEnd(const std::string& inputFile, int location,
                               const std::string& out) {
    return RunEditor([&] {
        Aspose::Pdf::Document d(inputFile);
        const int count = static_cast<int>(d.Pages().Count());
        const std::vector<int> del =
            Complement(count, PageRange(count, location, count));
        if (!del.empty()) d.Pages().Delete(del);
        d.Save(out);
    }, false, allow_concatenate_exceptions_);
}
std::vector<std::string> PdfFileEditor::SplitToPages(
        const std::string& inputFile) {
    std::vector<std::string> out_files;
    Aspose::Pdf::Document probe(inputFile);
    const int count = static_cast<int>(probe.Pages().Count());
    for (int i = 1; i <= count; ++i) {
        Aspose::Pdf::Document d(inputFile);
        const std::vector<int> del = Complement(count, {i});
        if (!del.empty()) d.Pages().Delete(del);
        const std::string name = SplitName(inputFile, std::to_string(i));
        d.Save(name);
        out_files.push_back(name);
    }
    return out_files;
}
std::vector<std::string> PdfFileEditor::SplitToBulks(
        const std::string& inputFile,
        const std::vector<std::vector<int>>& numberOfPage) {
    std::vector<std::string> out_files;
    const int count =
        static_cast<int>(Aspose::Pdf::Document(inputFile).Pages().Count());
    for (std::size_t g = 0; g < numberOfPage.size(); ++g) {
        Aspose::Pdf::Document d(inputFile);
        const std::vector<int> del = Complement(count, numberOfPage[g]);
        if (!del.empty()) d.Pages().Delete(del);
        const std::string name =
            SplitName(inputFile, "bulk" + std::to_string(g + 1));
        d.Save(name);
        out_files.push_back(name);
    }
    return out_files;
}
bool PdfFileEditor::TrySplitFromFirst(const std::string& inputFile,
                                      int location, const std::string& out) {
    return RunEditor([&] {
        Aspose::Pdf::Document d(inputFile);
        const int count = static_cast<int>(d.Pages().Count());
        const std::vector<int> del = Complement(count,
                                                 PageRange(count, 1, location));
        if (!del.empty()) d.Pages().Delete(del);
        d.Save(out);
    }, true, allow_concatenate_exceptions_);
}
bool PdfFileEditor::TrySplitToEnd(const std::string& inputFile, int location,
                                  const std::string& out) {
    return RunEditor([&] {
        Aspose::Pdf::Document d(inputFile);
        const int count = static_cast<int>(d.Pages().Count());
        const std::vector<int> del =
            Complement(count, PageRange(count, location, count));
        if (!del.empty()) d.Pages().Delete(del);
        d.Save(out);
    }, true, allow_concatenate_exceptions_);
}

bool PdfFileEditor::MakeBooklet(const std::string&,
                                 const std::string&)           { return false; }
bool PdfFileEditor::MakeBooklet(const std::string&,
                                 const std::string&,
                                 Aspose::Pdf::PageSize)        { return false; }
bool PdfFileEditor::TryMakeBooklet(const std::string&,
                                    const std::string&)        { return false; }

bool PdfFileEditor::MakeNUp(const std::string&, const std::string&,
                             const std::string&)               { return false; }
bool PdfFileEditor::MakeNUp(const std::vector<std::string>&,
                             const std::string&, bool)         { return false; }
bool PdfFileEditor::TryMakeNUp(const std::string&,
                                const std::string&,
                                const std::string&)            { return false; }

bool PdfFileEditor::ResizeContents(const std::string&,
                                    const std::string&,
                                    const std::vector<int>&,
                                    ContentsResizeParameters)  { return false; }
bool PdfFileEditor::ResizeContentsPct(const std::string&,
                                       const std::string&,
                                       double, double)         { return false; }
bool PdfFileEditor::TryResizeContents(const std::string&,
                                       const std::string&,
                                       const std::vector<int>&,
                                       ContentsResizeParameters)
                                                                { return false; }

bool PdfFileEditor::AddMargins(const std::string&, const std::string&,
                                const std::vector<int>&,
                                double, double, double, double){ return false; }
bool PdfFileEditor::AddMarginsPct(const std::string&,
                                   const std::string&,
                                   const std::vector<int>&,
                                   double, double, double, double)
                                                                { return false; }
bool PdfFileEditor::AddPageBreak(const std::string&,
                                  const std::string&,
                                  const std::vector<PageBreak>&)
                                                                { return false; }

// ===== Properties — real storage =============================================

bool PdfFileEditor::AllowConcatenateExceptions() const noexcept {
    return allow_concatenate_exceptions_;
}
void PdfFileEditor::AllowConcatenateExceptions(bool v) noexcept {
    allow_concatenate_exceptions_ = v;
}

bool PdfFileEditor::CloseConcatenatedStreams() const noexcept {
    return close_concatenated_streams_;
}
void PdfFileEditor::CloseConcatenatedStreams(bool v) noexcept {
    close_concatenated_streams_ = v;
}

int PdfFileEditor::ConcatenationPacketSize() const noexcept {
    return concatenation_packet_size_;
}
void PdfFileEditor::ConcatenationPacketSize(int v) noexcept {
    concatenation_packet_size_ = v;
}

const std::string& PdfFileEditor::ConversionLog() const noexcept {
    return conversion_log_;
}

void PdfFileEditor::ConvertTo(Aspose::Pdf::PdfFormat v) noexcept {
    convert_to_ = v;
}

bool PdfFileEditor::CopyLogicalStructure() const noexcept {
    return copy_logical_structure_;
}
void PdfFileEditor::CopyLogicalStructure(bool v) noexcept {
    copy_logical_structure_ = v;
}

bool PdfFileEditor::CopyOutlines() const noexcept {
    return copy_outlines_;
}
void PdfFileEditor::CopyOutlines(bool v) noexcept {
    copy_outlines_ = v;
}

PdfFileEditor::ConcatenateCorruptedFileAction
PdfFileEditor::CorruptedFileAction() const noexcept {
    return corrupted_file_action_;
}
void PdfFileEditor::CorruptedFileAction(
        ConcatenateCorruptedFileAction v) noexcept {
    corrupted_file_action_ = v;
}

const std::vector<PdfFileEditor::CorruptedItem>&
PdfFileEditor::CorruptedItems() const noexcept {
    return corrupted_items_;
}

bool PdfFileEditor::IncrementalUpdates() const noexcept {
    return incremental_updates_;
}
void PdfFileEditor::IncrementalUpdates(bool v) noexcept {
    incremental_updates_ = v;
}

bool PdfFileEditor::KeepActions() const noexcept { return keep_actions_; }
void PdfFileEditor::KeepActions(bool v) noexcept { keep_actions_ = v; }

bool PdfFileEditor::KeepFieldsUnique() const noexcept {
    return keep_fields_unique_;
}
void PdfFileEditor::KeepFieldsUnique(bool v) noexcept {
    keep_fields_unique_ = v;
}

void PdfFileEditor::LastException(const std::string& v) {
    (void)v;
    // v1 stub — System.Exception drops via translations; the
    // setter accepts the call but doesn't surface the exception.
}

bool PdfFileEditor::MergeDuplicateLayers() const noexcept {
    return merge_duplicate_layers_;
}
void PdfFileEditor::MergeDuplicateLayers(bool v) noexcept {
    merge_duplicate_layers_ = v;
}

bool PdfFileEditor::MergeDuplicateOutlines() const noexcept {
    return merge_duplicate_outlines_;
}
void PdfFileEditor::MergeDuplicateOutlines(bool v) noexcept {
    merge_duplicate_outlines_ = v;
}

bool PdfFileEditor::OptimizeSize() const noexcept {
    return optimize_size_;
}
void PdfFileEditor::OptimizeSize(bool v) noexcept { optimize_size_ = v; }

const std::string& PdfFileEditor::OwnerPassword() const noexcept {
    return owner_password_;
}
void PdfFileEditor::OwnerPassword(std::string v) {
    owner_password_ = std::move(v);
}

bool PdfFileEditor::PreserveUserRights() const noexcept {
    return preserve_user_rights_;
}
void PdfFileEditor::PreserveUserRights(bool v) noexcept {
    preserve_user_rights_ = v;
}

bool PdfFileEditor::RemoveSignatures() const noexcept {
    return remove_signatures_;
}
void PdfFileEditor::RemoveSignatures(bool v) noexcept {
    remove_signatures_ = v;
}

const std::string& PdfFileEditor::UniqueSuffix() const noexcept {
    return unique_suffix_;
}
void PdfFileEditor::UniqueSuffix(std::string v) {
    unique_suffix_ = std::move(v);
}

bool PdfFileEditor::UseDiskBuffer() const noexcept {
    return use_disk_buffer_;
}
void PdfFileEditor::UseDiskBuffer(bool v) noexcept {
    use_disk_buffer_ = v;
}

}  // namespace Aspose::Pdf::Facades
