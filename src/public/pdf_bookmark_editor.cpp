#include <aspose/pdf/facades/pdf_bookmark_editor.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>

#include <aspose/pdf/document.hpp>

#include "xml.hpp"

namespace Aspose::Pdf::Facades {

PdfBookmarkEditor::PdfBookmarkEditor(Aspose::Pdf::Document& document) {
    BindPdf(document);
}

// Create / extract stage and read the /Outlines tree; XML import / export
// and HTML export use the Aspose.PDF bookmark formats.

Aspose::Pdf::Document::OutlineNode PdfBookmarkEditor::ToNode(
        const Bookmark& bm) {
    Aspose::Pdf::Document::OutlineNode n;
    n.title = bm.Title();
    n.page = bm.PageNumber();
    Bookmarks kids = bm.ChildItems();
    for (const auto& child : kids) n.children.push_back(ToNode(child));
    return n;
}

void PdfBookmarkEditor::Restage() {
    if (document_ == nullptr) return;
    std::vector<Aspose::Pdf::Document::OutlineNode> nodes;
    nodes.reserve(staged_.size());
    for (const auto& b : staged_) nodes.push_back(ToNode(b));
    document_->SetStagedOutlines(std::move(nodes));
}

void PdfBookmarkEditor::CreateBookmarks() { Restage(); }
void PdfBookmarkEditor::CreateBookmarks(const Bookmark& bookmark) {
    staged_.push_back(bookmark);
    Restage();
}
void PdfBookmarkEditor::CreateBookmarkOfPage(const std::string& title,
                                             int pageNumber) {
    Bookmark b;
    b.Title(title);
    b.PageNumber(pageNumber);
    staged_.push_back(std::move(b));
    Restage();
}
void PdfBookmarkEditor::CreateBookmarkOfPage(
        const std::vector<std::string>& title,
        const std::vector<int>& pageNumber) {
    for (std::size_t i = 0; i < title.size(); ++i) {
        Bookmark b;
        b.Title(title[i]);
        b.PageNumber(i < pageNumber.size() ? pageNumber[i] : 1);
        staged_.push_back(std::move(b));
    }
    Restage();
}

void PdfBookmarkEditor::DeleteBookmarks() {
    staged_.clear();
    if (document_ != nullptr) document_->ClearStagedOutlines();
}
void PdfBookmarkEditor::DeleteBookmarks(const std::string& title) {
    for (auto it = staged_.begin(); it != staged_.end();) {
        if (it->Title() == title)
            it = staged_.erase(it);
        else
            ++it;
    }
    Restage();
}
void PdfBookmarkEditor::ModifyBookmarks(const std::string& oldTitle,
                                        const std::string& newTitle) {
    for (auto& b : staged_)
        if (b.Title() == oldTitle) b.Title(newTitle);
    Restage();
}

Bookmarks PdfBookmarkEditor::ExtractBookmarks() {
    Bookmarks result;
    if (document_ == nullptr) return result;
    for (const auto& [depth, title] : document_->ParseOutlineItems()) {
        Bookmark b;
        b.Title(title);
        b.Level(depth + 1);
        result.push_back(std::move(b));
    }
    return result;
}
Bookmarks PdfBookmarkEditor::ExtractBookmarks(bool upperLevel) {
    Bookmarks all = ExtractBookmarks();
    if (!upperLevel) return all;
    Bookmarks top;
    for (auto& b : all)
        if (b.Level() == 1) top.push_back(std::move(b));
    return top;
}

namespace {

// The bookmarks (depth-first, with levels) under each one titled `title`;
// `with_root` keeps the titled bookmark itself.
Bookmarks Subtrees(const Bookmarks& all, const std::string& title, int level,
                   bool with_root) {
    Bookmarks out;
    for (std::size_t i = 0; i < all.size(); ++i) {
        if (all[i].Title() != title || (level > 0 && all[i].Level() != level))
            continue;
        if (with_root) out.push_back(all[i]);
        for (std::size_t j = i + 1; j < all.size() && all[j].Level() > all[i].Level(); ++j)
            out.push_back(all[j]);
    }
    return out;
}

}  // namespace

Bookmarks PdfBookmarkEditor::ExtractBookmarks(const std::string& title) {
    return Subtrees(ExtractBookmarks(), title, 0, true);
}
Bookmarks PdfBookmarkEditor::ExtractBookmarks(const Bookmark& parent) {
    return Subtrees(ExtractBookmarks(), parent.Title(), parent.Level(), false);
}

namespace {

using Node = Aspose::Pdf::Document::OutlineNode;

std::string Num(double v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.6g", v);
    return buf;
}

// "<page> <verb> <operands>" (ExplicitDestination's text form).
std::string PageDest(const Node& n) {
    std::string s = std::to_string(n.page) + " " + (n.dest_verb.empty() ? "Fit" : n.dest_verb);
    for (double op : n.dest_ops) s += " " + Num(op);
    return s;
}

// Title text: ASCII as is (escaped), anything else as a character reference.
std::string XmlTitleText(const std::string& title) {
    std::string out;
    for (char32_t cp : foundation::xml::DecodeUtf8(title)) {
        if (cp < 32 || cp > 127) out += "&#" + std::to_string(static_cast<unsigned long>(cp)) + ";";
        else out += foundation::xml::Escape(std::string(1, static_cast<char>(cp)));
    }
    return out;
}

std::string Trim(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    return s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}

void WriteXmlTitles(std::string& out, const std::vector<Node>& nodes, int depth) {
    for (const auto& n : nodes) {
        out += std::string(static_cast<std::size_t>(2 * depth), ' ') + "<Title";
        if (n.bold && n.italic) out += " Style=\"italic bold\"";
        else if (n.italic) out += " Style=\"italic\"";
        else if (n.bold) out += " Style=\"bold\"";
        if (!n.named_dest.empty())
            out += " Named=\"" + foundation::xml::Escape(n.named_dest) + "\" Action=\"GoTo\"";
        else if (!n.uri.empty())
            out += " URI=\"" + foundation::xml::Escape(n.uri) + "\" Action=\"URI\"";
        else if (n.page > 0)
            out += " Page=\"" + PageDest(n) + "\" Action=\"GoTo\"";
        if (!n.children.empty()) out += n.open ? " Open=\"True\"" : " Open=\"False\"";
        out += ">" + XmlTitleText(Trim(n.title));
        if (!n.children.empty()) {
            out += "\n";
            WriteXmlTitles(out, n.children, depth + 1);
            out += std::string(static_cast<std::size_t>(2 * depth), ' ');
        }
        out += "</Title>\n";
    }
}

std::vector<Node> ReadXmlTitles(const foundation::xml::Element& parent) {
    std::vector<Node> out;
    for (const auto& e : parent.children) {
        if (e.name != "Title") continue;
        Node n;
        n.title = Trim(e.text);
        if (const auto* style = e.Attribute("Style")) {
            n.bold = style->find("bold") != std::string::npos;
            n.italic = style->find("italic") != std::string::npos;
        }
        const std::string* action = e.Attribute("Action");
        if (action == nullptr || *action == "GoTo") {
            if (const auto* page = e.Attribute("Page")) {
                std::istringstream in(*page);
                int p = 1;
                std::string verb;
                in >> p >> verb;
                n.page = p < 1 ? 1 : p;
                n.dest_verb = verb == "Fit" ? std::string() : verb;
                double op;
                while (in >> op) n.dest_ops.push_back(op);
            } else if (const auto* named = e.Attribute("Named")) {
                n.named_dest = *named;
            }
        } else if (*action == "URI") {
            if (const auto* uri = e.Attribute("URI")) n.uri = *uri;
        }
        if (const auto* open = e.Attribute("Open"))
            n.open = !(*open == "False" || *open == "false" || *open == "0");
        n.children = ReadXmlTitles(e);
        out.push_back(std::move(n));
    }
    return out;
}

void WriteText(const std::string& path, const std::string& text) {
    std::ofstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("PdfBookmarkEditor: cannot write " + path);
    f.write(text.data(), static_cast<std::streamsize>(text.size()));
}

void WriteHtmlList(std::string& out, const std::vector<Node>& nodes, const std::string& pdf,
                   int depth) {
    const std::string pad(static_cast<std::size_t>(2 * depth), ' ');
    out += pad + "<ul>\n";
    for (const auto& n : nodes) {
        out += pad + "  <li><a";
        if (!n.named_dest.empty())
            out += " href=\"" + foundation::xml::Escape(pdf + "#nameddest=" + n.named_dest) + "\"";
        else if (!n.uri.empty())
            out += " href=\"" + foundation::xml::Escape(n.uri) + "\"";
        else if (n.page > 0)
            out += " href=\"" + foundation::xml::Escape(pdf + "#page=" + std::to_string(n.page)) + "\"";
        std::string style;
        if (n.bold) style += "font-weight: bold;";
        if (n.italic) style += "font-style: italic;";
        if (!style.empty()) out += " style=\"" + style + "\"";
        out += ">" + foundation::xml::Escape(Trim(n.title)) + "</a>";
        if (!n.children.empty()) {
            out += "\n";
            WriteHtmlList(out, n.children, pdf, depth + 2);
            out += pad + "  ";
        }
        out += "</li>\n";
    }
    out += pad + "</ul>\n";
}

}  // namespace

void PdfBookmarkEditor::ExtractBookmarksToHTML(const std::string& pdfFile,
                                               const std::string&) {
    const std::filesystem::path p(pdfFile);
    ExportBookmarksToHtml(pdfFile,
                          (p.parent_path() / (p.stem().string() + "-index.html")).string());
}

void PdfBookmarkEditor::ExportBookmarksToHtml(const std::string& inPdfFile,
                                              const std::string& outHtmlFile) {
    Aspose::Pdf::Document src{inPdfFile};
    const auto nodes = src.OutlineTreeInternal();
    const std::string title = std::filesystem::path(outHtmlFile).stem().string();
    std::string html = "<!DOCTYPE html>\n<html>\n  <head>\n    <title>" +
                       foundation::xml::Escape(title) +
                       "</title>\n    <meta charset=\"utf-8\" />\n"
                       "    <style type=\"text/css\">ul { list-style-type: none; }</style>\n"
                       "  </head>\n  <body>\n";
    if (nodes.empty())
        html += "    This document has no bookmarks.\n";
    else
        WriteHtmlList(html, nodes, std::filesystem::path(inPdfFile).filename().string(), 2);
    html += "  </body>\n</html>\n";
    WriteText(outHtmlFile, html);
}

void PdfBookmarkEditor::ExportBookmarksToXML(const std::string& xmlFile) {
    if (document_ == nullptr) return;
    std::string xml = "<?xml version=\"1.0\" encoding=\"ISO8859-1\"?>\n<Bookmark>\n";
    WriteXmlTitles(xml, document_->OutlineTreeInternal(), 1);
    xml += "</Bookmark>\n";
    WriteText(xmlFile, xml);
}

void PdfBookmarkEditor::ImportBookmarksWithXML(const std::string& xmlFile) {
    if (document_ == nullptr) return;
    std::ifstream in(xmlFile, std::ios::binary);
    if (!in) throw std::runtime_error("PdfBookmarkEditor: cannot read " + xmlFile);
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const auto root = foundation::xml::Parse(text);
    if (root.name != "Bookmark")
        throw std::runtime_error("PdfBookmarkEditor: not a bookmark XML file");
    document_->AppendOutlinesInternal(ReadXmlTitles(root));
}

}  // namespace Aspose::Pdf::Facades
