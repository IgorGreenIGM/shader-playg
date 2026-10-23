# C++ shaders

Six animated, CPU-rendered shader experiments: flowing colors, light trails,
neon hearts, and chromatic waves. Includes a Linux live preview that recompiles
`shaders.cpp` whenever you save it. Compilation and library loading run in the
background while the current shader keeps animating. A ready shader replaces it
between frames; failed or superseded builds never replace the working version.

## Run

Requires Linux, GCC with C++17/OpenMP, Make, and FFmpeg (including `ffplay`).

```sh
make
OMP_NUM_THREADS=8 ./build/renderer shaders.cpp 480 640 30 | \
  ffplay -f rawvideo -pixel_format rgb24 -video_size 480x640 -framerate 30 -i -
```

Change the `shader1(...)` call inside `render()` in `shaders.cpp` to choose
`shader1` through `shader6`. Vector helpers live in `vectors.hpp`; the heart
with lettering (`shader4`) lives in `xD.hpp`. After editing a header, save
`shaders.cpp` too to trigger recompilation. Lower the preview resolution if playback is slow.

`shader1` exposes `SPEED`, `ZOOM`, and `GLOW` near the top of the file.
Change a value and save to see the live update.

Run `make test` to check visible `shader6` animation and uninterrupted rendering
during slow compilation/loading, failed builds, and rapid successive saves.

## Demo

[Watch the screen recording](demo/whatsapp-status.mp4): VS Code and ffplay
side by side, editing only `shader1` parameters. 30 seconds, silent H.264 MP4.

```sh
make demo
```

Recording requires an X11 desktop of at least 1840×1050, VS Code (`code`),
Python 3, `xwininfo`, `libX11`, `libXtst`, and FFmpeg with `libx264` and `x11grab`.
The script opens the two windows and saves a preview in `build/capture/preview.png`.
Press Enter to start; leave the mouse and keyboard alone for 30 seconds.
It types and saves three parameter edits in a copy under `build/capture/`,
using an isolated VS Code profile. Your working shader stays unchanged.
