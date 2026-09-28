// Image.cpp - stb_image loading, grayscale + box downsampling.
// Only implementation details live here; see Image.h for the interface.

#include "Image.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

// stb_image is a single-header library. Defining the implementation macro
// in exactly one TU compiles the decoder. Warnings from the header are
// silenced locally so /W4 builds stay clean.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100 4244 4267 4505 4996)
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace crushascii {
namespace {

// Rec.709 luminance: matches human perception of brightness.
inline float luminance(unsigned char r, unsigned char g, unsigned char b) {
    return static_cast<float>(0.2126 * r + 0.7152 * g + 0.0722 * b) / 255.0f;
}

inline float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

}  // namespace

bool Image::load(const std::string& path) {
    loaded_ = false;
    lastError_.clear();

    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(path, ec) || ec) {
        lastError_ = "File not found: " + path;
        return false;
    }

    int w = 0, h = 0, comp = 0;
    // Request 3 channels (RGB). stb_image drops/converts alpha for us,
    // so transparent backgrounds are simply ignored.
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &comp, 3);
    if (!data) {
        lastError_ = std::string("Failed to load image: ") + stbi_failure_reason();
        return false;
    }
    if (w <= 0 || h <= 0) {
        stbi_image_free(data);
        lastError_ = "Image has invalid dimensions.";
        return false;
    }

    const size_t count = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    pixels_.assign(data, data + count);
    stbi_image_free(data);

    width_ = w;
    height_ = h;
    channels_ = comp;  // original channel count, for info only
    loaded_ = true;
    return true;
}

ResizedImage Image::toResized(int targetWidth, double charAspect, double gamma,
                              double contrast, double brightness) const {
    ResizedImage out;
    if (!loaded_ || targetWidth <= 0) return out;

    const double srcAspect =
        static_cast<double>(height_) / static_cast<double>(width_);
    int targetHeight =
        static_cast<int>(std::round(targetWidth * srcAspect * charAspect));
    if (targetHeight < 1) targetHeight = 1;

    out.width = targetWidth;
    out.height = targetHeight;
    out.gray.assign(static_cast<size_t>(targetWidth) * targetHeight, 0.0f);
    out.red.assign(static_cast<size_t>(targetWidth) * targetHeight, 0);
    out.green.assign(static_cast<size_t>(targetWidth) * targetHeight, 0);
    out.blue.assign(static_cast<size_t>(targetWidth) * targetHeight, 0);

    // Box-average downsample: each ASCII cell averages its source rectangle.
    // Structured so the outer loop is trivially parallelizable later
    // (e.g. OpenMP / std::execution) without changing results.
    for (int cy = 0; cy < targetHeight; ++cy) {
        const int y0 = (cy * height_) / targetHeight;
        int y1 = ((cy + 1) * height_) / targetHeight;
        if (y1 <= y0) y1 = y0 + 1;
        for (int cx = 0; cx < targetWidth; ++cx) {
            const int x0 = (cx * width_) / targetWidth;
            int x1 = ((cx + 1) * width_) / targetWidth;
            if (x1 <= x0) x1 = x0 + 1;

            double sumR = 0.0, sumG = 0.0, sumB = 0.0;
            int n = 0;
            for (int y = y0; y < y1; ++y) {
                const size_t row = static_cast<size_t>(y) * width_ * 3u;
                for (int x = x0; x < x1; ++x) {
                    const size_t i = row + static_cast<size_t>(x) * 3u;
                    sumR += pixels_[i];
                    sumG += pixels_[i + 1];
                    sumB += pixels_[i + 2];
                    ++n;
                }
            }
            const auto avgR = static_cast<unsigned char>(sumR / n);
            const auto avgG = static_cast<unsigned char>(sumG / n);
            const auto avgB = static_cast<unsigned char>(sumB / n);

            float v = luminance(avgR, avgG, avgB);
            // Tone adjustments (applied once per output cell for speed).
            if (gamma != 1.0 && v > 0.0f) {
                v = static_cast<float>(std::pow(v, gamma));
            }
            if (contrast != 1.0) {
                v = static_cast<float>((v - 0.5) * contrast + 0.5);
            }
            if (brightness != 0.0) {
                v = static_cast<float>(v + brightness);
            }
            v = clamp01(v);

            const size_t o = static_cast<size_t>(cy) * targetWidth + cx;
            out.gray[o] = v;
            out.red[o] = avgR;
            out.green[o] = avgG;
            out.blue[o] = avgB;
        }
    }
    return out;
}

}  // namespace crushascii
