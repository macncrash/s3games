// S3 ARCHTAPE pictures and the face the rules use.
// Drawn at boot. Nothing is loaded from a file.
//
// The tape wants PIN, GOLD and RED.
// DOT, PALE and RUST pay those same counts and are not the tape.
#pragma once

#include <cmath>
#include <cstring>

#include "console/gfx.h"
#include "console/vdp.h"

namespace archtape {

constexpr int kTapeN = 3;
constexpr int kFace = 120;
constexpr float kFaceMid = 60.f;
constexpr float kCx = 214.f;
constexpr float kCy = 96.f;
constexpr float kLooseX = 96.f;
constexpr float kLooseY = 92.f;
constexpr float kBoss = 58.f;
constexpr int kFullDraw = 18;
constexpr int kFlight = 12;
constexpr int kCall = 22;
constexpr int kLeave = 36;
constexpr int kQuiver = 12;

struct Band {
    const char* name;
    int pay;
    int line;  // 0..2 on the tape, -1 if the twin stays out
    float rOut;
    int ink;
};

// Inner ring first. Twins sit just outside the beds they imitate.
constexpr Band kBand[6] = {
    {"PIN", 10, 0, 8.f, 1},
    {"DOT", 10, -1, 16.f, 2},
    {"GOLD", 9, 1, 28.f, 3},
    {"PALE", 9, -1, 38.f, 4},
    {"RED", 7, 2, 48.f, 5},
    {"RUST", 7, -1, 56.f, 6},
};

enum Pal {
    PAL_FACE = 0,
    PAL_ARCH = 1,
    PAL_ARROW = 2,
    PAL_SIGHT = 3,
    PAL_MARK = 4,
    PAL_WOOD = 5,
    PAL_WORD = 6,
    PAL_HUD = 7,
    PAL_HUD_GOLD = 8,
    PAL_HUD_ALERT = 9,
    PAL_FILL = 10
};

struct Hit {
    const char* name;
    int pay;
    int line;
    bool face;
};

inline Hit hitAt(float r) {
    if (r < 0.f) r = 0.f;
    for (const Band& b : kBand) {
        if (r <= b.rOut) return {b.name, b.pay, b.line, true};
    }
    return {"MISS", 0, -1, false};
}

inline const char* tapeName(int line) {
    for (const Band& b : kBand)
        if (b.line == line) return b.name;
    return "";
}

inline int tapePay(int line) {
    for (const Band& b : kBand)
        if (b.line == line) return b.pay;
    return 0;
}

inline int tapeSum() {
    int s = 0;
    for (int i = 0; i < kTapeN; i++) s += tapePay(i);
    return s;
}

// Centre of a tape bed, clear of the twin beside it.
inline void bedPoint(int line, float& x, float& y) {
    float r0 = 0.f;
    float r1 = kBand[0].rOut;
    for (const Band& b : kBand) {
        if (b.line == line) {
            r1 = b.rOut;
            break;
        }
        r0 = b.rOut;
    }
    float r = (line == 0) ? r1 * 0.25f : (r0 + r1) * 0.5f;
    float ang = (line == 1) ? -0.85f : 0.55f;
    if (line == 0) ang = 0.2f;
    x = kCx + std::sin(ang) * r;
    y = kCy - std::cos(ang) * r;
}

struct Art {
    gs::Image face;
    gs::Image archer[3];
    gs::Image arrow;
    gs::Image sight;
    gs::Image pip;
    gs::Image drawer;
    gs::Image wordArch;
    gs::Image wordTape;
    gs::Image wordLeave;
    gs::Image px;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace archtape
