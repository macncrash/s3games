#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace depotpurs {
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

void brickTile(uint8_t* p, bool dark) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int row = y / 4;
            int mx = (x + row * 4) & 7;
            bool mortar = (y % 4 == 3) || mx == 7;
            int c = mortar ? 3 : (dark ? 2 : 1);
            if (!mortar && ((x * 3 + y * 5) & 7) == 0) c = dark ? 1 : 2;
            p[y * 8 + x] = uint8_t(c);
        }
    }
}

void roofTile(uint8_t* p) {
    fill8(p, 9);
    for (int x = 0; x < 8; x++) p[x] = 10;
    p[3 * 8 + 2] = 14;
    p[5 * 8 + 5] = 1;
}

void windowTile(uint8_t* p, bool lit) {
    fill8(p, 1);
    for (int y = 1; y < 7; y++)
        for (int x = 1; x < 7; x++) p[y * 8 + x] = lit ? 6 : 7;
    for (int y = 1; y < 7; y++) p[y * 8 + 3] = 8;
    for (int x = 1; x < 7; x++) p[4 * 8 + x] = 8;
    if (lit) p[2 * 8 + 5] = 14;
}

void doorTile(uint8_t* p) {
    fill8(p, 11);
    for (int y = 1; y < 8; y++)
        for (int x = 1; x < 7; x++) p[y * 8 + x] = 12;
    for (int y = 1; y < 8; y++) p[y * 8 + 5] = 8;
    p[4 * 8 + 2] = 14;
}

void corniceTile(uint8_t* p) {
    fill8(p, 4);
    for (int x = 0; x < 8; x++) {
        p[x] = 10;
        p[8 + x] = 5;
        p[7 * 8 + x] = 5;
    }
}

void concreteTile(uint8_t* p) {
    fill8(p, 10);
    p[2 * 8 + 3] = 11;
    p[5 * 8 + 6] = 11;
    p[4 * 8 + 1] = 15;
    for (int x = 0; x < 8; x++) p[7 * 8 + x] = 11;
}

void hazardTile(uint8_t* p) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) p[y * 8 + x] = uint8_t((((x + y) / 2) & 1) ? 12 : 13);
}

void gravelTile(uint8_t* p, int salt) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int n = (x * 5 + y * 3 + salt * 7) % 13;
            int c = 1;
            if (n == 0) c = 3;
            else if (n == 4) c = 2;
            else if (n == 9) c = 9;
            else if ((x + y + salt) & 1) c = 14;
            p[y * 8 + x] = uint8_t(c);
        }
    }
}

void railTile(uint8_t* p) {
    gravelTile(p, 2);
    for (int x = 0; x < 8; x++) {
        p[1 * 8 + x] = 4;
        p[2 * 8 + x] = 5;
        p[5 * 8 + x] = 5;
        p[6 * 8 + x] = 4;
        if ((x & 3) == 0) {
            p[3 * 8 + x] = 6;
            p[4 * 8 + x] = 6;
        }
    }
}

void ballastTile(uint8_t* p) {
    gravelTile(p, 5);
    for (int i = 0; i < 64; i++)
        if (p[i] == 1) p[i] = 7;
}

void towerTile(uint8_t* p, bool lamp) {
    brickTile(p, true);
    if (!lamp) return;
    for (int y = 2; y < 6; y++)
        for (int x = 2; x < 6; x++) p[y * 8 + x] = 6;
    p[3 * 8 + 3] = 14;
}

void paintYard(gs::VDP& vdp, int roof, int corn, int brick, int brick2, int winLit, int winDim, int door, int tower,
               int concrete, int hazard, int gravel, int gravel2, int rail, int ballast) {
    auto put = [&](int x, int y, int tile, int pal) { vdp.B.set(x, y, gs::entry(tile, pal)); };
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 40; x++) {
            bool house = x <= 6;
            bool towerCol = x >= 1 && x <= 3 && y <= 3;
            if (towerCol) {
                if (y == 0) put(x, y, roof, PAL_HOUSE);
                else if (y == 2 && x == 2) put(x, y, tower, PAL_HOUSE);
                else put(x, y, brick2, PAL_HOUSE);
            } else if (house && y < 4) {
                put(x, y, y == 3 ? corn : brick2, PAL_HOUSE);
            } else if (house) {
                int tile = ((x + y) & 1) ? brick2 : brick;
                if (x == 5 && (y == 8 || y == 12 || y == 18)) tile = (y == 12) ? winLit : winDim;
                if (x == 4 && y >= 22) tile = door;
                if (y == 27) tile = corn;
                put(x, y, tile, PAL_HOUSE);
            } else if (y < 5) {
                // Sky. The backdrop shows through.
            } else if (y == 5) {
                put(x, y, roof, PAL_HOUSE);
            } else if (y == 6 || y == 7) {
                int tile = brick;
                if (y == 6 && (x == 12 || x == 18 || x == 24 || x == 31 || x == 36)) tile = winLit;
                if (y == 7 && (x == 12 || x == 18 || x == 24 || x == 31 || x == 36)) tile = winDim;
                put(x, y, tile, PAL_HOUSE);
            } else if (y == 8) {
                put(x, y, corn, PAL_HOUSE);
            } else if (y == 9) {
                put(x, y, concrete, PAL_YARD);
            } else if (y == 10) {
                put(x, y, hazard, PAL_YARD);
            } else if (y == kTrackRow[0] || y == kTrackRow[1] || y == kTrackRow[2]) {
                put(x, y, rail, PAL_YARD);
            } else if (y >= 23) {
                put(x, y, ballast, PAL_YARD);
            } else {
                put(x, y, ((x * 3 + y) & 1) ? gravel2 : gravel, PAL_YARD);
            }
        }
    }
}

