// ============================================================================
// image_utils.cpp  --  see image_utils.hpp
// Owner: FF (Fauzan Fauzy), item 1 (shared foundation).
// ============================================================================
#include "image_utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <stdexcept>

// ---- stb_image / stb_image_write (compiled into this file only) -------------
// STB*_WINDOWS_UTF8 makes stb open UTF-8 paths correctly on Windows (OneDrive
// folders often contain non-ASCII characters). The pragmas silence warnings
// from the third-party code only, so our own warning flags stay strict.
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WINDOWS_UTF8
#define STBIW_WINDOWS_UTF8
#if defined(_MSC_VER)
#pragma warning(push, 0)
#pragma warning(disable : 4996)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif
#include "../../third_party/stb/stb_image.h"
#include "../../third_party/stb/stb_image_write.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace {

void checkSize(int rows, int cols, int channels, const char* where) {
    if (rows < 1 || cols < 1)
        throw std::invalid_argument(std::string(where) + ": rows and cols must be at least 1");
    if (channels != 1 && channels != 3)
        throw std::invalid_argument(std::string(where) + ": channels must be 1 or 3");
    if (static_cast<long long>(rows) * cols > kMaxImagePixels)
        throw std::invalid_argument(std::string(where) + ": image is too large");
}

// Paths are UTF-8 strings (what stb expects with STB*_WINDOWS_UTF8). u8path
// converts them correctly on Windows, where a plain path(std::string) would use
// the local code page instead.
std::filesystem::path utf8Path(const std::string& path) { return std::filesystem::u8path(path); }

std::string lowerExtension(const std::string& path) {
    std::string ext = utf8Path(path).extension().u8string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return ext;
}

// Owns the buffer stb_image returns, so it is freed on every path (incl. exceptions).
struct StbFree {
    void operator()(unsigned char* p) const { stbi_image_free(p); }
};

}  // namespace

Image makeImage(int rows, int cols, int channels, std::uint8_t value) {
    checkSize(rows, cols, channels, "makeImage");
    Image image;
    image.rows = rows;
    image.cols = cols;
    image.channels = channels;
    image.pixels.assign(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols) *
                            static_cast<std::size_t>(channels),
                        value);
    return image;
}

Image loadImage(const std::string& path) {
    int cols = 0, rows = 0, fileChannels = 0;
    // First read only the header: rejects huge or broken files before decoding.
    if (!stbi_info(path.c_str(), &cols, &rows, &fileChannels)) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error("loadImage: cannot read '" + path + "' (" +
                                 (reason ? reason : "unknown error") + ")");
    }
    if (cols < 1 || rows < 1 || static_cast<long long>(rows) * cols > kMaxImagePixels)
        throw std::runtime_error("loadImage: '" + path + "' has an unsupported size");

    // 1 = grey, 2 = grey + alpha, 3 = RGB, 4 = RGBA. Ask stb for 1 or 3 channels:
    // it converts (and drops alpha) for us.
    const int channels = (fileChannels <= 2) ? 1 : 3;
    std::unique_ptr<unsigned char, StbFree> data(
        stbi_load(path.c_str(), &cols, &rows, &fileChannels, channels));
    if (!data) {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error("loadImage: cannot decode '" + path + "' (" +
                                 (reason ? reason : "unknown error") + ")");
    }
    Image image = makeImage(rows, cols, channels);
    std::copy(data.get(), data.get() + image.pixels.size(), image.pixels.begin());
    return image;
}

void saveImage(const std::string& path, const Image& image) {
    checkImage(image, "saveImage");
    const std::string ext = lowerExtension(path);
    if (ext != ".png" && ext != ".bmp" && ext != ".tga" && ext != ".jpg" && ext != ".jpeg")
        throw std::invalid_argument("saveImage: unsupported file type '" + ext +
                                    "' (use .png, .bmp, .tga, .jpg or .jpeg)");

    const std::filesystem::path parent = utf8Path(path).parent_path();
    if (!parent.empty()) {
        std::error_code ignored;  // if this fails, the write below reports the error
        std::filesystem::create_directories(parent, ignored);
    }

    const void* data = image.pixels.data();
    int ok = 0;
    if (ext == ".png")
        ok = stbi_write_png(path.c_str(), image.cols, image.rows, image.channels, data,
                            image.cols * image.channels);
    else if (ext == ".bmp")
        ok = stbi_write_bmp(path.c_str(), image.cols, image.rows, image.channels, data);
    else if (ext == ".tga")
        ok = stbi_write_tga(path.c_str(), image.cols, image.rows, image.channels, data);
    else
        ok = stbi_write_jpg(path.c_str(), image.cols, image.rows, image.channels, data, 95);
    if (!ok) throw std::runtime_error("saveImage: cannot write '" + path + "'");
}

std::vector<double> toGrayscale(const Image& image) {
    checkImage(image, "toGrayscale");
    const std::size_t n = static_cast<std::size_t>(image.rows) * static_cast<std::size_t>(image.cols);
    std::vector<double> gray(n);
    const std::uint8_t* p = image.pixels.data();
    if (image.channels == 1) {
        for (std::size_t i = 0; i < n; ++i) gray[i] = p[i];
    } else {
        for (std::size_t i = 0; i < n; ++i, p += 3)
            gray[i] = 0.299 * p[0] + 0.587 * p[1] + 0.114 * p[2];
    }
    return gray;
}

Image transpose(const Image& image) {
    checkImage(image, "transpose");
    Image out = makeImage(image.cols, image.rows, image.channels);
    for (int r = 0; r < image.rows; ++r)
        for (int c = 0; c < image.cols; ++c)
            for (int k = 0; k < image.channels; ++k) out.at(c, r, k) = image.at(r, c, k);
    return out;
}

Image toRgb(const Image& image) {
    checkImage(image, "toRgb");
    if (image.channels == 3) return image;
    Image out = makeImage(image.rows, image.cols, 3);
    for (std::size_t i = 0; i < image.pixels.size(); ++i)
        out.pixels[3 * i] = out.pixels[3 * i + 1] = out.pixels[3 * i + 2] = image.pixels[i];
    return out;
}
