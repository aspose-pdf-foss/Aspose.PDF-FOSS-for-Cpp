// Bodies for the PdfAction navigation cascade: PdfAction (abstract base),
// GoToURIAction, GoToAction and ExplicitDestination. Mirrors the canonical
// Aspose.Pdf.Annotations action hierarchy.

#include <aspose/pdf/annotations/explicit_destination.hpp>
#include <aspose/pdf/annotations/explicit_destination_type.hpp>
#include <aspose/pdf/annotations/fit_b_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_bh_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_bv_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_h_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_r_explicit_destination.hpp>
#include <aspose/pdf/annotations/fit_v_explicit_destination.hpp>
#include <aspose/pdf/annotations/go_to_action.hpp>
#include <aspose/pdf/annotations/go_to_uri_action.hpp>
#include <aspose/pdf/annotations/i_appointment.hpp>
#include <aspose/pdf/annotations/javascript_action.hpp>
#include <aspose/pdf/annotations/named_action.hpp>
#include <aspose/pdf/annotations/pdf_action.hpp>
#include <aspose/pdf/annotations/submit_form_action.hpp>
#include <aspose/pdf/annotations/xyz_explicit_destination.hpp>
#include <aspose/pdf/page.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Aspose::Pdf::Annotations {

// ---- PdfAction (abstract base) ---------------------------------------------

std::string PdfAction::GetECMAScriptString() const { return std::string(); }

std::string PdfAction::ToString() const { return std::string(); }

std::unique_ptr<IAppointment> PdfAction::CloneAppointment() const {
    return CloneAction();
}

// ---- GoToURIAction ---------------------------------------------------------

GoToURIAction::GoToURIAction(std::string uri) : uri_(std::move(uri)) {}

const std::string& GoToURIAction::URI() const noexcept { return uri_; }

void GoToURIAction::URI(std::string value) { uri_ = std::move(value); }

std::string GoToURIAction::GetECMAScriptString() const { return std::string(); }

std::string GoToURIAction::ToString() const { return uri_; }

std::unique_ptr<PdfAction> GoToURIAction::CloneAction() const {
    return std::make_unique<GoToURIAction>(uri_);
}

// ---- ExplicitDestination ---------------------------------------------------

ExplicitDestination::ExplicitDestination(int page_number) noexcept
    : page_number_(page_number) {}

int ExplicitDestination::PageNumber() const noexcept { return page_number_; }

std::string ExplicitDestination::ToString() const {
    return std::to_string(page_number_);
}

std::unique_ptr<IAppointment> ExplicitDestination::CloneAppointment() const {
    return std::unique_ptr<IAppointment>(new ExplicitDestination(page_number_));
}

// ---- GoToAction ------------------------------------------------------------

GoToAction::GoToAction() = default;

GoToAction::GoToAction(int page)
    : destination_(std::unique_ptr<IAppointment>(new ExplicitDestination(page))) {}

GoToAction::GoToAction(Aspose::Pdf::Page& page)
    : destination_(std::unique_ptr<IAppointment>(
          new ExplicitDestination(static_cast<int>(page.Number())))) {}

namespace {

// Operand-or-zero — the /D verb operands are positional and may be short.
double Operand(const std::vector<double>& v, std::size_t i) {
    return i < v.size() ? v[i] : 0.0;
}

// Build the typed destination for a fit-mode verb + its coordinate operands.
std::unique_ptr<IAppointment> MakeTypedDestination(
    Aspose::Pdf::Page& page, ExplicitDestinationType type,
    const std::vector<double>& v) {
    switch (type) {
        case ExplicitDestinationType::XYZ:
            return std::make_unique<XYZExplicitDestination>(
                page, Operand(v, 0), Operand(v, 1), Operand(v, 2));
        case ExplicitDestinationType::FitH:
            return std::make_unique<FitHExplicitDestination>(page,
                                                             Operand(v, 0));
        case ExplicitDestinationType::FitV:
            return std::make_unique<FitVExplicitDestination>(page,
                                                             Operand(v, 0));
        case ExplicitDestinationType::FitR:
            return std::make_unique<FitRExplicitDestination>(
                page, Operand(v, 0), Operand(v, 1), Operand(v, 2),
                Operand(v, 3));
        case ExplicitDestinationType::FitBH:
            return std::make_unique<FitBHExplicitDestination>(page,
                                                              Operand(v, 0));
        case ExplicitDestinationType::FitBV:
            return std::make_unique<FitBVExplicitDestination>(page,
                                                              Operand(v, 0));
        case ExplicitDestinationType::FitB:
            return std::make_unique<FitBExplicitDestination>(page);
        case ExplicitDestinationType::Fit:
        default:
            return std::make_unique<FitExplicitDestination>(page);
    }
}

}  // namespace

