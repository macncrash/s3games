#include "game/art.h"

#include <initializer_list>
#include <string>

namespace tower {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    if (i == 15) vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

int ashlarTile(gs::TileAlloc& al, gs::VDP& vdp, int shade) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int v = shade;
            int shift = (y >= 4) ? 3 : 0;
            if (y == 0 || y == 4 || ((x + shift) % 8) == 0) v = 5;
            else if ((x * 3 + y) % 11 == 0) v = 4;
            px[y * 8 + x] = uint8_t(v);
        }
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

int walkTile(gs::TileAlloc& al, gs::VDP& vdp) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int v = 2;
            if (y == 0) v = 6;
            else if (y > 5) v = 1;
            else if ((x + y) % 6 == 0) v = 3;
            px[y * 8 + x] = uint8_t(v);
        }
    }
    int t = al.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

void paintTower(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int a = ashlarTile(tiles, vdp, 2);
    int b = ashlarTile(tiles, vdp, 3);
    int walk = walkTile(tiles, vdp);
    vdp.B.resize(64, 32);
    vdp.A.resize(64, 32);
    vdp.B.clear();
    vdp.A.clear();
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            if (cy >= 6 && cy < 20) vdp.B.set(cx, cy, gs::entry((cx + cy) & 1 ? a : b, PAL_ASHLAR));
            if (cy >= 21 && cy <= 25) vdp.A.set(cx, cy, gs::entry(walk, PAL_WALK));
        }
    }
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
    gs::TextStyle big{2, 1, 0, 15, 1};
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

using gs::Bitmap;

Bitmap watcher(int pose) {
    Bitmap b(34, 58);
    const bool duck = pose == 3;
    const bool leap = pose == 4;
    const int y0 = duck ? 18 : 0;
    b.rect(11, y0 + 1, 12, 4, 8);
    b.rect(10, y0 + 4, 14, 3, 7);
    b.ellipse(17, y0 + 12, 6, 6, 4);
    b.set(20, y0 + 11, 1);
    b.rect(14, y0 + 16, 6, 2, 9);
    b.poly({{8, float(y0 + 20)}, {26, float(y0 + 20)}, {29, float(y0 + 40)}, {5, float(y0 + 40)}}, 2);
    b.rect(14, y0 + 22, 6, 12, 6);
    b.rect(8, y0 + 22, 4, 10, 3);
    if (pose == 1) b.rect(24, y0 + 22, 7, 8, 3);
    else if (pose == 2) b.rect(3, y0 + 24, 7, 8, 3);
    else b.rect(24, y0 + 22, 5, 12, 3);
    if (leap) {
        b.rect(8, y0 + 38, 6, 8, 5);
        b.rect(20, y0 + 38, 6, 8, 5);
        b.rect(6, y0 + 44, 8, 3, 7);
        b.rect(20, y0 + 44, 8, 3, 7);
    } else if (!duck) {
        int leg = pose == 1 ? 0 : (pose == 2 ? 4 : 2);
        b.rect(10 + leg, y0 + 38, 5, 14, 5);
        b.rect(18 - leg / 2, y0 + 38, 5, 14, 5);
        b.rect(9 + leg, y0 + 50, 7, 3, 7);
        b.rect(17 - leg / 2, y0 + 50, 7, 3, 7);
    } else {
        b.rect(6, y0 + 38, 10, 4, 5);
        b.rect(18, y0 + 38, 10, 4, 5);
        b.rect(4, y0 + 40, 8, 3, 7);
        b.rect(22, y0 + 40, 8, 3, 7);
    }
    return b.cropToContent(1);
}

Bitmap pouchBmp(int bob) {
    Bitmap b(22, 18);
    b.ellipse(11, 10 + bob, 8, 6, 2);
    b.rect(7, 4, 8, 4, 3);
    b.rect(9, 2, 4, 3, 6);
    b.rect(6, 8, 10, 2, 4);
    b.set(8, 11, 1);
    return b.cropToContent(1);
}

Bitmap merlonBmp() {
    Bitmap b(28, 36);
    b.rect(2, 10, 24, 26, 2);
    b.rect(2, 10, 24, 3, 5);
    b.rect(6, 2, 6, 10, 3);
    b.rect(16, 2, 6, 10, 3);
    b.rect(0, 28, 28, 4, 1);
    return b;
}

Bitmap bannerBmp() {
    Bitmap b(16, 28);
    b.rect(7, 0, 2, 28, 5);
    b.poly({{8, 4}, {15, 10}, {8, 16}}, 2);
    b.poly({{8, 8}, {13, 12}, {8, 14}}, 3);
    return b;
}

Bitmap lampBmp() {
    Bitmap b(14, 40);
    b.rect(6, 8, 2, 32, 3);
    b.rect(3, 4, 8, 8, 4);
    b.rect(5, 6, 4, 4, 6);
    b.rect(2, 36, 10, 3, 2);
    return b;
}

Bitmap flameBmp(int n) {
    Bitmap b(8, 10);
    b.ellipse(4, 6, 2, 3 + n, 2);
    b.ellipse(4, 7, 1, 2, 3);
    return b;
}

