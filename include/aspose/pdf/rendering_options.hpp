#pragma once

// =============================================================================
// Aspose::Pdf::RenderingOptions — bag of fine-grained rendering policy
// consulted by image-device pipelines. Sealed class, default ctor +
// 13 read-write instance properties (overloaded getter/setter pairs).
// =============================================================================

#include <string>

namespace Aspose::Pdf {

class RenderingOptions {
public:
    RenderingOptions();

    bool AnalyzeFonts() const noexcept;
    void AnalyzeFonts(bool value) noexcept;

    bool BarcodeOptimization() const noexcept;
    void BarcodeOptimization(bool value) noexcept;

    bool ConvertFontsToUnicodeTTF() const noexcept;
    void ConvertFontsToUnicodeTTF(bool value) noexcept;

    const std::string& DefaultFontName() const noexcept;
    void DefaultFontName(std::string value);

    float HeightExtraUnits() const noexcept;
    void HeightExtraUnits(float value) noexcept;

    bool IgnoreResourceFontErrors() const noexcept;
    void IgnoreResourceFontErrors(bool value) noexcept;

    bool InterpolationHighQuality() const noexcept;
    void InterpolationHighQuality(bool value) noexcept;

    int MaxFontsCacheSize() const noexcept;
    void MaxFontsCacheSize(int value) noexcept;

    int MaxSymbolsCacheSize() const noexcept;
    void MaxSymbolsCacheSize(int value) noexcept;

    bool OptimizeDimensions() const noexcept;
    void OptimizeDimensions(bool value) noexcept;

    bool SystemFontsNativeRendering() const noexcept;
    void SystemFontsNativeRendering(bool value) noexcept;

    bool UseFontHinting() const noexcept;
    void UseFontHinting(bool value) noexcept;

    float WidthExtraUnits() const noexcept;
    void WidthExtraUnits(float value) noexcept;

private:
    bool analyze_fonts_ = false;
    bool barcode_optimization_ = false;
    bool convert_fonts_to_unicode_ttf_ = false;
    std::string default_font_name_;
    float height_extra_units_ = 0.0f;
    bool ignore_resource_font_errors_ = false;
    bool interpolation_high_quality_ = false;
    int max_fonts_cache_size_ = 0;
    int max_symbols_cache_size_ = 0;
    bool optimize_dimensions_ = false;
    bool system_fonts_native_rendering_ = false;
    bool use_font_hinting_ = false;
    float width_extra_units_ = 0.0f;
};

}
