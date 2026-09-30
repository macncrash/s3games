// S3 TILE BELL pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace tilebell {

constexpr int kTries = 3;
constexpr int kSockets = 4;
constexpr int kFaces = 4;
constexpr int kDieAt = 180;
constexpr int kPattern[kSockets] = {2, 0, 3, 1};

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_TILE = 4,
    PAL_GHOST = 5,
    PAL_BELL = 6,
    PAL_WOOD = 7,
    PAL_LAMP = 8
};

struct Art {
    gs::Image tile[kFaces];
    gs::Image socket;
    gs::Image cursor;
    gs::Image bell;
    gs::Image yoke;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace tilebell
