#pragma once

#include "console/vdp.h"

namespace mazetape {

// One hedge on the S3-16. Cells are 16px so a walker fills the path.
constexpr int COLS = 7;
constexpr int ROWS = 5;
constexpr int MW = COLS * 2 + 1;
constexpr int MH = ROWS * 2 + 1;
constexpr int CELL = 16;
constexpr int OX = (gs::SCREEN_W - MW * CELL) / 2;
constexpr int OY = 24;

static_assert(OX % 8 == 0 && OY % 8 == 0, "maze origin sits on a tile");
static_assert(MW * CELL <= gs::SCREEN_W, "maze wider than the screen");
static_assert(OY + MH * CELL <= gs::SCREEN_H - 16, "leave a line for the till");

// The tape wants KEY, BELL, LAMP. LOCK, CHIME and TORCH pay the same and are not the tape.
enum Kind { KEY = 0, BELL = 1, LAMP = 2, LOCK = 3, CHIME = 4, TORCH = 5, KIND_N = 6 };

inline bool tapeKind(int k) { return k >= KEY && k <= LAMP; }
inline int twinOf(int k) { return tapeKind(k) ? k + 3 : -1; }

inline const char* kindName(int k) {
    switch (k) {
    case KEY: return "KEY";
    case BELL: return "BELL";
    case LAMP: return "LAMP";
    case LOCK: return "LOCK";
    case CHIME: return "CHIME";
    case TORCH: return "TORCH";
    default: return "?";
    }
}

inline int kindPay(int k) {
    int base = tapeKind(k) ? k : (k - 3);
    if (base == KEY) return 4;
    if (base == BELL) return 6;
    if (base == LAMP) return 8;
    return 0;
}

struct Art {
    gs::Image body[3][2];
    gs::Image shadow;
    gs::Image token[KIND_N];
    gs::Image gate;
    gs::Image titlePic;
    gs::Image titleName, titleSub, titleRule, titleMove, titleGo;
    gs::Image sayFind, sayWrong, sayMatch, sayOut;
    int fontBase = 1;

    void bake(gs::VDP& vdp, const uint8_t* hedge, int ex, int ey, int sx, int sy);
};

}  // namespace mazetape
