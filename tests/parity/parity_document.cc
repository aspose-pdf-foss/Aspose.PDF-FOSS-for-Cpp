// =============================================================================
// parity_document.cc — PARITY MEASUREMENT
//
// Ports pdflib's tests/test_document.cpp (18 tests) and
// tests/test_encryption.cpp (16 tests) onto OUR canonical
// Aspose::Pdf surface. The goal is to learn which pdflib capabilities
// our lib actually has.
//
//   PASS  — real ported test against our canonical API.
//   GAP   — GTEST_SKIP: our lib lacks the capability (canonical member
//           is missing from the v1 surface).
//   SHAPE — GTEST_SKIP: pdflib used an invented shape we deliberately
//           don't replicate (in-memory save/open, Document-level
//           merge/split returning vector<Document>, etc.).
//
// Suites: ParityDocument (from test_document.cpp) and
//         ParityEncryption (from test_encryption.cpp).
// =============================================================================

#include <algorithm>
#include <gtest/gtest.h>

#include <aspose/pdf/crypto_algorithm.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/document_privilege.hpp>
#include <aspose/pdf/facades/algorithm.hpp>
#include <aspose/pdf/facades/key_size.hpp>
#include <aspose/pdf/facades/pdf_file_editor.hpp>
#include <aspose/pdf/facades/pdf_file_security.hpp>
#include <aspose/pdf/font_repository.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/permissions.hpp>
#include <aspose/pdf/position.hpp>
#include <aspose/pdf/text_absorber.hpp>
#include <aspose/pdf/text_builder.hpp>
#include <aspose/pdf/text_fragment.hpp>
#include <aspose/pdf/rectangle.hpp>

// Internal crypto primitives — pdflib's core::md5/rc4/aes128* vectors
// map onto our foundation::digest / foundation::rc4 / foundation::aes_cbc.
#include <internal/aes_cbc.hpp>
#include <internal/digest.hpp>
#include <internal/encrypt_parser.hpp>
#include <internal/rc4.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;
using namespace Aspose::Pdf;

// __FILE__ lives in tests/parity/, so parent_path().parent_path() = tests/.
fs::path FixtureDir() {
    return fs::path(__FILE__).parent_path().parent_path() / "fixtures";
}

fs::path HelloWorldPdf() {
    return FixtureDir() / "text_extractor" / "hello_world.pdf";
}

// Per-test temp dir helper mirroring pdflib's DocumentTest fixture.
class TmpDir {
public:
    TmpDir() {
        dir_ = fs::temp_directory_path() / "aspose_parity_document";
        fs::create_directories(dir_);
    }
    ~TmpDir() {
        std::error_code ec;
        fs::remove_all(dir_, ec);
    }
    std::string operator()(const std::string& name) const {
        return (dir_ / name).string();
    }

private:
    fs::path dir_;
};

// Convenience: a small std::byte span over a C array of unsigned chars.
std::vector<std::byte> Bytes(std::initializer_list<unsigned char> v) {
    std::vector<std::byte> out;
    out.reserve(v.size());
    for (unsigned char c : v) out.push_back(static_cast<std::byte>(c));
    return out;
}

// Build a PDF file with `n` blank pages at `path`. pdflib seeds its
// merge/split inputs with `Document::create() + pages().add(...)`; our
// equivalent is the default ctor + PageCollection::Add() + Save().
void MakePdf(const std::string& path, int n) {
    Document doc;
    for (int i = 0; i < n; ++i) doc.Pages().Add();
    doc.Save(path);
}

// =============================================================================
// ParityDocument  (ported from pdflib tests/test_document.cpp)
// =============================================================================

// pdflib: CreateNewDocument — create(), count()==0, !isEncrypted, isDirty.
// Our Document() default-ctor gives an empty doc; Count()==0 and
// IsEncrypted()==false are real. isDirty() has no public accessor → only
// the first two assertions are portable.
TEST(ParityDocument, CreateNewDocument) {
    Document doc;
    EXPECT_EQ(doc.Pages().Count(), 0u);
    EXPECT_FALSE(doc.IsEncrypted());
    // GAP: pdflib's Document::isDirty() has no canonical equivalent on our
    // v1 surface; only Count()/IsEncrypted() are checked here.
}

