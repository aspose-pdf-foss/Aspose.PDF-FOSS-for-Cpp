#pragma once

// =============================================================================
// Aspose::Pdf::OutlineItemCollection — a single outline (bookmark) node, which
// is itself a collection of child outlines (PDF §12.3.3). Mirrors canonical
// Aspose.Pdf.OutlineItemCollection (Aspose.PDF 26.4.0): a node carrying a
// title, text style (Bold / Italic), expansion state (Open), a navigation
// target (Destination or Action), and its children (inherited from Outlines).
//
// Navigation (First / Last / Next / Prev / Parent / Level) reflects the node's
// position in the owned tree. Destination / Action store an owning copy of the
// supplied target (canonical reference semantics) via the same internal clone
// hook used by GoToAction / LinkAnnotation.
// =============================================================================

#include <memory>
#include <string>

#include <aspose/pdf/outlines.hpp>

namespace Aspose::Pdf::Annotations {
class IAppointment;
class PdfAction;
}  // namespace Aspose::Pdf::Annotations

namespace Aspose::Pdf {

class OutlineCollection;

class OutlineItemCollection : public Outlines {
public:
    // Canonical ctor — a new node bound to the document's outline root.
    explicit OutlineItemCollection(OutlineCollection& outlines);
    ~OutlineItemCollection() override;

    // Canonical Title — the bookmark label.
    const std::string& Title() const noexcept;
    void Title(std::string value);

    // Canonical Bold / Italic — the label's text style (/F flags).
    bool Bold() const noexcept;
    void Bold(bool value);
    bool Italic() const noexcept;
    void Italic(bool value);

    // Canonical Open — whether the node is expanded in the viewer.
    bool Open() const noexcept;
    void Open(bool value);

    // Canonical Destination — the in-document navigation target (an
    // ExplicitDestination / NamedDestination). Non-owning getter; nullptr when
    // the node has an Action (or no target).
    Aspose::Pdf::Annotations::IAppointment* Destination() const noexcept;
    void Destination(const Aspose::Pdf::Annotations::IAppointment& value);

    // Canonical Action — the action triggered on activation (e.g.
    // GoToURIAction). Non-owning getter; nullptr when the node has a
    // Destination (or no target).
    Aspose::Pdf::Annotations::PdfAction* Action() const noexcept;
    void Action(const Aspose::Pdf::Annotations::PdfAction& value);

    // Canonical Insert — insert `item` as a child at `index` (1-based).
    void Insert(int index, OutlineItemCollection& item);

    // Canonical Next / Prev — the sibling after / before this node (nullptr at
    // the ends). HasNext — whether Next is non-null.
    OutlineItemCollection* Next() const noexcept;
    OutlineItemCollection* Prev() const noexcept;
    bool HasNext() const noexcept;

    // Canonical Parent — the owning Outlines (nullptr for a detached node).
    Outlines* Parent() const noexcept;

    // Canonical Level — 1 for a top-level node, +1 per nesting depth (0 when
    // detached).
    int Level() const noexcept;

private:
    // Internal — a detached node used by DeepClone and the load parser.
    OutlineItemCollection();

    // Internal — deep copy `src` (title/style/target + children) into a fresh
    // owned node. Used by Add / Insert (the tree owns its nodes).
    static std::unique_ptr<OutlineItemCollection> DeepClone(
        const OutlineItemCollection& src);

    std::string title_;
    bool bold_ = false;
    bool italic_ = false;
    bool open_ = true;
    std::unique_ptr<Aspose::Pdf::Annotations::IAppointment> destination_;
    std::unique_ptr<Aspose::Pdf::Annotations::PdfAction> action_;
    Outlines* parent_ = nullptr;

    friend class Outlines;
    friend class Document;
};

}  // namespace Aspose::Pdf
