// S3 FLUTE CHIME pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace flutechime {

constexpr int kNotes = 6;
constexpr int kOpenSec = 11 * 3600 + 59 * 60 + 50;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_WOOD = 5,
    PAL_PLAYER = 6,
    PAL_NIGHT = 7,
    PAL_LAMP = 8,
    PAL_CLOCK = 9
};

struct Art {
    gs::Mipped player;
    gs::Mipped flute;
    gs::Mipped stand;
    gs::Mipped staff;
    gs::Mipped note;
    gs::Mipped noteOn;
    gs::Mipped breath;
    gs::Mipped lamp;
    gs::Mipped clock;
    gs::Mipped hand;
    gs::Mipped bell;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace flutechime
