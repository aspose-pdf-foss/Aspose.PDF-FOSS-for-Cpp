#pragma once

// =============================================================================
// Aspose::Pdf::Document — minimum viable v1 surface.
//
// Open an unencrypted PDF from a file path, expose its page collection,
// save it back to a file path, and edit the /Info dictionary through
// the small-write spine (`Info()` accessor + `SetTitle` shortcut, both
// flushed on the next `Save()` via foundation::pdf_writer_incremental).
// Stream-based ctors, encryption, format-conversion overloads, and
// the heavier mutation methods (annotations, form, outline, structure,
// XMP metadata, conversion) are deliberately out of scope until the
// foundation primitives that back them are stable —
// see capabilities/document.yaml.
// =============================================================================

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <aspose/pdf/crypto_algorithm.hpp>
#include <aspose/pdf/document_privilege.hpp>
#include <aspose/pdf/permissions.hpp>
#include <aspose/pdf/rectangle.hpp>

namespace foundation::pages_tree { struct Tree; }

namespace Aspose::Pdf {
namespace Text { class TextAbsorber; }
namespace Text { class TextFragmentAbsorber; }
namespace Text { class TextFragment; }
namespace Text { class TextBuilder; }
namespace Devices {
class ImageDevice;
class PngDevice;
class JpegDevice;
class BmpDevice;
class TextDevice;
class TiffDevice;
}
}

namespace Aspose::Pdf {

class Color;
class DocumentInfo;
class EmbeddedFileCollection;
class Metadata;
class NamedDestinationCollection;
class OutlineCollection;
class OutlineItemCollection;
class Page;
class PageLabelCollection;
class Paragraphs;
class Resources;
class Table;
class XImageCollection;
class ArtifactCollection;
class LoadOptions;
class PageCollection;

namespace Annotations { class AnnotationCollection; }
namespace Annotations { class RedactionAnnotation; }
namespace Forms { class Form; }
namespace Facades { class PdfFileEditor; }
namespace Facades { class PdfBookmarkEditor; }
namespace Facades { class PdfFileStamp; }
namespace Facades { class PdfFileSignature; }
namespace Facades { class PdfContentEditor; }
namespace Facades { class PdfExtractor; }

class Document {
public:
    // One text-stamp request: draw `text` at user-space (x, y) on the
    // page at `leaf` in `size`-pt Helvetica. Backs PdfFileStamp
    // headers / footers / page numbers.
    struct TextStampReq {
        std::size_t leaf = 0;
        std::string text;
        double x = 0.0;
        double y = 0.0;
        double size = 12.0;
    };

    // One digital-signature request (backs PdfFileSignature::Sign). All
    // strings are caller-supplied; `date_pdf` is a PDF date string
    // ("D:YYYYMMDDHHmmSS+00'00'") for the /M entry and `signing_time_utc`
    // a 13-char UTCTime ("YYMMDDHHMMSSZ") for the PKCS#7 signingTime
    // attribute. `contents_capacity` is the byte length reserved for the
    // /Contents placeholder (the PKCS#7 blob must fit, hex-encoded).
    struct SignatureRequest {
        std::string field_name = "Signature1";
        std::string name;
        std::string reason;
        std::string location;
        std::string contact;
        std::string date_pdf;
        std::string signing_time_utc;
        std::size_t contents_capacity = 8192;
    };

    // One read-back signature (backs PdfFileSignature enumerate / read /
    // verify). `covers_whole` is true when the /ByteRange spans the whole
    // file (last segment ends at EOF).
    struct SignatureInfo {
        std::string field_name;
        std::string name;
        std::string reason;
        std::string location;
        std::string contact;
        std::vector<int> byte_range;
        bool has_value = false;   // /V present (false => blank sig field)
        bool covers_whole = false;
    };

    // Staged-write outline tree node (backs PdfBookmarkEditor and the public
    // Outlines() tree): a bookmark title + 1-based target page (0 = no target)
    // + nested children. Internal wiring type for the /Outlines writer. The
    // style / target fields below back the public OutlineItemCollection
    // save-through; PdfBookmarkEditor leaves them at their defaults (writing a
    // plain /Fit destination on `page`).
    struct OutlineNode {
        std::string title;
        int page = 0;
        std::vector<OutlineNode> children;
        bool bold = false;
        bool italic = false;
        bool open = true;
        std::string dest_verb;         // "XYZ" / "FitH" / … (empty → /Fit)
        std::vector<double> dest_ops;  // operands for dest_verb
        std::string named_dest;        // target is a NamedDestination
        std::string uri;               // target is a GoToURIAction
    };

    // Default-construct an empty PDF document — a /Pages tree
    // with /Count 0 and /Kids[]. Save(outputFileName) writes the
    // empty doc to disk; Save() with no argument throws because
    // no source filename is associated.
    Document();

