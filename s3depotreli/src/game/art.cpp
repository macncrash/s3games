#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace depot {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void pxFill(uint8_t* p, int c) {
    for (int i = 0; i < 64; i++) p[i] = uint8_t(c);
}

void brickTile(uint8_t* p, bool dark) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int row = y / 4;
            int mx = (x + row * 4) & 7;
            bool mortar = (y % 4 == 3) || mx == 7;
            int c = mortar ? 3 : (dark ? 2 : 1);
            if (!mortar && ((x * 3 + y * 5) % 11 == 0)) c = 2;
            p[y * 8 + x] = uint8_t(c);
        }
    }
}

void roofTile(uint8_t* p) {
    pxFill(p, 9);
    for (int x = 0; x < 8; x++) p[7 * 8 + x] = 10;
    p[2 * 8 + 3] = 14;
    p[5 * 8 + 6] = 14;
}

void corniceTile(uint8_t* p) {
    pxFill(p, 4);
    for (int x = 0; x < 8; x++) {
        p[x] = 10;
        p[8 + x] = 5;
        p[6 * 8 + x] = 5;
        p[7 * 8 + x] = 8;
    }
}

void windowTile(uint8_t* p, bool lower) {
    pxFill(p, 8);
    for (int y = 1; y < 7; y++)
        for (int x = 1; x < 7; x++) p[y * 8 + x] = 6;
    for (int y = 1; y < 7; y++) p[y * 8 + 3] = 8;
    int bar = lower ? 2 : 5;
    for (int x = 1; x < 7; x++) p[bar * 8 + x] = 8;
    if (lower) {
        for (int y = 3; y < 7; y++)
            for (int x = 1; x < 7; x++)
                if (p[y * 8 + x] == 6) p[y * 8 + x] = 7;
    }
}

void doorTile(uint8_t* p) {
    pxFill(p, 11);
    for (int y = 0; y < 8; y++) {
        p[y * 8] = 8;
        p[y * 8 + 7] = 8;
        if (y > 0 && y < 7) p[y * 8 + 5] = 12;
    }
    p[3 * 8 + 2] = 4;
}

void openTile(uint8_t* p) {
    pxFill(p, 8);
    p[3 * 8 + 3] = 5;
    p[4 * 8 + 4] = 5;
}

void gravelTile(uint8_t* p, int salt) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int n = (x * 5 + y * 3 + salt) % 17;
            int c = 1;
            if (n == 0) c = 15;
            else if (n == 3) c = 2;
            else if (n == 7) c = 7;
            else if (n == 11) c = 6;
            else if ((x + y + salt) & 1) c = 2;
            p[y * 8 + x] = uint8_t(c);
        }
    }
}

void railTile(uint8_t* p) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int c = ((x + y) & 3) == 0 ? 2 : 1;
            if (y == 1 || y == 6) c = 5;
            if (y == 2 || y == 5) c = 4;
            if ((y == 3 || y == 4) && x >= 1 && x <= 6) c = 3;
            p[y * 8 + x] = uint8_t(c);
        }
    }
}

void concreteTile(uint8_t* p) {
    pxFill(p, 8);
    for (int x = 0; x < 8; x++) p[7 * 8 + x] = 9;
    p[2 * 8 + 4] = 14;
    p[5 * 8 + 1] = 9;
    p[4 * 8 + 6] = 14;
}

void hazardTile(uint8_t* p) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) p[y * 8 + x] = uint8_t((((x + y) / 2) & 1) ? 10 : 11);
}

void boots(Bitmap& b, int x, int y, int step, int cloth) {
    int la = step ? 3 : 0;
    int ra = step ? 0 : 3;
    b.rect(float(x), float(y + la), 5, float(11 - la), cloth);
    b.rect(float(x + 7), float(y + ra), 5, float(11 - ra), cloth);
    b.rect(float(x - 1), float(y + 10), 7, 3, 3);
    b.rect(float(x + 6), float(y + 10), 7, 3, 3);
}

