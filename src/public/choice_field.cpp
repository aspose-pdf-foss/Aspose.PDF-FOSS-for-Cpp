#include <aspose/pdf/forms/choice_field.hpp>

#include <memory>
#include <utility>

namespace Aspose::Pdf::Forms {

ChoiceField::ChoiceField(Aspose::Pdf::Page& /*page*/,
                          Aspose::Pdf::Rectangle rect) {
    Rect(std::move(rect));
}

ChoiceField::ChoiceField(Aspose::Pdf::Document& doc)
    : Field(doc) {}

ChoiceField::ChoiceField(Aspose::Pdf::Document& doc,
                          Aspose::Pdf::Rectangle rect)
    : Field(doc) {
    Rect(std::move(rect));
}

void ChoiceField::AddOption(const std::string& optionName) {
    Option o;
    o.Name(optionName);
    o.Value(optionName);
    Options().Add(std::move(o));
}

void ChoiceField::AddOption(const std::string& exportName,
                             const std::string& name) {
    Option o;
    o.Value(exportName);
    o.Name(name);
    Options().Add(std::move(o));
}

void ChoiceField::DeleteOption(const std::string& optionName) {
    if (!options_) return;
    for (int i = 0; i < options_->Count(); ++i) {
        if ((*options_)[i].Name() == optionName) {
            Option dummy;
            dummy.Name(optionName);
            dummy.Value((*options_)[i].Value());
            options_->Remove(dummy);
            return;
        }
    }
}

bool ChoiceField::CommitImmediately() const noexcept {
    return commit_immediately_;
}
void ChoiceField::CommitImmediately(bool v) noexcept {
    commit_immediately_ = v;
}

bool ChoiceField::MultiSelect() const noexcept { return multi_select_; }
void ChoiceField::MultiSelect(bool v) noexcept { multi_select_ = v; }

int ChoiceField::Selected() const noexcept { return selected_; }
void ChoiceField::Selected(int v) noexcept { selected_ = v; }

const std::vector<int>& ChoiceField::SelectedItems() const noexcept {
    return selected_items_;
}
void ChoiceField::SelectedItems(std::vector<int> v) noexcept {
    selected_items_ = std::move(v);
}

OptionCollection& ChoiceField::Options() {
    if (!options_) {
        options_ = std::make_unique<OptionCollection>();
    }
    return *options_;
}

const std::string& ChoiceField::Value() const noexcept { return value_; }
void ChoiceField::Value(std::string v) { value_ = std::move(v); }

}  // namespace Aspose::Pdf::Forms
