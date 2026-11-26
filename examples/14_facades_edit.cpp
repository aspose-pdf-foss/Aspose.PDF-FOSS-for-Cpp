// Edit an existing PDF with the classic Facades API:
//   * PdfFileStamp      — a footer stamp tagged with a StampId
//   * PdfContentEditor  — an attachment, JavaScript run on open, viewer
//                         preferences, and moving the stamp by its id
//   * PdfPageEditor     — shrink every page's content to 90 %, centred
//   * PdfBookmarkEditor — add bookmarks, export them as XML
//
//   usage: 14_facades_edit <input.pdf> <output.pdf> <bookmarks.xml>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/facades/bookmark.hpp>
#include <aspose/pdf/facades/pdf_bookmark_editor.hpp>
#include <aspose/pdf/facades/pdf_content_editor.hpp>
#include <aspose/pdf/facades/pdf_file_stamp.hpp>
#include <aspose/pdf/facades/pdf_page_editor.hpp>
#include <aspose/pdf/facades/viewer_preference.hpp>
#include <aspose/pdf/horizontal_alignment.hpp>
#include <aspose/pdf/vertical_alignment.hpp>

#include <iostream>

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: 14_facades_edit <input.pdf> <output.pdf> <bookmarks.xml>\n";
        return 2;
    }
    namespace fac = Aspose::Pdf::Facades;
    Aspose::Pdf::Document doc(argv[1]);

    // A footer stamp with id 7 on every page.
    fac::PdfFileStamp stamp{doc};
    stamp.StampId(7);
    stamp.AddFooter("Reviewed copy", 20.0f);

    // Content editor: attach this program's input, run JavaScript on open,
    // open with bookmarks shown in a two-column layout, move the stamp.
    fac::PdfContentEditor content{doc};
    content.AddDocumentAttachment(argv[1], "The original document");
    content.AddDocumentAdditionalAction(fac::PdfContentEditor::DocumentOpen,
                                        "app.alert('Opened the reviewed copy');");
    content.ChangeViewerPreference(fac::ViewerPreference::PageModeUseOutlines |
                                   fac::ViewerPreference::PageLayoutTwoColumnLeft);
    content.MoveStampById(1, 7, 72, 30);

    // Page editor: every page's content at 90 %, centred on the page.
    fac::PdfPageEditor pages{doc};
    pages.Zoom(0.9f);
    pages.HorizontalAlignment(Aspose::Pdf::HorizontalAlignment::Center);
    pages.VerticalAlignmentType(Aspose::Pdf::VerticalAlignment::Center);
    pages.ApplyChanges();

    // Bookmarks: one per page, saved with the document and exported as XML.
    fac::PdfBookmarkEditor bookmarks{doc};
    for (int i = 1; i <= pages.GetPages(); ++i)
        bookmarks.CreateBookmarkOfPage("Page " + std::to_string(i), i);
    doc.Save(argv[2]);

    Aspose::Pdf::Document saved(argv[2]);
    fac::PdfBookmarkEditor exporter{saved};
    exporter.ExportBookmarksToXML(argv[3]);

    std::cout << "Wrote " << argv[2] << " (stamp, attachment, open action, viewer "
              << "preferences, 90 % content, " << pages.GetPages()
              << " bookmarks) and " << argv[3] << "\n";
    return 0;
}
