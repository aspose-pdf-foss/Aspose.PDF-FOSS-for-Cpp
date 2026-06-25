#include <aspose/pdf/facades/pdf_file_info.hpp>

#include <utility>

#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_info.hpp>
#include <aspose/pdf/page_collection.hpp>

namespace Aspose::Pdf::Facades {

PdfFileInfo::PdfFileInfo(const std::string& inputFile)
    : input_file_(inputFile) {
    BindPdf(inputFile);
}

PdfFileInfo::PdfFileInfo(const std::string& inputFile,
                          const std::string& /*password*/)
    : input_file_(inputFile) {
    // v1 stub: password-bound open lands when Document gains
    // password-validating ctor signature support on the file-path
    // overload (currently only string/path + bytes-based crypto
    // open via foundation::encrypt_writer).
    BindPdf(inputFile);
}

PdfFileInfo::PdfFileInfo(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

void PdfFileInfo::BindPdf(Aspose::Pdf::Document& srcDoc) {
    Facade::BindPdf(srcDoc);
}

void PdfFileInfo::ClearInfo() {
    if (auto* doc = Document(); doc != nullptr) {
        doc->Info().Clear();
    }
}

std::string PdfFileInfo::GetMetaInfo(const std::string& name) const {
    if (auto* doc = const_cast<Aspose::Pdf::Document*>(Document());
            doc != nullptr) {
        if (name == "Title")    return doc->Info().Title();
        if (name == "Author")   return doc->Info().Author();
        if (name == "Subject")  return doc->Info().Subject();
        if (name == "Keywords") return doc->Info().Keywords();
        if (name == "Creator")  return doc->Info().Creator();
        if (name == "Producer") return doc->Info().Producer();
        if (name == "Trapped")  return doc->Info().Trapped();
    }
    auto it = extra_meta_.find(name);
    return it == extra_meta_.end() ? std::string{} : it->second;
}

void PdfFileInfo::SetMetaInfo(const std::string& name,
                               const std::string& value) {
    if (auto* doc = Document(); doc != nullptr) {
        if (name == "Title")    { doc->Info().Title(value);    return; }
        if (name == "Author")   { doc->Info().Author(value);   return; }
        if (name == "Subject")  { doc->Info().Subject(value);  return; }
        if (name == "Keywords") { doc->Info().Keywords(value); return; }
        if (name == "Creator")  { doc->Info().Creator(value);  return; }
        if (name == "Trapped")  { doc->Info().Trapped(value);  return; }
    }
    extra_meta_[name] = value;
}

// v1 page-geometry stubs (canonical defaults: US Letter, no rotation).
float PdfFileInfo::GetPageHeight(int /*pageNum*/) const { return 792.0f; }
float PdfFileInfo::GetPageRotation(int /*pageNum*/) const { return 0.0f; }
float PdfFileInfo::GetPageWidth(int /*pageNum*/) const { return 612.0f; }
float PdfFileInfo::GetPageXOffset(int /*pageNum*/) const { return 0.0f; }
float PdfFileInfo::GetPageYOffset(int /*pageNum*/) const { return 0.0f; }

std::string PdfFileInfo::GetPdfVersion() const {
    // v1 returns canonical default. Real %PDF-x.y header parse
    // lands when Document exposes the parsed PDF version.
    return "1.4";
}

bool PdfFileInfo::SaveNewInfo(const std::string& outputFile) {
    auto* doc = Document();
    if (doc == nullptr) return false;
    doc->Save(outputFile);
    return true;
}

bool PdfFileInfo::SaveNewInfoWithXmp(const std::string& outputFileName) {
    return SaveNewInfo(outputFileName);
}

// ---- /Info string entries (route through DocumentInfo) ----

std::string PdfFileInfo::Author() const {
    if (const auto* doc = Document(); doc != nullptr) {
        return const_cast<Aspose::Pdf::Document*>(doc)->Info().Author();
    }
    return {};
}
void PdfFileInfo::Author(std::string v) {
    if (auto* doc = Document(); doc != nullptr) {
        doc->Info().Author(std::move(v));
    }
}

std::string PdfFileInfo::CreationDate() const {
    return GetMetaInfo("CreationDate");
}
void PdfFileInfo::CreationDate(std::string v) {
    SetMetaInfo("CreationDate", v);
}

std::string PdfFileInfo::Creator() const {
    if (const auto* doc = Document(); doc != nullptr) {
        return const_cast<Aspose::Pdf::Document*>(doc)->Info().Creator();
    }
    return {};
}
void PdfFileInfo::Creator(std::string v) {
    if (auto* doc = Document(); doc != nullptr) {
        doc->Info().Creator(std::move(v));
    }
}

std::string PdfFileInfo::Keywords() const {
    if (const auto* doc = Document(); doc != nullptr) {
        return const_cast<Aspose::Pdf::Document*>(doc)->Info().Keywords();
    }
    return {};
}
void PdfFileInfo::Keywords(std::string v) {
    if (auto* doc = Document(); doc != nullptr) {
        doc->Info().Keywords(std::move(v));
    }
}

std::string PdfFileInfo::ModDate() const { return GetMetaInfo("ModDate"); }
void PdfFileInfo::ModDate(std::string v) {
    SetMetaInfo("ModDate", v);
}

std::string PdfFileInfo::Producer() const {
    if (const auto* doc = Document(); doc != nullptr) {
        return const_cast<Aspose::Pdf::Document*>(doc)->Info().Producer();
    }
    return {};
}

std::string PdfFileInfo::Subject() const {
    if (const auto* doc = Document(); doc != nullptr) {
        return const_cast<Aspose::Pdf::Document*>(doc)->Info().Subject();
    }
    return {};
}
void PdfFileInfo::Subject(std::string v) {
    if (auto* doc = Document(); doc != nullptr) {
        doc->Info().Subject(std::move(v));
    }
}

std::string PdfFileInfo::Title() const {
    if (const auto* doc = Document(); doc != nullptr) {
        return const_cast<Aspose::Pdf::Document*>(doc)->Info().Title();
    }
    return {};
}
void PdfFileInfo::Title(std::string v) {
    if (auto* doc = Document(); doc != nullptr) {
        doc->Info().Title(std::move(v));
    }
}

// ---- file-level metadata ----

bool PdfFileInfo::IsEncrypted() const noexcept {
    if (const auto* doc = Document(); doc != nullptr) {
        return doc->IsEncrypted();
    }
    return false;
}

bool PdfFileInfo::IsPdfFile() const noexcept {
    return Document() != nullptr;
}

bool PdfFileInfo::UseStrictValidation() const noexcept {
    return use_strict_validation_;
}
void PdfFileInfo::UseStrictValidation(bool v) noexcept {
    use_strict_validation_ = v;
}

bool PdfFileInfo::HasCollection() const noexcept {
    // /Collection catalog entry not yet surfaced — v1 default false.
    return false;
}

const std::string& PdfFileInfo::InputFile() const noexcept {
    return input_file_;
}
void PdfFileInfo::InputFile(std::string v) {
    input_file_ = std::move(v);
}

int PdfFileInfo::NumberOfPages() const noexcept {
    if (const auto* doc = Document(); doc != nullptr) {
        return static_cast<int>(
            const_cast<Aspose::Pdf::Document*>(doc)->Pages().Count());
    }
    return 0;
}

Aspose::Pdf::PasswordType PdfFileInfo::PasswordType() const noexcept {
    return password_type_;
}
bool PdfFileInfo::HasOpenPassword() const noexcept { return false; }
bool PdfFileInfo::HasEditPassword() const noexcept { return false; }

}  // namespace Aspose::Pdf::Facades
