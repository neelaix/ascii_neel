#pragma once

#include <string>
#include <vector>

namespace crushascii {

// Downsampled image data ready for ASCII mapping.
// gray values are normalized to [0,1] (0 = black, 1 = white).
// rgb channels hold per-cell average colors for optional color output.
struct ResizedImage {
    int width = 0;
    int height = 0;
    std::vector<float> gray;
    std::vector<unsigned char> red;
    std::vector<unsigned char> green;
    std::vector<unsigned char> blue;
};

class Image {
public:
    Image() = default;

    // Loads an image from disk (JPG/PNG/BMP/TGA/GIF/HDR via stb_image).
    // Forces 3 RGB channels; alpha (if present) is dropped, which
    // effectively ignores transparency. Returns false on failure.
    bool load(const std::string& path);

    bool isLoaded() const { return loaded_; }
    int width() const { return width_; }
    int height() const { return height_; }
    int channels() const { return channels_; }
    const std::string& lastError() const { return lastError_; }

    // Downsample to targetWidth columns, preserving aspect ratio.
    // charAspect corrects for terminal cells being taller than wide
    // (typical ~0.5-0.6). Tone adjustments are applied per output cell.
    ResizedImage toResized(int targetWidth, double charAspect,
                           double gamma, double contrast,
                           double brightness) const;

private:
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
    bool loaded_ = false;
    std::vector<unsigned char> pixels_;  // RGB, row-major
    std::string lastError_;
};

}  // namespace crushascii
