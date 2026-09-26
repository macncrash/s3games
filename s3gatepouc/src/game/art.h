// S3 GATE POUC sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace pouc {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_EARTH = 2,
    PAL_POUCH = 3,
    PAL_PLAYER = 4,
    PAL_PATROL = 5,
    PAL_WOOD = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_POST = 9
};

struct Art {
    gs::Mipped stand, runA, runB, duck, leap;
    gs::Mipped patrol[3];
    gs::Mipped pouch[2];
    gs::Mipped stool;
    gs::Mipped tower, arch, door;
    gs::Mipped lamp, flame[2];
    gs::Mipped post, guardShut, guardOpen, bell;
    gs::Mipped bar, beamPost;
    gs::Mipped hole, lip;
    gs::Mipped moon, stone, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace pouc
