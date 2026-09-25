// S3 SLED pictures. Drawn into VRAM at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sled {

enum Pal {
    PAL_WHITE = 0,
    PAL_PLAYER = 1,
    PAL_RIVAL = 2,
    PAL_TREE = 3,
    PAL_ROCK = 4,
    PAL_BANNER = 5,
    PAL_FX = 6,
    PAL_TITLE = 7,
    PAL_MOUNT = 8,
    PAL_GOLD = 9,
    PAL_ALERT = 10,
    PAL_ICE = 11,
    PAL_SNOW = 12
};

struct Art {
    gs::Mipped tree[3];
    gs::Mipped rock[2];
    gs::Mipped playerSit, playerTuck, playerLean;
    gs::Mipped rival[4];
    gs::Mipped flag;
    gs::Mipped banner, drop;
    gs::Mipped flake, shadow;
    gs::Image titleMain, titleA, titleB;
    gs::Image count[4];
    gs::Image passed, lostLead, winText, loseText;
    int font[128] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sled
