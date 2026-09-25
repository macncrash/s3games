#pragma once

#include "console/gfx.h"

namespace kart {

enum Pal {
    PAL_TEXT = 0,
    PAL_GOLD = 1,
    PAL_ALERT = 2,
    PAL_PLAYER = 3,
    PAL_R0 = 4,
    PAL_PROP = 9,
    PAL_MAP = 10,
    PAL_LAMP = 11,
    PAL_ROAD = 12,
    PAL_FINISH = 13
};

struct Stamp {
    gs::Image img{};
    int w = 0, h = 0;
};

struct Art {
    gs::Mipped kart[6];
    gs::Image shadow, tree, stand, flag, cloud, hill, sun, lampOn, lampOff, dot, puff, map;
    gs::Image glyph[96];
    int gw[96] = {};
    int gh = 9;
    Stamp title, sub, pitch, help, press, youwin, ahead, pack, paused, go, d1, d2, d3;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace kart
