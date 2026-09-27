#include "game/art.h"

#include <initializer_list>
#include <string>

namespace harborpurs {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void fill8(uint8_t* p, int c) {
    for (int i = 0; i < 64; i++) p[i] = uint8_t(c);
}

int makeTile(gs::TileAlloc& alloc, gs::VDP& vdp, const uint8_t* px) {
    int t = alloc.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void stoneTile(uint8_t* p, int salt) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int n = (x * 3 + y * 5 + salt * 11) & 7;
            int c = 1;
            if (n == 0) c = 3;
            else if (n == 3) c = 2;
            else if ((x + salt) % 4 == 0 && y == 7) c = 4;
            p[y * 8 + x] = uint8_t(c);
        }
    }
}

void brickTile(uint8_t* p, bool dark) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int mx = (x + (y / 4) * 4) & 7;
            bool mortar = (y % 4 == 3) || mx == 7;
            p[y * 8 + x] = uint8_t(mortar ? 3 : (dark ? 2 : 1));
        }
    }
}

void windowTile(uint8_t* p, bool lit) {
    fill8(p, 1);
    for (int y = 1; y < 7; y++)
        for (int x = 1; x < 7; x++) p[y * 8 + x] = uint8_t(lit ? 6 : 7);
    for (int y = 1; y < 7; y++) p[y * 8 + 3] = 8;
    for (int x = 1; x < 7; x++) p[4 * 8 + x] = 8;
}

void waterTile(uint8_t* p, int chop) {
    fill8(p, 1);
    for (int x = 0; x < 8; x++) {
        int y = 2 + ((x + chop) & 3);
        if (y < 8) p[y * 8 + x] = 2;
        int y2 = 6 + ((x * 2 + chop) & 1);
        if (y2 < 8) p[y2 * 8 + x] = 3;
    }
    if ((chop & 1) == 0) p[1 * 8 + 3] = 4;
}

void timberTile(uint8_t* p) {
    fill8(p, 9);
    for (int x = 0; x < 8; x++) {
        p[x] = 10;
        p[7 * 8 + x] = 5;
    }
    for (int y = 0; y < 8; y++) p[y * 8 + 0] = 5;
}

void lampTile(uint8_t* p) {
    fill8(p, 1);
    for (int y = 0; y < 6; y++) p[y * 8 + 3] = 8;
    for (int x = 1; x < 6; x++) p[8 + x] = 6;
    p[2 * 8 + 3] = 14;
}

void paintHarbor(gs::VDP& vdp, int stone, int stone2, int brick, int brick2, int winLit, int winDim, int waterA,
                 int waterB, int timber, int lamp) {
    auto put = [&](int x, int y, int tile, int pal) { vdp.B.set(x, y, gs::entry(tile, pal)); };
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 40; x++) {
            bool sheds = y >= 4 && y <= 8 && x >= 2 && x <= 34;
            if (y < 4) {
                continue;
            } else if (y == 4) {
                put(x, y, timber, PAL_HOUSE);
            } else if (sheds && y <= 7) {
                int tile = ((x + y) & 1) ? brick2 : brick;
                if (y == 6 && (x % 6 == 2)) tile = winLit;
                if (y == 7 && (x % 6 == 2)) tile = winDim;
                if (x == 33 && y == 5) tile = lamp;
                put(x, y, tile, (x == 33 && y == 5) ? PAL_LAMP : PAL_HOUSE);
            } else if (y == 8 || y == 9) {
                put(x, y, y == 8 ? timber : stone, PAL_QUAY);
            } else if (y == 10 || y == kLaneRow[0] - 1 || y == kLaneRow[1] - 1 || y == kLaneRow[2] - 1) {
                put(x, y, stone2, PAL_QUAY);
            } else if (y == kLaneRow[0] || y == kLaneRow[1] || y == kLaneRow[2]) {
                put(x, y, (x & 1) ? waterB : waterA, PAL_WATER);
            } else if (y > kLaneRow[2]) {
                put(x, y, stone, PAL_QUAY);
            } else {
                put(x, y, (x + y) & 1 ? waterB : waterA, PAL_WATER);
            }
        }
    }
}