void wheels(Bitmap& b, int x0, int x1, int y, int step) {
    b.ellipse(float(x0), float(y), 4.2f, 4.2f, 7);
    b.ellipse(float(x1), float(y), 4.2f, 4.2f, 7);
    b.ellipse(float(x0), float(y), 1.6f, 1.6f, 11);
    b.ellipse(float(x1), float(y), 1.6f, 1.6f, 11);
    float s = step ? 2.4f : -2.4f;
    b.line(float(x0), float(y), float(x0) + s, float(y) - s, 5, 1.f);
    b.line(float(x1), float(y), float(x1) - s, float(y) - s, 5, 1.f);
}

Bitmap shunterArt(int step) {
    Bitmap b(40, 28);
    wheels(b, 12, 28, 22, step);
    b.rect(8, 16, 24, 5, 3);
    b.rect(16, 11, 18, 9, 2);
    b.rect(18, 13, 12, 3, 1);
    b.rect(4, 8, 14, 13, 4);
    b.rect(6, 10, 8, 6, 8);
    b.set(8, 12, 6);
    b.rect(26, 4, 4, 8, 3);
    b.rect(24, 3, 8, 3, 5);
    b.rect(33, 12, 5, 3, 9);
    b.rect(34, 17, 4, 3, 12);
    b.outline(5, false);
    return b;
}

Bitmap drayArt(int step) {
    Bitmap b(46, 28);
    wheels(b, 12, 34, 22, step);
    b.rect(3, 13, 24, 8, 2);
    b.rect(5, 10, 18, 5, 12);
    b.line(6, 12, 20, 12, 3, 1.f);
    b.rect(26, 7, 15, 14, 4);
    b.rect(29, 9, 8, 6, 8);
    b.set(31, 11, 6);
    b.rect(39, 14, 4, 3, 9);
    b.rect(2, 15, 3, 4, 10);
    b.outline(5, false);
    return b;
}

Bitmap craneArt(int step) {
    Bitmap b(50, 32);
    wheels(b, 14, 36, 26, step);
    b.rect(6, 20, 36, 6, 2);
    b.rect(28, 13, 14, 10, 3);
    b.rect(31, 15, 8, 5, 8);
    b.line(18, 22, 42, 5, 12, 2.f);
    b.line(42, 5, 42, 14, 13, 1.1f);
    b.rect(39, 14, 6, 3, 5);
    b.rect(42, 21, 4, 3, 9);
    b.outline(5, false);
    return b;
}

Bitmap locoArt(int step) {
    Bitmap b(62, 30);
    wheels(b, 20, 34, 23, step);
    b.ellipse(48, 24, 3.6f, 3.6f, 7);
    b.ellipse(48, 24, 1.4f, 1.4f, 11);
    b.rect(8, 15, 40, 8, 2);
    b.rect(12, 16, 28, 3, 1);
    b.rect(10, 21, 36, 2, 4);
    b.rect(2, 8, 14, 16, 3);
    b.rect(5, 10, 8, 7, 8);
    b.set(7, 13, 6);
    b.ellipse(30, 14, 3.5f, 2.6f, 6);
    b.rect(40, 4, 5, 11, 5);
    b.rect(38, 3, 9, 3, 13);
    b.rect(50, 15, 6, 4, 9);
    b.rect(54, 20, 5, 3, 4);
    b.outline(5, false);
    return b;
}

Bitmap flameArt(int frame) {
    Bitmap b(14, 18);
    if (frame) {
        b.poly({{7, 1}, {12, 9}, {13, 17}, {1, 17}, {3, 8}}, 4);
        b.poly({{7, 6}, {10, 12}, {9, 17}, {5, 17}, {5, 11}}, 3);
    } else {
        b.poly({{7, 2}, {11, 10}, {12, 17}, {2, 17}, {4, 9}}, 3);
        b.poly({{7, 7}, {9, 12}, {8, 17}, {6, 17}, {5, 12}}, 2);
    }
    b.poly({{7, 11}, {8, 16}, {6, 16}}, 1);
    return b;
}