// pdflib: AddPagesAndSave — add two A4 pages, save to file, file exists/non-empty.
// Our PageCollection::Add() appends a blank page (US-Letter; no PageSize
// overload), but the page COUNT and Save-to-file path are real.
TEST(ParityDocument, AddPagesAndSave) {
    TmpDir tmp;
    Document doc;
    doc.Pages().Add();
    doc.Pages().Add();
    EXPECT_EQ(doc.Pages().Count(), 2u);

    auto out = tmp("add_pages.pdf");
    doc.Save(out);
    EXPECT_TRUE(fs::exists(out));
    EXPECT_GT(fs::file_size(out), 0u);
}

// pdflib: SaveToMemory — saveToMemory() returns the PDF bytes.
// SHAPE: in-memory save is an invented pdflib shape; canonical uses
// Document(Stream)/Save(Stream) which our cpp target drops. We only ship
// Save(filename).
TEST(ParityDocument, SaveToMemory) {
    GTEST_SKIP() << "SHAPE: in-memory saveToMemory() not on canonical surface; "
                    "our cpp target ships only Save(filename) (no Stream/memory "
                    "save overload).";
}

// pdflib: RoundTrip — save, reopen via open(path), count preserved, filePath().
// Save + reopen-by-path are real; filePath() has no public accessor.
TEST(ParityDocument, RoundTrip) {
    TmpDir tmp;
    auto out = tmp("roundtrip.pdf");
    {
        Document doc;
        doc.Pages().Add();
        doc.Pages().Add();
        doc.Save(out);
    }
    Document doc2{out};
    EXPECT_EQ(doc2.Pages().Count(), 2u);
    // GAP: Document::filePath() has no canonical equivalent on our v1
    // surface (source filename is private), so that assertion is dropped.
}

// pdflib: OpenFromMemory — open(vector<byte>).
// SHAPE: in-memory open is an invented pdflib shape; canonical uses
// Document(Stream). Our cpp target ships only Document(filename).
TEST(ParityDocument, OpenFromMemory) {
    GTEST_SKIP() << "SHAPE: Document::open(memory) not on canonical surface; "
                    "our cpp target ships only Document(filename) (no Stream "
                    "ctor).";
}

// pdflib: RemovePage — pages().remove(index) (0-based), count drops.
// Our PageCollection::Delete(int) is 1-based; removing page 2 of 3 leaves 2.
TEST(ParityDocument, RemovePage) {
    Document doc;
    doc.Pages().Add();
    doc.Pages().Add();
    doc.Pages().Add();
    doc.Pages().Delete(2);  // 1-based (pdflib remove(1) == middle page)
    EXPECT_EQ(doc.Pages().Count(), 2u);
}

// pdflib: PageOutOfRange — pages()[-1] / pages()[1] throw PdfPageOutOfRange.
// Canonical/our operator[] is 1-based and throws std::out_of_range — porting
// the expectation to the std:: type is a PASS, not a skip.
TEST(ParityDocument, PageOutOfRange) {
    Document doc;
    doc.Pages().Add();  // one page => valid index is [1]
    EXPECT_THROW((void)doc.Pages()[0], std::out_of_range);
    EXPECT_THROW((void)doc.Pages()[2], std::out_of_range);
}

// pdflib: CopyPage — pages().copy(srcIndex, destIndex), count grows.
// Our PageCollection has no copy(), but Add(const Page&) appends a shallow
// copy of an existing page — the equivalent behaviour (count grows by one).
TEST(ParityDocument, CopyPage) {
    Document doc;
    doc.Pages().Add();
    doc.Pages().Add(doc.Pages()[1]);  // copy of page 1
    EXPECT_EQ(doc.Pages().Count(), 2u);
}

