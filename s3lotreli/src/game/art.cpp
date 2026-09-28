#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace lot {
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
}

void asphaltTile(uint8_t* p, int salt) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int n = (x * 5 + y * 3 + salt) % 17;
            int c = 1;
            if (n == 0) c = 2;
            else if (n == 7) c = 8;
            else if ((x + y + salt) & 1) c = 2;
            p[y * 8 + x] = uint8_t(c);
        }
    }
}

void stallTile(uint8_t* p) {
    pxFill(p, 1);
    for (int x = 0; x < 8; x++) {
        p[1 * 8 + x] = 5;
        p[2 * 8 + x] = 4;
        p[6 * 8 + x] = 4;
    }
    p[4 * 8 + 3] = 10;
    p[4 * 8 + 4] = 10;
}

void fenceTile(uint8_t* p) {
    pxFill(p, 1);
    for (int y = 0; y < 8; y++) {
        p[y * 8 + 1] = 6;
        p[y * 8 + 6] = 6;
        if (y == 2 || y == 5) {
            for (int x = 0; x < 8; x++) p[y * 8 + x] = 7;
        }
    }
}

void stripeTile(uint8_t* p) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) p[y * 8 + x] = uint8_t((((x + y) / 2) & 1) ? 10 : 11);
}

void windowTile(uint8_t* p, bool lit) {
    pxFill(p, 8);
    for (int y = 1; y < 7; y++)
        for (int x = 1; x < 7; x++) p[y * 8 + x] = lit ? 6 : 12;
    for (int y = 1; y < 7; y++) p[y * 8 + 3] = 8;
}

void boots(Bitmap& b, int x, int y, int step, int cloth) {
    int la = step ? 3 : 0;
    int ra = step ? 0 : 3;
    b.rect(float(x), float(y + la), 5, float(11 - la), cloth);
    b.rect(float(x + 7), float(y + ra), 5, float(11 - ra), cloth);
    b.rect(float(x - 1), float(y + 10), 7, 3, 3);
    b.rect(float(x + 6), float(y + 10), 7, 3, 3);
}

Bitmap watchArt(int step) {
    Bitmap b(40, 52);
    b.rect(12, 2, 14, 4, 8);
    b.rect(10, 5, 18, 3, 2);
    b.ellipse(19, 14, 7, 7, 6);
    b.set(16, 13, 5);
    b.set(21, 13, 5);
    b.rect(14, 16, 8, 2, 3);
    b.poly({{12, 20}, {8, 28}, {10, 40}, {28, 40}, {30, 26}, {26, 20}}, 2);
    b.poly({{16, 22}, {15, 30}, {18, 39}, {24, 39}, {25, 28}}, 1);
    b.rect(11, 30, 16, 3, 4);
    b.rect(27, 24, 4, 8, 6);
    b.rect(30, 22, 6, 8, 7);
    b.rect(31, 23, 4, 5, 1);
    boots(b, 12, 38, step, 2);
    b.outline(5, false);
    return b;
}

Bitmap skateArt(int step) {
    Bitmap b(36, 48);
    b.ellipse(16, 12, 7, 7, 6);
    b.rect(10, 6, 12, 4, 4);
    b.set(13, 12, 5);
    b.set(18, 12, 5);
    b.poly({{10, 18}, {8, 26}, {10, 36}, {26, 36}, {28, 24}, {22, 18}}, 2);
    b.line(8, 22, 3, 32, 7, 1.6f);
    b.rect(1, 30, 6, 3, 3);
    b.rect(6, 38, 18, 3, 4);
    b.ellipse(10, 42, 3, 3, 5);
    b.ellipse(20, 42, 3, 3, 5);
    if (step) b.rect(22, 36, 6, 3, 2);
    b.outline(5, false);
    return b;
}

Bitmap sedanArt(int step) {
    Bitmap b(64, 36);
    b.rect(6, 14, 52, 12, 2);
    b.poly({{16, 14}, {22, 4}, {42, 4}, {48, 14}}, 4);
    b.rect(24, 6, 8, 7, 1);
    b.rect(34, 6, 8, 7, 1);
    b.rect(2, 16, 6, 6, 7);
    b.rect(56, 16, 6, 6, 6);
    float wob = step ? 1.f : 0.f;
    b.ellipse(16, 28 + wob, 6, 6, 3);
    b.ellipse(48, 28 - wob, 6, 6, 3);
    b.ellipse(16, 28 + wob, 2, 2, 1);
    b.ellipse(48, 28 - wob, 2, 2, 1);
    b.outline(5, false);
    return b;
}