Bitmap shoeArt() {
    Bitmap b(12, 8);
    b.rect(1, 2, 8, 4, 8);
    b.rect(7, 1, 4, 6, 9);
    b.rect(8, 2, 2, 2, 1);
    b.outline(5, false);
    return b;
}

Bitmap boltArt() {
    Bitmap b(10, 8);
    b.ellipse(5, 4, 4, 3, 3);
    b.ellipse(4, 4, 2, 1.4f, 2);
    b.set(3, 3, 1);
    return b;
}

Bitmap puffArt() {
    Bitmap b(14, 12);
    b.ellipse(7, 7, 6, 4, 6);
    b.ellipse(6, 6, 3, 2, 7);
    b.ellipse(8, 5, 1.4f, 1.f, 1);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(11, 11);
    b.line(1, 5, 9, 5, 2, 1.f);
    b.line(5, 1, 5, 9, 10, 1.f);
    b.line(2, 2, 8, 8, 1, 1.f);
    b.line(8, 2, 2, 8, 1, 1.f);
    return b;
}

Bitmap pipArt() {
    Bitmap b(6, 5);
    b.rect(1, 1, 4, 3, 1);
    b.outline(5, false);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(36, 8);
    b.ellipse(18, 4, 16, 3, 1);
    return b;
}

