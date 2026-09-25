// S3 METRO sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace metro {

enum Pal {
    PAL_WHITE = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_DIM = 4,
    PAL_TRAIN = 5,
    PAL_PLAT = 6,
    PAL_WALL = 7,
    PAL_PEEP = 8,
    PAL_LAMP = 9,
    PAL_POST = 10,
    PAL_SIGNAL = 11,
    PAL_FX = 12
};

// Side view. The cab mark sits at NOSE_X; the lead bitmap's nose pixel is LEAD_TIP.
constexpr float PPM = 10.f;
constexpr float NOSE_X = 200.f;
constexpr float TRAIN_Y = 96.f;
constexpr float PLAT_Y = 168.f;
constexpr float RAIL_Y = 156.f;
constexpr int LEAD_TIP = 142;
constexpr int LEAD_BOGIE0 = 40;
constexpr int LEAD_BOGIE1 = 108;
constexpr int MID_BOGIE0 = 36;
constexpr int MID_BOGIE1 = 104;

struct Art {
    gs::Mipped lead, mid, wheel[3];
    gs::Mipped slab, rail, post, edge, boxSeg, mark, chevron;
    gs::Mipped person[4];
    gs::Mipped lamp, pipe, poster[3], girder, cloud, signal, spark, rain, plate, sign;
    gs::Mipped glyph[96];
    int font[96] = {};
};

struct Theme {
    uint16_t top, mid, pit;
    uint16_t tile, tileDk, postA, postB;
    uint16_t glow;
    bool open;
    bool wet;
};

const Theme& themeFor(int i);
void buildArt(gs::VDP& vdp, Art& art);

}  // namespace metro