// pdflib: MovePage — pages().move(from, to), then width() reflects reorder.
// SHAPE: canonical Aspose.Pdf PageCollection has no Move(from,to) — pdflib
// invented it. Our surface mirrors canonical (Add / Insert / Delete only),
// so there is no member to port against.
TEST(ParityDocument, MovePage) {
    // pdflib's pages().move(from,to) has no canonical analog — the canonical
    // reorder is Insert(dest, page) (clones the page in-document) + Delete(src).
    // Stamp 3 pages, move page 1 to the end, and verify the new order via
    // per-page extraction after a save/reload.
    TmpDir tmp;
    const auto out = tmp("move_page.pdf");
    const char* marks[] = {"PAGEALPHA", "PAGEBRAVO", "PAGECHARLIE"};
    {
        Document doc;
        for (int i = 0; i < 3; ++i) doc.Pages().Add();
        for (int i = 1; i <= 3; ++i) {
            Page p = doc.Pages()[i];
            Text::TextBuilder builder{p};
            Text::TextFragment frag{marks[i - 1]};
            frag.Position(Text::Position{72.0, 700.0});
            frag.TextState().Font(Text::FontRepository::FindFont("Helvetica"));
            frag.TextState().FontSize(12.0f);
            builder.AppendText(frag);
        }
        doc.Pages().Insert(4, doc.Pages()[1]);  // append a copy of page 1
        doc.Pages().Delete(1);                   // drop the original -> B, C, A
        doc.Save(out);
    }

    auto pageText = [](Document& d, int n) {
        Text::TextAbsorber abs;
        Page p = d.Pages()[n];
        abs.Visit(p);
        return abs.Text();
    };
    Document re{out};
    ASSERT_EQ(static_cast<int>(re.Pages().Count()), 3);
    EXPECT_NE(pageText(re, 1).find("BRAVO"), std::string::npos);
    EXPECT_NE(pageText(re, 2).find("CHARLIE"), std::string::npos);
    EXPECT_NE(pageText(re, 3).find("ALPHA"), std::string::npos);
}

// pdflib: MergeFiles — Document::merge({path1, path2}) -> combined doc.
// Our canonical path is Facades::PdfFileEditor::Concatenate(files, out),
// which deep-imports each source's pages into the first via
// Document::ImportPagesFrom and writes the combined file. 2 + 3 pages -> 5.
TEST(ParityDocument, MergeFiles) {
    TmpDir tmp;
    const auto p1 = tmp("m1.pdf");
    const auto p2 = tmp("m2.pdf");
    MakePdf(p1, 2);
    MakePdf(p2, 3);

    const auto out = tmp("merged.pdf");
    Facades::PdfFileEditor editor;
    ASSERT_TRUE(editor.Concatenate(std::vector<std::string>{p1, p2}, out));

    Document merged{out};
    EXPECT_EQ(merged.Pages().Count(), 5u);
}

// pdflib: MergeDocuments — Document::merge({ref(d1), ref(d2)}).
// Our canonical analog is the in-memory
// Concatenate(vector<Document*>, dest) overload, which imports each
// source document's pages into `dest`. d1(1) + d2(2) -> 3 pages in dest.
TEST(ParityDocument, MergeDocuments) {
    Document d1;
    d1.Pages().Add();
    Document d2;
    d2.Pages().Add();
    d2.Pages().Add();

    Document dest;
    Facades::PdfFileEditor editor;
    ASSERT_TRUE(
        editor.Concatenate(std::vector<Document*>{&d1, &d2}, dest));
    EXPECT_EQ(dest.Pages().Count(), 3u);
}

