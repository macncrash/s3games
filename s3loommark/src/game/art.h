// S3 LOOMMARK pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace loommark {

constexpr int kCols = 9;
constexpr int kRows = 9;
constexpr int kWarp0 = 108;
constexpr int kWarpGap = 14;
constexpr int kFell0 = 158;
constexpr int kPickGap = 8;
constexpr int kFrameX = 58;
constexpr int kFrameY = 6;

enum Pal {
    PAL_WOOD = 0,
    PAL_CLOTH = 1,
    PAL_SHUTTLE = 2,
    PAL_INK = 3,
    PAL_TITLE = 4,
    PAL_GOLD = 5,
    PAL_ALERT = 6,
    PAL_WIN = 7,
    PAL_DIM = 8,
    PAL_YARN = 9
};

struct Art {
    gs::Image frame;
    gs::Image warp;
    gs::Image cream;
    gs::Image mark;
    gs::Image shuttle;
    gs::Image reed;
    gs::Image heddle;
    gs::Image skein;
    gs::Image dot;
    gs::Image logo;
    gs::Image fin;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

inline int markAt(int row, int col) {
    // House mark. Row 0 is the first pick and sits at the bottom of the cloth.
    static const int m[kRows][kCols] = {
        {0, 0, 0, 0, 1, 0, 0, 0, 0}, {0, 0, 0, 1, 1, 1, 0, 0, 0}, {0, 0, 1, 1, 0, 1, 1, 0, 0},
        {0, 1, 1, 0, 0, 0, 1, 1, 0}, {1, 1, 0, 0, 1, 0, 0, 1, 1}, {0, 1, 1, 0, 0, 0, 1, 1, 0},
        {0, 0, 1, 1, 0, 1, 1, 0, 0}, {0, 0, 0, 1, 1, 1, 0, 0, 0}, {0, 0, 0, 0, 1, 0, 0, 0, 0},
    };
    if (row < 0 || col < 0 || row >= kRows || col >= kCols) return 0;
    return m[row][col];
}

inline int warpX(int col) { return kWarp0 + col * kWarpGap; }
inline int pickY(int row) { return kFell0 - row * kPickGap; }

}  // namespace loommark