Bitmap hullBoat(int w, int h, int hull, int cabin, int trim, int stack, int step) {
    Bitmap b(w, h);
    int keel = h - 4;
    b.poly({{2.f, float(keel - 4)}, {float(w - 2), float(keel - 4)}, {float(w - 6), float(keel + 2)}, {8.f, float(keel + 2)}},
           hull);
    b.rect(6, keel - 10, w - 14, 7, cabin);
    b.rect(8, keel - 14, 8, 5, trim);
    if (stack) {
        b.rect(w / 2 + step, 2, 4, keel - 12, stack);
        b.rect(w / 2 + step - 1, 1, 6, 2, 14);
    }
    b.line(4, float(keel - 2), float(w - 5), float(keel - 2), trim, 1.f);
    b.rect(w - 10, keel - 8, 3, 3, 15);
    return b;
}

Bitmap pilotArt(int step) {
    Bitmap b = hullBoat(40, 22, 2, 4, 6, 0, step);
    b.rect(14, 6, 10, 5, 5);
    b.rect(16, 7, 3, 2, 15);
    b.line(4, 16, 36, 16, step ? 7 : 3, 1.f);
    return b;
}

Bitmap tugArt(int step) {
    Bitmap b = hullBoat(48, 26, 1, 3, 8, 9, step);
    b.rect(8, 14, 14, 5, 2);
    b.ellipse(10.f, 22.f, 3.f, 2.f, 5);
    return b;
}

Bitmap ferryArt(int step) {
    Bitmap b(64, 24);
    int keel = 18;
    b.poly({{2.f, float(keel - 3)}, {62.f, float(keel - 3)}, {56.f, float(keel + 3)}, {8.f, float(keel + 3)}}, 1);
    b.rect(10, 8, 44, 8, 3);
    for (int i = 0; i < 5; i++) b.rect(14 + i * 8, 10, 4, 3, (i + step) & 1 ? 6 : 7);
    b.rect(28, 3, 8, 6, 4);
    b.line(6, 16, 58, 16, 8, 1.f);
    return b;
}

Bitmap cutterArt(int step) {
    Bitmap b = hullBoat(36, 18, 2, 5, 9, 0, step);
    b.poly({{28.f, 12.f}, {34.f, 8.f}, {34.f, 14.f}}, 8);
    b.line(6, 13, 20, 13, step ? 14 : 6, 1.f);
    return b;
}

Bitmap wakeArt() {
    Bitmap b(16, 8);
    b.ellipse(8, 4, 7, 2.2f, 1);
    b.ellipse(5, 4, 2, 1, 2);
    return b;
}

Bitmap boltArt() {
    Bitmap b(10, 4);
    b.rect(0, 1, 10, 2, 1);
    b.rect(7, 0, 3, 4, 2);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.ellipse(4, 4, 1.2f, 1.2f, 2);
    return b;
}

Bitmap buoyArt() {
    Bitmap b(10, 16);
    b.rect(4, 2, 2, 10, 5);
    b.ellipse(5, 4, 4, 3, 1);
    b.ellipse(5, 4, 2, 1.4f, 2);
    b.rect(3, 12, 4, 2, 3);
    return b;
}

Bitmap gullArt() {
    Bitmap b(14, 6);
    b.line(1, 4, 7, 1, 1, 1.f);
    b.line(7, 1, 13, 4, 1, 1.f);
    return b;
}