    // Open the PDF stored at the given file path. The file header,
    // cross-reference table, and page tree are parsed at construction;
    // operations after this point work against the in-memory model.
    //
    // Throws:
    //   std::system_error      — file does not exist or cannot be read
    //   std::runtime_error     — file is not a valid PDF (header / xref /
    //                            page tree malformed)
    explicit Document(const std::string& filename);

    // Open / import the file at `filename` per `options`. When `options` is an
    // SvgLoadOptions the SVG is converted to a single-page PDF (see
    // SvgLoadOptions for the v1 conversion subset); otherwise the file is
    // loaded as a PDF. Canonical Document(string, LoadOptions).
    Document(const std::string& filename,
             const Aspose::Pdf::LoadOptions& options);

    // Open the PDF stored at the given file path with a password.
    // Validates the password against the trailer's /Encrypt dict
    // via foundation::encrypt_parser::Authenticate. On success the
    // recovered file-encryption key is stashed on the Document for
    // future Decrypt() and content-decryption consumers; on failure
    // the constructor throws.
    //
    // When the source PDF is NOT encrypted, the `password` is
    // ignored (matching canonical .NET behaviour — both
    // Document(filename) and Document(filename, password) work
    // identically on an unencrypted source).
    //
    // Throws:
    //   std::system_error      — file does not exist or cannot be read
    //   std::runtime_error     — file is not a valid PDF, OR /Encrypt
    //                            is present but the password is wrong,
    //                            OR /Encrypt is malformed.
    Document(const std::string& filename, const std::string& password);

    // True iff the source PDF was loaded with an /Encrypt entry in
    // its trailer AND Decrypt() has not yet been called.
    // Default-constructed (empty PDF) instances report false.
    bool IsEncrypted() const noexcept;

    // Stage encryption for the next Save() — Permissions-bitfield
    // overload. The actual cipher work (file_key derivation, /O /U
    // /OE /UE /Perms compute, per-object string + stream encryption,
    // /Encrypt dict insertion into the trailer) runs at Save() time.
    //
    // `cryptoAlgorithm` selects the (V, R) variant:
    //   RC4x40  -> V=1 R=2 RC4-40
    //   RC4x128 -> V=2 R=3 RC4-128
    //   AESx128 -> V=4 R=4 AES-128 CBC
    //   AESx256 -> V=5 R=6 AES-256 (PDF 2.0 Algorithm 2.B)
    //
    // The Permissions bitfield is encoded directly into the /P
    // entry of the /Encrypt dictionary; bits the canonical enum
    // doesn't define (1-2, 7-8, 13+) are passed through unchanged.
    void Encrypt(const std::string& userPassword,
                 const std::string& ownerPassword,
                 Aspose::Pdf::Permissions permissions,
                 CryptoAlgorithm cryptoAlgorithm);

    // Encrypt overload with explicit `usePdf20` selector. When
    // `cryptoAlgorithm == AESx256`, `usePdf20 = true` selects R=6
    // (PDF 2.0 iterative Algorithm 2.B hash); `usePdf20 = false`
    // selects R=5 (the deprecated Adobe Supplement single-SHA-256
    // path). For other algorithms `usePdf20` is ignored — they
    // pin a single (V, R) per the table above.
    void Encrypt(const std::string& userPassword,
                 const std::string& ownerPassword,
                 Aspose::Pdf::Permissions permissions,
                 CryptoAlgorithm cryptoAlgorithm,
                 bool usePdf20);

    // Canonical Document.Permissions — the current /P permission bitfield
    // (ISO 32000-1 Table 22). Reflects the staged encryption permissions, else
    // the open document's /Encrypt /P, else an all-permissions value when the
    // document is not encrypted.
    int Permissions() const;

    // Canonical Document.Optimize / OptimizeResources — request a compacting
    // resave that drops objects unreachable from the document catalog (garbage
    // collection of unused resources). Applied at the next Save().
    void Optimize();
    void OptimizeResources();

    // Encrypt overload taking a DocumentPrivilege preset. Internally
    // projects the privilege's eight Allow* flags onto the
    // Permissions bitfield (PrintDocument <- AllowPrint, etc.) and
    // calls the Permissions overload above.
    void Encrypt(const std::string& userPassword,
                 const std::string& ownerPassword,
                 const Aspose::Pdf::Facades::DocumentPrivilege& privileges,
                 CryptoAlgorithm cryptoAlgorithm);

    // Encrypt overload taking a DocumentPrivilege preset with
    // explicit `usePdf20` selector (same semantic as the
    // Permissions+usePdf20 overload).
    void Encrypt(const std::string& userPassword,
                 const std::string& ownerPassword,
                 const Aspose::Pdf::Facades::DocumentPrivilege& privileges,
                 CryptoAlgorithm cryptoAlgorithm,
                 bool usePdf20);

