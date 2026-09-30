// S3 BELLCHIME pictures. Drawn at boot. No asset files.
// Three bells hang. Only the short one can make the hour chime.
#pragma once

#include "console/gfx.h"
#include "console/vdp.h"

namespace bellchime {

constexpr int kBells = 3;
constexpr int kShort = 1;
constexpr int kPulls = 3;
constexpr int kFpc = 4;
constexpr int kGraceSec = 12;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 22;

constexpr int kLongH = 70;
constexpr int kShortH = 34;
constexpr int kDeepH = 54;

constexpr float kPi = 3.14159265f;

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_BELL = 4,
    PAL_WOOD = 5,
    PAL_ROPE = 6,
    PAL_NIGHT = 7,
    PAL_SHORT = 8
};

struct Art {
    gs::Image beam;
    gs::Image bell[kBells];
    int bellH[kBells] = {};
    gs::Image rope;
    gs::Image clock;
    gs::Image pip;
    gs::Image lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

inline const char* bellName(int i) {
    static const char* n[kBells] = {"LONG", "SHORT", "DEEP"};
    if (i < 0 || i >= kBells) return "-";
    return n[i];
}

inline float bellX(int i) { return 78.f + float(i) * 82.f; }

}  // namespace bellchime
