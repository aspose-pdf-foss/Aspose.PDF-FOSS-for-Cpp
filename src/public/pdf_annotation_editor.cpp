#include <aspose/pdf/facades/pdf_annotation_editor.hpp>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

#include <aspose/pdf/annotations/annotation.hpp>
#include <aspose/pdf/annotations/markup_annotation.hpp>
#include <aspose/pdf/annotations/text_annotation.hpp>
#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>

#include "objects.hpp"
#include "pages_tree.hpp"
#include "xml.hpp"

namespace Aspose::Pdf::Facades {

PdfAnnotationEditor::PdfAnnotationEditor(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

// ===== Import ================================================================

namespace {

namespace fo = foundation::objects;
using AnnotType = Aspose::Pdf::Annotations::AnnotationType;

std::string ReadAll(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("PdfAnnotationEditor: cannot read " + path);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::span<const std::byte> Span(const std::string& s) {
    return {reinterpret_cast<const std::byte*>(s.data()), s.size()};
}

// The PDF /Subtype of an annotation type.
std::string SubtypeOf(AnnotType t) {
    switch (t) {
        case AnnotType::Text: return "Text";
        case AnnotType::Circle: return "Circle";
        case AnnotType::Polygon: return "Polygon";
        case AnnotType::PolyLine: return "PolyLine";
        case AnnotType::Line: return "Line";
        case AnnotType::Square: return "Square";
        case AnnotType::FreeText: return "FreeText";
        case AnnotType::Highlight: return "Highlight";
        case AnnotType::Underline: return "Underline";
        case AnnotType::StrikeOut: return "StrikeOut";
        case AnnotType::Squiggly: return "Squiggly";
        case AnnotType::Link: return "Link";
        case AnnotType::Ink: return "Ink";
        case AnnotType::Stamp: return "Stamp";
        case AnnotType::Caret: return "Caret";
        case AnnotType::Redaction: return "Redact";
        case AnnotType::Watermark: return "Watermark";
        case AnnotType::FileAttachment: return "FileAttachment";
        case AnnotType::Popup: return "Popup";
        case AnnotType::Widget: return "Widget";
        default: return "";
    }
}

std::set<std::string> Filter(const std::vector<AnnotType>& types) {
    std::set<std::string> out;
    for (auto t : types) out.insert(SubtypeOf(t));
    return out;
}

const fo::IndirectObject* Find(const fo::Dump& d, std::uint32_t id) {
    for (const auto& o : d.objects)
        if (o.id == id) return &o;
    return nullptr;
}

const fo::Value* Get(const fo::Dict& d, const char* key) {
    for (const auto& kv : d.entries)
        if (kv.first == key) return &kv.second;
    return nullptr;
}

const fo::Value* Resolve(const fo::Dump& d, const fo::Value* v) {
    if (v == nullptr) return nullptr;
    if (const auto* r = std::get_if<fo::Ref>(&v->v)) {
        const auto* o = Find(d, r->id);
        return o ? &o->value : nullptr;
    }
    return v;
}

std::string NameOf(const fo::Value* v) {
    const auto* n = v ? std::get_if<std::string>(&v->v) : nullptr;
    return n ? *n : std::string();
}

// Add annotation `id` (and its popup) for `page` when its subtype passes.
void Queue(const fo::Dump& d, std::uint32_t id, int page, const std::set<std::string>& filter,
           std::vector<std::pair<int, std::uint32_t>>& out) {
    const auto* o = Find(d, id);
    const auto* dict = o ? std::get_if<fo::Dict>(&o->value.v) : nullptr;
    if (dict == nullptr) return;
    const std::string subtype = NameOf(Get(*dict, "Subtype"));
    if (subtype == "Widget" || subtype == "Popup") return;  // popups follow parents
    if (!filter.empty() && !filter.count(subtype)) return;
    out.emplace_back(page, id);
    if (const auto* pr = Get(*dict, "Popup"))
        if (const auto* r = std::get_if<fo::Ref>(&pr->v)) out.emplace_back(page, r->id);
}

// Parse a PDF / FDF body; files without a usable cross-reference table (as
// FDF often is) get one rebuilt from their "N G obj" headers.
fo::Dump ParseLoose(std::string& bytes) {
    try {
        return fo::Parse(Span(bytes));
    } catch (const std::exception&) {
    }
    std::map<std::uint32_t, std::size_t> offsets;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0 && bytes[i - 1] != '\n' && bytes[i - 1] != '\r') continue;
        unsigned id = 0, gen = 0;
        int used = 0;
        if (std::sscanf(bytes.c_str() + i, "%u %u obj%n", &id, &gen, &used) == 2 && used > 0)
            offsets[id] = i;
    }
    std::uint32_t root = 0;
    for (const auto& [id, off] : offsets) {
        const auto end = bytes.find("endobj", off);
        if (bytes.substr(off, end - off).find("/FDF") != std::string::npos) { root = id; break; }
    }
    if (offsets.empty()) throw std::runtime_error("PdfAnnotationEditor: no objects in file");
    if (root == 0) root = offsets.begin()->first;
    const std::uint32_t size = offsets.rbegin()->first + 1;
    bytes += "\n";
    const std::size_t xref = bytes.size();
    bytes += "xref\n0 " + std::to_string(size) + "\n0000000000 65535 f \n";
    for (std::uint32_t id = 1; id < size; ++id) {
        char line[24];
        if (auto it = offsets.find(id); it != offsets.end())
            std::snprintf(line, sizeof line, "%010zu 00000 n \n", it->second);
        else
            std::snprintf(line, sizeof line, "0000000000 65535 f \n");
        bytes += line;
    }
    bytes += "trailer\n<< /Size " + std::to_string(size) + " /Root " + std::to_string(root) +
             " 0 R >>\nstartxref\n" + std::to_string(xref) + "\n%%EOF\n";
    return fo::Parse(Span(bytes));
}

// ---- XFDF -> annotation dictionaries ----------------------------------------

fo::Value Name(const std::string& n) { return fo::Value(n); }
fo::Value Num(double v) { return fo::Value(v); }

// A PDF text string: PDFDocEncoding-compatible ASCII as is, else UTF-16BE.
fo::Value Text(const std::string& utf8) {
    fo::String s;
    s.kind = fo::StringKind::Literal;
    bool ascii = true;
    for (unsigned char c : utf8)
        if (c >= 0x80) ascii = false;
    if (ascii) {
        for (char c : utf8) s.bytes.push_back(static_cast<std::byte>(c));
    } else {
        s.bytes = {std::byte{0xFE}, std::byte{0xFF}};
        for (char32_t cp : foundation::xml::DecodeUtf8(utf8)) {
            auto unit = [&](unsigned u) {
                s.bytes.push_back(static_cast<std::byte>(u >> 8));
                s.bytes.push_back(static_cast<std::byte>(u & 0xFF));
            };
            if (cp >= 0x10000) {
                cp -= 0x10000;
                unit(0xD800 + static_cast<unsigned>(cp >> 10));
                unit(0xDC00 + static_cast<unsigned>(cp & 0x3FF));
            } else {
                unit(static_cast<unsigned>(cp));
            }
        }
    }
    return fo::Value(std::move(s));
}

std::vector<double> Numbers(const std::string& s) {
    std::vector<double> out;
    std::string cur;
    auto flush = [&] {
        if (!cur.empty()) {
            try { out.push_back(std::stod(cur)); } catch (...) {}
            cur.clear();
        }
    };
    for (char c : s) {
        if (c == ',' || c == ';' || c == ' ' || c == '\t' || c == '\n' || c == '\r') flush();
        else cur += c;
    }
    flush();
    return out;
}

fo::Value NumArray(const std::vector<double>& v) {
    fo::Array a;
    for (double x : v) a.items.push_back(Num(x));
    return fo::Value(std::move(a));
}

// "#RRGGBB" -> [r g b].
bool Color(const std::string& s, fo::Value& out) {
    if (s.size() != 7 || s[0] != '#') return false;
    try {
        const unsigned long rgb = std::stoul(s.substr(1), nullptr, 16);
        out = NumArray({((rgb >> 16) & 0xFF) / 255.0, ((rgb >> 8) & 0xFF) / 255.0,
                        (rgb & 0xFF) / 255.0});
        return true;
    } catch (...) {
        return false;
    }
}

int Flags(const std::string& s) {
    static const std::pair<const char*, int> kFlags[] = {
        {"invisible", 1}, {"hidden", 2}, {"print", 4}, {"nozoom", 8}, {"norotate", 16},
        {"noview", 32}, {"readonly", 64}, {"locked", 128}, {"togglenoview", 256},
        {"lockedcontents", 512}};
    int f = 0;
    std::stringstream in(s);
    std::string tok;
    while (std::getline(in, tok, ',')) {
        while (!tok.empty() && tok.front() == ' ') tok.erase(tok.begin());
        while (!tok.empty() && tok.back() == ' ') tok.pop_back();
        for (const auto& [n, bit] : kFlags)
            if (tok == n) f |= bit;
    }
    return f;
}

std::string XfdfSubtype(const std::string& element) {
    static const std::map<std::string, std::string> kMap = {
        {"text", "Text"}, {"caret", "Caret"}, {"circle", "Circle"}, {"freetext", "FreeText"},
        {"highlight", "Highlight"}, {"ink", "Ink"}, {"line", "Line"}, {"link", "Link"},
        {"polygon", "Polygon"}, {"polyline", "PolyLine"}, {"square", "Square"},
        {"squiggly", "Squiggly"}, {"stamp", "Stamp"}, {"strikeout", "StrikeOut"},
        {"underline", "Underline"}, {"redact", "Redact"}};
    auto it = kMap.find(element);
    return it == kMap.end() ? std::string() : it->second;
}

// The XFDF <annots> as a synthetic object set (ids from 1) plus the
// (1-based page, id) list to import.
void XfdfToObjects(const foundation::xml::Element& annots, const std::set<std::string>& filter,
                   fo::Dump& dump, std::vector<std::pair<int, std::uint32_t>>& list) {
    std::map<std::string, std::uint32_t> by_name;
    std::vector<std::pair<std::uint32_t, std::string>> replies;  // id, inreplyto
    std::uint32_t next = 1;
    for (const auto& e : annots.children) {
        const std::string subtype = XfdfSubtype(e.name);
        if (subtype.empty()) continue;  // fileattachment, sound, unknown
        if (!filter.empty() && !filter.count(subtype)) continue;
        const auto* page_attr = e.Attribute("page");
        const auto* rect_attr = e.Attribute("rect");
        if (page_attr == nullptr || rect_attr == nullptr) continue;
        const int page = std::atoi(page_attr->c_str()) + 1;
        fo::Dict d;
        d.entries.emplace_back("Type", Name("Annot"));
        d.entries.emplace_back("Subtype", Name(subtype));
        d.entries.emplace_back("Rect", NumArray(Numbers(*rect_attr)));
        auto attr = [&](const char* key) { return e.Attribute(key); };
        fo::Value colour;
        if (const auto* c = attr("color"); c && Color(*c, colour)) d.entries.emplace_back("C", colour);
        if (const auto* c = attr("interior-color"); c && Color(*c, colour)) d.entries.emplace_back("IC", colour);
        if (const auto* v = attr("title")) d.entries.emplace_back("T", Text(*v));
        if (const auto* v = attr("subject")) d.entries.emplace_back("Subj", Text(*v));
        if (const auto* v = attr("name")) d.entries.emplace_back("NM", Text(*v));
        if (const auto* v = attr("date")) d.entries.emplace_back("M", Text(*v));
        if (const auto* v = attr("creationdate")) d.entries.emplace_back("CreationDate", Text(*v));
        if (const auto* v = attr("flags")) d.entries.emplace_back("F", fo::Value(static_cast<std::int64_t>(Flags(*v))));
        if (const auto* v = attr("opacity")) d.entries.emplace_back("CA", Num(std::atof(v->c_str())));
        if (const auto* v = attr("rotation")) d.entries.emplace_back("Rotate", fo::Value(static_cast<std::int64_t>(std::atoi(v->c_str()))));
        if (const auto* v = attr("intent")) d.entries.emplace_back("IT", Name(*v));
        if (const auto* v = attr("icon")) d.entries.emplace_back("Name", Name(*v));
        if (const auto* v = attr("state")) d.entries.emplace_back("State", Text(*v));
        if (const auto* v = attr("statemodel")) d.entries.emplace_back("StateModel", Text(*v));
        if (const auto* v = attr("open")) d.entries.emplace_back("Open", fo::Value(*v == "yes" || *v == "true"));
        if (const auto* v = attr("replyType")) d.entries.emplace_back("RT", Name(*v == "group" ? "Group" : "R"));
        if (const auto* v = attr("width")) {
            fo::Dict bs;
            bs.entries.emplace_back("W", Num(std::atof(v->c_str())));
            if (const auto* st = attr("style")) bs.entries.emplace_back("S", Name(*st == "dash" ? "D" : *st == "bevelled" ? "B" : *st == "inset" ? "I" : *st == "underline" ? "U" : "S"));
            if (const auto* dashes = attr("dashes")) bs.entries.emplace_back("D", NumArray(Numbers(*dashes)));
            d.entries.emplace_back("BS", fo::Value(std::move(bs)));
        }
        if (const auto* v = attr("coords")) d.entries.emplace_back("QuadPoints", NumArray(Numbers(*v)));
        if (const auto* v = attr("vertices")) d.entries.emplace_back("Vertices", NumArray(Numbers(*v)));
        if (const auto* st = attr("start")) {
            if (const auto* en = attr("end")) {
                auto l = Numbers(*st);
                const auto r = Numbers(*en);
                l.insert(l.end(), r.begin(), r.end());
                d.entries.emplace_back("L", NumArray(l));
            }
        }
        if (attr("head") || attr("tail")) {
            fo::Array le;
            le.items.push_back(Name(attr("head") ? *attr("head") : "None"));
            le.items.push_back(Name(attr("tail") ? *attr("tail") : "None"));
            d.entries.emplace_back("LE", fo::Value(std::move(le)));
        }
        if (const auto* v = attr("justification"))
            d.entries.emplace_back("Q", fo::Value(static_cast<std::int64_t>(std::atoi(v->c_str()))));
        if (const auto* c = e.Child("contents")) d.entries.emplace_back("Contents", Text(c->text));
        if (const auto* c = e.Child("defaultappearance")) d.entries.emplace_back("DA", Text(c->text));
        if (const auto* c = e.Child("defaultstyle")) d.entries.emplace_back("DS", Text(c->text));
        if (const auto* ink = e.Child("inklist")) {
            fo::Array strokes;
            for (const auto& g : ink->children)
                if (g.name == "gesture") strokes.items.push_back(NumArray(Numbers(g.text)));
            d.entries.emplace_back("InkList", fo::Value(std::move(strokes)));
        }
        if (const auto* action = e.Child("action"))
            if (const auto* uri = action->Child("URI"))
                if (const auto* target = uri->Attribute("Name")) {
                    fo::Dict a;
                    a.entries.emplace_back("S", Name("URI"));
                    a.entries.emplace_back("URI", Text(*target));
                    d.entries.emplace_back("A", fo::Value(std::move(a)));
                }
        const std::uint32_t id = next++;
        if (const auto* v = attr("name")) by_name[*v] = id;
        if (const auto* v = attr("inreplyto")) replies.emplace_back(id, *v);
        list.emplace_back(page, id);
        if (const auto* pop = e.Child("popup")) {
            const std::uint32_t pid = next++;
            fo::Dict pd;
            pd.entries.emplace_back("Type", Name("Annot"));
            pd.entries.emplace_back("Subtype", Name("Popup"));
            if (const auto* r = pop->Attribute("rect")) pd.entries.emplace_back("Rect", NumArray(Numbers(*r)));
            if (const auto* o = pop->Attribute("open"))
                pd.entries.emplace_back("Open", fo::Value(*o == "yes" || *o == "true"));
            pd.entries.emplace_back("Parent", fo::Value(fo::Ref{id, 0}));
            d.entries.emplace_back("Popup", fo::Value(fo::Ref{pid, 0}));
            dump.objects.push_back(fo::IndirectObject{pid, 0, fo::Value(std::move(pd))});
            list.emplace_back(page, pid);
        }
        dump.objects.push_back(fo::IndirectObject{id, 0, fo::Value(std::move(d))});
    }
    for (const auto& [id, target] : replies) {
        auto it = by_name.find(target);
        if (it == by_name.end()) continue;
        for (auto& o : dump.objects)
            if (o.id == id)
                std::get<fo::Dict>(o.value.v).entries.emplace_back("IRT", fo::Value(fo::Ref{it->second, 0}));
    }
}

}  // namespace

void PdfAnnotationEditor::ImportAnnotationsFromXfdf(const std::string& xfdfFile) {
    ImportAnnotationFromXfdf(xfdfFile, {});
}
void PdfAnnotationEditor::ImportAnnotationFromXfdf(const std::string& xfdfFile) {
    ImportAnnotationFromXfdf(xfdfFile, {});
}
void PdfAnnotationEditor::ImportAnnotationFromXfdf(
    const std::string& xfdfFile,
    const std::vector<Aspose::Pdf::Annotations::AnnotationType>& annotType) {
    if (document_ == nullptr) return;
    const auto root = foundation::xml::Parse(ReadAll(xfdfFile));
    if (root.name != "xfdf") throw std::runtime_error("PdfAnnotationEditor: not an XFDF file");
    const auto* annots = root.Child("annots");
    if (annots == nullptr) return;
    fo::Dump dump;
    std::vector<std::pair<int, std::uint32_t>> list;
    XfdfToObjects(*annots, Filter(annotType), dump, list);
    document_->ImportAnnotationObjectsInternal(dump, list, {});
}

void PdfAnnotationEditor::ImportAnnotationsFromFdf(const std::string& fdfFile) {
    if (document_ == nullptr) return;
    std::string bytes = ReadAll(fdfFile);
    const fo::Dump dump = ParseLoose(bytes);
    std::vector<std::pair<int, std::uint32_t>> list;
    for (const auto& o : dump.objects) {
        const auto* d = std::get_if<fo::Dict>(&o.value.v);
        const auto* fdf = d ? Resolve(dump, Get(*d, "FDF")) : nullptr;
        const auto* fdf_dict = fdf ? std::get_if<fo::Dict>(&fdf->v) : nullptr;
        const auto* av = fdf_dict ? Resolve(dump, Get(*fdf_dict, "Annots")) : nullptr;
        const auto* arr = av ? std::get_if<fo::Array>(&av->v) : nullptr;
        if (arr == nullptr) continue;
        for (const auto& item : arr->items) {
            const auto* r = std::get_if<fo::Ref>(&item.v);
            const auto* ao = r ? Find(dump, r->id) : nullptr;
            const auto* ad = ao ? std::get_if<fo::Dict>(&ao->value.v) : nullptr;
            if (ad == nullptr) continue;
            const auto* pv = Get(*ad, "Page");
            const auto* pn = pv ? std::get_if<std::int64_t>(&pv->v) : nullptr;
            Queue(dump, r->id, pn ? static_cast<int>(*pn) + 1 : 1, {}, list);
        }
        break;
    }
    document_->ImportAnnotationObjectsInternal(dump, list, {});
}

void PdfAnnotationEditor::ImportAnnotations(
    const std::vector<std::string>& annotFile,
    const std::vector<Aspose::Pdf::Annotations::AnnotationType>& annotType) {
    if (document_ == nullptr) return;
    const auto filter = Filter(annotType);
    for (const auto& file : annotFile) {
        std::string bytes = ReadAll(file);
        const fo::Dump dump = ParseLoose(bytes);
        const auto tree = foundation::pages_tree::Parse(Span(bytes));
        std::vector<std::pair<int, std::uint32_t>> list;
        std::map<std::uint32_t, int> pages;
        for (std::size_t i = 0; i < tree.leaves.size(); ++i) {
            const int page = static_cast<int>(i) + 1;
            pages[tree.leaves[i].id] = page;
            const auto* po = Find(dump, tree.leaves[i].id);
            const auto* pd = po ? std::get_if<fo::Dict>(&po->value.v) : nullptr;
            const auto* av = pd ? Resolve(dump, Get(*pd, "Annots")) : nullptr;
            const auto* arr = av ? std::get_if<fo::Array>(&av->v) : nullptr;
            if (arr == nullptr) continue;
            for (const auto& item : arr->items)
                if (const auto* r = std::get_if<fo::Ref>(&item.v)) Queue(dump, r->id, page, filter, list);
        }
        document_->ImportAnnotationObjectsInternal(dump, list, pages);
    }
}
void PdfAnnotationEditor::ImportAnnotations(const std::vector<std::string>& annotFile) {
    ImportAnnotations(annotFile, {});
}

// ===== Modify ================================================================

void PdfAnnotationEditor::ModifyAnnotations(
    int start, int end, Aspose::Pdf::Annotations::Annotation& annotation) {
    if (document_ == nullptr) return;
    namespace A = Aspose::Pdf::Annotations;
    const auto type = annotation.AnnotationType();
    const auto* tpl_markup = dynamic_cast<const A::MarkupAnnotation*>(&annotation);
    const auto* tpl_text = dynamic_cast<const A::TextAnnotation*>(&annotation);
    const int last = std::min(end, static_cast<int>(document_->Pages().Count()));
    for (int p = std::max(start, 1); p <= last; ++p) {
        auto& annots = document_->Pages()[p].Annotations();
        for (int i = 0; i < annots.Count(); ++i) {
            A::Annotation& a = annots[i];
            if (a.AnnotationType() != type) continue;
            a.Color(annotation.Color());
            a.Contents(annotation.Contents());
            if (auto* m = dynamic_cast<A::MarkupAnnotation*>(&a); m && tpl_markup) {
                m->Title(tpl_markup->Title());
                m->Subject(tpl_markup->Subject());
            }
            if (auto* t = dynamic_cast<A::TextAnnotation*>(&a); t && tpl_text) t->Open(tpl_text->Open());
        }
    }
}

void PdfAnnotationEditor::ModifyAnnotationsAuthor(int start, int end,
                                                  const std::string& srcAuthor,
                                                  const std::string& desAuthor) {
    if (document_ == nullptr) return;
    namespace A = Aspose::Pdf::Annotations;
    const int last = std::min(end, static_cast<int>(document_->Pages().Count()));
    for (int p = std::max(start, 1); p <= last; ++p) {
        auto& annots = document_->Pages()[p].Annotations();
        for (int i = 0; i < annots.Count(); ++i)
            if (auto* m = dynamic_cast<A::MarkupAnnotation*>(&annots[i]); m && m->Title() == srcAuthor)
                m->Title(desAuthor);
    }
}

namespace {

// Canonical annotation-type name (AnnotationType.ToString()) for the
// delete-by-type filter.
const char* AnnotTypeName(Aspose::Pdf::Annotations::AnnotationType t) {
    using AT = Aspose::Pdf::Annotations::AnnotationType;
    switch (t) {
        case AT::Text:           return "Text";
        case AT::Circle:         return "Circle";
        case AT::Polygon:        return "Polygon";
        case AT::PolyLine:       return "PolyLine";
        case AT::Line:           return "Line";
        case AT::Square:         return "Square";
        case AT::FreeText:       return "FreeText";
        case AT::Highlight:      return "Highlight";
        case AT::Underline:      return "Underline";
        case AT::StrikeOut:      return "StrikeOut";
        case AT::Squiggly:       return "Squiggly";
        case AT::Link:           return "Link";
        case AT::Ink:            return "Ink";
        case AT::Stamp:          return "Stamp";
        case AT::Caret:          return "Caret";
        case AT::Redaction:      return "Redaction";
        case AT::Watermark:      return "Watermark";
        case AT::FileAttachment: return "FileAttachment";
        case AT::Popup:          return "Popup";
        case AT::Widget:         return "Widget";
        default:                 return "";
    }
}

}  // namespace

// ===== Flatten ===============================================================
// Staged on the bound document and applied by its next Save: appearances
// are drawn into the page content and the annotations removed. Works on the
// file's annotations, including subtypes the annotation collection does not
// model; one without an appearance is left in place.

void PdfAnnotationEditor::FlatteningAnnotations() {
    FlatteningAnnotations(Aspose::Pdf::Forms::Form::FlattenSettings{});
}

void PdfAnnotationEditor::FlatteningAnnotations(
        const Aspose::Pdf::Forms::Form::FlattenSettings& flattenSettings) {
    if (document_ == nullptr) return;
    // Every form field and every annotation of every page.
    Aspose::Pdf::Forms::Form& form = document_->Form();
    form.flatten_hide_buttons_ = flattenSettings.HideButtons();
    form.Flatten();
    Aspose::Pdf::Document::FlattenRequest::Rule rule;
    rule.hide_buttons = flattenSettings.HideButtons();
    document_->pending_flatten_rules_.push_back(std::move(rule));
}

void PdfAnnotationEditor::FlatteningAnnotations(
        int start, int end,
        const std::vector<Aspose::Pdf::Annotations::AnnotationType>&
            annotType) {
    if (document_ == nullptr) return;
    Aspose::Pdf::Document::FlattenRequest::Rule rule;
    rule.first_page = start;
    rule.last_page = end;
    for (const auto t : annotType) {
        const std::string name = AnnotTypeName(t);
        if (!name.empty()) rule.subtypes.insert(name);
    }
    // A type list with nothing this library can name selects nothing.
    if (!annotType.empty() && rule.subtypes.empty()) return;
    document_->pending_flatten_rules_.push_back(std::move(rule));
}

// ===== Delete ================================================================

void PdfAnnotationEditor::DeleteAnnotations() {
    if (document_ == nullptr) {
        return;
    }
    Aspose::Pdf::PageCollection& pages = document_->Pages();
    const int count = static_cast<int>(pages.Count());
    for (int i = 1; i <= count; ++i) {
        pages[i].Annotations().Delete();
    }
}


void PdfAnnotationEditor::DeleteAnnotations(const std::string& annotType) {
    if (document_ == nullptr) {
        return;
    }
    Aspose::Pdf::PageCollection& pages = document_->Pages();
    const int count = static_cast<int>(pages.Count());
    for (int p = 1; p <= count; ++p) {
        auto& annots = pages[p].Annotations();
        for (int i = annots.Count() - 1; i >= 0; --i) {
            if (annotType == AnnotTypeName(annots[i].AnnotationType())) {
                annots.Delete(i);
            }
        }
    }
}

void PdfAnnotationEditor::DeleteAnnotation(const std::string& annotName) {
    if (document_ == nullptr) {
        return;
    }
    Aspose::Pdf::PageCollection& pages = document_->Pages();
    const int pageCount = static_cast<int>(pages.Count());
    for (int p = 1; p <= pageCount; ++p) {
        Aspose::Pdf::Page page = pages[p];
        Aspose::Pdf::Annotations::AnnotationCollection& annots =
            page.Annotations();
        const int annotCount = annots.Count();
        // AnnotationCollection is 0-based (cf. PageCollection's
        // 1-based indexing).
        for (int a = 0; a < annotCount; ++a) {
            if (annots[a].Name() == annotName) {
                annots.Delete(a);
                return;
            }
        }
    }
}

}  // namespace Aspose::Pdf::Facades