Bitmap bufferArt() {
    Bitmap b(14, 20);
    b.rect(5, 6, 4, 12, 3);
    b.rect(2, 2, 10, 6, 4);
    b.rect(3, 3, 8, 3, 1);
    b.rect(4, 16, 6, 3, 2);
    b.outline(5, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(10, 28);
    b.rect(4, 6, 2, 20, 3);
    b.rect(2, 2, 6, 6, 2);
    b.rect(3, 3, 4, 4, 6);
    b.rect(3, 24, 4, 3, 10);
    b.outline(5, false);
    return b;
}

Bitmap barrelArt() {
    Bitmap b(14, 16);
    b.ellipse(7, 8, 6, 6, 8);
    b.ellipse(7, 8, 6, 2.4f, 9);
    b.rect(1, 5, 12, 2, 7);
    b.rect(1, 10, 12, 2, 7);
    b.outline(5, false);
    return b;
}

Bitmap crateArt() {
    Bitmap b(18, 14);
    b.rect(1, 1, 16, 12, 7);
    b.rect(3, 3, 12, 8, 8);
    b.line(2, 2, 15, 11, 9, 1.f);
    b.line(15, 2, 2, 11, 9, 1.f);
    b.rect(1, 6, 16, 2, 9);
    b.outline(5, false);
    return b;
}

Bitmap moonArt() {
    Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 5, 5, 2);
    b.ellipse(6, 7, 1.4f, 1.4f, 6);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(36, 14);
    b.ellipse(10, 8, 7, 4, 3);
    b.ellipse(20, 7, 9, 5, 4);
    b.ellipse(28, 8, 6, 3.5f, 3);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 15, 0, 1};
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
    const uint16_t ink = gs::rgb4(15, 15, 13);
    const uint16_t shade = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(3, 3, 5), gs::rgb4(8, 8, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 4), gs::rgb4(9, 2, 2), 0, 0, gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 8), gs::rgb4(3, 8, 4), 0, 0, gs::rgb4(1, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 2, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(12, 8, 2), 0, 0, gs::rgb4(4, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 0)});

    auto machine = [&](int pal, uint16_t hi, uint16_t body, uint16_t lo, uint16_t cab, uint16_t stripe) {
        setPal(vdp, pal,
               {0, hi, body, lo, cab, gs::rgb4(0, 0, 0), gs::rgb4(13, 9, 6), gs::rgb4(2, 2, 2), gs::rgb4(10, 14, 15),
                gs::rgb4(15, 14, 6), stripe, gs::rgb4(14, 14, 12), gs::rgb4(6, 6, 5), gs::rgb4(12, 10, 3),
                gs::rgb4(9, 9, 8), gs::rgb4(1, 1, 1)});
    };
    machine(PAL_YOU, gs::rgb4(8, 14, 7), gs::rgb4(3, 9, 4), gs::rgb4(1, 4, 2), gs::rgb4(2, 3, 3), gs::rgb4(12, 3, 2));
    machine(PAL_DRAY, gs::rgb4(13, 10, 6), gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 1), gs::rgb4(4, 3, 3), gs::rgb4(12, 3, 2));
    machine(PAL_CRANE, gs::rgb4(15, 14, 6), gs::rgb4(13, 11, 2), gs::rgb4(8, 6, 1), gs::rgb4(5, 5, 4), gs::rgb4(12, 3, 2));
    machine(PAL_LOCO, gs::rgb4(8, 8, 9), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2), gs::rgb4(2, 2, 3), gs::rgb4(12, 2, 2));

    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 13, 4), gs::rgb4(15, 8, 2), gs::rgb4(12, 3, 2), gs::rgb4(1, 1, 1),
            gs::rgb4(8, 8, 8), gs::rgb4(12, 12, 11), gs::rgb4(7, 7, 8), gs::rgb4(8, 5, 2), gs::rgb4(15, 15, 8),
            gs::rgb4(14, 14, 12), gs::rgb4(9, 9, 8), gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 2), gs::rgb4(7, 6, 5), gs::rgb4(9, 9, 10), gs::rgb4(3, 3, 4),
            gs::rgb4(6, 4, 2), gs::rgb4(4, 4, 3), gs::rgb4(3, 4, 2), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 7),
            gs::rgb4(5, 5, 4), gs::rgb4(13, 10, 2), gs::rgb4(1, 1, 1), gs::rgb4(4, 3, 2), gs::rgb4(12, 12, 11)});
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(9, 3, 2), gs::rgb4(6, 2, 1), gs::rgb4(4, 3, 3), gs::rgb4(7, 7, 6), gs::rgb4(3, 3, 3),
            gs::rgb4(14, 11, 5), gs::rgb4(6, 5, 2), gs::rgb4(2, 2, 3), gs::rgb4(5, 2, 2), gs::rgb4(3, 1, 1),
            gs::rgb4(4, 2, 1), gs::rgb4(7, 4, 2), gs::rgb4(12, 10, 6), gs::rgb4(15, 12, 4), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_PROP,
           {0, gs::rgb4(12, 12, 11), gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 4), gs::rgb4(8, 2, 2), gs::rgb4(1, 1, 1),
            gs::rgb4(15, 13, 5), gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(5, 3, 2), gs::rgb4(10, 10, 8),
            gs::rgb4(14, 14, 12), gs::rgb4(3, 3, 3), gs::rgb4(7, 2, 2), gs::rgb4(9, 8, 6), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(14, 14, 11), gs::rgb4(10, 10, 8), gs::rgb4(7, 7, 11), gs::rgb4(5, 5, 8), gs::rgb4(2, 2, 4),
            gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0, shade});

    uint8_t tile[64];
    gs::TileAlloc alloc(vdp);
    brickTile(tile, false);
    int brick = makeTile(alloc, vdp, tile);
    brickTile(tile, true);
    int brick2 = makeTile(alloc, vdp, tile);
    roofTile(tile);
    int roof = makeTile(alloc, vdp, tile);
    corniceTile(tile);
    int corn = makeTile(alloc, vdp, tile);
    windowTile(tile, true);
    int winLit = makeTile(alloc, vdp, tile);
    windowTile(tile, false);
    int winDim = makeTile(alloc, vdp, tile);
    doorTile(tile);
    int door = makeTile(alloc, vdp, tile);
    towerTile(tile, true);
    int tower = makeTile(alloc, vdp, tile);
    concreteTile(tile);
    int concrete = makeTile(alloc, vdp, tile);
    hazardTile(tile);
    int hazard = makeTile(alloc, vdp, tile);
    gravelTile(tile, 0);
    int gravel = makeTile(alloc, vdp, tile);
    gravelTile(tile, 3);
    int gravel2 = makeTile(alloc, vdp, tile);
    railTile(tile);
    int rail = makeTile(alloc, vdp, tile);
    ballastTile(tile);
    int ballast = makeTile(alloc, vdp, tile);
    paintYard(vdp, roof, corn, brick, brick2, winLit, winDim, door, tower, concrete, hazard, gravel, gravel2, rail, ballast);

    art.shunter[0] = gs::uploadMipped(vdp, shunterArt(0));
    art.shunter[1] = gs::uploadMipped(vdp, shunterArt(1));
    art.dray[0] = gs::uploadMipped(vdp, drayArt(0));
    art.dray[1] = gs::uploadMipped(vdp, drayArt(1));
    art.crane[0] = gs::uploadMipped(vdp, craneArt(0));
    art.crane[1] = gs::uploadMipped(vdp, craneArt(1));
    art.loco[0] = gs::uploadMipped(vdp, locoArt(0));
    art.loco[1] = gs::uploadMipped(vdp, locoArt(1));
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.shoe = gs::uploadMipped(vdp, shoeArt());
    art.bolt = gs::uploadMipped(vdp, boltArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.buffer = gs::uploadMipped(vdp, bufferArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sign = gs::uploadMipped(vdp, gs::textBitmap("DEPOT", {2, 13, 8, 0, 1}));
    loadFont(vdp, alloc, art);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.B.scroll(0, 0);
    vdp.hudEnabled = true;
}

}  // namespace depotpurs
