#include "game/art.h"

#include <initializer_list>

namespace keel {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

gs::Bitmap boatArt() {
    gs::Bitmap b(112, 56);
    b.poly({{8, 34}, {18, 22}, {78, 20}, {104, 30}, {100, 40}, {16, 42}}, 1);
    b.poly({{16, 42}, {100, 40}, {96, 46}, {22, 48}}, 2);
    b.rect(36, 10, 34, 16, 4);
    b.rect(42, 14, 10, 8, 5);
    b.rect(58, 14, 8, 8, 5);
    b.rect(48, 6, 3, 8, 6);
    b.line(49, 4, 62, 10, 7, 2);
    b.ellipse(30, 30, 3, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap keelArt() {
    gs::Bitmap b(28, 36);
    b.poly({{6, 2}, {20, 2}, {22, 8}, {16, 34}, {10, 34}, {6, 8}}, 3);
    b.line(13, 6, 13, 30, 2, 2);
    b.outline(9, false);
    return b;
}

gs::Bitmap waterArt() {
    gs::Bitmap b(48, 64);
    b.rect(0, 0, 48, 64, 1);
    for (int y = 4; y < 64; y += 8) {
        int wob = (y / 8) & 1 ? 4 : 0;
        b.rect(wob, y, 20, 2, 2);
        b.rect(24 + wob, y + 3, 16, 2, 4);
    }
    b.rect(0, 0, 48, 3, 3);
    return b;
}

gs::Bitmap grassArt() {
    gs::Bitmap b(48, 72);
    b.rect(0, 8, 48, 64, 1);
    b.rect(0, 8, 48, 6, 2);
    for (int x = 2; x < 48; x += 6) {
        int h = 8 + (x * 3) % 7;
        b.line(float(x), 10, float(x + ((x / 6) & 1 ? 2 : -2)), float(10 - h), 3, 2);
        b.set(x, 14 + (x % 5), 4);
        b.set(x + 2, 22 + (x % 7), 5);
    }
    b.rect(0, 28, 48, 44, 1);
    for (int y = 30; y < 70; y += 6)
        for (int x = (y & 4) ? 2 : 6; x < 48; x += 10) b.set(x, y, 4);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(40, 80);
    b.rect(0, 0, 40, 80, 1);
    for (int y = 4; y < 78; y += 12) {
        int off = ((y / 12) & 1) ? 10 : 0;
        b.line(0, float(y), 40, float(y), 2, 1);
        b.line(float(off), float(y), float(off), float(y + 12), 2, 1);
        b.line(float(off + 20), float(y), float(off + 20), float(y + 12), 2, 1);
    }
    b.rect(0, 0, 6, 80, 3);
    return b;
}

gs::Bitmap sprayArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 4, 1);
    b.ellipse(5, 6, 2, 2, 2);
    b.ellipse(11, 9, 2, 2, 2);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(20, 14);
    b.rect(0, 0, 20, 14, 1);
    b.rect(0, 0, 20, 5, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 10, 8), gs::rgb4(15, 12, 4), gs::rgb4(15, 5, 3),
                          gs::rgb4(6, 14, 6), gs::rgb4(4, 8, 14), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOAT,
           {0, gs::rgb4(14, 12, 8), gs::rgb4(8, 6, 4), gs::rgb4(5, 4, 3), gs::rgb4(15, 15, 13), gs::rgb4(3, 7, 12),
            gs::rgb4(6, 4, 2), gs::rgb4(13, 3, 2), gs::rgb4(12, 10, 4), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WATER,
           {0, gs::rgb4(2, 6, 12), gs::rgb4(4, 9, 14), gs::rgb4(10, 14, 15), gs::rgb4(3, 8, 13), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_GRASS,
           {0, gs::rgb4(2, 8, 2), gs::rgb4(4, 12, 3), gs::rgb4(6, 14, 4), gs::rgb4(3, 9, 2), gs::rgb4(8, 11, 3), 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 4), gs::rgb4(5, 5, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(14, 15, 15), gs::rgb4(10, 13, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    art.boat = gs::uploadMipped(vdp, boatArt());
    art.keel = gs::uploadMipped(vdp, keelArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    loadFont(vdp, art);
}

}  // namespace keel