Bitmap lampArt() {
    Bitmap b(10, 18);
    b.rect(4, 6, 2, 12, 3);
    b.ellipse(5, 4, 4, 3, 1);
    b.ellipse(5, 4, 2, 1.2f, 2);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 15, 0, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        if (g) {
            for (int y = 0; y < 7; y++)
                for (int x = 0; x < 5; x++)
                    if (g[y * 5 + x]) {
                        px[y * 8 + x + 1] = 1;
                        if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                    }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 14, 12);
    const uint16_t shade = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 10, 12), shade});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2), shade});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 9), gs::rgb4(3, 8, 4), shade});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(10, 7, 2), shade});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(14, 13, 9), gs::rgb4(6, 8, 11), gs::rgb4(3, 4, 6), gs::rgb4(12, 12, 13),
                          gs::rgb4(4, 6, 8), gs::rgb4(15, 12, 3), gs::rgb4(9, 6, 2), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0,
                          gs::rgb4(15, 15, 12), shade});
    setPal(vdp, PAL_TUG, {0, gs::rgb4(4, 4, 5), gs::rgb4(10, 3, 2), gs::rgb4(7, 7, 8), gs::rgb4(12, 8, 4),
                          gs::rgb4(2, 2, 3), gs::rgb4(14, 12, 6), 0, gs::rgb4(12, 4, 2), gs::rgb4(6, 2, 1), 0, 0, 0, 0,
                          gs::rgb4(15, 10, 3), shade});
    setPal(vdp, PAL_FERRY, {0, gs::rgb4(13, 13, 14), gs::rgb4(6, 7, 9), gs::rgb4(9, 10, 12), gs::rgb4(4, 5, 7),
                            gs::rgb4(2, 3, 4), gs::rgb4(15, 14, 8), gs::rgb4(5, 8, 11), gs::rgb4(11, 6, 3), 0, 0, 0, 0,
                            0, gs::rgb4(15, 15, 12), shade});
    setPal(vdp, PAL_CUT, {0, gs::rgb4(5, 7, 6), gs::rgb4(8, 11, 9), gs::rgb4(3, 4, 4), gs::rgb4(12, 12, 10),
                          gs::rgb4(6, 8, 7), gs::rgb4(14, 13, 6), 0, gs::rgb4(11, 4, 2), gs::rgb4(15, 8, 3), 0, 0, 0, 0,
                          0, gs::rgb4(15, 15, 12), shade});
    setPal(vdp, PAL_FX, {0, gs::rgb4(12, 14, 15), gs::rgb4(7, 10, 12), gs::rgb4(15, 12, 4), gs::rgb4(15, 6, 2),
                         gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_QUAY, {0, gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 4), gs::rgb4(9, 9, 8), gs::rgb4(3, 3, 3),
                           gs::rgb4(11, 8, 4), 0, 0, 0, gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3)});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(8, 5, 4), gs::rgb4(5, 3, 3), gs::rgb4(11, 9, 7), gs::rgb4(3, 2, 2),
                            gs::rgb4(6, 4, 3), gs::rgb4(14, 12, 6), gs::rgb4(3, 4, 6), gs::rgb4(2, 2, 3),
                            gs::rgb4(9, 6, 3), gs::rgb4(12, 8, 4)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 5, 8), gs::rgb4(4, 8, 11), gs::rgb4(1, 3, 6), gs::rgb4(8, 12, 13)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(6, 6, 6), gs::rgb4(10, 8, 4), gs::rgb4(3, 3, 3), 0, 0, gs::rgb4(15, 13, 5),
                           gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, gs::rgb4(15, 15, 10), shade});

    uint8_t px[64];
    gs::TileAlloc tiles(vdp);
    stoneTile(px, 1);
    int stone = makeTile(tiles, vdp, px);
    stoneTile(px, 4);
    int stone2 = makeTile(tiles, vdp, px);
    brickTile(px, false);
    int brick = makeTile(tiles, vdp, px);
    brickTile(px, true);
    int brick2 = makeTile(tiles, vdp, px);
    windowTile(px, true);
    int winLit = makeTile(tiles, vdp, px);
    windowTile(px, false);
    int winDim = makeTile(tiles, vdp, px);
    waterTile(px, 0);
    int waterA = makeTile(tiles, vdp, px);
    waterTile(px, 3);
    int waterB = makeTile(tiles, vdp, px);
    timberTile(px);
    int timber = makeTile(tiles, vdp, px);
    lampTile(px);
    int lamp = makeTile(tiles, vdp, px);
    paintHarbor(vdp, stone, stone2, brick, brick2, winLit, winDim, waterA, waterB, timber, lamp);
    loadFont(vdp, tiles, art);

    art.pilot[0] = gs::uploadMipped(vdp, pilotArt(0));
    art.pilot[1] = gs::uploadMipped(vdp, pilotArt(1));
    art.tug[0] = gs::uploadMipped(vdp, tugArt(0));
    art.tug[1] = gs::uploadMipped(vdp, tugArt(1));
    art.ferry[0] = gs::uploadMipped(vdp, ferryArt(0));
    art.ferry[1] = gs::uploadMipped(vdp, ferryArt(1));
    art.cutter[0] = gs::uploadMipped(vdp, cutterArt(0));
    art.cutter[1] = gs::uploadMipped(vdp, cutterArt(1));
    art.wake = gs::uploadMipped(vdp, wakeArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    vdp.A.enabled = false;
    vdp.hudEnabled = true;
}

}  // namespace harborpurs
