// main.cpp - CLI parsing, interactive menu, conversion orchestration.
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "ASCIIConverter.h"
#include "Image.h"
#include "Renderer.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

using crushascii::ASCIIConverter;
using crushascii::AsciiArt;
using crushascii::ConvertMode;
using crushascii::ConvertOptions;
using crushascii::Image;
using crushascii::Renderer;
using crushascii::ResizedImage;

struct CliArgs {
    std::vector<std::string> inputs;  // one per positional/--input (drag-drop safe)
    std::string output;
    std::string html;
    std::string png;
    int pngScale = 2;
    std::string message;   // typed out before the art (typewriter effect)
    int speed = 0;         // ms per art line; 0 = instant drop
    bool speedSet = false;
    int width = 120;
    double gamma = 1.0;
    double contrast = 1.0;
    double brightness = 0.0;
    ConvertMode mode = ConvertMode::Detailed;
    std::string modeStr = "detailed";
    bool invert = false;
    bool color = false;
    bool pause = false;
    bool help = false;
    bool widthSet = false;
};

void printHelp(const char* prog) {
    std::cout
        << "CrushASCII - image to ASCII art converter\n"
        << "\n"
        << "Usage:\n"
        << "  " << prog << " <image> [options]\n"
        << "  " << prog << " --input <image> [options]\n"
        << "  " << prog << " <image1> <image2> ...   (each converted in turn)\n"
        << "  " << prog << "                     (interactive menu)\n"
        << "\n"
        << "Tip: drag & drop image file(s) onto CrushASCII.exe in Explorer.\n"
        << "Each dropped image is converted and a color PNG is saved next\n"
        << "to it (<name>_ascii.png), and the window stays open.\n"
        << "\n"
        << "Options:\n"
        << "  --input <path>       Input image (JPG/PNG/BMP/TGA/GIF/HDR via stb_image)\n"
        << "  --output <path>      Save ASCII art to text file\n"
        << "  --html <path>        Save color HTML page (implies color data;\n"
        << "                       terminal stays mono unless --color is given)\n"
        << "  --png <path>         Save color PNG image of the art (implies color\n"
        << "                       data; same terminal rule as --html)\n"
        << "  --scale <number>     PNG glyph scale, 1-8 (default: 2)\n"
        << "  --message <text>     Type this out, then drop the art slowly\n"
        << "  --speed <number>     Drop speed, ms per line, 0-2000 (default: 0\n"
        << "                       = instant; 40 when --message is given alone)\n"
        << "  --width <number>     ASCII columns, 10-800 (default: 120)\n"
        << "  --gamma <number>     Gamma correction, 0.1-5.0 (default: 1.0)\n"
        << "  --contrast <number>  Contrast multiplier, 0.1-5.0 (default: 1.0)\n"
        << "  --brightness <num>   Brightness offset, -1.0..1.0 (default: 0)\n"
        << "  --mode <basic|detailed>  Rendering mode (default: detailed)\n"
        << "  --invert             Invert brightness mapping\n"
        << "  --color              ANSI truecolor terminal output\n"
        << "  --pause              Wait for Enter before exiting (for shortcuts/\n"
        << "                       drag & drop; automatic in that case)\n"
        << "  --help               Show this help\n"
        << "\n"
        << "Examples:\n"
        << "  " << prog << " images/photo.jpg\n"
        << "  " << prog << " --input images/photo.jpg --width 120\n"
        << "  " << prog << " --input images/photo.jpg --width 160 --mode detailed\n"
        << "  " << prog << " --input images/photo.jpg --width 140 --gamma 1.0 --contrast 1.2\n"
        << "  " << prog << " --input images/photo.jpg --width 120 --output output/result.txt\n";
}

bool parseDouble(const std::string& s, double& out) {
    try {
        size_t pos = 0;
        out = std::stod(s, &pos);
        return pos == s.size();
    } catch (...) {
        return false;
    }
}

bool parseInt(const std::string& s, int& out) {
    try {
        size_t pos = 0;
        long v = std::stol(s, &pos);
        if (pos != s.size()) return false;
        out = static_cast<int>(v);
        return true;
    } catch (...) {
        return false;
    }
}

std::string toLower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower((unsigned char)c));
    return s;
}

