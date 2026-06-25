#pragma once

// =============================================================================
// Aspose::Pdf::Facades::PdfFileStamp — stamps headers / footers / page
// numbers onto a PDF, input-file -> output-file. Mirrors canonical
// Aspose.Pdf.Facades.PdfFileStamp; extends SaveableFacade.
//
// AddHeader / AddFooter / AddPageNumber draw real Helvetica text into each
// page's content stream (via Document::ApplyTextStamps, an incremental
// update) — the stamped text is present and extractable after Save.
// Properties + the 8 position constants are real. PageHeight / PageWidth
// return canonical defaults.
//
// Phased drops:
//   * Stream ctors / Save(Stream) / InputStream / OutputStream /
//     AddHeader(Stream…) / AddFooter(Stream…) — Stream cascade.
//   * FormattedText overloads of AddPageNumber / AddHeader / AddFooter —
//     Aspose.Pdf.Facades.FormattedText cascade.
//   * AddStamp(Stamp) — Aspose.Pdf.Facades.Stamp not in cpp catalog.
//   * NumberingStyle — Aspose.Pdf.NumberingStyle enum not in cpp catalog.
// =============================================================================

#include <string>

#include <aspose/pdf/facades/saveable_facade.hpp>
#include <aspose/pdf/pdf_format.hpp>

namespace Aspose::Pdf {
class Document;
}

namespace Aspose::Pdf::Facades {

class PdfFileStamp : public SaveableFacade {
public:
    // ---- Stamp position presets (canonical 0..7) ----
    static constexpr int PosBottomMiddle = 0;
    static constexpr int PosBottomRight = 1;
    static constexpr int PosUpperRight = 2;
    static constexpr int PosSidesRight = 3;
    static constexpr int PosUpperMiddle = 4;
    static constexpr int PosBottomLeft = 5;
    static constexpr int PosSidesLeft = 6;
    static constexpr int PosUpperLeft = 7;

    PdfFileStamp() noexcept = default;
    PdfFileStamp(const std::string& inputFile, const std::string& outputFile);
    PdfFileStamp(const std::string& inputFile, const std::string& outputFile,
                 bool keepSecurity);
    explicit PdfFileStamp(Aspose::Pdf::Document& document);
    PdfFileStamp(Aspose::Pdf::Document& document,
                 const std::string& outputFile);

    // ---- Stamping (v1 stubs) ----

    void AddPageNumber(const std::string& format);
    void AddPageNumber(const std::string& format, int startingNumber);
    void AddPageNumber(const std::string& format, float left, float top);
    void AddPageNumber(const std::string& format, int startingNumber,
                       float llx, float lly, float urx, float ury);
    void AddHeader(const std::string& text, float topMargin);
    void AddHeader(const std::string& text, float topMargin, float leftMargin,
                   float rightMargin);
    void AddFooter(const std::string& text, float bottomMargin);
    void AddFooter(const std::string& text, float bottomMargin,
                   float leftMargin, float rightMargin);

    // ---- Properties ----

    bool OptimizeSize() const noexcept;
    void OptimizeSize(bool value) noexcept;
    bool KeepSecurity() const noexcept;
    void KeepSecurity(bool value) noexcept;
    const std::string& InputFile() const noexcept;
    void InputFile(std::string value);
    const std::string& OutputFile() const noexcept;
    void OutputFile(std::string value);
    float PageNumberRotation() const noexcept;
    void PageNumberRotation(float value) noexcept;
    void ConvertTo(Aspose::Pdf::PdfFormat value) noexcept;
    float PageHeight() const noexcept;
    float PageWidth() const noexcept;
    int StartingNumber() const noexcept;
    void StartingNumber(int value) noexcept;
    int StampId() const noexcept;
    void StampId(int value) noexcept;

private:
    // Real draw helpers (content-stream text via Document::ApplyTextStamps).
    void DrawHeader(const std::string& text, float topMargin,
                    float leftMargin);
    void DrawFooter(const std::string& text, float bottomMargin,
                    float leftMargin);
    void DrawPageNumber(const std::string& format, int startingNumber,
                        float x, float y);

    bool optimize_size_ = false;
    bool keep_security_ = false;
    std::string input_file_;
    std::string output_file_;
    float page_number_rotation_ = 0.0f;
    Aspose::Pdf::PdfFormat convert_to_ = Aspose::Pdf::PdfFormat::v_1_7;
    int starting_number_ = 1;
    int stamp_id_ = 0;
};

}  // namespace Aspose::Pdf::Facades
