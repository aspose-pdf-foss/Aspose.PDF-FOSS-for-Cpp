#pragma once

// =============================================================================
// Aspose::Pdf::PageSize — value-type carrier for page dimensions.
//
// DLL surface (Aspose.PDF 26.4.0): sealed class. Single explicit
// (x, y) ctor (parameters named `x` and `y` in the assembly), three
// read-write instance properties (Width/Height/IsLandscape), and
// twelve static read-only paper-size factories.
// =============================================================================

namespace Aspose::Pdf {

class PageSize {
public:
    PageSize(float x, float y);

    static PageSize A0();
    static PageSize A1();
    static PageSize A2();
    static PageSize A3();
    static PageSize A4();
    static PageSize A5();
    static PageSize A6();
    static PageSize B5();
    static PageSize PageLetter();
    static PageSize PageLegal();
    static PageSize PageLedger();
    static PageSize P11x17();

    float Width() const noexcept;
    void Width(float value) noexcept;

    float Height() const noexcept;
    void Height(float value) noexcept;

    bool IsLandscape() const noexcept;
    void IsLandscape(bool value) noexcept;

private:
    float width_;
    float height_;
};

}