    // Mark the document as decrypted so future surface queries
    // (`IsEncrypted` and downstream consumers) treat it as
    // plaintext. The recovered file-encryption key is preserved so
    // streams and strings can still be decrypted on-demand by
    // content consumers (TextAbsorber, rendering devices, etc.)
    // when they wire through the key in a future beat.
    //
    // v1 limitation: this method does NOT rewrite `bytes_` to remove
    // the /Encrypt dictionary from the trailer or to convert
    // object streams + strings to plaintext. The next Save() will
    // therefore write back the source bytes unchanged (still
    // encrypted). Full "decrypt-then-save-as-plaintext" landings
    // are deferred to a follow-on beat alongside the write-path
    // crypto integration in Document::Encrypt.
    //
    // A Document loaded without /Encrypt is a no-op call.
    void Decrypt();

    Document(const Document&) = delete;
    Document& operator=(const Document&) = delete;
    Document(Document&&) noexcept;
    Document& operator=(Document&&) noexcept;
    ~Document();

    // Serialise the document to the given file path. Overwrites any
    // existing file at that path. If the in-memory model carries
    // pending mutations (DocumentInfo edits via Info() / SetTitle),
    // the original bytes are written followed by an incremental
    // update appendix from foundation::pdf_writer_incremental;
    // otherwise the original byte stream is round-tripped verbatim.
    //
    // Throws:
    //   std::system_error — destination cannot be opened for writing
    void Save(const std::string& outputFileName) const;

    // Save the document back to the source filename tracked by
    // Document(filename). Throws std::runtime_error when the
    // document was constructed via the no-arg ctor (no source
    // file to write back to).
    void Save() const;

    // Convenience: set PDF /Info /Title. Equivalent to
    // ``Info().Title(title)``. The mutation is staged in the in-
    // memory model until the next Save() flushes it.
    void SetTitle(const std::string& title);

    // Borrowed reference to the document's DocumentInfo. Property
    // assignments and Add / Remove on the returned object are
    // persisted by the next Save().
    DocumentInfo& Info();

    // Borrowed reference to the document's XMP metadata. Lazily
    // parses the /Metadata stream from the source PDF on first
    // access; an empty Metadata is returned for documents that
    // have no /Metadata in their catalog.
    Aspose::Pdf::Metadata& Metadata();

    // Page collection of this document. The returned reference is
    // bound to the Document — callers must not hold it beyond the
    // Document's lifetime.
    PageCollection& Pages();

    // Document-level embedded file attachments (PDF §7.11.4). The
    // returned reference is bound to the Document — callers must
    // not hold it beyond the Document's lifetime. Entries persist
    // through Save() via the Catalog's /Names /EmbeddedFiles name
    // tree.
    EmbeddedFileCollection& EmbeddedFiles();

    // Document-level named destinations (PDF §12.3.2.3 — the Catalog's
    // /Names /Dests name tree). Lazily constructed on first access; the
    // returned reference is bound to the Document — callers must not hold
    // it beyond the Document's lifetime.
    NamedDestinationCollection& NamedDestinations();

    // Document outline / bookmark tree (PDF §12.3.3 — the Catalog's
    // /Outlines dictionary). Lazily constructed on first access and populated
    // from the existing /Outlines tree; the returned reference is bound to the
    // Document — callers must not hold it beyond the Document's lifetime.
    OutlineCollection& Outlines();

    // Document page labels (PDF §12.4.2 — the Catalog's /PageLabels number
    // tree). Lazily constructed on first access and populated from the
    // existing tree; the returned reference is bound to the Document.
    PageLabelCollection& PageLabels();

    // Interactive AcroForm of this document (PDF §12.7.3). Lazily
    // constructed on first access. The returned reference is
    // bound to the Document — callers must not hold it beyond
    // the Document's lifetime. Field add/edit operations persist
    // through Save() once AcroForm save-through lands at F8.
    Aspose::Pdf::Forms::Form& Form();

private:
    friend class Aspose::Pdf::Annotations::RedactionAnnotation;
    friend class Aspose::Pdf::Text::TextAbsorber;
    friend class Aspose::Pdf::Text::TextFragmentAbsorber;
    friend class Aspose::Pdf::Text::TextFragment;
    friend class Aspose::Pdf::Text::TextBuilder;
    friend class Aspose::Pdf::Devices::ImageDevice;
    friend class Aspose::Pdf::Devices::PngDevice;
    friend class Aspose::Pdf::Devices::JpegDevice;
    friend class Aspose::Pdf::Devices::BmpDevice;
    friend class Aspose::Pdf::Devices::TextDevice;
    friend class Aspose::Pdf::Devices::TiffDevice;
    friend class DocumentInfo;
    friend class PageCollection;
    friend class Page;
    friend class Aspose::Pdf::Facades::PdfFileEditor;
    friend class Aspose::Pdf::Facades::PdfBookmarkEditor;
    friend class Aspose::Pdf::Facades::PdfFileStamp;
    friend class Aspose::Pdf::Facades::PdfFileSignature;
    friend class Aspose::Pdf::Facades::PdfContentEditor;
    friend class Aspose::Pdf::Facades::PdfExtractor;