Bitmap clockBmp() {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 16, 16, 3);
    b.ellipse(20, 20, 12, 12, 1);
    b.rect(18, 4, 4, 6, 5);
    for (int i = 0; i < 4; i++) b.set(20, 10 + i * 6, 6);
    b.set(14, 20, 6);
    b.set(26, 20, 6);
    return b;
}

Bitmap handBmp() {
    Bitmap b(8, 28);
    b.rect(3, 0, 2, 24, 6);
    b.ellipse(4, 24, 3, 3, 4);
    return b;
}

Bitmap wellBmp() {
    Bitmap b(48, 28);
    b.rect(0, 0, 48, 28, 1);
    for (int x = 4; x < 44; x += 8) b.rect(x, 4, 2, 20, 2);
    return b;
}

Bitmap lipBmp() {
    Bitmap b(10, 18);
    b.rect(2, 0, 6, 18, 3);
    b.rect(0, 0, 10, 4, 5);
    return b;
}

Bitmap doorBmp() {
    Bitmap b(36, 52);
    b.rect(2, 8, 32, 44, 2);
    b.poly({{2, 10}, {18, 0}, {34, 10}}, 3);
    b.rect(10, 20, 16, 28, 1);
    b.rect(22, 32, 3, 3, 6);
    b.rect(8, 46, 20, 4, 4);
    return b;
}

Bitmap stairBmp() {
    Bitmap b(28, 16);
    b.rect(0, 10, 28, 6, 2);
    b.rect(6, 6, 16, 5, 3);
    b.rect(10, 2, 8, 5, 5);
    return b;
}

Bitmap shadowBmp() {
    Bitmap b(30, 8);
    b.ellipse(15, 4, 13, 3, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10), gs::rgb4(8, 7, 6), gs::rgb4(4, 3, 3)});
    setPal(vdp, PAL_ASHLAR, {0, gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(10, 10, 11), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_WALK, {0, gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), gs::rgb4(9, 8, 7), gs::rgb4(3, 3, 4), 0, gs::rgb4(12, 11, 9)});
    setPal(vdp, PAL_POUCH, {0, gs::rgb4(2, 1, 1), gs::rgb4(10, 6, 2), gs::rgb4(13, 9, 3), gs::rgb4(6, 3, 1), 0, gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_WATCH, {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 6, 8), gs::rgb4(5, 9, 10), gs::rgb4(12, 9, 7), gs::rgb4(4, 3, 3),
                           gs::rgb4(9, 3, 3), gs::rgb4(6, 6, 7), gs::rgb4(13, 12, 8), gs::rgb4(8, 5, 4)});
    setPal(vdp, PAL_COPPER, {0, gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(10, 6, 3), gs::rgb4(13, 8, 3), gs::rgb4(8, 7, 5), gs::rgb4(15, 13, 7)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(2, 2, 3), gs::rgb4(10, 2, 3), gs::rgb4(14, 10, 3), 0, gs::rgb4(7, 6, 5)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 3, 1), gs::rgb4(14, 10, 2), gs::rgb4(15, 14, 6), gs::rgb4(8, 6, 3), 0, gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(12, 8, 2), gs::rgb4(6, 2, 2)});
    setPal(vdp, PAL_VOID, {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 5), gs::rgb4(5, 5, 7)});
    setPal(vdp, PAL_DOOR, {0, gs::rgb4(3, 2, 2), gs::rgb4(7, 4, 3), gs::rgb4(10, 6, 4), gs::rgb4(5, 3, 2), 0, gs::rgb4(14, 12, 5)});
    setPal(vdp, PAL_GO, {0, gs::rgb4(4, 12, 6), gs::rgb4(8, 14, 8), gs::rgb4(2, 6, 3)});
    vdp.setFogColor(gs::rgb4(2, 2, 4));

    gs::TileAlloc tiles(vdp, 1);
    paintTower(vdp, tiles);
    loadFont(vdp, art, tiles);

    art.stand = gs::uploadMipped(vdp, watcher(0));
    art.runA = gs::uploadMipped(vdp, watcher(1));
    art.runB = gs::uploadMipped(vdp, watcher(2));
    art.duck = gs::uploadMipped(vdp, watcher(3));
    art.leap = gs::uploadMipped(vdp, watcher(4));
    art.pouch[0] = gs::uploadMipped(vdp, pouchBmp(0));
    art.pouch[1] = gs::uploadMipped(vdp, pouchBmp(1));
    art.merlon = gs::uploadMipped(vdp, merlonBmp());
    art.banner = gs::uploadMipped(vdp, bannerBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.flame[0] = gs::uploadMipped(vdp, flameBmp(0));
    art.flame[1] = gs::uploadMipped(vdp, flameBmp(1));
    art.clock = gs::uploadMipped(vdp, clockBmp());
    art.hand = gs::uploadMipped(vdp, handBmp());
    art.well = gs::uploadMipped(vdp, wellBmp());
    art.lip = gs::uploadMipped(vdp, lipBmp());
    art.door = gs::uploadMipped(vdp, doorBmp());
    art.stair = gs::uploadMipped(vdp, stairBmp());
    art.shadow = gs::uploadMipped(vdp, shadowBmp());
}

}  // namespace tower
