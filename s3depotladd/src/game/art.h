// S3 DEPOT LADD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace depotladd {

enum Pal {
    PAL_HUD = 0,
    PAL_ALERT = 1,
    PAL_OK = 2,
    PAL_DIM = 3,
    PAL_AMBER = 4,
    PAL_YARD = 5,
    PAL_IRON = 6,
    PAL_CAR = 7,
    PAL_WOOD = 8,
    PAL_STEAM = 9,
    PAL_TANK = 10,
    PAL_CONC = 11,
    PAL_NIGHT = 12,
    PAL_DRUM = 13
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped ladder, drum, hook, cable, steam[2];
    gs::Mipped lamp, glow, signal, crate, wheel;
    gs::Mipped tower, clock, moon, dust, shadow, whistle;
    gs::Mipped glyph[96];
    int font[96] = {};
    int clap = 1, window = 1, winLit = 1;
    int conc = 1, safety = 1, brick = 1;
    int car = 1, carDoor = 1, roof = 1;
    int wood = 1, grate = 1, beam = 1, post = 1;
    int ballast = 1, rail = 1, tie = 1;
    int gold = 1;
    int tank = 1, tankBand = 1;
    int star = 1, starB = 1, shed = 1, shedB = 1, skyWin = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace depotladd