    // v1.1 metadata write-through. Builds the dirty list for an
    // incremental /Metadata stream update over the supplied
    // working buffer. When the Catalog already references a
    // /Metadata indirect, the stream is rewritten at that id
    // (in-place overwrite). When /Metadata is absent, a fresh
    // stream id is allocated (trailer.size) and the Catalog
    // joins the dirty list with the new /Metadata Ref injected.
    // Returns `working` unchanged on parse failure — forgiving
    // posture matching the /Info path.
    std::vector<std::byte> AppendMetadataUpdate(
        const std::vector<std::byte>& working) const;

    // Per-page annotation save-through. Walks page_annotations_,
    // allocates fresh indirect-object ids for each annotation,
    // emits PDF /Annot dictionaries, and updates each affected
    // Page dict's /Annots array. Pages whose collection is empty
    // are untouched. Returns `working` unchanged when no page has
    // a non-empty collection. v1 baseline emits /Type /Annot,
    // /Subtype <from AnnotationType>, /Rect, /Contents, /P
    // (parent page ref) — subtype-specific entries (Open / Icon
    // / etc.) deferred to follow-on beats.
    std::vector<std::byte> AppendAnnotationsUpdate(
        const std::vector<std::byte>& working) const;

    // Document-level embedded file save-through. For each
    // FileSpecification with a non-empty file path, emits an
    // /EmbeddedFile stream + /Filespec dict and updates the
    // Catalog with /Names /EmbeddedFiles → flat-leaf name tree.
    // v1 baseline reads file bytes from disk at save time;
    // uncompressed (no /Filter). When the Catalog already has
    // /Names /EmbeddedFiles, the entry is replaced (merge is a
    // follow-on). Files whose path is empty (or unreadable) are
    // skipped silently.
    std::vector<std::byte> AppendEmbeddedFilesUpdate(
        const std::vector<std::byte>& working) const;

    // Named-destination save-through (PDF §12.3.2.3). For each entry in
    // NamedDestinations(), serialises the destination as a /D array
    // ([page-ref /verb operands…]) and updates the Catalog with
    // /Names /Dests → flat-leaf name tree. Entries whose target is not an
    // ExplicitDestination (the resolvable case) are skipped. When loaded
    // entries were all removed, an empty /Dests tree is written (clears it).
    std::vector<std::byte> AppendNamedDestinationsUpdate(
        const std::vector<std::byte>& working) const;

    // AcroForm save-through (PDF §12.7.3). Walks
    // Document::Form()::Fields(), emits a combined field+widget
    // /Annot dict per field (canonical /T / /FT / /V / /Rect +
    // /Subtype /Widget per ISO 32000-1 §12.7.3.2 — terminal field
    // / widget merge), and injects an /AcroForm dict on the
    // Catalog with /Fields [refs]. v1 baseline:
    //   * Field-subtype dispatch via dynamic_cast → /FT name + /Ff
    //   * /T from PartialName; /V from Value() (string form)
    //   * /Rect from the Widget side
    //   * /Ff combines ReadOnly + Required + !Exportable + subtype
    //     bits (Multiline / Combo / Radio / Pushbutton / etc.)
    // The widget refs are NOT added to page /Annots in v1 — that
    // page-binding step lands in a follow-on beat together with
    // pageNumber tracking on Form::Add.
    std::vector<std::byte> AppendAcroFormUpdate(
        const std::vector<std::byte>& working) const;

    // Load-on-open: read the existing /AcroForm /Fields into `form` —
    // concrete Field subclass per /FT + /Ff, with /T, /V, /Rect, flags,
    // and type state (Multiline / Checked / Options + Selected). Each
    // loaded field is owned by `form` and keyed to its source object id
    // so AppendAcroFormUpdate can preserve it losslessly on re-save.
    void LoadAcroFormFields(Aspose::Pdf::Forms::Form& form);

    // Load-on-open: read the page-`leaf` /Annots into `coll` — concrete
    // Annotation subclass per /Subtype, with /Rect + /Contents. Each loaded
    // annotation is owned by `coll` and keyed to its source object id so
    // AppendAnnotationsUpdate preserves it losslessly on re-save. `page` is
    // the owning Page (annotation ctors bind to it).
    void LoadPageAnnotations(
        std::size_t leaf,
        Aspose::Pdf::Annotations::AnnotationCollection& coll,
        Aspose::Pdf::Page& page);

    // Load-on-open: read the catalog's /Names /EmbeddedFiles name tree into
    // `coll` — one FileSpecification per entry (Name / Description + the
    // decoded /EmbeddedFile stream bytes carried in FileSpecification's
    // content_). Backs attachment read / extract / remove round-trips.
    void LoadEmbeddedFiles(EmbeddedFileCollection& coll);

