// S3 BARGE BOOM sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace bargeboom {

enum Pal : int {
    PAL_HUD = 0,
    PAL_BARGE = 1,
    PAL_DRIVE = 2,
    PAL_BOOM = 3,
    PAL_BANK = 4,
    PAL_FOAM = 5,
    PAL_END = 6,
    PAL_CREW = 7,
    PAL_WIN = 8,
    PAL_ALERT = 9,
    PAL_BANNER = 10,
    PAL_BIRD = 11,
    PAL_CH = 12,
    PAL_MARK = 13
};

struct Art {
    gs::Mipped barge[16];
    gs::Mipped drive[16];
    gs::Mipped shade;
    gs::Mipped timber, pile, boomArm, reed, lamp, foam, bird[2], pin, chain;
    gs::Mipped title, delivered, shortOf, offBoom, broke, crew, missed, paused;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace bargeboom