Bitmap vanArt(int step) {
    Bitmap b(78, 42);
    b.rect(8, 6, 58, 22, 2);
    b.rect(50, 8, 12, 10, 1);
    b.rect(12, 10, 16, 10, 3);
    b.rect(30, 10, 16, 10, 4);
    b.rect(4, 16, 6, 8, 7);
    float wob = step ? 1.f : -1.f;
    b.ellipse(22, 32 + wob, 7, 7, 5);
    b.ellipse(54, 32 - wob, 7, 7, 5);
    b.ellipse(22, 32 + wob, 2, 2, 1);
    b.ellipse(54, 32 - wob, 2, 2, 1);
    b.outline(5, false);
    return b;
}

Bitmap reliefArt() {
    Bitmap b(84, 40);
    b.rect(10, 8, 62, 18, 2);
    b.rect(48, 10, 18, 10, 1);
    b.rect(14, 12, 20, 8, 6);
    b.rect(6, 16, 8, 6, 7);
    b.rect(18, 2, 8, 8, 3);
    b.ellipse(26, 30, 7, 7, 4);
    b.ellipse(58, 30, 7, 7, 4);
    b.ellipse(26, 30, 2, 2, 1);
    b.ellipse(58, 30, 2, 2, 1);
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

Bitmap lampPostArt() {
    Bitmap b(14, 28);
    b.rect(6, 8, 2, 18, 4);
    b.rect(2, 2, 10, 8, 2);
    b.rect(4, 4, 6, 4, 1);
    b.outline(5, false);
    return b;
}

Bitmap coneArt() {
    Bitmap b(16, 22);
    b.poly({{8, 1}, {14, 18}, {2, 18}}, 6);
    b.rect(4, 8, 8, 2, 1);
    b.rect(3, 18, 10, 3, 3);
    b.outline(5, false);
    return b;
}

Bitmap bollardArt() {
    Bitmap b(14, 22);
    b.rect(4, 2, 6, 16, 4);
    b.rect(3, 2, 8, 3, 6);
    b.rect(2, 17, 10, 3, 3);
    b.outline(5, false);
    return b;
}

Bitmap crateArt() {
    Bitmap b(22, 18);
    b.rect(1, 1, 20, 16, 2);
    b.rect(3, 3, 16, 12, 1);
    b.line(2, 2, 19, 15, 3, 1.2f);
    b.line(19, 2, 2, 15, 3, 1.2f);
    b.outline(5, false);
    return b;
}

Bitmap drumArt() {
    Bitmap b(16, 20);
    b.ellipse(8, 10, 7, 8, 2);
    b.ellipse(8, 10, 7, 3, 3);
    b.rect(1, 5, 14, 2, 4);
    b.rect(1, 12, 14, 2, 4);
    b.outline(5, false);
    return b;
}

Bitmap flareArt() {
    Bitmap b(10, 14);
    b.rect(3, 0, 4, 2, 4);
    b.ellipse(5, 7, 4, 5, 6);
    b.ellipse(5, 7, 2, 2, 1);
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

bool stallRow(int y) {
    for (int row : kRowCell)
        if (y == row) return true;
    return false;
}

void paint(gs::VDP& vdp, int roof, int brick, int brick2, int winHi, int winLo, int asphalt, int asphalt2, int stall,
           int fence, int stripe) {
    auto put = [&](int x, int y, int tile, int pal) { vdp.B.set(x, y, gs::entry(tile, pal)); };
    for (int y = 0; y < 28; y++) {
        for (int x = 0; x < 40; x++) {
            if (y <= 4) {
                put(x, y, (y == 4 && x >= 6) ? roof : 0, PAL_WALL);
            } else if (x <= 7 && y > 4) {
                int tile = ((x + y) & 1) ? brick2 : brick;
                if (x == 5 && y == 6) tile = winHi;
                if (x == 5 && y == 8) tile = winLo;
                if (x == 7) tile = fence;
                put(x, y, tile, x == 7 ? PAL_CHAIN : PAL_WALL);
            } else if (y >= 5 && y <= 8 && x >= 8) {
                put(x, y, (y == 8) ? stripe : brick, (y == 8) ? PAL_LOT : PAL_WALL);
            } else if (x >= 8 && stallRow(y)) {
                put(x, y, stall, PAL_LOT);
            } else if (x >= 8) {
                put(x, y, ((x * 3 + y) & 1) ? asphalt2 : asphalt, PAL_LOT);
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
    setPal(vdp, PAL_WATCH,
           {0, gs::rgb4(15, 13, 3), gs::rgb4(2, 3, 6), gs::rgb4(1, 1, 2), gs::rgb4(12, 10, 4), gs::rgb4(1, 1, 2),
            gs::rgb4(13, 9, 6), gs::rgb4(15, 14, 6), gs::rgb4(3, 3, 5), 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_SKATE,
           {0, gs::rgb4(12, 10, 8), gs::rgb4(8, 2, 3), gs::rgb4(3, 2, 2), gs::rgb4(10, 8, 6), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 9, 6), gs::rgb4(14, 12, 4), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_SEDAN,
           {0, gs::rgb4(10, 12, 14), gs::rgb4(2, 4, 8), gs::rgb4(1, 1, 2), gs::rgb4(6, 8, 12), gs::rgb4(1, 1, 1),
            gs::rgb4(15, 12, 4), gs::rgb4(14, 4, 3), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(14, 10, 6), gs::rgb4(8, 4, 2), gs::rgb4(4, 2, 1), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 11, 8), gs::rgb4(12, 8, 3), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_CHAIN,
           {0, gs::rgb4(4, 4, 4), gs::rgb4(8, 8, 7), gs::rgb4(2, 2, 2), gs::rgb4(11, 11, 10), gs::rgb4(1, 1, 1),
            gs::rgb4(9, 9, 8), gs::rgb4(6, 6, 6), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 9, 3), gs::rgb4(7, 5, 2), gs::rgb4(8, 7, 5), gs::rgb4(1, 1, 1),
            gs::rgb4(11, 8, 4), gs::rgb4(15, 15, 11), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_WALL,
           {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(4, 4, 4), gs::rgb4(7, 7, 6), gs::rgb4(3, 3, 3),
            gs::rgb4(14, 12, 5), gs::rgb4(6, 6, 7), gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 4), gs::rgb4(9, 9, 8),
            gs::rgb4(1, 1, 2), gs::rgb4(6, 7, 8), 0, gs::rgb4(10, 10, 9), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(14, 14, 12), gs::rgb4(10, 10, 8), gs::rgb4(8, 8, 11), gs::rgb4(7, 7, 8), gs::rgb4(1, 1, 2),
            gs::rgb4(3, 3, 4), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, shade});
    setPal(vdp, PAL_LOT,
           {0, gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 4), gs::rgb4(8, 8, 7), gs::rgb4(12, 12, 11),
            gs::rgb4(2, 3, 2), gs::rgb4(6, 6, 5), gs::rgb4(1, 1, 1), gs::rgb4(4, 4, 4), gs::rgb4(13, 12, 3),
            gs::rgb4(14, 12, 2), gs::rgb4(2, 2, 1), 0, 0, 0, shade});
    setPal(vdp, PAL_RELIEF,
           {0, gs::rgb4(12, 14, 10), gs::rgb4(2, 7, 3), gs::rgb4(1, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1),
            gs::rgb4(14, 13, 6), gs::rgb4(12, 9, 3), gs::rgb4(8, 8, 8), 0, 0, 0, 0, 0, 0, shade});
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
    windowTile(tile, true);
    int winHi = makeTile(alloc, vdp, tile);
    windowTile(tile, false);
    int winLo = makeTile(alloc, vdp, tile);
    asphaltTile(tile, 0);
    int asphalt = makeTile(alloc, vdp, tile);
    asphaltTile(tile, 5);
    int asphalt2 = makeTile(alloc, vdp, tile);
    stallTile(tile);
    int stall = makeTile(alloc, vdp, tile);
    fenceTile(tile);
    int fence = makeTile(alloc, vdp, tile);
    stripeTile(tile);
    int stripe = makeTile(alloc, vdp, tile);
    paint(vdp, roof, brick, brick2, winHi, winLo, asphalt, asphalt2, stall, fence, stripe);

    art.watch[0] = gs::uploadMipped(vdp, watchArt(0));
    art.watch[1] = gs::uploadMipped(vdp, watchArt(1));
    art.skate[0] = gs::uploadMipped(vdp, skateArt(0));
    art.skate[1] = gs::uploadMipped(vdp, skateArt(1));
    art.sedan[0] = gs::uploadMipped(vdp, sedanArt(0));
    art.sedan[1] = gs::uploadMipped(vdp, sedanArt(1));
    art.van[0] = gs::uploadMipped(vdp, vanArt(0));
    art.van[1] = gs::uploadMipped(vdp, vanArt(1));
    art.relief = gs::uploadMipped(vdp, reliefArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.lampPost = gs::uploadMipped(vdp, lampPostArt());
    art.cone = gs::uploadMipped(vdp, coneArt());
    art.bollard = gs::uploadMipped(vdp, bollardArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.flare = gs::uploadMipped(vdp, flareArt());
    art.glint = gs::uploadMipped(vdp, glintArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.sign = gs::uploadMipped(vdp, gs::textBitmap("LOT", {2, 1, 0, 0, 1}));
    loadFont(vdp, alloc, art);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.B.scroll(0, 0);
    vdp.hudEnabled = true;
}

}  // namespace lot
