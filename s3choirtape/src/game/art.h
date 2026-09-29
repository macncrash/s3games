// S3 CHOIRTAPE pictures. Drawn into VRAM at boot. No asset files.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace choirtape {

enum Pal {
    PAL_TEXT = 0,
    PAL_NAVE = 1,
    PAL_TREBLE = 2,
    PAL_ALTO = 3,
    PAL_BASS = 4,
    PAL_HUM = 5,
    PAL_GOLD = 6,
    PAL_PAPER = 7,
    PAL_WOOD = 8,
    PAL_ALERT = 9
};

constexpr int kVoices = 4;
constexpr int kTapeN = 3;

struct Part {
    const char* name;
    int pay;
    bool decoy;
    int pal;
    float pitch;
};

// The tape is the first three. HUM pays the same as ALTO and stays out.
constexpr Part kPart[kVoices] = {
    {"TREBLE", 8, false, PAL_TREBLE, 523.25f},
    {"ALTO", 5, false, PAL_ALTO, 392.00f},
    {"BASS", 3, false, PAL_BASS, 196.00f},
    {"HUM", 5, true, PAL_HUM, 110.00f},
};

constexpr int kTape[kTapeN] = {0, 1, 2};

struct Art {
    int font[96] = {};
    gs::Mipped singer[kVoices];
    gs::Mipped reel;
    gs::Mipped slip;
    gs::Mipped note;
    gs::Mipped solid;
    gs::Mipped wordChoir;
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace choirtape
