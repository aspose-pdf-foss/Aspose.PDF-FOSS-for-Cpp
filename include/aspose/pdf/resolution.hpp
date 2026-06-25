#pragma once

// =============================================================================
// Aspose::Pdf::Devices::Resolution — output resolution in DPI.
//
// Sealed class on the .NET side; carries X (horizontal) and Y (vertical)
// DPI as 32-bit signed integers, both read-write. The single-arg ctor
// sets both axes to the same value.
// =============================================================================

namespace Aspose::Pdf::Devices {

class Resolution {
public:
    explicit Resolution(int value);
    Resolution(int valueX, int valueY);

    int X() const noexcept;
    void X(int value) noexcept;

    int Y() const noexcept;
    void Y(int value) noexcept;

private:
    int x_;
    int y_;
};

}
