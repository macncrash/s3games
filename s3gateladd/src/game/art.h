// S3 GATE LADD sprites. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gateladd {

enum Pal {
    PAL_HUD = 0,
    PAL_STONE = 1,
    PAL_PLAYER = 2,
    PAL_IRON = 3,
    PAL_BRASS = 4,
    PAL_WARDEN = 5,
    PAL_WOOD = 6,
    PAL_FX = 7,
    PAL_ALERT = 8,
    PAL_OK = 9,
    PAL_FAR = 10,
    PAL_GOLD = 11,
    PAL_DIM = 12
};

struct Art {
    gs::Mipped stand, walkA, walkB, jump, climbA, climbB;
    gs::Mipped grate, post, winch, gem;
    gs::Mipped ladder, bell, torch, flame[2];
    gs::Mipped guard[2], banner, keystone;
    gs::Mipped moon, dust, shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
    int cope = 1, ash = 1, ashB = 1, slit = 1, slitLit = 1;
    int voidT = 1, yard = 1, yardB = 1, vous = 1, jamb = 1;
    int door = 1, mer = 1, ivy = 1;
    int star = 1, starB = 1, farW = 1, farR = 1;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace gateladd