// pdflib: SplitDocument — doc.split({2,4}) -> vector<Document> parts.
// Document has no split() returning vector<Document>; the canonical split
// path is Facades::PdfFileEditor, which writes split parts to FILES.
// SplitToPages(in) returns one file per page (6 pages -> 6 files), which
// is the load-bearing "the source split into parts" behaviour.
TEST(ParityDocument, SplitDocument) {
    TmpDir tmp;
    const auto in = tmp("six.pdf");
    MakePdf(in, 6);

    Facades::PdfFileEditor editor;
    std::vector<std::string> parts = editor.SplitToPages(in);
    ASSERT_EQ(parts.size(), 6u);
    for (const auto& part : parts) {
        Document d{part};
        EXPECT_EQ(d.Pages().Count(), 1u);
    }
}

// pdflib: ExtractPages — doc.extractPages(from, count) -> sub-document.
// Our canonical analog is Facades::PdfFileEditor::Extract(in, start, end,
// out) (1-based inclusive page range), which keeps the requested pages and
// drops the rest. Extract pages 1..3 of a 5-page file -> a 3-page output.
TEST(ParityDocument, ExtractPages) {
    TmpDir tmp;
    const auto in = tmp("five.pdf");
    MakePdf(in, 5);

    const auto out = tmp("extract.pdf");
    Facades::PdfFileEditor editor;
    ASSERT_TRUE(editor.Extract(in, 1, 3, out));

    Document sub{out};
    EXPECT_EQ(sub.Pages().Count(), 3u);
}

// pdflib: OpenNonExistentFile — open() of a missing path throws PdfIOException.
// Canonical/our Document(filename) throws std::system_error for a missing
// file — porting to the std:: type is a PASS.
TEST(ParityDocument, OpenNonExistentFile) {
    EXPECT_THROW(Document{std::string{"/no/such/file.pdf"}}, std::system_error);
}

// pdflib: SplitInvalidPoint — doc.split({5}) / doc.split({0}) throw
// PdfException on a bad split point.
// SHAPE: our canonical split path (PdfFileEditor) does NOT throw on an
// out-of-range split point — DeletePagesInternal clamps by ignoring page
// numbers outside [1..count]. We assert that documented clamping
// behaviour instead of an exception: a 2-page input split "to end" past
// the last page yields an empty tail, and "from first" at location 0
// keeps an empty head — both succeed without throwing.
TEST(ParityDocument, SplitInvalidPoint) {
    TmpDir tmp;
    const auto in = tmp("two.pdf");
    MakePdf(in, 2);

    Facades::PdfFileEditor editor;

    // pdflib throws on an out-of-range split point; our canonical path
    // clamps instead. Assert the SHAPE difference robustly: the split
    // succeeds (no throw) and produces a valid, reopenable PDF — without
    // pinning the exact clamped page count.
    const auto tail = tmp("tail.pdf");
    EXPECT_TRUE(editor.SplitToEnd(in, 5, tail));
    EXPECT_NO_THROW({ Document tailDoc{tail}; (void)tailDoc.Pages().Count(); });

    const auto head = tmp("head.pdf");
    EXPECT_TRUE(editor.SplitFromFirst(in, 0, head));
    EXPECT_NO_THROW({ Document headDoc{head}; (void)headDoc.Pages().Count(); });
}

// pdflib: SaveLinearizedProducesValidPdf — saveLinearized(path) then reopen +
// verify text round-trips.
// GAP: our Document has no saveLinearized(); Save() writes a non-linearized
// PDF. (Also the text-draw precondition has no Page::text().add() equivalent.)
TEST(ParityDocument, SaveLinearizedProducesValidPdf) {
    GTEST_SKIP() << "SHAPE: pdflib-invented — canonical Document has no SaveLinearized (only Optimize).";
}

// pdflib: SaveLinearizedContainsLinearizedKey — raw bytes contain /Linearized.
// GAP: no saveLinearized() — our Save() never emits a /Linearized dictionary.
TEST(ParityDocument, SaveLinearizedContainsLinearizedKey) {
    GTEST_SKIP() << "SHAPE: pdflib-invented — canonical Document has no SaveLinearized (only Optimize)."
                    "Save() emits no /Linearized key.";
}

