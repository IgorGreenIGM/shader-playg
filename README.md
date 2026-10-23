# C++ shader-playground

Six animated, CPU-rendered shader experiments: flowing colors, light trails,
neon hearts, and chromatic waves. Includes a Linux live preview that recompiles
`shaders.cpp` whenever you save it. Compilation and library loading run in the
background while the current shader keeps animating. A ready shader replaces it
between frames; failed or superseded builds never replace the working version.

## Run

Requires Linux, GCC with C++17/OpenMP, Make, and FFmpeg (including `ffplay`).

```sh
make
./build/renderer shaders.cpp 480 640 30 | \
  ffplay -f rawvideo -pixel_format rgb24 -video_size 480x640 -framerate 30 -i -
```

Change the `shader1(...)` call inside `render()` in `shaders.cpp` to choose
`shader1` through `shader6`. Vector helpers live in `vectors.hpp`; the heart
with lettering (`shader4`) lives in `xD.hpp`. After editing a header, save
`shaders.cpp` too to trigger recompilation. Lower the preview resolution if playback is slow.

`shader1` exposes `SPEED`, `ZOOM`, and `GLOW` near the top of the file.
Change a value and save to see the live update.