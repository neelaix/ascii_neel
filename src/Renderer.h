#pragma once

#include <string>

#include "ASCIIConverter.h"

namespace crushascii {

// Console + file output for generated ASCII art.
class Renderer {
public:
    // Enables ANSI escape-sequence processing on Windows 10+.
    // No-op on other platforms. Safe to call multiple times.
    static void enableAnsi();

    // Prints art to stdout. Uses 24-bit truecolor escapes when
    // art.hasColor is true; otherwise plain monochrome text.
    static void print(const AsciiArt& art);

    // Prints art line by line with lineDelayMs milliseconds between lines
    // (flushed per line), so the image "drops" slowly. Honors color the
    // same way print() does. No delay after the final line.
    static void printAnimated(const AsciiArt& art, int lineDelayMs);

    // Typewriter effect: prints text character by character with
    // charDelayMs between keystrokes, then a newline. Flushed per char.
    static void typeText(const std::string& text, int charDelayMs);

    // Saves plain monochrome text (no ANSI codes), preserving line
    // breaks exactly. Returns false and sets errMsg on failure.
    static bool saveToFile(const AsciiArt& art, const std::string& path,
                           std::string& errMsg);

    // Saves a self-contained HTML page: black background, monospace,
    // one <span> per character. Uses per-character truecolor when
    // art.hasColor is true, otherwise light-on-dark monochrome.
    // This is how color art is preserved to a file (ANSI codes would
    // not survive in plain text). Returns false + errMsg on failure.
    static bool saveToHtml(const AsciiArt& art, const std::string& path,
                           std::string& errMsg);

    // Renders the art into a real color image file (PNG): each ASCII
    // character is drawn with an 8x8 bitmap font, scaled by `scale`
    // (1-8), in its true color on a black background (white glyphs
    // when art.hasColor is false). Returns false + errMsg on failure.
    static bool saveToPng(const AsciiArt& art, const std::string& path,
                          int scale, std::string& errMsg);

    // Width of the attached console in columns, or 0 when unknown
    // (e.g. output redirected to a file). Used to warn about wrapping
    // that would break the art's alignment.
    static int consoleWidth();
};

}  // namespace crushascii