GoToAction::GoToAction(Aspose::Pdf::Page& page, ExplicitDestinationType type,
                       const std::vector<double>& values)
    : destination_(MakeTypedDestination(page, type, values)) {}

GoToAction::GoToAction(const ExplicitDestination& destination)
    : destination_(destination.CloneAppointment()) {}

GoToAction::GoToAction(Aspose::Pdf::Document& /*doc*/, std::string name)
    : named_destination_(std::move(name)) {}

IAppointment* GoToAction::Destination() const noexcept {
    return destination_.get();
}

void GoToAction::Destination(const IAppointment& value) {
    destination_ = value.CloneAppointment();
}

std::string GoToAction::GetECMAScriptString() const { return std::string(); }

std::string GoToAction::ToString() const {
    if (destination_) return destination_->ToString();
    return named_destination_;
}

std::unique_ptr<PdfAction> GoToAction::CloneAction() const {
    auto copy = std::make_unique<GoToAction>();
    if (destination_) copy->destination_ = destination_->CloneAppointment();
    copy->named_destination_ = named_destination_;
    return copy;
}

// ---- NamedAction -----------------------------------------------------------

namespace {

// The PDF /N name for a predefined action. The four ISO-standard named
// actions map to their canonical names; Aspose's viewer-menu extensions have
// no PDF /N form and default to empty (set explicitly via Name()).
std::string PredefinedActionName(PredefinedAction action) {
    switch (action) {
        case PredefinedAction::FirstPage: return "FirstPage";
        case PredefinedAction::LastPage: return "LastPage";
        case PredefinedAction::NextPage: return "NextPage";
        case PredefinedAction::PrevPage: return "PrevPage";
        case PredefinedAction::Print:
        case PredefinedAction::PrintDialog: return "Print";
        default: return std::string();
    }
}

}  // namespace

NamedAction::NamedAction(PredefinedAction action)
    : name_(PredefinedActionName(action)) {}

const std::string& NamedAction::Name() const noexcept { return name_; }

void NamedAction::Name(std::string value) { name_ = std::move(value); }

std::string NamedAction::ToString() const { return name_; }

std::unique_ptr<PdfAction> NamedAction::CloneAction() const {
    auto copy = std::make_unique<NamedAction>(PredefinedAction::NextPage);
    copy->name_ = name_;
    return copy;
}

// ---- JavascriptAction ------------------------------------------------------

JavascriptAction::JavascriptAction(std::string javaScript)
    : script_(std::move(javaScript)) {}

const std::string& JavascriptAction::Script() const noexcept { return script_; }

void JavascriptAction::Script(std::string value) { script_ = std::move(value); }

std::string JavascriptAction::GetECMAScriptString() const { return script_; }

std::string JavascriptAction::ToString() const { return script_; }

std::unique_ptr<PdfAction> JavascriptAction::CloneAction() const {
    return std::make_unique<JavascriptAction>(script_);
}

// ---- SubmitFormAction ------------------------------------------------------

SubmitFormAction::SubmitFormAction() = default;

int SubmitFormAction::Flags() const noexcept { return flags_; }

void SubmitFormAction::Flags(int value) { flags_ = value; }

const Aspose::Pdf::FileSpecification& SubmitFormAction::Url() const noexcept {
    return url_;
}

void SubmitFormAction::Url(const Aspose::Pdf::FileSpecification& value) {
    url_ = value;
}

std::string SubmitFormAction::ToString() const { return url_.Name(); }

std::unique_ptr<PdfAction> SubmitFormAction::CloneAction() const {
    auto copy = std::make_unique<SubmitFormAction>();
    copy->flags_ = flags_;
    copy->url_ = url_;
    return copy;
}

}  // namespace Aspose::Pdf::Annotations
