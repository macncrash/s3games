// S3 GAUNTLET pictures. Everything is drawn at boot. There are no asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace gauntlet {

enum Pal {
    PAL_WHITE = 0,
    PAL_GOLD = 1,
    PAL_RED = 2,
    PAL_GREEN = 3,
    PAL_KNIGHT = 4,
    PAL_GRUNT = 5,
    PAL_ARCHER = 6,
    PAL_WARDEN = 7,
    PAL_FX = 8,
    PAL_ITEM = 9,
    PAL_DOOR = 10,
    PAL_FLOOR = 11,
    PAL_DIM = 12
};

struct Art {
    gs::Mipped knight[3][2];  // north, east, south; two steps. West is the east view flipped.
    gs::Mipped grunt[2];
    gs::Mipped archer[2];
    gs::Mipped warden[2];
    gs::Mipped slash[3];
    gs::Mipped bolt;
    gs::Mipped spark;
    gs::Mipped door;
    gs::Mipped doorOpen;
    gs::Mipped key;
    gs::Mipped flask;
    gs::Mipped torch[2];
    gs::Mipped pillar;
    gs::Mipped skull;
    gs::Mipped shadow;
    gs::Mipped glyph[96];
    int font[96] = {};
    int fl[4] = {};
    int carpet[2] = {};
    int wall[2] = {};
    int skirt = 0;
    int heart = 0;
};

void buildArt(gs::VDP& vdp, Art& art);
void paintHall(gs::VDP& vdp, const Art& art);

}  // namespace gauntlet
