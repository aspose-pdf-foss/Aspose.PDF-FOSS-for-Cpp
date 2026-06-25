#include <aspose/pdf/page_size.hpp>

namespace Aspose::Pdf {

PageSize::PageSize(float x, float y) : width_(x), height_(y) {}

PageSize PageSize::A0()         { return PageSize(2384.0f, 3370.0f); }
PageSize PageSize::A1()         { return PageSize(1684.0f, 2384.0f); }
PageSize PageSize::A2()         { return PageSize(1190.0f, 1684.0f); }
PageSize PageSize::A3()         { return PageSize( 842.0f, 1190.0f); }
PageSize PageSize::A4()         { return PageSize( 595.0f,  842.0f); }
PageSize PageSize::A5()         { return PageSize( 421.0f,  595.0f); }
PageSize PageSize::A6()         { return PageSize( 297.0f,  421.0f); }
PageSize PageSize::B5()         { return PageSize( 501.0f,  709.0f); }
PageSize PageSize::PageLetter() { return PageSize( 612.0f,  792.0f); }
PageSize PageSize::PageLegal()  { return PageSize( 612.0f, 1008.0f); }
PageSize PageSize::PageLedger() { return PageSize(1224.0f,  792.0f); }
PageSize PageSize::P11x17()     { return PageSize( 792.0f, 1224.0f); }

float PageSize::Width()  const noexcept { return width_; }
void  PageSize::Width(float value)  noexcept { width_  = value; }

float PageSize::Height() const noexcept { return height_; }
void  PageSize::Height(float value) noexcept { height_ = value; }

bool  PageSize::IsLandscape() const noexcept { return width_ > height_; }
void  PageSize::IsLandscape(bool value) noexcept {
    const bool current = width_ > height_;
    if (current != value) {
        const float tmp = width_;
        width_ = height_;
        height_ = tmp;
    }
}

}  // namespace Aspose::Pdf
