// S3 FISHTAPE pictures. Drawn at boot. Nothing is loaded from a file.
//
// The tape wants DAB, BASS and PIKE.
// GOBY, PERCH and CARP pay those same counts and are not the tape.
#pragma once

#include <cmath>
#include <cstring>

#include "console/gfx.h"
#include "console/vdp.h"

namespace fishtape {

constexpr int kFishN = 6;
constexpr int kTapeN = 3;
constexpr int kMaxCasts = 6;
constexpr float kSweet = 0.08f;

constexpr float kSlotX[kTapeN] = {112.f, 180.f, 248.f};
constexpr float kSlotY = 208.f;
constexpr float kRodX = 62.f;
constexpr float kFeetY = 118.f;

enum class Kind : uint8_t { Goby = 0, Dab = 1, Perch = 2, Bass = 3, Carp = 4, Pike = 5 };

struct Fish {
    const char* name;
    int pay;
    int line;  // 0..2 on the tape, -1 if the twin stays in the water
    Kind kind;
    float x, y;
    float sink;  // how deep a firm cast drops
};

constexpr Fish kFish[kFishN] = {
    {"GOBY", 1, -1, Kind::Goby, 86.f, 170.f, 8.f},
    {"DAB", 1, 0, Kind::Dab, 132.f, 148.f, 10.f},
    {"PERCH", 3, -1, Kind::Perch, 176.f, 184.f, 16.f},
    {"BASS", 3, 1, Kind::Bass, 220.f, 140.f, 20.f},
    {"CARP", 6, -1, Kind::Carp, 258.f, 168.f, 30.f},
    {"PIKE", 6, 2, Kind::Pike, 298.f, 126.f, 42.f},
};

enum Pal {
    PAL_INK = 0,
    PAL_GOLD = 1,
    PAL_GOOD = 2,
    PAL_BAD = 3,
    PAL_SKY = 4,
    PAL_KIT = 5,
    PAL_DAB = 6,
    PAL_BASS = 7,
    PAL_PIKE = 8,
    PAL_TWIN = 9,
    PAL_WOOD = 10,
    PAL_WATER = 11,
    PAL_PAPER = 12,
    PAL_REED = 13,
    PAL_LURE = 14,
    PAL_SUN = 15
};

struct Art {
    int font[96] = {};
    gs::Image logo, matchW, openW;
    gs::Image angler[3];
    gs::Image dab, bass, pike, twin;
    gs::Image lure, creel, tape, slip[kTapeN];
    gs::Image jetty, reed, sun, cloud, blot;
};

inline bool sweetMeter(float m) { return std::fabs(m - 0.5f) <= kSweet; }

inline const char* tapeName(int line) {
    for (int i = 0; i < kFishN; i++)
        if (kFish[i].line == line) return kFish[i].name;
    return "";
}

inline int tapePay(int line) {
    for (int i = 0; i < kFishN; i++)
        if (kFish[i].line == line) return kFish[i].pay;
    return 0;
}

inline int tapeSum() {
    int s = 0;
    for (int i = 0; i < kTapeN; i++) s += tapePay(i);
    return s;
}

inline int fishOfLine(int line) {
    for (int i = 0; i < kFishN; i++)
        if (kFish[i].line == line) return i;
    return -1;
}

inline int twinOf(int line) {
    int pay = tapePay(line);
    for (int i = 0; i < kFishN; i++)
        if (kFish[i].line < 0 && kFish[i].pay == pay) return i;
    return -1;
}

inline int takenLine(bool firm, int fish) {
    if (!firm || fish < 0 || fish >= kFishN) return -1;
    return kFish[fish].line;
}

inline bool isTwin(bool firm, int fish) {
    return firm && fish >= 0 && fish < kFishN && kFish[fish].line < 0;
}

inline int shapeOf(Kind k) {
    switch (k) {
    case Kind::Dab: return 0;
    case Kind::Bass:
    case Kind::Perch: return 1;
    case Kind::Pike:
    case Kind::Carp: return 2;
    case Kind::Goby: return 3;
    }
    return 3;
}

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace fishtape
