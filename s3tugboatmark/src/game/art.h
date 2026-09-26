// S3 TUGBOAT MARK sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tugmark {

// Mark bitmap is 64 square. The yellow paint is 26px from center, the heart 8px.
// The leg sizes the sprite so those radii match the water.
constexpr int kMarkBmp = 64;
constexpr int kPaintPx = 26;
constexpr int kHeartPx = 8;
constexpr float kPixelsPerMetre = 4.65f;

enum Pal : int {
    PAL_HUD = 0,
    PAL_TUG = 1,
    PAL_QUAY = 2,
    PAL_MARK = 3,
    PAL_END = 4,
    PAL_FOAM = 5,
    PAL_SMOKE = 6,
    PAL_GULL = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_BANNER = 10,
    PAL_LAMP = 11,
    PAL_WATER = 12
};

struct Art {
    gs::Mipped tug[16];
    gs::Mipped shade;
    gs::Mipped quay, shed, crane, lamp, dolphin;
    gs::Mipped bulk, mark, boom;
    gs::Mipped buoy, foam, smoke, gull[2], pip, pin;
    gs::Mipped title, setDown, missed, off, shortB, longB, late, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tugmark
