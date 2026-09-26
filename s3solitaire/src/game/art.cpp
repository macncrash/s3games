#include "game/art.h"

namespace sol {
namespace {

constexpr int PAPER = 1;
constexpr int SHADE = 2;
constexpr int INK = 3;
constexpr int RED = 4;
constexpr int EDGE = 5;
constexpr int HILITE = 6;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void stamp(gs::Bitmap& b, char ch, int x, int y, int c, int scale) {
    const uint8_t* g = gs::glyph(ch);
    for (int gy = 0; gy < 7; gy++)
        for (int gx = 0; gx < 5; gx++)
            if (g[gy * 5 + gx])
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        b.set(x + gx * scale + sx, y + gy * scale + sy, c);
}

void blitRows(gs::Bitmap& b, const char* const* rows, int n, int x, int y, int c, int scale) {
    for (int j = 0; j < n; j++) {
        const char* row = rows[j];
        for (int i = 0; row[i]; i++)
            if (row[i] == '#')
                for (int sy = 0; sy < scale; sy++)
                    for (int sx = 0; sx < scale; sx++)
                        b.set(x + i * scale + sx, y + j * scale + sy, c);
    }
}

const char* const kHeart[] = {
    ".##.##.",
    "##.#.##",
    "#######",
    ".#####.",
    "..###..",
    "...#...",
};
const char* const kSpade[] = {
    "...#...",
    "..###..",
    ".#####.",
    "#######",
    ".#####.",
    "...#...",
    "..###..",
};
const char* const kDiamond[] = {
    "...#...",
    "..###..",
    ".#####.",
    "#######",
    ".#####.",
    "..###..",
    "...#...",
};
const char* const kClub[] = {
    "..###..",
    ".#####.",
    "..#.#..",
    ".##.##.",
    "###.###",
    "...#...",
    "..###..",
};

struct SuitPat {
    const char* const* rows;
    int n;
};

SuitPat suitPat(int suit) {
    static const SuitPat p[4] = {{kSpade, 7}, {kHeart, 6}, {kDiamond, 7}, {kClub, 7}};
    return p[suit & 3];
}

bool redSuit(int suit) { return suit == 1 || suit == 2; }

void drawSuit(gs::Bitmap& b, int suit, int x, int y, int scale, int c) {
    SuitPat p = suitPat(suit);
    blitRows(b, p.rows, p.n, x, y, c, scale);
}

int suitW(int suit, int scale) {
    (void)suit;
    return 7 * scale;
}

int suitH(int suit, int scale) { return suitPat(suit).n * scale; }

void roundCorners(gs::Bitmap& b) {
    const int w = b.w, h = b.h;
    const int cut[3][2] = {{0, 0}, {1, 0}, {0, 1}};
    for (auto [dx, dy] : cut) {
        b.set(dx, dy, 0);
        b.set(w - 1 - dx, dy, 0);
        b.set(dx, h - 1 - dy, 0);
        b.set(w - 1 - dx, h - 1 - dy, 0);
    }
}

void pip(gs::Bitmap& b, int suit, int cx, int cy, int scale, int c) {
    int w = suitW(suit, scale);
    int h = suitH(suit, scale);
    drawSuit(b, suit, cx - w / 2, cy - h / 2, scale, c);
}

// Centre pips for ranks A..7. Coordinates are centres, origin the card centre.
void pipsFor(gs::Bitmap& b, int suit, int rank, int c) {
    const int cx = CARD_W / 2;
    const int cy = 31;
    if (rank == 0) {
        pip(b, suit, cx, cy + 2, 2, c);
        return;
    }
    const int ax = 8;
    const int ay = 11;
    struct P {
        int x, y;
    };
    P pos[7];
    int n = 0;
    auto add = [&](int x, int y) { pos[n++] = {x, y}; };
    if (rank == 1) {
        add(0, -ay);
        add(0, ay);
    } else if (rank == 2) {
        add(0, -ay);
        add(0, 0);
        add(0, ay);
    } else if (rank == 3) {
        add(-ax, -ay);
        add(ax, -ay);
        add(-ax, ay);
        add(ax, ay);
    } else if (rank == 4) {
        add(-ax, -ay);
        add(ax, -ay);
        add(0, 0);
        add(-ax, ay);
        add(ax, ay);
    } else if (rank == 5) {
        add(-ax, -ay);
        add(ax, -ay);
        add(-ax, 0);
        add(ax, 0);
        add(-ax, ay);
        add(ax, ay);
    } else {
        add(-ax, -ay);
        add(ax, -ay);
        add(0, -5);
        add(-ax, 0);
        add(ax, 0);
        add(-ax, ay);
        add(ax, ay);
    }
    for (int i = 0; i < n; i++) pip(b, suit, cx + pos[i].x, cy + pos[i].y, 1, c);
}

gs::Bitmap makeCard(int suit, int rank) {
    gs::Bitmap b(CARD_W, CARD_H);
    b.rect(0, 0, CARD_W, CARD_H, PAPER);
    for (int x = 0; x < CARD_W; x++) {
        b.set(x, 0, EDGE);
        b.set(x, CARD_H - 1, EDGE);
        if (x > 0 && x < CARD_W - 1) {
            b.set(x, 1, HILITE);
            b.set(x, CARD_H - 2, SHADE);
        }
    }
    for (int y = 0; y < CARD_H; y++) {
        b.set(0, y, EDGE);
        b.set(CARD_W - 1, y, EDGE);
        if (y > 0 && y < CARD_H - 1) {
            b.set(1, y, HILITE);
            b.set(CARD_W - 2, y, SHADE);
        }
    }
    int ink = redSuit(suit) ? RED : INK;
    static const char* ranks = "A234567";
    stamp(b, ranks[rank], 4, 4, ink, 1);
    drawSuit(b, suit, 12, 4, 1, ink);
    pipsFor(b, suit, rank, ink);
    roundCorners(b);
    return b;
}

gs::Bitmap makeSlot(int suit) {
    gs::Bitmap b(CARD_W + 6, CARD_H + 6);
    const int w = b.w, h = b.h;
    for (int x = 2; x < w - 2; x++) {
        b.set(x, 1, 2);
        b.set(x, 2, 2);
        b.set(x, h - 2, 2);
        b.set(x, h - 3, 2);
    }
    for (int y = 2; y < h - 2; y++) {
        b.set(1, y, 2);
        b.set(2, y, 2);
        b.set(w - 2, y, 2);
        b.set(w - 3, y, 2);
    }
    int sw = suitW(suit, 2);
    int sh = suitH(suit, 2);
    drawSuit(b, suit, (w - sw) / 2, (h - sh) / 2, 2, 1);
    return b;
}

gs::Bitmap makeCursor() {
    gs::Bitmap b(CARD_W + 8, CARD_H + 8);
    const int w = b.w, h = b.h;
    auto box = [&](int inset, int c) {
        for (int x = inset; x < w - inset; x++) {
            b.set(x, inset, c);
            b.set(x, h - 1 - inset, c);
        }
        for (int y = inset; y < h - inset; y++) {
            b.set(inset, y, c);
            b.set(w - 1 - inset, y, c);
        }
    };
    box(1, 1);
    box(2, 2);
    // Open the long sides so the corner index stays readable.
    for (int y = 16; y < h - 16; y++) {
        b.set(1, y, 0);
        b.set(2, y, 0);
        b.set(w - 2, y, 0);
        b.set(w - 3, y, 0);
    }
    return b;
}

gs::Bitmap makeFile() {
    gs::Bitmap b(CARD_W + 6, CARD_H + 6);
    const int w = b.w, h = b.h;
    for (int x = 2; x < w - 2; x++) {
        b.set(x, 1, 1);
        b.set(x, 2, 2);
        b.set(x, h - 2, 1);
        b.set(x, h - 3, 2);
    }
    for (int y = 2; y < h - 2; y++) {
        b.set(1, y, 1);
        b.set(2, y, 2);
        b.set(w - 2, y, 1);
        b.set(w - 3, y, 2);
    }
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 10, 6), gs::rgb4(15, 8, 6), gs::rgb4(8, 14, 8),
                          gs::rgb4(6, 8, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 0)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(15, 14, 11), gs::rgb4(11, 9, 7), gs::rgb4(2, 1, 2), gs::rgb4(13, 2, 2),
                           gs::rgb4(4, 3, 3), gs::rgb4(15, 15, 14), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SLOT, {0, gs::rgb4(4, 8, 4), gs::rgb4(12, 10, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CURSOR, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FELT, {0, gs::rgb4(1, 7, 3), gs::rgb4(0, 5, 2), gs::rgb4(2, 9, 4), gs::rgb4(0, 4, 2), 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(14, 11, 4), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FILE, {0, gs::rgb4(8, 12, 6), gs::rgb4(3, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);

    uint8_t felt[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = 1;
            if (((x + y) & 3) == 0) c = 2;
            if (((x * 3 + y * 5) & 7) == 1) c = 3;
            felt[y * 8 + x] = uint8_t(c);
        }
    int feltTile = tiles.alloc(1);
    vdp.loadTile(feltTile, felt);
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 64; x++) vdp.B.set(x, y, gs::entry(feltTile, PAL_FELT));
    vdp.A.enabled = false;
    vdp.B.enabled = true;

    for (int suit = 0; suit < NSUIT; suit++) {
        art.slot[suit] = gs::uploadMipped(vdp, makeSlot(suit));
        for (int rank = 0; rank < NRANK; rank++)
            art.face[(rank << 2) | suit] = gs::uploadMipped(vdp, makeCard(suit, rank));
    }
    art.cursor = gs::uploadMipped(vdp, makeCursor());
    art.file = gs::uploadMipped(vdp, makeFile());
    gs::Bitmap sh(CARD_W, CARD_H);
    sh.rect(0, 0, CARD_W, CARD_H, 1);
    roundCorners(sh);
    art.shadow = gs::uploadMipped(vdp, sh);
    gs::Bitmap rule(16, 2);
    rule.rect(0, 0, 16, 1, 1);
    rule.rect(0, 1, 16, 1, 2);
    art.rule = gs::uploadMipped(vdp, rule);
}

}  // namespace sol
