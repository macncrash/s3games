// S3 ARCH pictures and the scoring rings. Drawn at boot. No asset files.
#pragma once

#include <cmath>

#include "console/gfx.h"
#include "console/vdp.h"

namespace arch {

constexpr int kArrows = 36;
constexpr int kLine = 300;

// Face bitmap. The painted rings and ringAt share these radii.
constexpr int kFace = 168;
constexpr float kFaceMid = 84.f;
constexpr float kR10 = 10.f;   // inner gold, face 10, counts double -> 20
constexpr float kR9 = 20.f;    // outer gold, face 9, counts double -> 18
constexpr float kR7 = 34.f;    // red
constexpr float kR5 = 48.f;    // blue
constexpr float kR3 = 60.f;    // black
constexpr float kR1 = 72.f;    // white
constexpr float kBoss = 78.f;  // straw butt, a miss

// Archer sprite. The loose point is the arrow tip in that bitmap.
constexpr float kArchX = 4.f;
constexpr float kArchY = 68.f;
constexpr int kArchW = 108;
constexpr int kArchH = 120;
constexpr int kBowX = 104;
constexpr int kBowY = 36;
constexpr float kLooseX = kArchX + float(kBowX);
constexpr float kLooseY = kArchY + float(kBowY);

// Target centre sits on the loose height so a full draw flies flat.
constexpr float kCx = 228.f;
constexpr float kCy = kLooseY;

constexpr float kWindPx = 16.f;
constexpr float kMissDrop = 84.f;

enum Pal {
    PAL_FACE = 0,
    PAL_ARCH = 1,
    PAL_ARROW = 2,
    PAL_SIGHT = 3,
    PAL_GHOST = 4,
    PAL_MARK = 5,
    PAL_WORLD = 6,
    PAL_HUD = 7,
    PAL_HUD_GOLD = 8,
    PAL_HUD_ALERT = 9,
    PAL_WORD = 10,
    PAL_SHORT = 11,
    PAL_BADGE = 12,
    PAL_TRACK = 13,
    PAL_FILL = 14
};

struct Ring {
    const char* name;
    int face;
    int points;
    bool gold;
};

// Gold face values count double. Other rings score their face.
inline Ring ringAt(float r) {
    if (r <= kR10) return {"GOLD", 10, 20, true};
    if (r <= kR9) return {"GOLD", 9, 18, true};
    if (r <= kR7) return {"RED", 7, 7, false};
    if (r <= kR5) return {"BLUE", 5, 5, false};
    if (r <= kR3) return {"BLACK", 3, 3, false};
    if (r <= kR1) return {"WHITE", 1, 1, false};
    return {"MISS", 0, 0, false};
}

struct Art {
    gs::Image face;
    gs::Image archer[3];  // idle, draw, loose
    gs::Image arrow;
    gs::Image sight;
    gs::Image ghost;
    gs::Image dot[6];  // gold, red, blue, black, white, miss
    gs::Image stand;
    gs::Image flag;
    gs::Image limp;
    gs::Image hill;
    gs::Image tree;
    gs::Image hut;
    gs::Image cloud;
    gs::Image sun;
    gs::Image bush;
    gs::Image shadow;
    gs::Image badge;
    gs::Image wordArch;
    gs::Image wordWin;
    gs::Image wordShort;
    gs::Image px;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace arch
