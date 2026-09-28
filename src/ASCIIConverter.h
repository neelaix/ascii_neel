#pragma once

#include <string>
#include <vector>

#include "Image.h"

namespace crushascii {

enum class ConvertMode { Basic, Detailed };

struct ConvertOptions {
    int width = 120;             // ASCII columns
    double gamma = 1.0;          // >1 darkens mid-tones, <1 brightens
    double contrast = 1.0;       // 1.0 = unchanged
    double brightness = 0.0;     // -1..1 offset added after gamma/contrast
    ConvertMode mode = ConvertMode::Detailed;
    bool invert = false;
    bool color = false;
    // Dense (dark) -> light (bright). Index 0 = darkest pixel.
    std::string ramp = "@%#*+=-:. ";
    double charAspect = 0.55;  // terminal cell height/width correction
};

struct Color {
    unsigned char r = 0, g = 0, b = 0;
};

struct AsciiArt {
    std::vector<std::string> lines;
    // Per-character colors, row-major (w*h). Only filled when color=true.
    std::vector<Color> colors;
    int width = 0;
    int height = 0;
    bool hasColor = false;
};

class ASCIIConverter {
public:
    static AsciiArt convert(const ResizedImage& img, const ConvertOptions& opt);

private:
    static char mapBasic(float v, const ConvertOptions& opt);
    static char mapDetailed(const ResizedImage& img, int x, int y,
                            const ConvertOptions& opt);
};

}  // namespace crushascii
