// S3 BOARD sprites and the cabinet. Everything is drawn at boot.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace board {

enum Pal {
    PAL_WHITE = 0,
    PAL_BG = 1,
    PAL_PIER = 2,
    PAL_INN = 3,
    PAL_FIRE = 4,
    PAL_CAB = 5,
    PAL_HALL = 6,
    PAL_WIRE = 7,
    PAL_CORD = 8,
    PAL_OP = 9,
    PAL_AMBER = 10,
    PAL_RED = 11,
    PAL_GREEN = 12
};

constexpr int JACKS = 6;
constexpr int TRUNK_X = 76;
constexpr int LINE_X = 244;
constexpr int JACK_Y0 = 86;
constexpr int JACK_DY = 22;

inline int jackY(int i) { return JACK_Y0 + i * JACK_DY; }

inline int callPal(int line) {
    if (line < 0) line = 0;
    if (line > 5) line = 5;
    return PAL_PIER + line;
}

inline const char* lineName(int i) {
    static const char* names[] = {"PIER", "INN", "FIRE", "CAB", "HALL", "WIRE"};
    if (i < 0) i = 0;
    if (i > 5) i = 5;
    return names[i];
}

struct Art {
    gs::Mipped lamp;
    gs::Mipped badge[JACKS];
    gs::Mipped plug;
    gs::Mipped bead;
    gs::Mipped ring;
    gs::Mipped bar;
    gs::Mipped spark;
    gs::Mipped op;
    gs::Mipped card;
    gs::Mipped glyph[96];
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace board
