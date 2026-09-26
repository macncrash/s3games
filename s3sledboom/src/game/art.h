// S3 SLED BOOM sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace sledboom {

enum Pal : int {
    PAL_HUD = 0,
    PAL_SLED = 1,
    PAL_DRIVE = 2,
    PAL_BOOM = 3,
    PAL_RIVAL = 4,
    PAL_TREE = 5,
    PAL_MILL = 6,
    PAL_SNOW = 7,
    PAL_MARK = 8,
    PAL_WIN = 9,
    PAL_ALERT = 10,
    PAL_BANNER = 11,
    PAL_ICE = 12,
    PAL_PAD = 13
};

struct Art {
    gs::Mipped team[16];
    gs::Mipped drive;
    gs::Mipped tower, beam, chain, hook, ring;
    gs::Mipped spruce, cornice, mill, smoke[2], lamp, stake;
    gs::Mipped puff, flake, pin;
    gs::Mipped title, delivered, onBoom, paused, leg;
    gs::Mipped crewTook, wentOver, broke, spilled, leftIce;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace sledboom
