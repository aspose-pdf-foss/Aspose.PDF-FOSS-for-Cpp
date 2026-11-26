#include <aspose/pdf/facades/pdf_file_editor.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/page_size.hpp>

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
bool PdfFileEditor::ContentsResizeValue::IsAuto() const noexcept {
    return is_auto_;
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

// ===== Page layout ===========================================================
// Booklet / N-up draw source pages as Form XObjects onto new pages
// (Document::ComposePagesInternal); margins / resize / page break redraw a
// page's own content through a transform (TransformPageContentInternal).

namespace {

// The cm placing a w x h page centred in the box (x, y, bw, bh), scaled
// uniformly to fit.
std::array<double, 6> FitInto(double w, double h, double x, double y,
                              double bw, double bh) {
    if (w <= 0 || h <= 0) return {1, 0, 0, 1, x, y};
    const double s = std::min(bw / w, bh / h);
    return {s, 0, 0, s, x + (bw - w * s) / 2.0, y + (bh - h * s) / 2.0};
}

// All pages of `doc` (1-based).
std::vector<int> PagesOf(Aspose::Pdf::Document& doc) {
    std::vector<int> v;
    for (int i = 1; i <= static_cast<int>(doc.Pages().Count()); ++i)
        v.push_back(i);
    return v;
}

// `pages` filtered to [1..count]; empty means every page.
std::vector<int> TargetPages(const std::vector<int>& pages, int count) {
    std::vector<int> v;
    if (pages.empty()) {
        for (int i = 1; i <= count; ++i) v.push_back(i);
        return v;
    }
    for (int p : pages)
        if (p >= 1 && p <= count) v.push_back(p);
    return v;
}

}  // namespace

void PdfFileEditor::DoMakeBooklet(const std::string& input,
                                  const std::string& output,
                                  const Aspose::Pdf::PageSize* size) {
    using Composed = Aspose::Pdf::Document::ComposedPage;
    Aspose::Pdf::Document d(input);
    const int n = static_cast<int>(d.Pages().Count());
    if (n == 0) throw std::runtime_error("PdfFileEditor::MakeBooklet: no pages");
    const int total = (n + 3) / 4 * 4;  // padded with blank pages
    const auto first = d.PageDisplaySizeInternal(1);
    const double W = size ? size->Width() : first[0] * 2;
    const double H = size ? size->Height() : first[1];
    // Sheet s: front [total-2s, 2s+1], back [2s+2, total-2s-1].
    std::vector<Composed> out;
    auto sheet_side = [&](int left, int right) {
        Composed c;
        c.width = W;
        c.height = H;
        int k = 0;
        for (int p : {left, right}) {
            const double x = k++ == 0 ? 0.0 : W / 2.0;
            if (p > n) continue;  // padding page: left blank
            const auto sz = d.PageDisplaySizeInternal(p);
            Aspose::Pdf::Document::PagePlacement pl;
            pl.page = p;
            pl.cm = FitInto(sz[0], sz[1], x, 0, W / 2.0, H);
            c.items.push_back(pl);
        }
        out.push_back(std::move(c));
    };
    for (int s = 0; s < total / 4; ++s) {
        sheet_side(total - 2 * s, 2 * s + 1);
        sheet_side(2 * s + 2, total - 2 * s - 1);
    }
    d.ComposePagesInternal(out);
    std::vector<int> originals;
    for (int i = 1; i <= n; ++i) originals.push_back(i);
    d.Pages().Delete(originals);
    d.Save(output);
}

void PdfFileEditor::DoMakeNUp(const std::vector<std::string>& inputs,
                              const std::string& output, bool sidewise) {
    using Composed = Aspose::Pdf::Document::ComposedPage;
    if (inputs.empty()) throw std::runtime_error("PdfFileEditor::MakeNUp: no input");
    Aspose::Pdf::Document d(inputs[0]);
    // Every input's pages, appended to the first document: file j's pages
    // start at base[j] + 1.
    std::vector<int> base{0}, count{static_cast<int>(d.Pages().Count())};
    for (std::size_t j = 1; j < inputs.size(); ++j) {
        Aspose::Pdf::Document other(inputs[j]);
        base.push_back(static_cast<int>(d.Pages().Count()));
        count.push_back(static_cast<int>(other.Pages().Count()));
        d.ImportPagesFrom(other, PagesOf(other), 0);
    }
    const int originals = static_cast<int>(d.Pages().Count());
    int rows = 0;
    for (int c : count) rows = std::max(rows, c);
    std::vector<Composed> out;
    for (int i = 1; i <= rows; ++i) {
        // Slot j: file j's page i. A file with fewer pages leaves its slot
        // empty, sized like its first page so the others keep their place.
        std::vector<std::array<double, 2>> sz(inputs.size(), {0, 0});
        for (std::size_t j = 0; j < inputs.size(); ++j)
            if (count[j] > 0)
                sz[j] = d.PageDisplaySizeInternal(
                    base[j] + (i <= count[j] ? i : 1));
        Composed c;
        for (const auto& s : sz) {
            c.width = sidewise ? c.width + s[0] : std::max(c.width, s[0]);
            c.height = sidewise ? std::max(c.height, s[1]) : c.height + s[1];
        }
        double offset = sidewise ? 0.0 : c.height;
        for (std::size_t j = 0; j < inputs.size(); ++j) {
            if (i > count[j]) {  // empty slot: keep its space
                if (sidewise) offset += sz[j][0];
                else offset -= sz[j][1];
                continue;
            }
            Aspose::Pdf::Document::PagePlacement p;
            p.page = base[j] + i;
            if (sidewise) {
                p.cm = {1, 0, 0, 1, offset, c.height - sz[j][1]};
                offset += sz[j][0];
            } else {
                offset -= sz[j][1];  // first file on top
                p.cm = {1, 0, 0, 1, 0, offset};
            }
            c.items.push_back(p);
        }
        out.push_back(std::move(c));
    }
    d.ComposePagesInternal(out);
    std::vector<int> drop;
    for (int k = 1; k <= originals; ++k) drop.push_back(k);
    d.Pages().Delete(drop);
    d.Save(output);
}

// Margins in points around each target page: the page grows, the content
// keeps its size and moves by (left, bottom).
void PdfFileEditor::DoAddMargins(const std::string& input,
                                 const std::string& output,
                                 const std::vector<int>& pages, double l,
                                 double r, double t, double b, bool percent) {
    Aspose::Pdf::Document d(input);
    for (int p : TargetPages(pages, static_cast<int>(d.Pages().Count()))) {
        const auto rect = d.Pages()[p].Rect();
        const double w = rect.Width(), h = rect.Height();
        const double L = percent ? w * l / 100.0 : l;
        const double R = percent ? w * r / 100.0 : r;
        const double T = percent ? h * t / 100.0 : t;
        const double B = percent ? h * b / 100.0 : b;
        d.TransformPageContentInternal(
            p, {1, 0, 0, 1, L - rect.LLX(), B - rect.LLY()}, w + L + R,
            h + T + B);
    }
    d.Save(output);
}

bool PdfFileEditor::MakeBooklet(const std::string& inputFile,
                                const std::string& outputFile) {
    return RunEditor([&] { DoMakeBooklet(inputFile, outputFile, nullptr); },
                     false, allow_concatenate_exceptions_);
}

bool PdfFileEditor::MakeBooklet(const std::string& inputFile,
                                const std::string& outputFile,
                                Aspose::Pdf::PageSize pageSize) {
    return RunEditor([&] { DoMakeBooklet(inputFile, outputFile, &pageSize); },
                     false, allow_concatenate_exceptions_);
}

bool PdfFileEditor::TryMakeBooklet(const std::string& inputFile,
                                   const std::string& outputFile) {
    return RunEditor([&] { DoMakeBooklet(inputFile, outputFile, nullptr); },
                     true, false);
}

bool PdfFileEditor::MakeNUp(const std::string& firstInputFile,
                            const std::string& secondInputFile,
                            const std::string& outputFile) {
    return RunEditor(
        [&] { DoMakeNUp({firstInputFile, secondInputFile}, outputFile, true); },
        false, allow_concatenate_exceptions_);
}

bool PdfFileEditor::MakeNUp(const std::vector<std::string>& inputFiles,
                            const std::string& outputFile, bool isSidewise) {
    return RunEditor([&] { DoMakeNUp(inputFiles, outputFile, isSidewise); },
                     false, allow_concatenate_exceptions_);
}

bool PdfFileEditor::TryMakeNUp(const std::string& firstInputFile,
                               const std::string& secondInputFile,
                               const std::string& outputFile) {
    return RunEditor(
        [&] { DoMakeNUp({firstInputFile, secondInputFile}, outputFile, true); },
        true, false);
}

namespace {

// Resolve one dimension of ContentsResizeParameters: page size P, margins
// a (left / bottom) and b (right / top), contents size c; percent values
// are of the new page size. Auto values are derived from the others; two
// automatic margins share what is left.
struct Axis { double before, contents; };
Axis ResolveAxis(double page, const PdfFileEditor::ContentsResizeValue& a,
                 const PdfFileEditor::ContentsResizeValue& c,
                 const PdfFileEditor::ContentsResizeValue& b) {
    auto val = [&](const PdfFileEditor::ContentsResizeValue& v) {
        return v.IsPercent() ? page * v.Value() / 100.0 : v.Value();
    };
    const bool aa = a.IsAuto();
    const bool ca = c.IsAuto();
    const bool ba = b.IsAuto();
    double before = aa ? 0 : val(a), after = ba ? 0 : val(b);
    double contents = ca ? 0 : val(c);
    if (ca) {
        contents = page - before - after;
    } else if (aa && ba) {
        before = after = (page - contents) / 2.0;
    } else if (aa) {
        before = page - contents - after;
    }
    return {before, std::max(0.0, contents)};
}

}  // namespace

void PdfFileEditor::DoResize(const std::string& input,
                             const std::string& output,
                             const std::vector<int>& pages,
                             const ContentsResizeParameters& params) {
    Aspose::Pdf::Document d(input);
    for (int p : TargetPages(pages, static_cast<int>(d.Pages().Count()))) {
        const auto rect = d.Pages()[p].Rect();
        const double w = rect.Width(), h = rect.Height();
        auto page_dim = [](const PdfFileEditor::ContentsResizeValue& v,
                           double old) {
            if (v.IsAuto()) return old;
            return v.IsPercent() ? old * v.Value() / 100.0 : v.Value();
        };
        const double W = page_dim(params.NewPageWidth(), w);
        const double H = page_dim(params.NewPageHeight(), h);
        const Axis x = ResolveAxis(W, params.LeftMargin(),
                                   params.ContentsWidth(), params.RightMargin());
        const Axis y = ResolveAxis(H, params.BottomMargin(),
                                   params.ContentsHeight(), params.TopMargin());
        const double sx = w > 0 ? x.contents / w : 1.0;
        const double sy = h > 0 ? y.contents / h : 1.0;
        d.TransformPageContentInternal(
            p, {sx, 0, 0, sy, x.before - rect.LLX() * sx,
                y.before - rect.LLY() * sy},
            W, H);
    }
    d.Save(output);
}

bool PdfFileEditor::ResizeContents(const std::string& inputFile,
                                   const std::string& outputFile,
                                   const std::vector<int>& pages,
                                   ContentsResizeParameters parameters) {
    return RunEditor([&] { DoResize(inputFile, outputFile, pages, parameters); },
                     false, allow_concatenate_exceptions_);
}

bool PdfFileEditor::ResizeContentsPct(const std::string& inputFile,
                                      const std::string& outputFile,
                                      double xPercent, double yPercent) {
    ContentsResizeParameters params;
    params.ContentsWidth(ContentsResizeValue::Percents(xPercent));
    params.ContentsHeight(ContentsResizeValue::Percents(yPercent));
    return RunEditor([&] { DoResize(inputFile, outputFile, {}, params); },
                     false, allow_concatenate_exceptions_);
}

bool PdfFileEditor::TryResizeContents(const std::string& inputFile,
                                      const std::string& outputFile,
                                      const std::vector<int>& pages,
                                      ContentsResizeParameters parameters) {
    return RunEditor([&] { DoResize(inputFile, outputFile, pages, parameters); },
                     true, false);
}

bool PdfFileEditor::AddMargins(const std::string& inputFile,
                               const std::string& outputFile,
                               const std::vector<int>& pages,
                               double leftMargin, double rightMargin,
                               double topMargin, double bottomMargin) {
    return RunEditor(
        [&] {
            DoAddMargins(inputFile, outputFile, pages, leftMargin, rightMargin,
                         topMargin, bottomMargin, false);
        },
        false, allow_concatenate_exceptions_);
}

bool PdfFileEditor::AddMarginsPct(const std::string& inputFile,
                                  const std::string& outputFile,
                                  const std::vector<int>& pages,
                                  double leftMarginPct, double rightMarginPct,
                                  double topMarginPct, double bottomMarginPct) {
    return RunEditor(
        [&] {
            DoAddMargins(inputFile, outputFile, pages, leftMarginPct,
                         rightMarginPct, topMarginPct, bottomMarginPct, true);
        },
        false, allow_concatenate_exceptions_);
}

bool PdfFileEditor::AddPageBreak(const std::string& inputFile,
                                 const std::string& outputFile,
                                 const std::vector<PageBreak>& pageBreaks) {
    return RunEditor([&] { DoAddPageBreak(inputFile, outputFile, pageBreaks); },
                     false, allow_concatenate_exceptions_);
}

void PdfFileEditor::DoAddPageBreak(const std::string& inputFile,
                                   const std::string& outputFile,
                                   const std::vector<PageBreak>& pageBreaks) {
    Aspose::Pdf::Document d(inputFile);
    {
        {
            // Breaks per page, highest first; pages processed last-first so
            // earlier page numbers stay valid while pages are inserted.
            std::map<int, std::vector<double>, std::greater<int>> by_page;
            const int n = static_cast<int>(d.Pages().Count());
            for (const auto& b : pageBreaks)
                if (b.PageNumber() >= 1 && b.PageNumber() <= n)
                    by_page[b.PageNumber()].push_back(b.Position());
            for (auto& [page, positions] : by_page) {
                std::sort(positions.begin(), positions.end(),
                          std::greater<double>());
                int cur = page;
                double shift = 0.0;  // how far the current part moved up
                for (double pos : positions) {
                    const auto rect = d.Pages()[cur].Rect();
                    const double w = rect.Width(), h = rect.Height();
                    const double y = pos + shift;  // break in current space
                    if (y <= 0 || y >= h) continue;
                    // The copy after `cur` shows the part below the break,
                    // moved to the top; `cur` keeps the part above it.
                    d.Pages().Insert(cur + 1, d.Pages()[cur]);
                    const std::array<double, 4> upper{0, y, w, h - y};
                    d.TransformPageContentInternal(cur, {1, 0, 0, 1, 0, 0}, w,
                                                   h, &upper);
                    const std::array<double, 4> lower{0, h - y, w, y};
                    d.TransformPageContentInternal(
                        cur + 1, {1, 0, 0, 1, 0, h - y}, w, h, &lower);
                    shift += h - y;
                    ++cur;
                }
            }
            d.Save(outputFile);
        }
    }
}

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
