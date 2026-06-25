#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PdfExtractor — extracts text / images /
// attachments from the bound PDF. Mirrors canonical
// Aspose.Pdf.Facades.PdfExtractor; extends Facade.
//
// v1 capabilities:
//   * Text extraction is REAL — ExtractText / GetText(file) /
//     HasNextPageText / GetNextPageText(file) walk the bound
//     document through the foundation TextAbsorber, honouring the
//     StartPage / EndPage range. Whole-document text and the
//     per-page cursor are both backed by a single extraction pass.
//   * Image + attachment extraction are v1 stubs (return false /
//     empty / no-op). The pixel-level image decode + /EmbeddedFile
//     stream plumbing land in follow-on beats.
//
// Phased drops:
//   * Every Stream-based overload (GetText(Stream) / GetNextPageText
//     (Stream) / GetNextImage(Stream…) / BindPdf(Stream)) — Stream
//     cascade. GetAttachment() → MemoryStream[] — MemoryStream
//     cascade.
//   * GetNextImage(*, ImageFormat) — System.Drawing.Imaging.
//     ImageFormat cascade.
//   * ExtractText(Encoding) — Encoding-controlled extraction
//     deferred; v1 ExtractText() produces UTF-8 (drops.yaml).
//   * ExtractImageMode property — Aspose.Pdf.ExtractImageMode enum
//     not yet in cpp catalog (cascade).
//   * TextSearchOptions property — Aspose.Pdf.Text.TextSearchOptions
//     not yet in cpp catalog (cascade).
// =============================================================================

#include <cstddef>
#include <string>
#include <vector>

#include <aspose/pdf/facades/facade.hpp>
#include <aspose/pdf/file_specification.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Facades {

class PdfExtractor : public Facade {
public:
    PdfExtractor() noexcept = default;
    explicit PdfExtractor(Aspose::Pdf::Document& document);

    // Re-expose the inherited BindPdf(file) / BindPdf(Document&)
    // overloads so callers can rebind without ambiguity.
    using Facade::BindPdf;

    // ---- Text extraction (real) ----

    // Extract text from the StartPage..EndPage range into the
    // internal buffer. Subsequent GetText / GetNextPageText read
    // from this pass.
    void ExtractText();

    // Write the extracted whole-document text to a file. Extracts
    // lazily if ExtractText() wasn't called first.
    void GetText(const std::string& outputFile);

    // True while pages remain in the per-page text cursor.
    bool HasNextPageText();

    // Write the next page's text to a file and advance the cursor.
    void GetNextPageText(const std::string& outputFile);

    // ---- Image extraction (v1 stubs) ----

    void ExtractImage();
    bool HasNextImage();
    bool GetNextImage(const std::string& outputFile);

    // ---- Attachment extraction (v1 stubs) ----

    std::vector<std::string> GetAttachNames();
    void ExtractAttachment();
    void ExtractAttachment(const std::string& attachmentFileName);
    void GetAttachment(const std::string& outputPath);
    std::vector<Aspose::Pdf::FileSpecification> GetAttachmentInfo();

    // ---- Properties ----

    int StartPage() const noexcept;
    void StartPage(int value) noexcept;
    int EndPage() const noexcept;
    void EndPage(int value) noexcept;
    int ExtractTextMode() const noexcept;
    void ExtractTextMode(int value) noexcept;
    int Resolution() const noexcept;
    void Resolution(int value) noexcept;
    const std::string& Password() const noexcept;
    void Password(std::string value);
    bool IsBidi() const noexcept;

private:
    // Run the StartPage..EndPage extraction pass once (idempotent
    // until StartPage / EndPage change). Populates full_text_ +
    // page_texts_ and resets the cursor.
    void EnsureExtracted();

    std::string full_text_;
    std::vector<std::string> page_texts_;
    std::size_t page_cursor_ = 0;
    bool extracted_ = false;

    int start_page_ = 1;
    int end_page_ = 0;  // 0 == to last page
    int extract_text_mode_ = 0;
    int resolution_ = 150;
    std::string password_;
    bool is_bidi_ = false;
    int image_cursor_ = 0;  // GetNextImage walk position
};

}  // namespace Aspose::Pdf::Facades
