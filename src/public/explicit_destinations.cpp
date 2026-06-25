// Bodies for the typed ExplicitDestination family — the eight fit-mode
// destinations (XYZ / Fit / FitH / FitV / FitR / FitB / FitBH / FitBV) that
// mirror the canonical Aspose.Pdf.Annotations.*ExplicitDestination classes.
// Each derives from ExplicitDestination (which carries the 1-based target
// PageNumber) and adds its mode-specific coordinates, a ToString() that emits
// the PDF /D-array verb form, and a CloneAppointment() that copies the derived
// type for IAppointment-typed properties (e.g. GoToAction::Destination).

#include <aspose/pdf/annotations/fit_b_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_bh_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_bv_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_h_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_r_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_v_explicit_destination.hpp>
#include <aspose/pdf/annotations/xyz_explicit_destination.hpp>
#include <aspose/pdf/page.hpp>

#include <memory>
#include <sstream>
#include <string>

namespace Aspose::Pdf::Annotations {

namespace {

// Compact numeric form for the /D array (no trailing zeros).
std::string Num(double v) {
    std::ostringstream os;
    os << v;
    return os.str();
}

int PageOf(Aspose::Pdf::Page& page) {
    return static_cast<int>(page.Number());
}

}  // namespace

// ---- XYZExplicitDestination ------------------------------------------------

XYZExplicitDestination::XYZExplicitDestination(Aspose::Pdf::Page& page,
                                               double left, double top,
                                               double zoom)
    : ExplicitDestination(PageOf(page)), left_(left), top_(top), zoom_(zoom) {}

XYZExplicitDestination::XYZExplicitDestination(Aspose::Pdf::Document& /*document*/,
                                               int page_number, double left,
                                               double top, double zoom)
    : ExplicitDestination(page_number), left_(left), top_(top), zoom_(zoom) {}

XYZExplicitDestination::XYZExplicitDestination(int page_number, double left,
                                               double top, double zoom)
    : ExplicitDestination(page_number), left_(left), top_(top), zoom_(zoom) {}

double XYZExplicitDestination::Left() const noexcept { return left_; }
double XYZExplicitDestination::Top() const noexcept { return top_; }
double XYZExplicitDestination::Zoom() const noexcept { return zoom_; }

std::string XYZExplicitDestination::ToString() const {
    return std::to_string(PageNumber()) + " XYZ " + Num(left_) + " " +
           Num(top_) + " " + Num(zoom_);
}

std::unique_ptr<IAppointment> XYZExplicitDestination::CloneAppointment() const {
    return std::unique_ptr<IAppointment>(
        new XYZExplicitDestination(PageNumber(), left_, top_, zoom_));
}

// ---- FitExplicitDestination ------------------------------------------------

FitExplicitDestination::FitExplicitDestination(Aspose::Pdf::Page& page)
    : ExplicitDestination(PageOf(page)) {}

FitExplicitDestination::FitExplicitDestination(Aspose::Pdf::Document& /*document*/,
                                               int page_number)
    : ExplicitDestination(page_number) {}

FitExplicitDestination::FitExplicitDestination(int page_number)
    : ExplicitDestination(page_number) {}

std::string FitExplicitDestination::ToString() const {
    return std::to_string(PageNumber()) + " Fit";
}

std::unique_ptr<IAppointment> FitExplicitDestination::CloneAppointment() const {
    return std::unique_ptr<IAppointment>(
        new FitExplicitDestination(PageNumber()));
}

// ---- FitHExplicitDestination -----------------------------------------------

FitHExplicitDestination::FitHExplicitDestination(Aspose::Pdf::Page& page,
                                                 double top)
    : ExplicitDestination(PageOf(page)), top_(top) {}

FitHExplicitDestination::FitHExplicitDestination(Aspose::Pdf::Document& /*document*/,
                                                 int page_number, double top)
    : ExplicitDestination(page_number), top_(top) {}

FitHExplicitDestination::FitHExplicitDestination(int page_number, double top)
    : ExplicitDestination(page_number), top_(top) {}

double FitHExplicitDestination::Top() const noexcept { return top_; }

std::string FitHExplicitDestination::ToString() const {
    return std::to_string(PageNumber()) + " FitH " + Num(top_);
}

std::unique_ptr<IAppointment> FitHExplicitDestination::CloneAppointment() const {
    return std::unique_ptr<IAppointment>(
        new FitHExplicitDestination(PageNumber(), top_));
}

// ---- FitVExplicitDestination -----------------------------------------------

FitVExplicitDestination::FitVExplicitDestination(Aspose::Pdf::Page& page,
                                                 double left)
    : ExplicitDestination(PageOf(page)), left_(left) {}

FitVExplicitDestination::FitVExplicitDestination(Aspose::Pdf::Document& /*document*/,
                                                 int page_number, double left)
    : ExplicitDestination(page_number), left_(left) {}

FitVExplicitDestination::FitVExplicitDestination(int page_number, double left)
    : ExplicitDestination(page_number), left_(left) {}

double FitVExplicitDestination::Left() const noexcept { return left_; }

std::string FitVExplicitDestination::ToString() const {
    return std::to_string(PageNumber()) + " FitV " + Num(left_);
}

std::unique_ptr<IAppointment> FitVExplicitDestination::CloneAppointment() const {
    return std::unique_ptr<IAppointment>(
        new FitVExplicitDestination(PageNumber(), left_));
}

// ---- FitRExplicitDestination -----------------------------------------------

FitRExplicitDestination::FitRExplicitDestination(Aspose::Pdf::Page& page,
                                                 double left, double bottom,
                                                 double right, double top)
    : ExplicitDestination(PageOf(page)),
      left_(left), bottom_(bottom), right_(right), top_(top) {}

FitRExplicitDestination::FitRExplicitDestination(Aspose::Pdf::Document& /*document*/,
                                                 int page_number, double left,
                                                 double bottom, double right,
                                                 double top)
    : ExplicitDestination(page_number),
      left_(left), bottom_(bottom), right_(right), top_(top) {}

FitRExplicitDestination::FitRExplicitDestination(int page_number, double left,
                                                 double bottom, double right,
                                                 double top)
    : ExplicitDestination(page_number),
      left_(left), bottom_(bottom), right_(right), top_(top) {}

double FitRExplicitDestination::Left() const noexcept { return left_; }
double FitRExplicitDestination::Bottom() const noexcept { return bottom_; }
double FitRExplicitDestination::Right() const noexcept { return right_; }
double FitRExplicitDestination::Top() const noexcept { return top_; }

std::string FitRExplicitDestination::ToString() const {
    return std::to_string(PageNumber()) + " FitR " + Num(left_) + " " +
           Num(bottom_) + " " + Num(right_) + " " + Num(top_);
}

std::unique_ptr<IAppointment> FitRExplicitDestination::CloneAppointment() const {
    return std::unique_ptr<IAppointment>(new FitRExplicitDestination(
        PageNumber(), left_, bottom_, right_, top_));
}

// ---- FitBExplicitDestination -----------------------------------------------

FitBExplicitDestination::FitBExplicitDestination(Aspose::Pdf::Page& page)
    : ExplicitDestination(PageOf(page)) {}

FitBExplicitDestination::FitBExplicitDestination(Aspose::Pdf::Document& /*document*/,
                                                 int page_number)
    : ExplicitDestination(page_number) {}

FitBExplicitDestination::FitBExplicitDestination(int page_number)
    : ExplicitDestination(page_number) {}

std::string FitBExplicitDestination::ToString() const {
    return std::to_string(PageNumber()) + " FitB";
}

std::unique_ptr<IAppointment> FitBExplicitDestination::CloneAppointment() const {
    return std::unique_ptr<IAppointment>(
        new FitBExplicitDestination(PageNumber()));
}

// ---- FitBHExplicitDestination ----------------------------------------------

FitBHExplicitDestination::FitBHExplicitDestination(Aspose::Pdf::Page& page,
                                                   double top)
    : ExplicitDestination(PageOf(page)), top_(top) {}

FitBHExplicitDestination::FitBHExplicitDestination(Aspose::Pdf::Document& /*document*/,
                                                   int page_number, double top)
    : ExplicitDestination(page_number), top_(top) {}

FitBHExplicitDestination::FitBHExplicitDestination(int page_number, double top)
    : ExplicitDestination(page_number), top_(top) {}

double FitBHExplicitDestination::Top() const noexcept { return top_; }

std::string FitBHExplicitDestination::ToString() const {
    return std::to_string(PageNumber()) + " FitBH " + Num(top_);
}

std::unique_ptr<IAppointment> FitBHExplicitDestination::CloneAppointment() const {
    return std::unique_ptr<IAppointment>(
        new FitBHExplicitDestination(PageNumber(), top_));
}

// ---- FitBVExplicitDestination ----------------------------------------------

FitBVExplicitDestination::FitBVExplicitDestination(Aspose::Pdf::Page& page,
                                                   double left)
    : ExplicitDestination(PageOf(page)), left_(left) {}

FitBVExplicitDestination::FitBVExplicitDestination(Aspose::Pdf::Document& /*document*/,
                                                   int page_number, double left)
    : ExplicitDestination(page_number), left_(left) {}

FitBVExplicitDestination::FitBVExplicitDestination(int page_number, double left)
    : ExplicitDestination(page_number), left_(left) {}

double FitBVExplicitDestination::Left() const noexcept { return left_; }

std::string FitBVExplicitDestination::ToString() const {
    return std::to_string(PageNumber()) + " FitBV " + Num(left_);
}

std::unique_ptr<IAppointment> FitBVExplicitDestination::CloneAppointment() const {
    return std::unique_ptr<IAppointment>(
        new FitBVExplicitDestination(PageNumber(), left_));
}

}  // namespace Aspose::Pdf::Annotations
