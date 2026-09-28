#include "game/art.h"

namespace tower {
namespace {

void paintFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap towerArt() {
    gs::Bitmap b(120, 200);
    b.rect(18, 28, 84, 172, 4);
    b.rect(10, 18, 100, 16, 5);
    for (int i = 0; i < 5; i++) {
        b.rect(12 + i * 20, 4, 12, 16, 6);
        b.rect(14 + i * 20, 6, 8, 6, 2);
    }
    b.rect(28, 48, 16, 22, 1);
    b.rect(76, 48, 16, 22, 1);
    b.rect(46, 90, 28, 18, 1);
    b.rect(40, 128, 40, 62, 0);
    for (int y = 30; y < 196; y += 8) b.rect(18, y, 84, 1, 3);
    for (int x = 26; x < 100; x += 14) b.rect(x, 28, 1, 168, 3);
    b.rect(52, 8, 16, 22, 7);
    b.rect(56, 2, 8, 10, 8);
    return b;
}

gs::Bitmap doorArt(bool right) {
    gs::Bitmap b(28, 70);
    b.rect(2, 2, 24, 66, 4);
    for (int y = 8; y < 64; y += 10) b.rect(4, y, 20, 2, 2);
    b.rect(right ? 4 : 18, 30, 6, 6, 8);
    b.rect(6, 4, 16, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap guardArt() {
    gs::Bitmap b(24, 36);
    b.ellipse(12, 7, 5, 5, 6);
    b.rect(8, 5, 8, 3, 8);
    b.rect(7, 13, 10, 14, 4);
    b.rect(4, 14, 4, 10, 5);
    b.rect(16, 14, 4, 12, 9);
    b.rect(8, 26, 4, 8, 3);
    b.rect(13, 26, 4, 8, 3);
    b.rect(14, 8, 10, 2, 7);
    return b;
}

gs::Bitmap climberArt() {
    gs::Bitmap b(18, 28);
    b.ellipse(9, 5, 4, 4, 5);
    b.rect(6, 10, 7, 10, 3);
    b.rect(2, 12, 4, 8, 4);
    b.rect(13, 11, 4, 9, 2);
    b.rect(6, 19, 3, 7, 1);
    b.rect(10, 19, 3, 7, 1);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 3);
    b.ellipse(18, 12, 8, 8, 0);
    b.rect(6, 8, 2, 2, 1);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(22, 16);
    b.rect(2, 1, 2, 14, 2);
    b.poly({{4, 2}, {20, 6}, {4, 10}}, 5);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(48, 8);
    b.rect(0, 2, 48, 4, 3);
    b.rect(2, 1, 6, 6, 5);
    b.rect(40, 1, 6, 6, 5);
    return b;
}

void colors(gs::VDP& vdp) {
    auto put = [&](int pal, int i, int r, int g, int b) { vdp.setColor(pal * 16 + i, gs::rgb4(r, g, b)); };
    for (int p = 0; p < 8; p++) put(p, 0, 0, 0, 0);
    put(PAL_HUD, 1, 15, 14, 10);
    put(PAL_STONE, 1, 2, 3, 6);
    put(PAL_STONE, 2, 8, 10, 13);
    put(PAL_STONE, 3, 5, 6, 8);
    put(PAL_STONE, 4, 7, 7, 8);
    put(PAL_STONE, 5, 9, 9, 10);
    put(PAL_STONE, 6, 11, 11, 12);
    put(PAL_STONE, 7, 4, 4, 5);
    put(PAL_STONE, 8, 13, 4, 3);
    put(PAL_WOOD, 1, 4, 2, 1);
    put(PAL_WOOD, 2, 6, 3, 1);
    put(PAL_WOOD, 3, 5, 4, 2);
    put(PAL_WOOD, 4, 9, 6, 2);
    put(PAL_WOOD, 5, 7, 5, 2);
    put(PAL_WOOD, 6, 12, 9, 4);
    put(PAL_WOOD, 7, 14, 12, 6);
    put(PAL_WOOD, 8, 15, 13, 4);
    put(PAL_GUARD, 1, 3, 3, 4);
    put(PAL_GUARD, 2, 5, 5, 6);
    put(PAL_GUARD, 3, 6, 7, 9);
    put(PAL_GUARD, 4, 8, 9, 11);
    put(PAL_GUARD, 5, 4, 5, 7);
    put(PAL_GUARD, 6, 13, 10, 7);
    put(PAL_GUARD, 7, 14, 13, 8);
    put(PAL_GUARD, 8, 15, 14, 6);
    put(PAL_GUARD, 9, 10, 10, 12);
    put(PAL_FOE, 1, 3, 2, 2);
    put(PAL_FOE, 2, 10, 4, 3);
    put(PAL_FOE, 3, 6, 3, 3);
    put(PAL_FOE, 4, 8, 5, 4);
    put(PAL_FOE, 5, 12, 8, 6);
    put(PAL_MOON, 1, 10, 10, 8);
    put(PAL_MOON, 2, 6, 6, 7);
    put(PAL_MOON, 3, 14, 14, 11);
    put(PAL_MOON, 5, 12, 3, 3);
    put(PAL_GOLD, 1, 8, 6, 2);
    put(PAL_GOLD, 3, 14, 11, 3);
    put(PAL_GOLD, 5, 15, 13, 5);
    put(PAL_IRON, 2, 8, 8, 9);
    put(PAL_IRON, 3, 11, 11, 12);
    put(PAL_IRON, 5, 6, 6, 7);
    vdp.setFogColor(gs::rgb4(1, 2, 4));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    colors(vdp);
    paintFont(vdp, art);
    art.tower = gs::uploadImage(vdp, towerArt());
    art.doorL = gs::uploadImage(vdp, doorArt(false));
    art.doorR = gs::uploadImage(vdp, doorArt(true));
    art.guard = gs::uploadImage(vdp, guardArt());
    art.climber = gs::uploadImage(vdp, climberArt());
    art.moon = gs::uploadImage(vdp, moonArt());
    art.flag = gs::uploadImage(vdp, flagArt());
    art.bar = gs::uploadImage(vdp, barArt());
}

}  // namespace tower
