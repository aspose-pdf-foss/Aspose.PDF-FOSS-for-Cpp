#pragma once

// =============================================================================
// Aspose::Pdf::Color — RGBA color carrier used by annotations,
// page rendering options, and document-info accents. Mirrors
// canonical Aspose.Pdf.Color (Aspose.PDF 26.4.0).
//
// Components are stored as normalised doubles in [0.0, 1.0]
// matching the canonical .NET surface. The integer FromArgb /
// FromRgb factories accept 0..255 values and divide by 255.0.
//
// 140 named-color static presets follow the W3C CSS color list
// (X11 names + a few additions). Each preset returns a fresh
// Color instance; the cost is one constexpr table lookup.
// =============================================================================

namespace Aspose::Pdf {

class Document;

class Color {
public:
    // Default-constructed instance is fully transparent black
    // (a=0, r=0, g=0, b=0) — equivalent to Empty().
    Color() noexcept = default;

    // 0..255 integer factories — alpha defaults to 255 in the
    // 3-arg overload.
    static Color FromArgb(int a, int r, int g, int b) noexcept;
    static Color FromArgb(int r, int g, int b) noexcept;

    // 0.0..1.0 normalised double factories.
    static Color FromRgb(double r, double g, double b) noexcept;

    // Alpha-channel accessor (read-only; 0.0..1.0).
    double A() const noexcept;

    // ---- Special static instances ---------------------------

    // Empty — fully transparent black; same as default ctor.
    static Color Empty();

    // Transparent — alpha=0 white. PDF spec convention.
    static Color Transparent();

    // ---- Named color presets (X11 / CSS) --------------------

    static Color AliceBlue();
    static Color AntiqueWhite();
    static Color Aqua();
    static Color Aquamarine();
    static Color Azure();
    static Color Beige();
    static Color Bisque();
    static Color Black();
    static Color BlanchedAlmond();
    static Color Blue();
    static Color BlueViolet();
    static Color Brown();
    static Color BurlyWood();
    static Color CadetBlue();
    static Color Chartreuse();
    static Color Chocolate();
    static Color Coral();
    static Color CornflowerBlue();
    static Color Cornsilk();
    static Color Crimson();
    static Color Cyan();
    static Color DarkBlue();
    static Color DarkCyan();
    static Color DarkGoldenrod();
    static Color DarkGray();
    static Color DarkGreen();
    static Color DarkKhaki();
    static Color DarkMagenta();
    static Color DarkOliveGreen();
    static Color DarkOrange();
    static Color DarkOrchid();
    static Color DarkRed();
    static Color DarkSalmon();
    static Color DarkSeaGreen();
    static Color DarkSlateBlue();
    static Color DarkSlateGray();
    static Color DarkTurquoise();
    static Color DarkViolet();
    static Color DeepPink();
    static Color DeepSkyBlue();
    static Color DimGray();
    static Color DodgerBlue();
    static Color Firebrick();
    static Color FloralWhite();
    static Color ForestGreen();
    static Color Fuchsia();
    static Color Gainsboro();
    static Color GhostWhite();
    static Color Gold();
    static Color Goldenrod();
    static Color Gray();
    static Color Green();
    static Color GreenYellow();
    static Color Honeydew();
    static Color HotPink();
    static Color IndianRed();
    static Color Indigo();
    static Color Ivory();
    static Color Khaki();
    static Color Lavender();
    static Color LavenderBlush();
    static Color LawnGreen();
    static Color LemonChiffon();
    static Color LightBlue();
    static Color LightCoral();
    static Color LightCyan();
    static Color LightGoldenrodYellow();
    static Color LightGreen();
    static Color LightGray();
    static Color LightPink();
    static Color LightSalmon();
    static Color LightSeaGreen();
    static Color LightSkyBlue();
    static Color LightSlateGray();
    static Color LightSteelBlue();
    static Color LightYellow();
    static Color Lime();
    static Color LimeGreen();
    static Color Linen();
    static Color Magenta();
    static Color Maroon();
    static Color MediumAquamarine();
    static Color MediumBlue();
    static Color MediumOrchid();
    static Color MediumPurple();
    static Color MediumSeaGreen();
    static Color MediumSlateBlue();
    static Color MediumSpringGreen();
    static Color MediumTurquoise();
    static Color MediumVioletRed();
    static Color MidnightBlue();
    static Color MintCream();
    static Color MistyRose();
    static Color Moccasin();
    static Color NavajoWhite();
    static Color Navy();
    static Color OldLace();
    static Color Olive();
    static Color OliveDrab();
    static Color Orange();
    static Color OrangeRed();
    static Color Orchid();
    static Color PaleGoldenrod();
    static Color PaleGreen();
    static Color PaleTurquoise();
    static Color PaleVioletRed();
    static Color PapayaWhip();
    static Color PeachPuff();
    static Color Peru();
    static Color Pink();
    static Color Plum();
    static Color PowderBlue();
    static Color Purple();
    static Color Red();
    static Color RosyBrown();
    static Color RoyalBlue();
    static Color SaddleBrown();
    static Color Salmon();
    static Color SandyBrown();
    static Color SeaGreen();
    static Color SeaShell();
    static Color Sienna();
    static Color Silver();
    static Color SkyBlue();
    static Color SlateBlue();
    static Color SlateGray();
    static Color Snow();
    static Color SpringGreen();
    static Color SteelBlue();
    static Color Tan();
    static Color Teal();
    static Color Thistle();
    static Color Tomato();
    static Color Turquoise();
    static Color Violet();
    static Color Wheat();
    static Color White();
    static Color WhiteSmoke();
    static Color Yellow();
    static Color YellowGreen();

private:
    // Save-through reads the RGB components to serialise annotation /C and to
    // paint generated /AP appearance streams in the authored colour.
    friend class Aspose::Pdf::Document;
    double a_ = 0.0;
    double r_ = 0.0;
    double g_ = 0.0;
    double b_ = 0.0;
};

}  // namespace Aspose::Pdf
