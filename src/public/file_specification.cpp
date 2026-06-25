#include <aspose/pdf/file_specification.hpp>

#include <utility>

namespace Aspose::Pdf {

FileSpecification::FileSpecification(std::string file)
    : file_(std::move(file)),
      name_(file_),
      unicode_name_(file_) {}

FileSpecification::FileSpecification(std::string file,
                                     std::string description)
    : file_(std::move(file)),
      description_(std::move(description)),
      name_(file_),
      unicode_name_(file_) {}

FileSpecification::FileSpecification(std::string fileName,
                                     Aspose::Pdf::Annotations::Annotation& annot)
    : file_(std::move(fileName)),
      name_(file_),
      unicode_name_(file_),
      annot_(&annot) {}

void FileSpecification::Dispose() {
    file_.clear();
    description_.clear();
    mime_type_.clear();
    name_.clear();
    unicode_name_.clear();
    file_system_.clear();
    ci_values_.clear();
    encoding_ = Aspose::Pdf::FileEncoding::None;
    af_relationship_ = Aspose::Pdf::AFRelationship::Unspecified;
    include_contents_ = true;
    annot_ = nullptr;
}

std::string FileSpecification::GetValue(const std::string& key) const {
    auto it = ci_values_.find(key);
    return it == ci_values_.end() ? std::string{} : it->second;
}
void FileSpecification::SetValue(const std::string& key,
                                  std::string value) {
    ci_values_[key] = std::move(value);
}

Aspose::Pdf::FileEncoding FileSpecification::Encoding() const noexcept {
    return encoding_;
}
void FileSpecification::Encoding(Aspose::Pdf::FileEncoding v) noexcept {
    encoding_ = v;
}

bool FileSpecification::IncludeContents() const noexcept {
    return include_contents_;
}
void FileSpecification::IncludeContents(bool v) noexcept {
    include_contents_ = v;
}

const std::string& FileSpecification::Description() const noexcept {
    return description_;
}
void FileSpecification::Description(std::string v) noexcept {
    description_ = std::move(v);
}

Aspose::Pdf::AFRelationship FileSpecification::AFRelationship() const noexcept {
    return af_relationship_;
}
void FileSpecification::AFRelationship(Aspose::Pdf::AFRelationship v) noexcept {
    af_relationship_ = v;
}

const std::string& FileSpecification::MIMEType() const noexcept {
    return mime_type_;
}
void FileSpecification::MIMEType(std::string v) noexcept {
    mime_type_ = std::move(v);
}

const std::string& FileSpecification::Name() const noexcept {
    return name_;
}
void FileSpecification::Name(std::string v) noexcept {
    name_ = std::move(v);
}

const std::string& FileSpecification::UnicodeName() const noexcept {
    return unicode_name_;
}
void FileSpecification::UnicodeName(std::string v) noexcept {
    unicode_name_ = std::move(v);
}

const std::string& FileSpecification::FileSystem() const noexcept {
    return file_system_;
}
void FileSpecification::FileSystem(std::string v) noexcept {
    file_system_ = std::move(v);
}

}  // namespace Aspose::Pdf
