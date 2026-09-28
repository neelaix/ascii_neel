// Renderer.cpp - terminal printing (optional ANSI truecolor) + file saving.

#include "Renderer.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// 8x8 public-domain bitmap font: font8x8_basic[c][row], LSB = left pixel.
#include "font8x8_basic.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100 4244 4267 4505 4996)
#endif
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace crushascii {

void Renderer::enableAnsi() {
#ifdef _WIN32
    // Ask the Windows console to interpret ANSI escapes (needed for
    // --color on PowerShell / Windows Terminal / recent cmd.exe).
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return;
    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, mode);
#endif
}

void Renderer::print(const AsciiArt& art) {
    printAnimated(art, 0);
}

void Renderer::printAnimated(const AsciiArt& art, int lineDelayMs) {
    if (lineDelayMs < 0) lineDelayMs = 0;
    for (int y = 0; y < art.height; ++y) {
        const std::string& line = art.lines[(size_t)y];
        if (art.hasColor) {
            // One escape sequence per character: simplest correct
            // approach; keeps color cells aligned with characters.
            for (int x = 0; x < art.width; ++x) {
                const Color& c =
                    art.colors[(size_t)y * (size_t)art.width + (size_t)x];
                std::printf("\x1b[38;2;%d;%d;%dm%c", c.r, c.g, c.b,
                            line[(size_t)x]);
            }
            std::printf("\x1b[0m\n");
        } else {
            std::fputs(line.c_str(), stdout);
            std::fputc('\n', stdout);
        }
        // Flush every line so the drop animates even when piped, and
        // pause between lines (never after the last one).
        std::fflush(stdout);
        if (lineDelayMs > 0 && y + 1 < art.height) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(lineDelayMs));
        }
    }
}

void Renderer::typeText(const std::string& text, int charDelayMs) {
    if (charDelayMs < 0) charDelayMs = 0;
    for (char ch : text) {
        std::fputc(ch, stdout);
        std::fflush(stdout);
        if (charDelayMs > 0) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(charDelayMs));
        }
    }
    std::fputc('\n', stdout);
    std::fflush(stdout);
}

bool Renderer::saveToFile(const AsciiArt& art, const std::string& path,
                          std::string& errMsg) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        errMsg = "Cannot open output file for writing: " + path;
        return false;
    }
    for (const auto& line : art.lines) {
        out << line << '\n';
        if (!out) {
            errMsg = "Failed while writing output file: " + path;
            return false;
        }
    }
    out.close();
    if (!out) {
        errMsg = "Failed to finalize output file: " + path;
        return false;
    }
    return true;
}

int Renderer::consoleWidth() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return 0;
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(hOut, &info)) return 0;  // redirected?
    const int cols = static_cast<int>(info.srWindow.Right) -
                     static_cast<int>(info.srWindow.Left) + 1;
    return cols > 0 ? cols : 0;
#else
    return 0;  // primary target is Windows; unknown elsewhere
#endif
}

bool Renderer::saveToHtml(const AsciiArt& art, const std::string& path,
                          std::string& errMsg) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        errMsg = "Cannot open HTML file for writing: " + path;
        return false;
    }
    // Consecutive same-color characters share one span to keep files small.
    out << "<!DOCTYPE html>\n<html><head><meta charset=\"utf-8\">\n"
        << "<title>CrushASCII</title>\n<style>\n"
        << "body{background:#000;margin:2em;display:flex;"
           "justify-content:center;}\n"
        << "pre{font-family:Consolas,'Cascadia Mono',monospace;"
           "font-size:8px;line-height:1;letter-spacing:0;}\n"
        << "</style></head><body><pre>\n";
    for (int y = 0; y < art.height; ++y) {
        const std::string& line = art.lines[(size_t)y];
        int openR = -1, openG = -1, openB = -1;  // -1 = no span open
        for (int x = 0; x < art.width; ++x) {
            int r = -1, g = -1, b = -1;
            if (art.hasColor) {
                const Color& c =
                    art.colors[(size_t)y * (size_t)art.width + (size_t)x];
                r = c.r;
                g = c.g;
                b = c.b;
            }
            if (r != openR || g != openG || b != openB) {
                if (openR >= 0) out << "</span>";  // close previous run
                if (art.hasColor)
                    out << "<span style=\"color:rgb(" << r << ',' << g << ','
                        << b << ")\">";
                openR = r;
                openG = g;
                openB = b;
            }
            const char ch = line[(size_t)x];
            // Escape the few characters with HTML meaning; spaces pass
            // through untouched so column alignment is preserved exactly.
            if (ch == '&')
                out << "&amp;";
            else if (ch == '<')
                out << "&lt;";
            else if (ch == '>')
                out << "&gt;";
            else
                out.put(ch);
        }
        if (art.hasColor) out << "</span>";
        out << '\n';
        if (!out) {
            errMsg = "Failed while writing HTML file: " + path;
            return false;
        }
    }
    out << "</pre></body></html>\n";
    out.close();
    if (!out) {
        errMsg = "Failed to finalize HTML file: " + path;
        return false;
    }
    return true;
}

bool Renderer::saveToPng(const AsciiArt& art, const std::string& path,
                         int scale, std::string& errMsg) {
    if (scale < 1) scale = 1;
    if (scale > 8) scale = 8;
    if (art.width <= 0 || art.height <= 0) {
        errMsg = "Nothing to render: empty ASCII art.";
        return false;
    }
    constexpr int kCell = 8;  // font8x8 glyph size
    const int pxW = art.width * kCell * scale;
    const int pxH = art.height * kCell * scale;
    std::vector<unsigned char> px(static_cast<size_t>(pxW) * pxH * 3u, 0);

    for (int cy = 0; cy < art.height; ++cy) {
        const std::string& line = art.lines[(size_t)cy];
        for (int cx = 0; cx < art.width; ++cx) {
            unsigned char fr = 255, fg = 255, fb = 255;  // mono: white
            if (art.hasColor) {
                const Color& c =
                    art.colors[(size_t)cy * (size_t)art.width + (size_t)cx];
                fr = c.r;
                fg = c.g;
                fb = c.b;
            }
            const unsigned char glyph = static_cast<unsigned char>(line[(size_t)cx]);
            const unsigned char* rows = font8x8_basic[glyph < 128 ? glyph : 127];
            for (int gy = 0; gy < kCell; ++gy) {
                const unsigned char bits = static_cast<unsigned char>(rows[gy]);
                for (int gx = 0; gx < kCell; ++gx) {
                    if (!(bits & (1u << gx))) continue;  // background stays black
                    for (int sy = 0; sy < scale; ++sy) {
                        const int py = (cy * kCell + gy) * scale + sy;
                        const size_t base =
                            (static_cast<size_t>(py) * pxW +
                             (cx * kCell + gx) * scale) * 3u;
                        for (int sx = 0; sx < scale; ++sx) {
                            const size_t o = base + (size_t)sx * 3u;
                            px[o] = fr;
                            px[o + 1] = fg;
                            px[o + 2] = fb;
                        }
                    }
                }
            }
        }
    }

    if (!stbi_write_png(path.c_str(), pxW, pxH, 3, px.data(), pxW * 3)) {
        errMsg = "Cannot write PNG file: " + path;
        return false;
    }
    return true;
}

}  // namespace crushascii
