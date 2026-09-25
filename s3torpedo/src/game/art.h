// S3 TORPEDO sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace torpedo {

constexpr int SHIP_W = 208;
constexpr int SHIP_H = 104;
constexpr float WL_X = 104.f;
constexpr float WL_Y = 52.f;

// Deep bilge under the boiler room, in bitmap pixels.
// sealHull() forces this box to vital plating and keeps the lane ahead of it
// (every pixel forward of x0, at or below y0) free of hull.
constexpr int BELLY_X0 = 128;
constexpr int BELLY_Y0 = 78;
constexpr int BELLY_X1 = 176;
constexpr int BELLY_Y1 = 98;
// Depths that clear the bow keel and still sit inside the bilge.
constexpr int SAFE_Y0 = 80;
constexpr int SAFE_Y1 = 96;

constexpr float FUNNEL_X = 154.f;
constexpr float FUNNEL_Y = 16.f;

enum Pal {
    PAL_HUD = 0,
    PAL_AMBER = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_SHIP = 4,
    PAL_SUB = 5,
    PAL_FX = 6,
    PAL_SKY = 7
};

struct Art {
    gs::Bitmap hull;  // unrotated ship, sampled for hits
    gs::Mipped ship[3];
    gs::Mipped sub;
    gs::Mipped torp;
    gs::Mipped bubble, splash, puff, wave, cloud, sun, oil, dash, tick;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace torpedo
