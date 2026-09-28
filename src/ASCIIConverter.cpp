// ASCIIConverter.cpp - brightness ramp mapping plus the "detailed"
// contrast-adaptive mode (3x3 neighborhood analysis).

#include "ASCIIConverter.h"

#include <algorithm>
#include <cmath>

namespace crushascii {
namespace {

inline float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

}  // namespace

char ASCIIConverter::mapBasic(float v, const ConvertOptions& opt) {
    v = clamp01(v);
    const size_t n = opt.ramp.size();
    if (n == 0) return ' ';
    if (n == 1) return opt.ramp[0];
    size_t idx = static_cast<size_t>(std::round(v * (n - 1)));
    if (idx >= n) idx = n - 1;
    if (opt.invert) idx = (n - 1) - idx;
    return opt.ramp[idx];
}

char ASCIIConverter::mapDetailed(const ResizedImage& img, int x, int y,
                                 const ConvertOptions& opt) {
    const int w = img.width;
    const int h = img.height;
    const float center = img.gray[static_cast<size_t>(y) * w + x];

    // 3x3 neighborhood mean + standard deviation. High deviation means
    // local structure (edge/texture); tiny deviation is just sensor/JPEG
    // noise in an otherwise flat area.
    float sum = 0.0f;
    int count = 0;
    for (int dy = -1; dy <= 1; ++dy) {
        const int yy = y + dy;
        if (yy < 0 || yy >= h) continue;
        for (int dx = -1; dx <= 1; ++dx) {
            const int xx = x + dx;
            if (xx < 0 || xx >= w) continue;
            sum += img.gray[static_cast<size_t>(yy) * w + xx];
            ++count;
        }
    }
    const float mean = count ? sum / count : center;
    float var = 0.0f;
    for (int dy = -1; dy <= 1; ++dy) {
        const int yy = y + dy;
        if (yy < 0 || yy >= h) continue;
        for (int dx = -1; dx <= 1; ++dx) {
            const int xx = x + dx;
            if (xx < 0 || xx >= w) continue;
            const float d = img.gray[static_cast<size_t>(yy) * w + xx] - mean;
            var += d * d;
        }
    }
    if (count) var /= count;
    const float stddev = std::sqrt(var);  // ~0..0.5 for natural images

    // Below this deviation the neighborhood is effectively flat: blend the
    // pixel toward the local mean so noise renders as one solid tone
    // instead of scattered speckles/holes.
    constexpr float kFlatThreshold = 0.03f;
    constexpr float kFlatBlend = 0.5f;

    float v = center;
    if (stddev < kFlatThreshold) {
        v = center + (mean - center) * kFlatBlend;
    } else if (center <= mean) {
        // Dark side of a real edge: deepen slightly to preserve detail.
        // The bright side is deliberately left untouched so no dark holes
        // get punched into highlights and smooth gradients.
        const float edgeBoost = std::min(0.12f, (stddev - kFlatThreshold) * 0.9f);
        // Mid-tones get the strongest boost; pure black/white are left alone.
        const float midWeight = 1.0f - std::fabs(center - 0.5f) * 2.0f;
        v = center - edgeBoost * midWeight;
    }

    return mapBasic(clamp01(v), opt);
}

AsciiArt ASCIIConverter::convert(const ResizedImage& img,
                                 const ConvertOptions& opt) {
    AsciiArt art;
    art.width = img.width;
    art.height = img.height;
    if (img.width <= 0 || img.height <= 0) return art;

    art.lines.reserve(img.height);
    if (opt.color) {
        art.colors.reserve(static_cast<size_t>(img.width) * img.height);
        art.hasColor = true;
    }

    for (int y = 0; y < img.height; ++y) {
        std::string line;
        line.reserve(img.width);
        for (int x = 0; x < img.width; ++x) {
            const char c = (opt.mode == ConvertMode::Detailed)
                               ? mapDetailed(img, x, y, opt)
                               : mapBasic(
                                     img.gray[static_cast<size_t>(y) *
                                              img.width + x],
                                     opt);
            line.push_back(c);
            if (opt.color) {
                const size_t i = static_cast<size_t>(y) * img.width + x;
                art.colors.push_back(
                    {img.red[i], img.green[i], img.blue[i]});
            }
        }
        art.lines.push_back(line);
    }
    return art;
}

}  // namespace crushascii