Bitmap clerkArt(int step) {
    Bitmap b(40, 52);
    b.rect(12, 2, 14, 4, 8);
    b.rect(10, 5, 18, 3, 2);
    b.ellipse(19, 14, 7, 7, 6);
    b.set(16, 13, 5);
    b.set(21, 13, 5);
    b.rect(14, 16, 8, 2, 3);
    b.poly({{12, 20}, {8, 28}, {10, 40}, {28, 40}, {30, 26}, {26, 20}}, 2);
    b.poly({{16, 22}, {15, 30}, {18, 39}, {24, 39}, {25, 28}}, 3);
    b.rect(11, 30, 16, 3, 4);
    b.rect(27, 24, 4, 8, 6);
    b.rect(30, 22, 6, 8, 7);
    b.rect(31, 23, 4, 5, 1);
    b.rect(32, 30, 2, 6, 4);
    boots(b, 12, 38, step, 2);
    b.outline(5, false);
    return b;
}

Bitmap runnerArt(int step) {
    Bitmap b(36, 48);
    b.ellipse(14, 12, 7, 7, 6);
    b.rect(8, 6, 12, 4, 4);
    b.rect(7, 8, 4, 3, 4);
    b.set(11, 12, 5);
    b.set(15, 12, 5);
    b.poly({{8, 18}, {6, 26}, {8, 38}, {24, 38}, {26, 24}, {20, 18}}, 2);
    b.poly({{10, 22}, {10, 30}, {14, 36}, {18, 36}, {18, 24}}, 3);
    b.line(6, 22, 2, 34, 7, 1.6f);
    b.rect(1, 32, 4, 3, 7);
    boots(b, 8, 36, step, 2);
    b.outline(5, false);
    return b;
}

Bitmap drayArt(int step) {
    Bitmap b(52, 46);
    b.rect(2, 22, 22, 12, 4);
    b.rect(4, 16, 16, 8, 2);
    b.rect(6, 18, 12, 4, 1);
    b.ellipse(8, 36, 5, 5, 3);
    b.ellipse(20, 36, 5, 5, 3);
    b.ellipse(8, 36, 2, 2, 5);
    b.ellipse(20, 36, 2, 2, 5);
    b.ellipse(36, 12, 6, 6, 6);
    b.rect(31, 8, 10, 3, 3);
    b.set(34, 12, 5);
    b.poly({{30, 16}, {28, 24}, {30, 34}, {44, 34}, {46, 22}, {40, 16}}, 2);
    boots(b, 30, 32, step, 2);
    b.line(28, 24, 22, 22, 7, 1.4f);
    b.outline(5, false);
    return b;
}

Bitmap wagonArt(int step) {
    Bitmap b(78, 42);
    b.rect(8, 8, 62, 20, 2);
    b.rect(10, 6, 58, 4, 8);
    b.rect(12, 12, 18, 12, 6);
    b.rect(34, 12, 18, 12, 3);
    b.rect(54, 14, 10, 8, 1);
    b.rect(6, 16, 4, 6, 4);
    b.rect(2, 18, 5, 3, 7);
    float wob = step ? 1.f : -1.f;
    b.ellipse(22, 32 + wob, 7, 7, 4);
    b.ellipse(54, 32 - wob, 7, 7, 4);
    b.ellipse(22, 32 + wob, 2, 2, 1);
    b.ellipse(54, 32 - wob, 2, 2, 1);
    b.line(22, 32, 22 + (step ? 4.f : -4.f), 32, 5, 1.2f);
    b.outline(5, false);
    return b;
}

Bitmap locoArt() {
    Bitmap b(84, 40);
    b.rect(18, 12, 40, 14, 2);
    b.rect(20, 14, 34, 6, 1);
    b.rect(46, 4, 24, 22, 2);
    b.rect(50, 6, 14, 10, 6);
    b.rect(52, 8, 10, 6, 1);
    b.rect(22, 2, 6, 12, 3);
    b.rect(20, 2, 10, 3, 8);
    b.rect(8, 16, 12, 8, 3);
    b.rect(4, 18, 6, 4, 7);
    b.ellipse(24, 30, 7, 7, 4);
    b.ellipse(42, 30, 7, 7, 4);
    b.ellipse(62, 32, 5, 5, 4);
    b.ellipse(24, 30, 2, 2, 1);
    b.ellipse(42, 30, 2, 2, 1);
    b.outline(5, false);
    return b;
}

