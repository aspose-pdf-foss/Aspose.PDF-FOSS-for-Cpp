#include <aspose/pdf/color.hpp>

namespace Aspose::Pdf {

// =============================================================================
// Integer / double factories.
// =============================================================================

Color Color::FromArgb(int a, int r, int g, int b) noexcept {
    Color c;
    c.a_ = static_cast<double>(a) / 255.0;
    c.r_ = static_cast<double>(r) / 255.0;
    c.g_ = static_cast<double>(g) / 255.0;
    c.b_ = static_cast<double>(b) / 255.0;
    return c;
}

Color Color::FromArgb(int r, int g, int b) noexcept {
    return FromArgb(255, r, g, b);
}

Color Color::FromRgb(double r, double g, double b) noexcept {
    Color c;
    c.a_ = 1.0;
    c.r_ = r;
    c.g_ = g;
    c.b_ = b;
    return c;
}

double Color::A() const noexcept { return a_; }

// =============================================================================
// Empty / Transparent statics.
// =============================================================================

Color Color::Empty() { return Color{}; }

Color Color::Transparent() { return FromArgb(0, 255, 255, 255); }

// =============================================================================
// Named-color presets (X11 / CSS — RGB hex values per the W3C
// CSS Color Module Level 4 named-color list).
// =============================================================================

namespace {
// Pack a 24-bit RGB into the FromArgb(255, r, g, b) call. The
// hex form keeps the table compact + greppable against the spec.
inline Color Hex(unsigned int rgb) noexcept {
    return Color::FromArgb(
        static_cast<int>((rgb >> 16) & 0xFF),
        static_cast<int>((rgb >> 8) & 0xFF),
        static_cast<int>(rgb & 0xFF));
}
}  // namespace

Color Color::AliceBlue()            { return Hex(0xF0F8FF); }
Color Color::AntiqueWhite()         { return Hex(0xFAEBD7); }
Color Color::Aqua()                 { return Hex(0x00FFFF); }
Color Color::Aquamarine()           { return Hex(0x7FFFD4); }
Color Color::Azure()                { return Hex(0xF0FFFF); }
Color Color::Beige()                { return Hex(0xF5F5DC); }
Color Color::Bisque()               { return Hex(0xFFE4C4); }
Color Color::Black()                { return Hex(0x000000); }
Color Color::BlanchedAlmond()       { return Hex(0xFFEBCD); }
Color Color::Blue()                 { return Hex(0x0000FF); }
Color Color::BlueViolet()           { return Hex(0x8A2BE2); }
Color Color::Brown()                { return Hex(0xA52A2A); }
Color Color::BurlyWood()            { return Hex(0xDEB887); }
Color Color::CadetBlue()            { return Hex(0x5F9EA0); }
Color Color::Chartreuse()           { return Hex(0x7FFF00); }
Color Color::Chocolate()            { return Hex(0xD2691E); }
Color Color::Coral()                { return Hex(0xFF7F50); }
Color Color::CornflowerBlue()       { return Hex(0x6495ED); }
Color Color::Cornsilk()             { return Hex(0xFFF8DC); }
Color Color::Crimson()              { return Hex(0xDC143C); }
Color Color::Cyan()                 { return Hex(0x00FFFF); }
Color Color::DarkBlue()             { return Hex(0x00008B); }
Color Color::DarkCyan()             { return Hex(0x008B8B); }
Color Color::DarkGoldenrod()        { return Hex(0xB8860B); }
Color Color::DarkGray()             { return Hex(0xA9A9A9); }
Color Color::DarkGreen()            { return Hex(0x006400); }
Color Color::DarkKhaki()            { return Hex(0xBDB76B); }
Color Color::DarkMagenta()          { return Hex(0x8B008B); }
Color Color::DarkOliveGreen()       { return Hex(0x556B2F); }
Color Color::DarkOrange()           { return Hex(0xFF8C00); }
Color Color::DarkOrchid()           { return Hex(0x9932CC); }
Color Color::DarkRed()              { return Hex(0x8B0000); }
Color Color::DarkSalmon()           { return Hex(0xE9967A); }
Color Color::DarkSeaGreen()         { return Hex(0x8FBC8F); }
Color Color::DarkSlateBlue()        { return Hex(0x483D8B); }
Color Color::DarkSlateGray()        { return Hex(0x2F4F4F); }
Color Color::DarkTurquoise()        { return Hex(0x00CED1); }
Color Color::DarkViolet()           { return Hex(0x9400D3); }
Color Color::DeepPink()             { return Hex(0xFF1493); }
Color Color::DeepSkyBlue()          { return Hex(0x00BFFF); }
Color Color::DimGray()              { return Hex(0x696969); }
Color Color::DodgerBlue()           { return Hex(0x1E90FF); }
Color Color::Firebrick()            { return Hex(0xB22222); }
Color Color::FloralWhite()          { return Hex(0xFFFAF0); }
Color Color::ForestGreen()          { return Hex(0x228B22); }
Color Color::Fuchsia()              { return Hex(0xFF00FF); }
Color Color::Gainsboro()            { return Hex(0xDCDCDC); }
Color Color::GhostWhite()           { return Hex(0xF8F8FF); }
Color Color::Gold()                 { return Hex(0xFFD700); }
Color Color::Goldenrod()            { return Hex(0xDAA520); }
Color Color::Gray()                 { return Hex(0x808080); }
Color Color::Green()                { return Hex(0x008000); }
Color Color::GreenYellow()          { return Hex(0xADFF2F); }
Color Color::Honeydew()             { return Hex(0xF0FFF0); }
Color Color::HotPink()              { return Hex(0xFF69B4); }
Color Color::IndianRed()            { return Hex(0xCD5C5C); }
Color Color::Indigo()               { return Hex(0x4B0082); }
Color Color::Ivory()                { return Hex(0xFFFFF0); }
Color Color::Khaki()                { return Hex(0xF0E68C); }
Color Color::Lavender()             { return Hex(0xE6E6FA); }
Color Color::LavenderBlush()        { return Hex(0xFFF0F5); }
Color Color::LawnGreen()            { return Hex(0x7CFC00); }
Color Color::LemonChiffon()         { return Hex(0xFFFACD); }
Color Color::LightBlue()            { return Hex(0xADD8E6); }
Color Color::LightCoral()           { return Hex(0xF08080); }
Color Color::LightCyan()            { return Hex(0xE0FFFF); }
Color Color::LightGoldenrodYellow() { return Hex(0xFAFAD2); }
Color Color::LightGreen()           { return Hex(0x90EE90); }
Color Color::LightGray()            { return Hex(0xD3D3D3); }
Color Color::LightPink()            { return Hex(0xFFB6C1); }
Color Color::LightSalmon()          { return Hex(0xFFA07A); }
Color Color::LightSeaGreen()        { return Hex(0x20B2AA); }
Color Color::LightSkyBlue()         { return Hex(0x87CEFA); }
Color Color::LightSlateGray()       { return Hex(0x778899); }
Color Color::LightSteelBlue()       { return Hex(0xB0C4DE); }
Color Color::LightYellow()          { return Hex(0xFFFFE0); }
Color Color::Lime()                 { return Hex(0x00FF00); }
Color Color::LimeGreen()            { return Hex(0x32CD32); }
Color Color::Linen()                { return Hex(0xFAF0E6); }
Color Color::Magenta()              { return Hex(0xFF00FF); }
Color Color::Maroon()               { return Hex(0x800000); }
Color Color::MediumAquamarine()     { return Hex(0x66CDAA); }
Color Color::MediumBlue()           { return Hex(0x0000CD); }
Color Color::MediumOrchid()         { return Hex(0xBA55D3); }
Color Color::MediumPurple()         { return Hex(0x9370DB); }
Color Color::MediumSeaGreen()       { return Hex(0x3CB371); }
Color Color::MediumSlateBlue()      { return Hex(0x7B68EE); }
Color Color::MediumSpringGreen()    { return Hex(0x00FA9A); }
Color Color::MediumTurquoise()      { return Hex(0x48D1CC); }
Color Color::MediumVioletRed()      { return Hex(0xC71585); }
Color Color::MidnightBlue()         { return Hex(0x191970); }
Color Color::MintCream()            { return Hex(0xF5FFFA); }
Color Color::MistyRose()            { return Hex(0xFFE4E1); }
Color Color::Moccasin()             { return Hex(0xFFE4B5); }
Color Color::NavajoWhite()          { return Hex(0xFFDEAD); }
Color Color::Navy()                 { return Hex(0x000080); }
Color Color::OldLace()              { return Hex(0xFDF5E6); }
Color Color::Olive()                { return Hex(0x808000); }
Color Color::OliveDrab()            { return Hex(0x6B8E23); }
Color Color::Orange()               { return Hex(0xFFA500); }
Color Color::OrangeRed()            { return Hex(0xFF4500); }
Color Color::Orchid()               { return Hex(0xDA70D6); }
Color Color::PaleGoldenrod()        { return Hex(0xEEE8AA); }
Color Color::PaleGreen()            { return Hex(0x98FB98); }
Color Color::PaleTurquoise()        { return Hex(0xAFEEEE); }
Color Color::PaleVioletRed()        { return Hex(0xDB7093); }
Color Color::PapayaWhip()           { return Hex(0xFFEFD5); }
Color Color::PeachPuff()            { return Hex(0xFFDAB9); }
Color Color::Peru()                 { return Hex(0xCD853F); }
Color Color::Pink()                 { return Hex(0xFFC0CB); }
Color Color::Plum()                 { return Hex(0xDDA0DD); }
Color Color::PowderBlue()           { return Hex(0xB0E0E6); }
Color Color::Purple()               { return Hex(0x800080); }
Color Color::Red()                  { return Hex(0xFF0000); }
Color Color::RosyBrown()            { return Hex(0xBC8F8F); }
Color Color::RoyalBlue()            { return Hex(0x4169E1); }
Color Color::SaddleBrown()          { return Hex(0x8B4513); }
Color Color::Salmon()               { return Hex(0xFA8072); }
Color Color::SandyBrown()           { return Hex(0xF4A460); }
Color Color::SeaGreen()             { return Hex(0x2E8B57); }
Color Color::SeaShell()             { return Hex(0xFFF5EE); }
Color Color::Sienna()               { return Hex(0xA0522D); }
Color Color::Silver()               { return Hex(0xC0C0C0); }
Color Color::SkyBlue()              { return Hex(0x87CEEB); }
Color Color::SlateBlue()            { return Hex(0x6A5ACD); }
Color Color::SlateGray()            { return Hex(0x708090); }
Color Color::Snow()                 { return Hex(0xFFFAFA); }
Color Color::SpringGreen()          { return Hex(0x00FF7F); }
Color Color::SteelBlue()            { return Hex(0x4682B4); }
Color Color::Tan()                  { return Hex(0xD2B48C); }
Color Color::Teal()                 { return Hex(0x008080); }
Color Color::Thistle()              { return Hex(0xD8BFD8); }
Color Color::Tomato()               { return Hex(0xFF6347); }
Color Color::Turquoise()            { return Hex(0x40E0D0); }
Color Color::Violet()               { return Hex(0xEE82EE); }
Color Color::Wheat()                { return Hex(0xF5DEB3); }
Color Color::White()                { return Hex(0xFFFFFF); }
Color Color::WhiteSmoke()           { return Hex(0xF5F5F5); }
Color Color::Yellow()               { return Hex(0xFFFF00); }
Color Color::YellowGreen()          { return Hex(0x9ACD32); }

}  // namespace Aspose::Pdf
