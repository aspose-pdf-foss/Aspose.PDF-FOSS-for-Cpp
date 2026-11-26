// Embed a video (or audio) file in a PDF page as a RichMedia annotation,
// with an optional PNG / JPEG poster shown until it plays.
//
//   usage: 15_rich_media <input.pdf> <media file> <output.pdf> [poster.png]
//
// The media is embedded as an asset and played directly by the viewer
// (Acrobat plays MP4 / MP3). For a quick try-out any file will do, e.g.
//   ./15_rich_media pdfs/hello_world.pdf examples/pdflib.png out.pdf
// produces a valid annotation whose "media" just isn't playable.
#include <aspose/pdf/annotations/annotation_collection.hpp>
#include <aspose/pdf/annotations/rich_media_annotation.hpp>
#include <aspose/pdf/document.hpp>
#include <aspose/pdf/page.hpp>
#include <aspose/pdf/page_collection.hpp>
#include <aspose/pdf/rectangle.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {

std::vector<std::byte> ReadBytes(const char* path) {
    std::ifstream in(path, std::ios::binary);
    const std::vector<char> raw((std::istreambuf_iterator<char>(in)), {});
    return {reinterpret_cast<const std::byte*>(raw.data()),
            reinterpret_cast<const std::byte*>(raw.data()) + raw.size()};
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) {
        std::cerr << "usage: 15_rich_media <input.pdf> <media file> <output.pdf> [poster.png]\n";
        return 2;
    }
    namespace ann = Aspose::Pdf::Annotations;
    Aspose::Pdf::Document doc(argv[1]);
    auto page = doc.Pages()[1];

    // Keep the annotation alive until Save: the page holds a reference.
    ann::RichMediaAnnotation media(page, Aspose::Pdf::Rectangle(100, 300, 500, 600, true));
    const std::string name = std::filesystem::path(argv[2]).filename().string();
    media.SetContent(name, ReadBytes(argv[2]));
    const bool audio = name.size() > 4 && name.substr(name.size() - 4) == ".mp3";
    media.Type(audio ? ann::RichMediaAnnotation::ContentType::Audio
                     : ann::RichMediaAnnotation::ContentType::Video);
    media.ActivateOn(ann::RichMediaAnnotation::ActivationEvent::Click);
    if (argc == 5) media.SetPoster(ReadBytes(argv[4]));
    media.Update();  // checks the content is set
    page.Annotations().Add(media);

    doc.Save(argv[3]);
    std::cout << "Embedded " << name << " (" << media.Content().size() << " bytes) on page 1 of "
              << argv[3] << "\n";
    return 0;
}
