// S3 BOARDMARK pictures. Drawn into the VDP at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace boardmark {

enum Pal {
    PAL_INK = 0,
    PAL_WOOD = 1,
    PAL_GOLD = 2,
    PAL_TEAL = 3,
    PAL_ROSE = 4,
    PAL_CORD = 5,
    PAL_OK = 6,
    PAL_BAD = 7,
    PAL_NIGHT = 8
};

constexpr int JACKS = 5;
constexpr int MARK_LINE = 2;
constexpr int TRUNK_X = 72;
constexpr int LINE_X = 248;
constexpr int JACK_Y0 = 78;
constexpr int JACK_DY = 24;

inline int jackY(int i) { return JACK_Y0 + i * JACK_DY; }

struct Art {
    gs::Mipped lamp;
    gs::Mipped coin;
    gs::Mipped plug;
    gs::Mipped bead;
    gs::Mipped ring;
    gs::Mipped bar;
    gs::Mipped op;
    gs::Mipped door;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace boardmark