// Returns exit code: 0 ok, 1 user/usage error. Sets args on success.
int parseArgs(int argc, char* argv[], CliArgs& args, std::string& err) {
    const std::string prog = argc > 0 ? argv[0] : "CrushASCII";
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto needValue = [&](const std::string& flag, std::string& dest) -> bool {
            if (i + 1 >= argc) {
                err = "Missing value for " + flag + ".";
                return false;
            }
            dest = argv[++i];
            return true;
        };
        if (a == "--help" || a == "-h" || a == "/?") {
            args.help = true;
        } else if (a == "--input" || a == "-i") {
            std::string v;
            if (!needValue("--input", v)) return 1;
            args.inputs.push_back(v);
        } else if (a == "--output" || a == "-o") {
            if (!needValue("--output", args.output)) return 1;
        } else if (a == "--html") {
            if (!needValue("--html", args.html)) return 1;
        } else if (a == "--png") {
            if (!needValue("--png", args.png)) return 1;
        } else if (a == "--message") {
            if (!needValue("--message", args.message)) return 1;
        } else if (a == "--scale") {
            std::string v;
            if (!needValue("--scale", v)) return 1;
            if (!parseInt(v, args.pngScale)) {
                err = "Invalid scale '" + v + "'. Expected an integer 1-8.";
                return 1;
            }
        } else if (a == "--speed") {
            std::string v;
            if (!needValue("--speed", v)) return 1;
            if (!parseInt(v, args.speed)) {
                err = "Invalid speed '" + v + "'. Expected an integer 0-2000.";
                return 1;
            }
            args.speedSet = true;
        } else if (a == "--width" || a == "-w") {
            std::string v;
            if (!needValue("--width", v)) return 1;
            if (!parseInt(v, args.width)) {
                err = "Invalid width '" + v + "'. Expected an integer 10-800.";
                return 1;
            }
            args.widthSet = true;
        } else if (a == "--gamma") {
            std::string v;
            if (!needValue("--gamma", v)) return 1;
            if (!parseDouble(v, args.gamma)) {
                err = "Invalid gamma '" + v + "'. Expected a number 0.1-5.0.";
                return 1;
            }
        } else if (a == "--contrast") {
            std::string v;
            if (!needValue("--contrast", v)) return 1;
            if (!parseDouble(v, args.contrast)) {
                err = "Invalid contrast '" + v + "'. Expected a number 0.1-5.0.";
                return 1;
            }
        } else if (a == "--brightness") {
            std::string v;
            if (!needValue("--brightness", v)) return 1;
            if (!parseDouble(v, args.brightness)) {
                err = "Invalid brightness '" + v + "'. Expected a number -1.0..1.0.";
                return 1;
            }
        } else if (a == "--mode" || a == "-m") {
            std::string v;
            if (!needValue("--mode", v)) return 1;
            v = toLower(v);
            if (v == "basic") {
                args.mode = ConvertMode::Basic;
                args.modeStr = "basic";
            } else if (v == "detailed") {
                args.mode = ConvertMode::Detailed;
                args.modeStr = "detailed";
            } else {
                err = "Invalid mode '" + v + "'. Use basic or detailed.";
                return 1;
            }
        } else if (a == "--invert") {
            args.invert = true;
        } else if (a == "--color") {
            args.color = true;
        } else if (a == "--pause") {
            args.pause = true;
        } else if (a.rfind("--", 0) == 0) {
            err = "Unsupported option '" + a + "'. Use --help to list options.";
            (void)prog;
            return 1;
        } else if (a.rfind("-", 0) == 0 && a.size() > 1) {
            err = "Unsupported option '" + a + "'. Use --help to list options.";
            return 1;
        } else {
            // Positional argument = input path. Several may be given
            // (Explorer passes one per dropped file); each is converted.
            args.inputs.push_back(a);
        }
    }
    return 0;
}

