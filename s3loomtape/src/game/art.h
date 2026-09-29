// S3 LOOMTAPE pictures. Drawn at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace loomtape {

constexpr int kTapeN = 3;
constexpr int kSlips = 6;
constexpr int kHold = 28;
constexpr int kBeats = 6;

enum Pal {
    PAL_WOOD = 0,
    PAL_WARP = 1,
    PAL_SHUTTLE = 2,
    PAL_CLOTH = 3,
    PAL_INK = 4,
    PAL_GOLD = 5,
    PAL_BAD = 6,
    PAL_DIM = 7,
    PAL_TAPE = 8,
    PAL_DRAWER = 9
};

struct Art {
    gs::Image frame;
    gs::Image warp;
    gs::Image shuttle;
    gs::Image reed;
    gs::Image heddle;
    gs::Image pick;
    gs::Image beam;
    gs::Image drawer;
    gs::Image spool;
    int font[96] = {};
};

struct Slip {
    const char* name;
    int pay;
    int tape;  // 0..2 if this slip belongs on the tape, else -1
    bool twin;
};

const Slip& slipAt(int i);
const char* tapeName(int i);
int tapePay(int i);

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace loomtape
