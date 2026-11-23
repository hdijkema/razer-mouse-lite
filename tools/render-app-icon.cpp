// Rebuild checked-in desktop icons from the SVG using FLTK's own renderer.
#include <FL/Fl_SVG_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc, char** argv)
{
    if (argc != 2) { std::cerr << "Usage: render-app-icon <assets-directory>\n"; return 1; }
    const std::filesystem::path directory(argv[1]);
    const int sizes[] = {16, 24, 32, 48, 64, 128, 256};
    std::vector<std::vector<char>> images;
    for (const auto size : sizes) {
        Fl_SVG_Image image((directory / "razer-badge.svg").string().c_str());
        if (image.fail()) return 1;
        image.resize(size, size); image.normalize();
        const auto png = directory / "razer-mouse-lite.png";
        if (fl_write_png(png.string().c_str(), &image) != 0) return 1;
        std::ifstream input(png, std::ios::binary);
        images.emplace_back(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        if (images.back().empty()) return 1;
    }
    std::ofstream output(directory / "razer-mouse-lite.ico", std::ios::binary);
    const auto word = [&output](unsigned value, int bytes) {
        for (int i = 0; i < bytes; ++i) output.put(static_cast<char>((value >> (8 * i)) & 255));
    };
    word(0, 2); word(1, 2); word(7, 2);
    unsigned offset = 6 + 7 * 16;
    for (int i = 0; i < 7; ++i) {
        word(sizes[i] == 256 ? 0 : sizes[i], 1); word(sizes[i] == 256 ? 0 : sizes[i], 1);
        word(0, 1); word(0, 1); word(1, 2); word(32, 2);
        word(static_cast<unsigned>(images[i].size()), 4); word(offset, 4);
        offset += static_cast<unsigned>(images[i].size());
    }
    for (const auto& bytes : images) output.write(bytes.data(), bytes.size());
    return output ? 0 : 1;
}