int validateArgs(const CliArgs& args, std::string& err) {
    if (args.width < 10 || args.width > 800) {
        err = "Invalid width " + std::to_string(args.width) +
              ". Width must be between 10 and 800.";
        return 1;
    }
    if (args.gamma < 0.1 || args.gamma > 5.0) {
        err = "Invalid gamma. Expected 0.1-5.0.";
        return 1;
    }
    if (args.contrast < 0.1 || args.contrast > 5.0) {
        err = "Invalid contrast. Expected 0.1-5.0.";
        return 1;
    }
    if (args.brightness < -1.0 || args.brightness > 1.0) {
        err = "Invalid brightness. Expected -1.0..1.0.";
        return 1;
    }
    if (args.pngScale < 1 || args.pngScale > 8) {
        err = "Invalid scale " + std::to_string(args.pngScale) +
              ". Scale must be between 1 and 8.";
        return 1;
    }
    if (args.speed < 0 || args.speed > 2000) {
        err = "Invalid speed " + std::to_string(args.speed) +
              ". Speed must be between 0 and 2000 ms.";
        return 1;
    }
    return 0;
}

// Full conversion pipeline for one image. `pngPath` is the resolved PNG
// destination (explicit --png or drag-drop default, empty = skip).
// Returns process exit code.
int runConversion(const CliArgs& args, const std::string& input,
                  const std::string& pngPath) {
    std::cout << "========================================\n"
              << "        CRUSH ASCII ART\n"
              << "========================================\n"
              << "\n"
              << "Input: " << input << "\n"
              << "Width: " << args.width << "\n"
              << "Mode: " << args.modeStr << "\n"
              << "Rendering...\n\n";

    Image img;
    if (!img.load(input)) {
        std::cerr << "Error: " << img.lastError() << "\n";
        std::cerr << "Tip: check the path and use a supported format "
                     "(JPG/PNG/BMP/TGA/GIF).\n";
        return 1;
    }

    ConvertOptions opt;
    opt.width = args.width;
    opt.gamma = args.gamma;
    opt.contrast = args.contrast;
    opt.brightness = args.brightness;
    opt.mode = args.mode;
    opt.invert = args.invert;
    // HTML/PNG export needs per-character colors; collect them whenever
    // either is requested, independent of terminal --color.
    opt.color = args.color || !args.html.empty() || !pngPath.empty();

    ResizedImage resized =
        img.toResized(opt.width, opt.charAspect, opt.gamma, opt.contrast,
                      opt.brightness);
    if (resized.width <= 0 || resized.height <= 0) {
        std::cerr << "Error: failed to process image (out of memory?).\n";
        return 1;
    }

    AsciiArt art = ASCIIConverter::convert(resized, opt);

    // Every line is exactly art.width characters, so alignment holds as
    // long as the console is wide enough. Warn instead of wrapping silently.
    const int consoleCols = Renderer::consoleWidth();
    if (consoleCols > 0 && art.width > consoleCols) {
        std::cout << "Note: ASCII width (" << art.width
                  << ") exceeds console width (" << consoleCols
                  << "); lines will wrap and look misaligned.\n"
                  << "Try --width " << consoleCols << " or widen the terminal.\n\n";
    }

    Renderer::enableAnsi();
    // Terminal honors --color only; the full-color art is preserved for HTML.
    AsciiArt termArt = art;
    if (!args.color) {
        termArt.hasColor = false;
        termArt.colors.clear();
    }
    // Optional romance mode: type the message, then drop the art slowly.
    // --speed sets the per-line delay; a bare --message drops at 40ms/line.
    if (!args.message.empty()) {
        Renderer::typeText(args.message, 35);
        std::cout << '\n';
    }
    const int lineDelay = args.speedSet  ? args.speed
                          : !args.message.empty() ? 40
                                                  : 0;
    Renderer::printAnimated(termArt, lineDelay);

    if (!args.output.empty()) {
        // Ensure parent directory exists for convenience.
        std::error_code ec;
        std::filesystem::path p(args.output);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path(), ec);
        }
        std::string err;
        if (!Renderer::saveToFile(art, args.output, err)) {
            std::cerr << "Error: " << err << "\n";
            return 1;
        }
        std::cout << "\nSaved to: " << args.output << "\n";
    }

    if (!args.html.empty()) {
        std::error_code ec;
        std::filesystem::path p(args.html);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path(), ec);
        }
        std::string err;
        if (!Renderer::saveToHtml(art, args.html, err)) {
            std::cerr << "Error: " << err << "\n";
            return 1;
        }
        std::cout << "\nColor HTML saved to: " << args.html << "\n";
    }

    if (!pngPath.empty()) {
        std::error_code ec;
        std::filesystem::path p(pngPath);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path(), ec);
        }
        std::string err;
        if (!Renderer::saveToPng(art, pngPath, args.pngScale, err)) {
            std::cerr << "Error: " << err << "\n";
            return 1;
        }
        std::cout << "\nColor PNG saved to: " << pngPath << "\n";
    }

    std::cout << "\n========================================\n"
              << "Rendering complete.\n"
              << "========================================\n";
    return 0;
}