    // Load-on-open: read the catalog's /Names /Dests name tree into `coll` —
    // one entry per name, the /D array parsed into the matching typed
    // ExplicitDestination (the leading page reference resolved to a 1-based
    // page number via the page tree). Backs named-destination read round-trips.
    void LoadNamedDestinations(NamedDestinationCollection& coll);

    // Load-on-open: enumerate the page-`leaf` image XObjects into `coll` — one
    // XImage per /Resources /XObject /Subtype /Image (name, /Width, /Height,
    // /ImageMask, transparency, raw stream bytes).
    void LoadPageImages(std::size_t leaf, XImageCollection& coll) const;

    // Page-label save-through / load-on-open (PDF §12.4.2). Writes / reads the
    // Catalog's /PageLabels number tree (flat /Nums: index → label dict with
    // /S style, /St starting value, /P prefix).
    std::vector<std::byte> AppendPageLabelsUpdate(
        const std::vector<std::byte>& working) const;
    void LoadPageLabels(PageLabelCollection& coll);

    // Load-on-open: parse the Catalog's /Outlines tree into `root` — one
    // OutlineItemCollection per /Outlines item, with Title, Bold / Italic
    // (/F flags), Open (/Count sign), and the navigation target (/Dest →
    // ExplicitDestination / NamedDestination, or /A /URI → GoToURIAction),
    // recursing through /First / /Next.
    void LoadOutlines(OutlineCollection& root);

    // Convert a public OutlineItemCollection node (with its children) into the
    // staged-write OutlineNode form consumed by AppendOutlinesUpdate. Backs
    // the public Outlines() save-through.
    OutlineNode OutlineNodeFromItem(const OutlineItemCollection& item) const;

    // Attachment extraction (backs PdfExtractor): the embedded-file names,
    // and writing the named attachment's bytes (empty name => first) to a
    // file. Reads through EmbeddedFiles() (load-on-open).
    std::vector<std::string> AttachmentNamesInternal();
    bool ExtractAttachmentInternal(const std::string& name,
                                   const std::string& outPath);

    // ---- Page geometry (read + staged write) ----
    // Backing for Page::Rect / MediaBox / CropBox / Rotate / SetPageSize.
    // Reads resolve from the parsed pages_tree leaf (MediaBox origin is
    // assumed (0,0) — the common case) unless a write has been staged,
    // in which case the staged rectangle (with its real origin) is
    // returned so set->get round-trips faithfully. Writes are flushed at
    // Save() via AppendPageGeometryUpdate, which rewrites the affected
    // page dict's /MediaBox / /CropBox / /Rotate entries.
    Aspose::Pdf::Rectangle GetPageRectInternal(
        std::size_t leaf_index) const;
    Aspose::Pdf::Rectangle GetPageCropBoxInternal(
        std::size_t leaf_index) const;
    int GetPageRotationInternal(std::size_t leaf_index) const;
    void SetPageRectInternal(std::size_t leaf_index,
                             const Aspose::Pdf::Rectangle& rect);
    void SetPageCropBoxInternal(std::size_t leaf_index,
                                const Aspose::Pdf::Rectangle& rect);
    // TrimBox / BleedBox / ArtBox — each defaults to the CropBox when not
    // explicitly set (PDF §14.11.2), and stages a write on set.
    Aspose::Pdf::Rectangle ResolvePageBoxInternal(
        std::size_t leaf_index,
        const std::map<std::size_t, std::array<double, 4>>& pending,
        const char* key) const;
    Aspose::Pdf::Rectangle GetPageTrimBoxInternal(std::size_t leaf_index) const;
    void SetPageTrimBoxInternal(std::size_t leaf_index,
                                const Aspose::Pdf::Rectangle& rect);
    Aspose::Pdf::Rectangle GetPageBleedBoxInternal(
        std::size_t leaf_index) const;
    void SetPageBleedBoxInternal(std::size_t leaf_index,
                                 const Aspose::Pdf::Rectangle& rect);
    Aspose::Pdf::Rectangle GetPageArtBoxInternal(std::size_t leaf_index) const;
    void SetPageArtBoxInternal(std::size_t leaf_index,
                               const Aspose::Pdf::Rectangle& rect);
    void SetPageRotationInternal(std::size_t leaf_index, int degrees);
    std::vector<std::byte> AppendPageGeometryUpdate(
        const std::vector<std::byte>& working) const;

    // Render Page.Paragraphs() (Table layout) into each page's content stream
    // at Save — draws cell backgrounds, borders and text.
    std::vector<std::byte> AppendParagraphsUpdate(
        const std::vector<std::byte>& working) const;

    // Persist Page.Resources().Images() edits at Save: embed images staged by
    // XImageCollection::Add as new /XObject entries (no content draw — the
    // canonical Add only adds the resource) and remove images deleted from a
    // loaded collection (drops the /XObject entry + strips its `Do`).
    std::vector<std::byte> AppendImagesUpdate(
        const std::vector<std::byte>& working) const;

