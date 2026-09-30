// S3 FLUTE GOLD pictures. Drawn at boot. No asset files.
#pragma once
#include "console/gfx.h"
#include "console/vdp.h"

namespace flutegold {

constexpr int kNotes = 6;
constexpr int kGoldFace = 5;
constexpr int kCreamFace = 3;
constexpr int kLine = 42;
constexpr int kGolds = 4;
constexpr int kCreams = 2;

// Phrase order: gold, cream, gold, cream, gold, gold. The last note is gold.
inline bool noteGold(int i) { return i == 0 || i == 2 || i == 4 || i == 5; }

enum Pal {
    PAL_HUD = 0,
    PAL_GOLD = 1,
    PAL_DIM = 2,
    PAL_BAD = 3,
    PAL_CREAM = 4,
    PAL_WOOD = 5,
    PAL_PLAYER = 6,
    PAL_NIGHT = 7,
    PAL_LAMP = 8
};

struct Art {
    gs::Mipped player;
    gs::Mipped flute;
    gs::Mipped stand;
    gs::Mipped staff;
    gs::Mipped note;
    gs::Mipped noteOn;
    gs::Mipped breath;
    gs::Mipped lamp;
    int font[96] = {};
};

void buildArt(gs::VDP& vdp, Art& art);

}  // namespace flutegold
