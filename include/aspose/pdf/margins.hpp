#pragma once

// =============================================================================
// Aspose::Pdf::Devices::Margins — left/right/top/bottom inset values
// (pixels, Int32) consumed by TiffSettings.
// =============================================================================

namespace Aspose::Pdf::Devices {

class Margins {
public:
    Margins();
    Margins(int left, int right, int top, int bottom);

    int Left() const noexcept;
    void Left(int value) noexcept;

    int Right() const noexcept;
    void Right(int value) noexcept;

    int Top() const noexcept;
    void Top(int value) noexcept;

    int Bottom() const noexcept;
    void Bottom(int value) noexcept;

private:
    int left_ = 0;
    int right_ = 0;
    int top_ = 0;
    int bottom_ = 0;
};

}