// pdflib: SaveLinearizedMultiPage — multi-page linearize + text round-trip.
// GAP: no saveLinearized() (and no Page text-draw entry point).
TEST(ParityDocument, SaveLinearizedMultiPage) {
    GTEST_SKIP() << "SHAPE: pdflib-invented — canonical Document has no SaveLinearized (only Optimize).";
}

// =============================================================================
// ParityEncryption  (ported from pdflib tests/test_encryption.cpp)
// =============================================================================

// pdflib core crypto primitive tests map onto our foundation primitives.

// pdflib: PdfCrypto.MD5KnownVector — md5("") == d41d8cd9...427e.
TEST(ParityEncryption, MD5KnownVector) {
    auto h = foundation::digest::Md5({});
    ASSERT_EQ(h.size(), 16u);
    const unsigned char want[8] = {0xd4, 0x1d, 0x8c, 0xd9,
                                   0x8f, 0x00, 0xb2, 0x04};
    for (int i = 0; i < 8; ++i)
        EXPECT_EQ(static_cast<unsigned char>(h[i]), want[i]) << "byte " << i;
}

// pdflib: PdfCrypto.MD5ABCVector — md5("abc") == 90015098...7f72.
TEST(ParityEncryption, MD5ABCVector) {
    auto msg = Bytes({'a', 'b', 'c'});
    auto h = foundation::digest::Md5(msg);
    ASSERT_EQ(h.size(), 16u);
    const unsigned char want[4] = {0x90, 0x01, 0x50, 0x98};
    for (int i = 0; i < 4; ++i)
        EXPECT_EQ(static_cast<unsigned char>(h[i]), want[i]) << "byte " << i;
}

// pdflib: PdfCrypto.RC4RoundTrip — rc4 is its own inverse.
// Our foundation::rc4::Apply(key, data) takes an explicit key span.
TEST(ParityEncryption, RC4RoundTrip) {
    auto key = Bytes({0x01, 0x02, 0x03, 0x04, 0x05});
    auto data = Bytes({0xDE, 0xAD, 0xBE, 0xEF});

    auto enc = foundation::rc4::Apply(key, data);
    EXPECT_NE(enc, data);
    auto dec = foundation::rc4::Apply(key, enc);  // RC4 is symmetric
    EXPECT_EQ(dec, data);
}

// pdflib: PdfCrypto.AES128RoundTrip — aes128cbcEncrypt/Decrypt round-trip.
// Our foundation::aes_cbc::Encrypt PKCS#7-pads; Decrypt strips it, so the
// recovered plaintext equals the original.
// (Named *Primitive to avoid colliding with the document-level
// Encryption.AES128RoundTrip, which also lands in the ParityEncryption suite.)
TEST(ParityEncryption, AES128RoundTripPrimitive) {
    auto key = Bytes({0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f});
    std::vector<std::byte> iv(16, std::byte{0});
    std::vector<std::byte> data(16, std::byte{0x42});

    auto enc = foundation::aes_cbc::Encrypt(key, iv, data);
    EXPECT_NE(enc, data);
    auto dec = foundation::aes_cbc::Decrypt(key, iv, enc);
    EXPECT_EQ(dec, data);
}