Bitmap bellArt() {
    Bitmap b(26, 30);
    b.rect(3, 1, 20, 4, 4);
    b.rect(12, 5, 2, 3, 4);
    b.poly({{7, 8}, {19, 8}, {23, 24}, {3, 24}}, 2);
    b.poly({{10, 10}, {16, 10}, {18, 22}, {8, 22}}, 1);
    b.rect(5, 24, 16, 3, 3);
    b.ellipse(13, 26, 2, 2, 7);
    b.outline(5, false);
    return b;
}

Bitmap ropeArt() {
    Bitmap b(6, 36);
    for (int y = 0; y < 36; y++) {
        int x = 2 + int(std::sin(y * 0.45f) * 1.2f);
        b.set(x, y, 6);
        if (x + 1 < 6) b.set(x + 1, y, 3);
    }
    return b;
}

Bitmap lanternArt() {
    Bitmap b(12, 18);
    b.rect(5, 0, 2, 3, 4);
    b.rect(2, 3, 8, 10, 2);
    b.rect(4, 5, 4, 6, 1);
    b.rect(3, 13, 6, 3, 3);
    b.outline(5, false);
    return b;
}

Bitmap signalArt() {
    Bitmap b(14, 32);
    b.rect(6, 10, 2, 20, 4);
    b.rect(2, 4, 10, 8, 6);
    b.rect(3, 5, 8, 3, 8);
    b.rect(3, 8, 8, 3, 2);
    b.outline(5, false);
    return b;
}

Bitmap bufferArt() {
    Bitmap b(26, 18);
    b.rect(4, 7, 18, 5, 4);
    b.ellipse(7, 9, 5, 5, 6);
    b.ellipse(19, 9, 5, 5, 2);
    b.ellipse(7, 9, 2, 2, 1);
    b.ellipse(19, 9, 2, 2, 1);
    b.outline(5, false);
    return b;
}

Bitmap crateArt() {
    Bitmap b(22, 18);
    b.rect(1, 1, 20, 16, 2);
    b.rect(3, 3, 16, 12, 1);
    b.line(2, 2, 19, 15, 3, 1.2f);
    b.line(19, 2, 2, 15, 3, 1.2f);
    b.rect(1, 8, 20, 2, 4);
    b.outline(5, false);
    return b;
}