    // Render Page.Artifacts() (watermarks) into each page's content stream at
    // Save: a text or image overlay/underlay per artifact, honouring Position,
    // Rotation, Opacity and IsBackground (background = drawn beneath content).
    std::vector<std::byte> AppendArtifactsUpdate(
        const std::vector<std::byte>& working) const;

    // Compacting resave (backs Optimize / OptimizeResources): parse the
    // working buffer, keep only objects reachable from the catalog / Info, and
    // serialise a fresh single-section PDF (header + bodies + xref + trailer).
    std::vector<std::byte> CompactDocument(
        const std::vector<std::byte>& working) const;

    // ---- Page-tree mutation (eager: rewrites bytes_ + re-parses) ----
    // Backs PageCollection Add / Insert / Delete. v1 supports a flat
    // /Pages tree (root /Kids lists all leaf /Page refs directly) —
    // throws on a nested tree. Each call rewrites bytes_ via an
    // incremental /Kids + /Count update and re-parses tree_, so Count()
    // and indexing reflect the change immediately. Per-page caches
    // (annotations, staged geometry) are cleared on mutation since leaf
    // indices shift. `insertAt1Based <= 0` appends. When `copy` is true
    // the new page shallow-copies srcLeaf's /MediaBox / /Contents /
    // /Resources / /Rotate (shared object refs). Returns the new leaf
    // index.
    std::size_t AddPageInternal(int insertAt1Based, bool copy,
                                std::size_t srcLeaf, double width,
                                double height);
    void DeletePagesInternal(std::vector<int> pageNumbers1Based);
    void DeleteAllPagesInternal();

    // Cross-document page import (backs PdfFileEditor concatenate /
    // append / insert). Deep-copies each requested source page and the
    // object graph it reaches (contents / resources / fonts / xobjects)
    // into this document with renumbered ids, appends the new page refs
    // to the /Pages root at `insertAt1Based` (<=0 appends), rewrites
    // bytes_ via incremental update, and re-parses tree_. v1: flat
    // /Pages tree on both sides; /Parent is rebound, /Annots dropped on
    // imported pages. The source must outlive the call.
    void ImportPagesFrom(const Document& src,
                         const std::vector<int>& srcPages1Based,
                         int insertAt1Based);

    // ---- Document outline (/Outlines) — backs PdfBookmarkEditor ----
    // Stage the full outline tree for the next Save (replaces any
    // existing /Outlines). Empty + cleared=true removes /Outlines.
    void SetStagedOutlines(std::vector<OutlineNode> nodes);
    void ClearStagedOutlines();
    // Parse the current /Outlines into a flat (depth, title) list.
    std::vector<std::pair<int, std::string>> ParseOutlineItems() const;
    // Save-time /Outlines writer (builds the dict tree + injects the
    // Catalog /Outlines ref).
    std::vector<std::byte> AppendOutlinesUpdate(
        const std::vector<std::byte>& working) const;

    // Draw text stamps onto pages — appends a content stream (BT/Tf/Td/
    // Tj/ET in Helvetica) to each affected page's /Contents and adds the
    // shared Helvetica font to its /Resources, via one incremental
    // update; rewrites bytes_ + re-parses tree_. Eager (not staged).
    int PageCountInternal() const;
    double PageHeightInternal(std::size_t leaf) const;
    double PageWidthInternal(std::size_t leaf) const;
    void ApplyTextStamps(const std::vector<TextStampReq>& stamps);

    // Digitally sign the document (backs PdfFileSignature::Sign). Adds a
    // signature field + widget to the AcroForm (creating it, setting
    // /SigFlags), writes a /Sig dict with /Filter /Adobe.PPKLite,
    // /SubFilter /adbe.pkcs7.detached, a byte-exact /ByteRange and a
    // /Contents placeholder — then computes the SHA-256 digest over the
    // signed byte ranges, builds a detached PKCS#7 (foundation::pkcs7),
    // and patches it into the /Contents gap in place. Rewrites bytes_ +
    // re-parses tree_. Eager.
    void SignInternal(const SignatureRequest& req);

    // Read back every signature field in the AcroForm (/FT /Sig),
    // resolving /V to its /Sig dict. Backs the PdfFileSignature
    // enumerate / read / verify surface.
    std::vector<SignatureInfo> ReadSignaturesInternal() const;

    // Replace the literal `(src)` text operand with `(dest)` in the
    // content stream(s) of the given page (1-based; 0 = all pages).
    // Decodes FlateDecode content, does a literal `(src)`→`(dest)`
    // substitution (v1: contiguous literals in Tj/TJ), and rewrites the
    // stream uncompressed via incremental update. Returns the number of
    // replacements made. Backs PdfContentEditor::ReplaceText.
    int ReplaceTextInContent(const std::string& src, const std::string& dest,
                             int pageNumber1BasedOr0);