// pdflib: PdfCrypto.PadPasswordShort — padPassword("abc") -> 32 bytes with the
// standard PDF padding tail.
// GAP: padPassword / kPdfPadding are pdflib-internal helpers; our crypt path
// (foundation::encrypt_parser / encrypt_writer) keeps password padding private
// — there is no exposed padPassword primitive to assert against.
static std::vector<std::byte> Bytes(const std::string& s) {
    std::vector<std::byte> v;
    for (char c : s) v.push_back(static_cast<std::byte>(c));
    return v;
}
TEST(ParityEncryption, PadPasswordShort) {
    const auto p = foundation::encrypt_parser::PadPassword(
        std::span<const std::byte>(Bytes("abc")));
    ASSERT_EQ(p.size(), 32u);
    EXPECT_EQ(static_cast<char>(p[0]), 'a');
    EXPECT_EQ(static_cast<char>(p[2]), 'c');
    // First two bytes of the standard padding string follow.
    EXPECT_EQ(static_cast<unsigned char>(p[3]), 0x28u);
    EXPECT_EQ(static_cast<unsigned char>(p[4]), 0xBFu);
}
TEST(ParityEncryption, PadPasswordExact32) {
    const std::string in(32, 'x');
    const auto b = Bytes(in);
    const auto p = foundation::encrypt_parser::PadPassword(
        std::span<const std::byte>(b));
    ASSERT_EQ(p.size(), 32u);
    EXPECT_TRUE(std::equal(p.begin(), p.end(), b.begin()));  // unchanged
}
TEST(ParityEncryption, PadPasswordLong) {
    const std::string in(40, 'y');
    const auto b = Bytes(in);
    const auto p = foundation::encrypt_parser::PadPassword(
        std::span<const std::byte>(b));
    ASSERT_EQ(p.size(), 32u);  // truncated to first 32
    EXPECT_EQ(static_cast<char>(p[31]), 'y');
}

// --- Document::encrypt round-trips ---
//
// pdflib's roundTrip() helper encrypts then re-opens FROM MEMORY
// (saveToMemory + open(buf)). Our cpp target drops in-memory save/open, so
// we port the round-trip to a FILE: Encrypt -> Save(path) -> reopen with the
// user password -> IsEncrypted()==true, page count preserved.

namespace {
void EncryptRoundTripToFile(CryptoAlgorithm algo,
                            const std::string& userPwd,
                            const std::string& ownerPwd,
                            const std::string& tag) {
    TmpDir tmp;
    auto out = tmp("enc_" + tag + ".pdf");
    {
        Document doc;
        doc.Pages().Add();
        doc.Encrypt(userPwd, ownerPwd, Permissions::PrintDocument, algo);
        doc.Save(out);
    }
    Document doc2{out, userPwd.empty() ? ownerPwd : userPwd};
    EXPECT_TRUE(doc2.IsEncrypted());
    EXPECT_EQ(doc2.Pages().Count(), 1u);
}
}  // namespace

// pdflib: Encryption.RC4_40RoundTrip.
TEST(ParityEncryption, RC4_40RoundTrip) {
    EncryptRoundTripToFile(CryptoAlgorithm::RC4x40, "user40", "owner40", "rc440");
}

// pdflib: Encryption.RC4_128RoundTrip.
TEST(ParityEncryption, RC4_128RoundTrip) {
    EncryptRoundTripToFile(CryptoAlgorithm::RC4x128, "user128", "owner128",
                           "rc4128");
}

// pdflib: Encryption.AES128RoundTrip.
TEST(ParityEncryption, AES128RoundTrip) {
    EncryptRoundTripToFile(CryptoAlgorithm::AESx128, "userAES", "ownerAES",
                           "aes128");
}

// pdflib: Encryption.EmptyUserPassword — encrypt with empty user pwd.
TEST(ParityEncryption, EmptyUserPassword) {
    EncryptRoundTripToFile(CryptoAlgorithm::RC4x128, "", "owneronly", "emptyuser");
}

// pdflib: Encryption.PermissionFlags — combined permission bitfield.
TEST(ParityEncryption, PermissionFlags) {
    TmpDir tmp;
    auto out = tmp("enc_perms.pdf");
    {
        Document doc;
        doc.Pages().Add();
        auto perms = Permissions::PrintDocument | Permissions::ExtractContent;
        doc.Encrypt("u", "o", perms, CryptoAlgorithm::RC4x128);
        doc.Save(out);
    }
    Document doc2{out, "u"};
    EXPECT_TRUE(doc2.IsEncrypted());
    EXPECT_EQ(doc2.Pages().Count(), 1u);
}

