# ascii_neel

# CrushASCII

A fast, dependency-light **image → ASCII art** converter for the Windows console, written in clean C++17. Drop in any JPG/PNG/BMP image later and get high-detail terminal portraits.

## Romantic web experience (`web/`)

A premium one-page React surprise site lives in [`web/`](web/): a dark
cinematic landing page invites someone to type the magic words
(`i love you`), then reveals your image with glowing particles.

```bat
cd web
npm install
npm run dev
```

Place your image at `web/public/image.jpg` (any size — a
CrushASCII color PNG works beautifully) and open the shown URL
(usually http://localhost:5173/). Typing `i love you` (any case)
triggers the reveal automatically; a subtle Replay button resets it.
`npm run build` produces a static site with no backend. Your private
image is git-ignored and never committed.

## Features

- Image loading via **stb_image** (single header, public domain): JPG/JPEG, PNG, BMP, TGA, GIF, HDR
- Rec.709 luminance (`0.2126 R + 0.7152 G + 0.0722 B`) with alpha ignored
- Configurable brightness ramp `"@%#*+=-:. "` (dark → light)
- Box-average downsampling to any ASCII width with terminal cell aspect correction
- Two modes: **basic** (pure brightness mapping) and **detailed** (3×3 neighborhood contrast-adaptive edge preservation)
- Tone controls: `--gamma`, `--contrast`, `--brightness`, `--invert`
- Optional ANSI **truecolor** output (`--color`); monochrome works everywhere
- Save plain-text result with `--output` (no ANSI codes in the file)
- Interactive menu when run with no arguments; CLI is the primary interface
- Graceful errors for every bad input — never crashes on user input

## Requirements

- Windows 10/11 (PowerShell, Windows Terminal, or Command Prompt)
- One of:
  - **MinGW-w64 g++** (easiest — direct compile, no extra install), or
  - **Visual Studio 2019+ with C++ workload + CMake 3.16+** (MSVC build)
- No other dependencies. `include/stb_image.h` is already in the repo.

## Build instructions

### Option A — direct g++ (recommended if you have MinGW)

PowerShell or CMD, from the project root:

```bat
g++ -std=c++17 -O2 -Wall -Wextra -Isrc -Iinclude src/main.cpp src/Image.cpp src/ASCIIConverter.cpp src/Renderer.cpp -o CrushASCII.exe
```

> In PowerShell the `src/*.cpp` wildcard is **not** expanded by every g++ port, so list the four files explicitly as above — that command is tested.

### Option B — CMake (MSVC or MinGW)

```bat
cmake -S . -B build
cmake --build build --config Release
```

The executable lands at `build\Release\CrushASCII.exe` (MSVC multi-config) or `build\CrushASCII.exe` (MinGW Makefiles).

## Usage

```bat
CrushASCII.exe images\photo.jpg
CrushASCII.exe --input images\photo.jpg
CrushASCII.exe --input images\photo.jpg --width 120
CrushASCII.exe --input images\photo.jpg --width 160 --mode detailed
CrushASCII.exe --input images\photo.jpg --width 140 --gamma 1.0 --contrast 1.2
CrushASCII.exe --input images\photo.jpg --width 120 --output output\result.txt
CrushASCII.exe --input images\photo.jpg --width 100 --color
CrushASCII.exe --input images\photos1.jpg --width 120 --mode detailed --contrast 1.1 --png output\photos1_color.png --scale 2
CrushASCII.exe --help
```

### All options

| Flag | Default | Meaning |
|---|---|---|
| `--input <path>` (or positional image path) | — | Input image |
| `--output <path>` | — | Save plain-text ASCII art to file |
| `--html <path>` | — | Save self-contained color HTML page (open in any browser); implies color data, terminal stays mono unless `--color` is also given |
| `--png <path>` | — | Render the art into a real color PNG image (8×8 glyphs, truecolor on black; white glyphs without color data) |
| `--scale <n>` | `2` | PNG glyph scale, 1–8 |
| `--message <text>` | — | Type the text out, then drop the art slowly |
| `--speed <n>` | `0` | Drop speed in ms per line, 0–2000 (`0` = instant; `40` when `--message` is given alone) |
| `--width <n>` | `120` | ASCII columns, 10–800 |
| `--gamma <n>` | `1.0` | Gamma, 0.1–5.0 (`>1` darkens mid-tones) |
| `--contrast <n>` | `1.0` | Contrast multiplier, 0.1–5.0 |
| `--brightness <n>` | `0` | Offset added after gamma/contrast, −1.0…1.0 |
| `--mode <basic\|detailed>` | `detailed` | `basic` = brightness only; `detailed` = + 3×3 edge preservation |
| `--invert` | off | Flip dark/light mapping (good for light backgrounds) |
| `--color` | off | ANSI truecolor output |
| `--pause` | off | Wait for Enter before exiting (automatic for drag & drop) |
| `--help` | — | Show help |

### Drag & drop (easiest for color images)

In Windows Explorer, drag any image file (or several) and drop it onto
`CrushASCII.exe`. Each dropped image is converted with default settings and
a **color PNG** (`<name>_ascii.png`) is saved right next to the original —
a dropped color image gives a color ASCII image back. The window stays open
so you can see the result. Add `--pause` to your own shortcuts for the same
keep-open behavior from the command line.

### Interactive mode

Run with no arguments:

```bat
CrushASCII.exe
```

```
========================================
          CRUSH ASCII ART
========================================

1. Convert image
2. Help
3. Exit
```

Choose **1** and it lists every image in `images/` — type its number
(or just press Enter for the first one), then width and mode.
Choose **2. Animated color drop** to type a message (default `I LOVE YOU`)
and watch the color image drop slowly, line by line.
Any supported image you drop into `images/` (JPG, PNG, BMP, TGA, GIF, HDR —
including transparency, grayscale, tiny or very large files) renders;
unreadable files fail with a clear error instead of crashing.
CLI mode remains recommended for repeatability.

## Supported image formats

Via stb_image: **JPG/JPEG, PNG, BMP, TGA, GIF (first frame), HDR**. Other formats fail with a clear error. Alpha channels are dropped (transparent areas render as their RGB color, typically black).

## ASCII character mapping

Brightness `v ∈ [0,1]` maps into the ramp by `index = round(v × (len−1))`:

```
@ % # * + = - : . (space)
dark ──────────────► light
```

So a black pixel → `@`, a white pixel → ` `. `--invert` reverses the index. `detailed` mode computes the 3×3 neighborhood standard deviation per cell and nudges high-contrast mid-tones one step darker, preserving edges that flat mapping would wash out.

## Width / aspect-ratio

Terminal glyphs are roughly twice as tall as wide, so a naive 1-pixel→1-char mapping looks vertically stretched. The converter downsamples to `--width` columns and computes rows as `rows = width × (imgH/imgW) × 0.55`. Practical advice:

- Start with `--width 120`; use 80 for narrow windows, 160–200 for wide Windows Terminal panes.
- If output wraps, **widen the terminal** or shrink `--width` — wrapping destroys alignment.
- Always use a **monospaced font** (Cascadia Mono, Consolas, JetBrains Mono).

## Troubleshooting

| Symptom | Fix |
|---|---|
| `File not found` | Check backslashes/quoting; drag-&-drop paths work (quotes are stripped) |
| `Failed to load image` | Format unsupported or file corrupt — try re-exporting as PNG/JPG |
| Art wraps into garbage | The program now warns when `--width` exceeds your console width — follow its suggestion, widen the terminal, or reduce `--width` |
| Gaps/speckle in flat areas | `detailed` mode smooths flat regions and only deepens the dark side of real edges; if a photo is very noisy, try `--contrast 1.1` or `basic` mode |
| `--color` shows escape codes (old cmd.exe) | Use Windows Terminal / newer console; monochrome default is unaffected |
| Output file not written | Check `--output` directory is writable; parents are auto-created |
| Too dark / washed out | Try `--gamma 0.8`, `--contrast 1.2`, or `--invert` for white backgrounds |

## Manual tests (no image committed)

```bat
CrushASCII.exe --help
CrushASCII.exe
CrushASCII.exe --input does-not-exist.jpg
CrushASCII.exe --input images\photo.jpg --width 80
CrushASCII.exe --input images\photo.jpg --width 160 --mode basic
CrushASCII.exe --input images\photo.jpg --width 160 --mode detailed
CrushASCII.exe --input images\photo.jpg --invert --width 100
CrushASCII.exe --input images\photo.jpg --width 120 --output output\result.txt
```

`images/` ships empty (only `.gitkeep`) — add your own photo later; nothing is hardcoded.

## Future improvements

- Multithreading (row-parallel loops are already structured for it; see `Image::toResized`)
- Custom `--ramp` and `--aspect` CLI flags
- Floyd–Steinberg dithering option
- HTML colored export
- Batch-folder conversion