Bitmap barrelArt() {
    Bitmap b(16, 20);
    b.ellipse(8, 10, 7, 8, 2);
    b.ellipse(8, 10, 7, 3, 3);
    b.rect(1, 5, 14, 2, 4);
    b.rect(1, 12, 14, 2, 4);
    b.outline(5, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(10, 14);
    b.rect(3, 0, 4, 2, 4);
    b.ellipse(5, 7, 4, 5, 6);
    b.ellipse(5, 7, 2, 2, 1);
    b.rect(4, 11, 2, 2, 3);
    return b;
}

Bitmap glintArt() {
    Bitmap b(14, 14);
    b.poly({{7, 0}, {9, 5}, {13, 7}, {9, 9}, {7, 13}, {5, 9}, {1, 7}, {5, 5}}, 1);
    b.ellipse(7, 7, 2, 2, 6);
    return b;
}

Bitmap dustArt() {
    Bitmap b(28, 22);
    b.ellipse(14, 12, 12, 8, 2);
    b.ellipse(12, 11, 7, 5, 3);
    b.ellipse(11, 10, 3, 2, 1);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(48, 12);
    b.ellipse(24, 6, 20, 4, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(40, 16);
    b.ellipse(12, 9, 8, 5, 3);
    b.ellipse(22, 8, 10, 6, 2);
    b.ellipse(30, 9, 7, 4, 3);
    return b;
}

Bitmap moonArt() {
    Bitmap b(22, 22);
    b.ellipse(11, 11, 9, 9, 1);
    b.ellipse(11, 11, 6, 6, 2);
    b.ellipse(8, 8, 2, 2, 3);
    return b;
}

Bitmap starArt() {
    Bitmap b(5, 5);
    b.set(2, 0, 7);
    b.set(2, 1, 7);
    b.set(2, 2, 1);
    b.set(2, 3, 7);
    b.set(2, 4, 7);
    b.set(0, 2, 7);
    b.set(1, 2, 7);
    b.set(3, 2, 7);
    b.set(4, 2, 7);
    return b;
}

int makeTile(gs::TileAlloc& alloc, gs::VDP& vdp, const uint8_t* px) {
    int t = alloc.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

bool doorRow(int y) {
    for (int i = 0; i < 3; i++) {
        int row = kTrackRow[i];
        if (y >= row - 2 && y <= row) return true;
    }
    return false;
}

bool railRow(int y) {
    for (int row : kTrackRow)
        if (y == row) return true;
    return false;
}

bool windowCol(int x) { return x == 11 || x == 15 || x == 33 || x == 37; }

void paint(gs::VDP& vdp, int roof, int cornice, int brick, int brick2, int winHi, int winLo, int door, int open,
           int gravel, int gravel2, int rail, int concrete, int hazard) {
    auto put = [&](int x, int y, int tile, int pal) { vdp.B.set(x, y, gs::entry(tile, pal)); };
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 40; x++) {
            bool tower = x >= 1 && x <= 3 && y <= 4;
            if (tower) {
                if (y == 0) put(x, y, roof, PAL_HOUSE);
                else if (y <= 3 && x == 2) put(x, y, open, PAL_HOUSE);
                else put(x, y, brick, PAL_HOUSE);
            } else if (y == 4) {
                put(x, y, roof, PAL_HOUSE);
            } else if (x >= 6 && x <= 8 && doorRow(y)) {
                put(x, y, door, PAL_HOUSE);
            } else if (y == 8) {
                put(x, y, cornice, PAL_HOUSE);
            } else if (x <= 8 && y > 4) {
                int tile = ((x + y) & 1) ? brick2 : brick;
                if (x == 5 && y == 6) tile = winHi;
                if (x == 5 && y == 7) tile = winLo;
                if (y >= 26) tile = cornice;
                put(x, y, tile, PAL_HOUSE);
            } else if (y >= 5 && y <= 7 && x >= 9) {
                int tile = brick;
                if ((y == 6 || y == 7) && windowCol(x)) tile = y == 6 ? winHi : winLo;
                put(x, y, tile, PAL_HOUSE);
            } else if ((y == 9 || y == 10) && x >= 9) {
                put(x, y, y == 10 ? hazard : concrete, PAL_YARD);
            } else if (x >= 9 && railRow(y)) {
                put(x, y, rail, PAL_YARD);
            } else if (x >= 9 && y >= 11) {
                put(x, y, ((x * 3 + y) & 1) ? gravel2 : gravel, PAL_YARD);
            }
        }
    }
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
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shade = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 11, 12), gs::rgb4(6, 7, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_AMBER,
           {0, gs::rgb4(15, 12, 4), gs::rgb4(12, 8, 2), gs::rgb4(7, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_RED,
           {0, gs::rgb4(15, 5, 3), gs::rgb4(9, 2, 2), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_GREEN,
           {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 8, 4), gs::rgb4(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 2, 1)});

    setPal(vdp, PAL_CLERK,
           {0, gs::rgb4(15, 14, 8), gs::rgb4(2, 3, 7), gs::rgb4(1, 1, 3), gs::rgb4(12, 10, 4), gs::rgb4(1, 1, 2),
            gs::rgb4(13, 9, 6), gs::rgb4(15, 13, 5), gs::rgb4(3, 3, 6), 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_LOOT,
           {0, gs::rgb4(12, 10, 8), gs::rgb4(6, 5, 3), gs::rgb4(3, 2, 2), gs::rgb4(10, 2, 2), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 9, 6), gs::rgb4(8, 8, 7), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_DRAY,
           {0, gs::rgb4(12, 9, 5), gs::rgb4(4, 5, 3), gs::rgb4(2, 3, 2), gs::rgb4(8, 5, 2), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 9, 6), gs::rgb4(9, 7, 3), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_WAGON,
           {0, gs::rgb4(14, 9, 6), gs::rgb4(10, 3, 2), gs::rgb4(6, 2, 1), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 11, 8), gs::rgb4(12, 10, 3), gs::rgb4(7, 7, 8), 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(12, 8, 4), gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(11, 9, 4), gs::rgb4(1, 1, 1),
            gs::rgb4(7, 7, 6), 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 9, 3), gs::rgb4(7, 5, 2), gs::rgb4(8, 7, 5), gs::rgb4(1, 1, 1),
            gs::rgb4(11, 8, 4), gs::rgb4(15, 15, 11), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(9, 3, 2), gs::rgb4(6, 2, 1), gs::rgb4(5, 4, 4), gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 4),
            gs::rgb4(14, 11, 4), gs::rgb4(8, 6, 2), gs::rgb4(1, 1, 3), gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7),
            gs::rgb4(2, 2, 3), gs::rgb4(6, 8, 5), 0, gs::rgb4(8, 4, 2), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(14, 14, 12), gs::rgb4(10, 10, 8), gs::rgb4(8, 8, 11), gs::rgb4(7, 7, 8), gs::rgb4(1, 1, 2),
            gs::rgb4(3, 3, 4), gs::rgb4(15, 15, 14), gs::rgb4(13, 3, 2), 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(4, 4, 3), gs::rgb4(2, 2, 2), gs::rgb4(6, 4, 2), gs::rgb4(9, 9, 10), gs::rgb4(13, 13, 14),
            gs::rgb4(2, 4, 2), gs::rgb4(1, 1, 1), gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 4), gs::rgb4(13, 11, 2),
            gs::rgb4(1, 1, 1), gs::rgb4(8, 5, 3), gs::rgb4(5, 3, 2), gs::rgb4(3, 3, 2), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_LOCO,
           {0, gs::rgb4(12, 14, 11), gs::rgb4(2, 6, 3), gs::rgb4(1, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 12, 6), gs::rgb4(12, 9, 3), gs::rgb4(8, 8, 8), 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 15, 13), gs::rgb4(9, 8, 6), gs::rgb4(5, 4, 3), gs::rgb4(8, 7, 5), gs::rgb4(1, 1, 1),
            gs::rgb4(15, 12, 4), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, shade});

    uint8_t tile[64];
    gs::TileAlloc alloc(vdp);
    brickTile(tile, false);
    int brick = makeTile(alloc, vdp, tile);
    brickTile(tile, true);
    int brick2 = makeTile(alloc, vdp, tile);
    roofTile(tile);
    int roof = makeTile(alloc, vdp, tile);
    corniceTile(tile);
    int cornice = makeTile(alloc, vdp, tile);
    windowTile(tile, false);
    int winHi = makeTile(alloc, vdp, tile);
    windowTile(tile, true);
    int winLo = makeTile(alloc, vdp, tile);
    doorTile(tile);
    int door = makeTile(alloc, vdp, tile);
    openTile(tile);
    int open = makeTile(alloc, vdp, tile);
    gravelTile(tile, 0);
    int gravel = makeTile(alloc, vdp, tile);
    gravelTile(tile, 4);
    int gravel2 = makeTile(alloc, vdp, tile);
    railTile(tile);
    int rail = makeTile(alloc, vdp, tile);
    concreteTile(tile);
    int concrete = makeTile(alloc, vdp, tile);
    hazardTile(tile);
    int hazard = makeTile(alloc, vdp, tile);
    paint(vdp, roof, cornice, brick, brick2, winHi, winLo, door, open, gravel, gravel2, rail, concrete, hazard);

    art.clerk[0] = gs::uploadMipped(vdp, clerkArt(0));
    art.clerk[1] = gs::uploadMipped(vdp, clerkArt(1));
    art.runner[0] = gs::uploadMipped(vdp, runnerArt(0));
    art.runner[1] = gs::uploadMipped(vdp, runnerArt(1));
    art.dray[0] = gs::uploadMipped(vdp, drayArt(0));
    art.dray[1] = gs::uploadMipped(vdp, drayArt(1));
    art.wagon[0] = gs::uploadMipped(vdp, wagonArt(0));
    art.wagon[1] = gs::uploadMipped(vdp, wagonArt(1));
    art.loco = gs::uploadMipped(vdp, locoArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.signal = gs::uploadMipped(vdp, signalArt());
    art.buffer = gs::uploadMipped(vdp, bufferArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.glint = gs::uploadMipped(vdp, glintArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.sign = gs::uploadMipped(vdp, gs::textBitmap("DEPOT", {2, 1, 0, 0, 1}));
    loadFont(vdp, alloc, art);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.B.scroll(0, 0);
    vdp.hudEnabled = true;
}

}  // namespace depot