std::string promptLine(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    // Trim surrounding whitespace/quotes (drag&drop paths are quoted).
    while (!s.empty() && std::isspace((unsigned char)s.front()))
        s.erase(s.begin());
    while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
    if (s.size() >= 2 &&
        ((s.front() == '"' && s.back() == '"') ||
         (s.front() == '\'' && s.back() == '\''))) {
        s = s.substr(1, s.size() - 2);
    }
    return s;
}

// Scans the images/ folder next to the working directory for supported
// image files. Lets the user pick whatever they dropped in by number,
// so any image "just shows" without typing a path.
std::vector<std::string> listImagesInFolder() {
    static const char* kExts[] = {".jpg", ".jpeg", ".png",
                                  ".bmp", ".tga",  ".gif", ".hdr"};
    std::vector<std::string> found;
    std::error_code ec;
    std::filesystem::directory_iterator it("images", ec);
    if (ec) return found;  // no images/ folder -> no list, typed path still works
    for (const auto& entry : it) {
        if (ec) break;
        if (!entry.is_regular_file(ec) || ec) continue;
        std::string ext = toLower(entry.path().extension().string());
        for (const char* k : kExts) {
            if (ext == k) {
                found.push_back(entry.path().string());
                break;
            }
        }
    }
    std::sort(found.begin(), found.end());
    return found;
}

// Shared prompts for options 1 & 2: pick an image (gallery list),
// ASCII width, rendering mode. Returns false when the user input is
// invalid (caller should `continue` the menu loop).
bool promptConvertArgs(CliArgs& args) {
    const std::vector<std::string> gallery = listImagesInFolder();
    if (!gallery.empty()) {
        std::cout << "\nImages found in images/:\n";
        for (size_t i = 0; i < gallery.size(); ++i) {
            std::cout << "  " << (i + 1) << ". " << gallery[i] << "\n";
        }
    }
    std::string picked = promptLine(
        gallery.empty() ? "Image path: "
                        : (gallery.size() == 1 ? "Image path [1]: "
                                               : "Image number or path [1]: "));
    if (picked.empty() && !gallery.empty()) {
        picked = gallery[0];  // Enter just shows it
    } else if (!gallery.empty()) {
        int n = 0;
        if (parseInt(picked, n) && n >= 1 &&
            n <= static_cast<int>(gallery.size())) {
            picked = gallery[(size_t)n - 1];
        }  // else: typed path, kept as-is
    }
    if (picked.empty()) {
        std::cerr << "Error: no image path given.\n\n";
        return false;
    }
    args.inputs = {picked};
    std::string w = promptLine("ASCII width [120]: ");
    if (!w.empty()) {
        if (!parseInt(w, args.width)) {
            std::cerr << "Error: invalid width. Use 10-800.\n\n";
            return false;
        }
    }
    std::string m =
        toLower(promptLine("Rendering mode (basic/detailed) [detailed]: "));
    if (!m.empty()) {
        if (m == "basic") {
            args.mode = ConvertMode::Basic;
            args.modeStr = "basic";
        } else if (m == "detailed") {
            args.mode = ConvertMode::Detailed;
            args.modeStr = "detailed";
        } else {
            std::cerr << "Error: invalid mode. Use basic or detailed.\n\n";
            return false;
        }
    }
    std::string err;
    if (validateArgs(args, err) != 0) {
        std::cerr << "Error: " << err << "\n\n";
        return false;
    }
    return true;
}