    // One located text show (a Tj literal) with its origin + font size,
    // backing TextFragmentAbsorber positions + write-back. `text` is the
    // Latin-1-decoded literal; `(x, y)` is the text-space origin and
    // `font_size` the active /Tf size. `stream_id` + `occurrence` locate
    // the literal for write-back (the `occurrence`-th identical `(text)` in
    // that content stream). v1: unscaled single-string Tj only.
    struct TextShow {
        std::string text;
        double x = 0.0;
        double y = 0.0;
        double font_size = 0.0;
        std::size_t leaf = 0;
        std::uint32_t stream_id = 0;
        std::size_t occurrence = 0;
    };
    // Scan the content streams of page `leaf` (or every page when
    // `allPages`) and return the located text shows in reading order.
    std::vector<TextShow> ScanTextShowsInternal(std::size_t leaf,
                                                bool allPages) const;

    // A pending fragment text edit (registered by TextFragment::Text on a
    // search result; applied at Save). Replaces the `occurrence`-th
    // `(old_literal)` with `(new_literal)` in content stream `stream_id`.
    struct TextEdit {
        std::uint32_t stream_id = 0;
        std::size_t occurrence = 0;
        std::string old_literal;
        std::string new_literal;
    };
    void RegisterTextEditInternal(const TextEdit& edit);

    // Apply a redaction over `region` on page `leaf`: stage removal of every
    // text show intersecting the region (so the text is gone from the saved
    // content, not merely hidden) and stage an opaque cover box + overlay text
    // drawn at Save. Backs RedactionAnnotation::Redact().
    void ApplyRedactionInternal(std::size_t leaf,
                                const Aspose::Pdf::Rectangle& region,
                                const std::string& overlayText,
                                double fontSize, int alignment);
    std::vector<std::byte> AppendRedactionsUpdate(
        const std::vector<std::byte>& working) const;
    std::vector<std::byte> AppendTextEditsUpdate(
        const std::vector<std::byte>& working) const;

    // Embed an image file (JPEG → DCTDecode passthrough; PNG → decode +
    // FlateDecode) as an /XObject on the page at `leaf` and draw it in
    // the given user-space rectangle (q W 0 0 H X Y cm /Img Do Q), via
    // incremental update. Backs Page::AddImage. Returns false on an
    // unreadable / unsupported image.
    bool AddImageToPage(std::size_t leaf, const std::string& imagePath,
                        double llx, double lly, double urx, double ury);
    // As AddImageToPage but from in-memory image-file bytes (JPEG / PNG).
    // When `autoAdjust` is true the drawn rectangle is shrunk to preserve
    // the image's aspect ratio and centred within (llx,lly)-(urx,ury). When
    // `clipBbox` is non-null and non-empty the draw is clipped to it. Backs
    // both Page::AddImage(file,...) (defaults) and Page::AddImage(bytes,...).
    bool AddImageBytesToPage(std::size_t leaf,
                             const std::vector<std::byte>& file_bytes,
                             double llx, double lly, double urx, double ury,
                             bool autoAdjust = false,
                             const Aspose::Pdf::Rectangle* clipBbox = nullptr);
    // Draw `text` on the given page at (x, y) in `fontName` at `fontSize`
    // points via an appended content stream + a Standard-14 font resource,
    // applied as an incremental update. Backs TextBuilder::AppendText.
    // `color` (when non-null and opaque, alpha > 0) is emitted as a non-stroking
    // RGB fill so the text shows in that colour; otherwise the text is black.
    bool AddTextToPage(std::size_t leaf, const std::string& text, double x,
                       double y, const std::string& fontName, double fontSize,
                       const Aspose::Pdf::Color* color = nullptr);
    // As AddTextToPage but embeds `fontProgram` (a TrueType font, from
    // FontRepository::OpenFont) into the page: emits a /TrueType font with a
    // FontFile2 program, FontDescriptor and a WinAnsi /Widths array, so the
    // text renders in the supplied font. Backs embedded-font text insertion.
    bool AddEmbeddedTextToPage(std::size_t leaf, const std::string& text,
                               double x, double y,
                               const std::vector<std::byte>& fontProgram,
                               const std::string& baseFont, double fontSize,
                               bool subset = false,
                               const Aspose::Pdf::Color* color = nullptr);
    // Write each image XObject on the given page (1-based; 0 = all) to a
    // file: `<basePath>` for the first, `<base>_<n><ext>` thereafter.
    // JPEG (DCTDecode) streams are written verbatim. Returns the number
    // of images written. Backs PdfExtractor image extraction.
    int ExtractImagesToFiles(int pageNumber1BasedOr0,
                             const std::string& basePath) const;
    // Write the `globalIndex`-th image XObject (across all pages, 0-based)
    // to `path` (DCTDecode streams written verbatim). Returns false when
    // out of range.
    bool ExtractImageToFile(int globalIndex, const std::string& path) const;
    // Count image XObjects across the given page (1-based; 0 = all).
    int CountImages(int pageNumber1BasedOr0) const;
    // Delete image XObjects from a page: removes the named entries from the
    // page /Resources /XObject and strips their `/<name> Do` draw operators
    // from the content streams, via incremental update. `imageNumbers1Based`
    // selects images by 1-based per-page index (empty = all on the page);
    // `pageNumber1BasedOr0` 0 = every page. Backs PdfContentEditor::
    // DeleteImage. Returns the number of images removed.
    int DeleteImagesFromPage(int pageNumber1BasedOr0,
                             const std::vector<int>& imageNumbers1Based);