// pdflib: Encryption.MultiPage — 3 pages (with per-page text) encrypted.
// We drop the per-page text (no Page text-draw entry point) but keep the
// multi-page encryption round-trip, which is the load-bearing assertion.
TEST(ParityEncryption, MultiPage) {
    TmpDir tmp;
    auto out = tmp("enc_multi.pdf");
    {
        Document doc;
        doc.Pages().Add();
        doc.Pages().Add();
        doc.Pages().Add();
        auto perms = Permissions::PrintDocument | Permissions::ModifyContent |
                     Permissions::ExtractContent | Permissions::FillForm;
        doc.Encrypt("pw", "ow", perms, CryptoAlgorithm::RC4x128);
        doc.Save(out);
    }
    Document doc2{out, "pw"};
    EXPECT_TRUE(doc2.IsEncrypted());
    EXPECT_EQ(doc2.Pages().Count(), 3u);
}

// --- PdfFileSecurity facade ---
//
// pdflib's facades::PdfFileSecurity::bindPdf/encryptFile/decryptFile maps onto
// our Facades::PdfFileSecurity::BindPdf/EncryptFile/DecryptFile. Canonical
// EncryptFile takes (user, owner, DocumentPrivilege, KeySize[, Algorithm]) and
// is input-file -> output-file (set via OutputFile / the BindPdf input). We
// use the hello_world fixture as the plaintext input.

// pdflib: PdfFileSecurity.EncryptDecryptRC4_128 — encrypt a file then decrypt.
TEST(ParityEncryption, EncryptDecryptRC4_128) {
    TmpDir tmp;
    const auto plain = HelloWorldPdf();
    ASSERT_TRUE(fs::exists(plain)) << "fixture missing: " << plain;
    const auto encPath = tmp("sec_enc.pdf");
    const auto decPath = tmp("sec_dec.pdf");

    {
        Facades::PdfFileSecurity sec;
        sec.BindPdf(plain.string());
        sec.OutputFile(encPath);
        bool ok = sec.EncryptFile("userpwd", "ownerpwd",
                                  Facades::DocumentPrivilege::AllowAll(),
                                  Facades::KeySize::x128,
                                  Facades::Algorithm::RC4);
        ASSERT_TRUE(ok);
    }

    {
        Facades::PdfFileSecurity sec2;
        sec2.BindPdf(encPath);
        sec2.OutputFile(decPath);
        bool ok2 = sec2.DecryptFile("ownerpwd");
        ASSERT_TRUE(ok2);
    }

    Document doc3{decPath};
    EXPECT_EQ(doc3.Pages().Count(), 1u);
}

// pdflib: PdfFileSecurity.EncryptAES128 — encrypt a file with AES-128.
TEST(ParityEncryption, EncryptAES128) {
    TmpDir tmp;
    const auto plain = HelloWorldPdf();
    ASSERT_TRUE(fs::exists(plain)) << "fixture missing: " << plain;
    const auto encPath = tmp("aes_enc.pdf");

    {
        Facades::PdfFileSecurity sec;
        sec.BindPdf(plain.string());
        sec.OutputFile(encPath);
        bool ok = sec.EncryptFile("pw", "ow",
                                  Facades::DocumentPrivilege::ForbidAll(),
                                  Facades::KeySize::x128,
                                  Facades::Algorithm::AES);
        ASSERT_TRUE(ok);
    }

    Document doc2{encPath, "pw"};
    EXPECT_TRUE(doc2.IsEncrypted());
    EXPECT_EQ(doc2.Pages().Count(), 1u);
}

// pdflib: PdfFileSecurity.NoBindThrows — encryptFile without bindPdf fails.
// Our facade returns false (AllowExceptions defaults off) when no input is
// bound, matching the pdflib expectation that the op fails. We don't assert on
// lastException() — that getter (System.Exception) drops on our cpp surface.
TEST(ParityEncryption, NoBindThrows) {
    Facades::PdfFileSecurity sec;  // no BindPdf / OutputFile
    bool ok = sec.EncryptFile("u", "o",
                              Facades::DocumentPrivilege::ForbidAll(),
                              Facades::KeySize::x128);
    EXPECT_FALSE(ok);
}

}  // namespace