int interactiveMenu(const char* prog) {
    while (true) {
        std::cout << "========================================\n"
                  << "          CRUSH ASCII ART\n"
                  << "========================================\n"
                  << "\n"
                  << "1. Convert image\n"
                  << "2. Animated color drop\n"
                  << "3. Help\n"
                  << "4. Exit\n"
                  << "\n"
                  << "Select an option (1-4): ";
        std::string choice;
        if (!std::getline(std::cin, choice)) return 0;
        if (choice == "1") {
            CliArgs args;
            if (!promptConvertArgs(args)) continue;
            int rc = runConversion(args, args.inputs[0], args.png);
            std::cout << "\nPress Enter to continue...";
            std::string dummy;
            std::getline(std::cin, dummy);
            (void)rc;
        } else if (choice == "2") {
            // Romance mode: type a message, then drop the color image slowly.
            CliArgs args;
            if (!promptConvertArgs(args)) continue;
            args.message = promptLine("Message [I LOVE YOU]: ");
            if (args.message.empty()) args.message = "I LOVE YOU";
            std::string s = promptLine("Drop speed, ms per line [40]: ");
            if (!s.empty()) {
                if (!parseInt(s, args.speed) || args.speed < 0 ||
                    args.speed > 2000) {
                    std::cerr << "Error: invalid speed. Use 0-2000.\n\n";
                    continue;
                }
                args.speedSet = true;
            }
            args.color = true;  // animated drops are always color
            int rc = runConversion(args, args.inputs[0], args.png);
            std::cout << "\nPress Enter to continue...";
            std::string dummy;
            std::getline(std::cin, dummy);
            (void)rc;
        } else if (choice == "3") {
            printHelp(prog);
            std::cout << "\nPress Enter to continue...";
            std::string dummy;
            std::getline(std::cin, dummy);
        } else if (choice == "4") {
            std::cout << "Goodbye.\n";
            return 0;
        } else {
            std::cout << "Invalid choice. Enter 1, 2, 3 or 4.\n\n";
        }
    }
}

}  // namespace

// True when this process owns its console (double-click or drag & drop
// from Explorer): a fresh console hosts only our process, while a
// terminal session also hosts its shell. Used to keep the window open
// and to auto-save results for dropped files.
bool launchedWithOwnConsole() {
#ifdef _WIN32
    DWORD list[16] = {};
    const DWORD n = GetConsoleProcessList(list, 16);
    return n == 1;
#else
    return false;
#endif
}

// Default color-PNG destination next to a dropped image:
// "<dir>/<stem>_ascii.png".
std::string defaultDropPng(const std::string& input) {
    std::filesystem::path p(input);
    std::string stem = p.stem().string();
    if (stem.empty()) stem = "ascii";
    return (p.parent_path() / (stem + "_ascii.png")).string();
}

int main(int argc, char* argv[]) {
    const char* prog = argc > 0 ? argv[0] : "CrushASCII";

    // No arguments -> interactive menu (command-line remains primary).
    if (argc == 1) {
        return interactiveMenu(prog);
    }

    CliArgs args;
    std::string err;
    if (parseArgs(argc, argv, args, err) != 0) {
        std::cerr << "Error: " << err << "\n";
        std::cerr << "Use '" << prog << " --help' for usage.\n";
        return 1;
    }
    if (args.help) {
        printHelp(prog);
        return 0;
    }
    if (args.inputs.empty()) {
        std::cerr << "Error: no input image supplied.\n";
        std::cerr << "Usage: " << prog << " --input <image> [--width 120] "
                  << "[--output result.txt]\n";
        std::cerr << "Use '" << prog << " --help' for details.\n";
        return 1;
    }
    if (validateArgs(args, err) != 0) {
        std::cerr << "Error: " << err << "\n";
        return 1;
    }
    // Drag & drop: own console + no explicit outputs -> save a color PNG
    // next to each dropped image, so a dropped color image yields a color
    // ASCII image back.
    const bool dropMode =
        launchedWithOwnConsole() && args.output.empty() &&
        args.html.empty() && args.png.empty();
    int rc = 0;
    for (size_t i = 0; i < args.inputs.size(); ++i) {
        const std::string pngPath =
            dropMode ? defaultDropPng(args.inputs[i]) : args.png;
        if (args.inputs.size() > 1) {
            std::cout << "\n--- Image " << (i + 1) << " of "
                      << args.inputs.size() << " ---\n\n";
        }
        const int one = runConversion(args, args.inputs[i], pngPath);
        if (one != 0) rc = one;  // keep going with the rest, report failure
    }
    if (dropMode || args.pause || launchedWithOwnConsole()) {
        std::cout << "\nPress Enter to exit...";
        std::string dummy;
        std::getline(std::cin, dummy);
    }
    return rc;
}