    std::vector<std::byte> bytes_;
    std::optional<std::string> source_filename_;
    std::unique_ptr<foundation::pages_tree::Tree> tree_;

    // Set once during the password-validating ctor and on every
    // load that finds /Encrypt in the trailer. Cleared by
    // Decrypt(). file_key_ is the recovered document encryption
    // key (5 bytes for V=1, 5..16 for V=2, 16 for V=4, 32 for V=5)
    // — wired through to content consumers (TextAbsorber etc.) in
    // a future beat.
    bool is_encrypted_ = false;
    std::vector<std::byte> file_key_;

    // Staged encryption parameters set by Encrypt(...) and consumed
    // by the next Save(). When `encrypt_requested_` is true, Save
    // runs encrypt_writer + crypt_filter against every indirect
    // object in the current bytes_, then writes an encrypted PDF
    // via pdf_writer_incremental v2 with trailer /Encrypt + /ID
    // injection.
    bool encrypt_requested_ = false;
    std::string pending_user_password_;
    std::string pending_owner_password_;
    Aspose::Pdf::Permissions pending_permissions_ =
        Aspose::Pdf::Permissions::PrintDocument;
    CryptoAlgorithm pending_algorithm_ = CryptoAlgorithm::AESx128;
    bool pending_use_pdf20_ = false;

    std::uint32_t info_id_ = 0;
    std::vector<std::pair<std::string, std::string>> info_entries_;
    bool info_loaded_ = false;
    bool dirty_ = false;
    std::unique_ptr<DocumentInfo> info_;
    std::unique_ptr<Aspose::Pdf::Metadata> metadata_;
    std::unique_ptr<PageCollection> pages_;
    std::unique_ptr<EmbeddedFileCollection> embedded_files_;
    std::unique_ptr<NamedDestinationCollection> named_destinations_;
    std::unique_ptr<OutlineCollection> outlines_collection_;
    std::unique_ptr<PageLabelCollection> page_labels_;
    std::unique_ptr<Aspose::Pdf::Forms::Form> form_;

    // Staged page-geometry writes (leaf_index -> value), flushed by
    // AppendPageGeometryUpdate at Save(). MediaBox/CropBox stored as
    // [llx, lly, urx, ury]; rotation in degrees {0,90,180,270}.
    std::map<std::size_t, std::array<double, 4>> pending_mediabox_;
    std::map<std::size_t, std::array<double, 4>> pending_cropbox_;
    std::map<std::size_t, std::array<double, 4>> pending_trimbox_;
    std::map<std::size_t, std::array<double, 4>> pending_bleedbox_;
    std::map<std::size_t, std::array<double, 4>> pending_artbox_;
    std::map<std::size_t, int> pending_rotation_;
    bool page_geom_dirty_ = false;

    // Pending TextFragment text edits (Text setter on search results),
    // flushed by AppendTextEditsUpdate at Save().
    std::vector<TextEdit> pending_text_edits_;

    // A staged redaction overlay (cover box + overlay text), flushed by
    // AppendRedactionsUpdate at Save(). The text under the region is removed
    // via pending_text_edits_; this paints the opaque box on top.
    struct PendingRedaction {
        std::size_t leaf = 0;
        double llx = 0.0, lly = 0.0, urx = 0.0, ury = 0.0;
        std::string overlay_text;
        double font_size = 0.0;
        int alignment = 0;
    };
    std::vector<PendingRedaction> pending_redactions_;

    // Mutable: the public Outlines() tree is synced into the staged-write form
    // during the (const) Save() so it persists through AppendOutlinesUpdate.
    mutable std::vector<OutlineNode> staged_outlines_;
    mutable bool outlines_dirty_ = false;
    bool optimize_requested_ = false;

    // Per-page AnnotationCollection storage. Pages are value-
    // returned by PageCollection::operator[] so they can't own
    // their own collection; the Document holds one per leaf and
    // lazily grows the vector through Page::Annotations(). Held
    // as unique_ptr so AnnotationCollection's full definition
    // stays out of document.hpp.
    mutable std::vector<
        std::unique_ptr<Aspose::Pdf::Annotations::AnnotationCollection>>
        page_annotations_;
    mutable std::vector<std::unique_ptr<Paragraphs>> page_paragraphs_;
    mutable std::vector<std::unique_ptr<Resources>> page_resources_;
    mutable std::vector<std::unique_ptr<ArtifactCollection>> page_artifacts_;
};

}  // namespace Aspose::Pdf
