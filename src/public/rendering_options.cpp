#include <aspose/pdf/rendering_options.hpp>

#include <utility>

namespace Aspose::Pdf {

RenderingOptions::RenderingOptions() = default;

bool RenderingOptions::AnalyzeFonts() const noexcept { return analyze_fonts_; }
void RenderingOptions::AnalyzeFonts(bool value) noexcept { analyze_fonts_ = value; }

bool RenderingOptions::BarcodeOptimization() const noexcept { return barcode_optimization_; }
void RenderingOptions::BarcodeOptimization(bool value) noexcept { barcode_optimization_ = value; }

bool RenderingOptions::ConvertFontsToUnicodeTTF() const noexcept { return convert_fonts_to_unicode_ttf_; }
void RenderingOptions::ConvertFontsToUnicodeTTF(bool value) noexcept { convert_fonts_to_unicode_ttf_ = value; }

const std::string& RenderingOptions::DefaultFontName() const noexcept { return default_font_name_; }
void RenderingOptions::DefaultFontName(std::string value) { default_font_name_ = std::move(value); }

float RenderingOptions::HeightExtraUnits() const noexcept { return height_extra_units_; }
void RenderingOptions::HeightExtraUnits(float value) noexcept { height_extra_units_ = value; }

bool RenderingOptions::IgnoreResourceFontErrors() const noexcept { return ignore_resource_font_errors_; }
void RenderingOptions::IgnoreResourceFontErrors(bool value) noexcept { ignore_resource_font_errors_ = value; }

bool RenderingOptions::InterpolationHighQuality() const noexcept { return interpolation_high_quality_; }
void RenderingOptions::InterpolationHighQuality(bool value) noexcept { interpolation_high_quality_ = value; }

int RenderingOptions::MaxFontsCacheSize() const noexcept { return max_fonts_cache_size_; }
void RenderingOptions::MaxFontsCacheSize(int value) noexcept { max_fonts_cache_size_ = value; }

int RenderingOptions::MaxSymbolsCacheSize() const noexcept { return max_symbols_cache_size_; }
void RenderingOptions::MaxSymbolsCacheSize(int value) noexcept { max_symbols_cache_size_ = value; }

bool RenderingOptions::OptimizeDimensions() const noexcept { return optimize_dimensions_; }
void RenderingOptions::OptimizeDimensions(bool value) noexcept { optimize_dimensions_ = value; }

bool RenderingOptions::SystemFontsNativeRendering() const noexcept { return system_fonts_native_rendering_; }
void RenderingOptions::SystemFontsNativeRendering(bool value) noexcept { system_fonts_native_rendering_ = value; }

bool RenderingOptions::UseFontHinting() const noexcept { return use_font_hinting_; }
void RenderingOptions::UseFontHinting(bool value) noexcept { use_font_hinting_ = value; }

float RenderingOptions::WidthExtraUnits() const noexcept { return width_extra_units_; }
void RenderingOptions::WidthExtraUnits(float value) noexcept { width_extra_units_ = value; }

}  // namespace Aspose::Pdf
