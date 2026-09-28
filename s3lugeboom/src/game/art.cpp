#include "game/art.h"

namespace luge {
namespace {

gs::Bitmap shiftX(const gs::Bitmap& src, int dx) {
    gs::Bitmap b(src.w, src.h);
    for (int y = 0; y < src.h; y++) {
        const int push = y < 34 ? dx : dx / 3;
        for (int x = 0; x < src.w; x++) {
            const int c = src.get(x - push, y);
            if (c) b.set(x, y, c);
        }
    }
    return b;
}

gs::Bitmap lugeArt() {
    gs::Bitmap b(48, 64);
    b.ellipse(24, 50, 16, 5, 1);
    b.rect(10, 46, 28, 4, 2);
    b.line(8, 50, 6, 58, 2, 2.f);
    b.line(40, 50, 42, 58, 2, 2.f);
    b.rect(16, 40, 16, 10, 8);
    b.rect(18, 42, 12, 6, 7);
    b.set(28, 44, 9);
    b.ellipse(24, 28, 7, 9, 3);
    b.ellipse(24, 16, 6, 6, 4);
    b.rect(20, 16, 8, 3, 5);
    b.rect(21, 32, 6, 8, 6);
    b.line(18, 30, 12, 40, 3, 2.f);
    b.line(30, 30, 36, 40, 3, 2.f);
    return b;
}

gs::Bitmap boomArt() {
    gs::Bitmap b(72, 88);
    b.rect(8, 10, 6, 70, 2);
    b.rect(10, 12, 2, 66, 3);
    b.rect(8, 8, 56, 6, 2);
    for (int i = 0; i < 8; i++) b.rect(14 + i * 6, 9, 3, 4, i & 1 ? 5 : 1);
    b.line(14, 14, 40, 36, 4, 1.4f);
    b.line(40, 36, 40, 58, 4, 1.6f);
    b.rect(30, 58, 20, 8, 6);
    b.rect(32, 60, 16, 4, 7);
    b.rect(36, 54, 8, 4, 2);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(32, 48);
    b.poly({{16, 2}, {28, 22}, {4, 22}}, 1);
    b.poly({{16, 12}, {30, 34}, {2, 34}}, 2);
    b.poly({{16, 22}, {30, 42}, {2, 42}}, 1);
    b.rect(14, 38, 4, 8, 3);
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(28, 18);
    b.ellipse(14, 11, 12, 6, 1);
    b.ellipse(10, 9, 5, 3, 2);
    b.ellipse(18, 10, 4, 2, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(48, 12);
    b.ellipse(24, 6, 18, 4, 1);
    return b;
}

gs::Bitmap flakeArt() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(2, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    return b;
}

void pal(gs::VDP& v, int p, const uint16_t* c, int n) {
    for (int i = 0; i < n; i++) v.setColor(p * 16 + i, c[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    const uint16_t hud[] = {
        gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 15), gs::rgb4(15, 12, 4), gs::rgb4(14, 3, 3),
        gs::rgb4(6, 14, 15), gs::rgb4(8, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)};
    pal(vdp, PAL_HUD, hud, 16);
    const uint16_t ink[] = {0, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 4)};
    pal(vdp, PAL_INK, ink, 16);
    const uint16_t amber[] = {0, gs::rgb4(15, 11, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 0)};
    pal(vdp, PAL_AMBER, amber, 16);
    const uint16_t fx[] = {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    pal(vdp, PAL_FX, fx, 16);
    const uint16_t rider[] = {
        0, gs::rgb4(2, 3, 5), gs::rgb4(8, 10, 12), gs::rgb4(12, 2, 2), gs::rgb4(14, 14, 15),
        gs::rgb4(2, 6, 12), gs::rgb4(12, 8, 6), gs::rgb4(15, 9, 2), gs::rgb4(8, 4, 1), gs::rgb4(15, 14, 6)};
    pal(vdp, PAL_RIDER, rider, 10);
    const uint16_t rival[] = {
        0, gs::rgb4(2, 2, 4), gs::rgb4(6, 7, 9), gs::rgb4(3, 4, 8), gs::rgb4(10, 11, 13),
        gs::rgb4(4, 8, 10), gs::rgb4(8, 7, 6), gs::rgb4(5, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(9, 9, 8)};
    pal(vdp, PAL_RIVAL, rival, 10);
    const uint16_t boom[] = {
        0, gs::rgb4(1, 1, 1), gs::rgb4(6, 7, 8), gs::rgb4(11, 12, 13), gs::rgb4(4, 5, 6),
        gs::rgb4(15, 12, 2), gs::rgb4(10, 6, 2), gs::rgb4(15, 8, 1)};
    pal(vdp, PAL_BOOM, boom, 8);
    const uint16_t tree[] = {0, gs::rgb4(1, 6, 3), gs::rgb4(2, 9, 4), gs::rgb4(5, 3, 1)};
    pal(vdp, PAL_TREE, tree, 4);
    const uint16_t rock[] = {0, gs::rgb4(6, 6, 7), gs::rgb4(9, 9, 10), gs::rgb4(4, 4, 5)};
    pal(vdp, PAL_ROCK, rock, 4);
    const uint16_t road[] = {
        0, gs::rgb4(14, 14, 15), gs::rgb4(11, 12, 14), gs::rgb4(8, 9, 11), gs::rgb4(9, 11, 13),
        gs::rgb4(6, 8, 11), gs::rgb4(12, 14, 15), gs::rgb4(8, 12, 14), gs::rgb4(5, 6, 7),
        gs::rgb4(10, 12, 14), gs::rgb4(13, 14, 15), gs::rgb4(7, 10, 13), gs::rgb4(6, 9, 12),
        gs::rgb4(4, 8, 12), gs::rgb4(13, 15, 15), gs::rgb4(15, 15, 15)};
    pal(vdp, PAL_ROAD, road, 16);

    const gs::Bitmap sled = lugeArt();
    a.luge[1] = gs::uploadMipped(vdp, sled);
    a.luge[0] = gs::uploadMipped(vdp, shiftX(sled, -4));
    a.luge[2] = gs::uploadMipped(vdp, shiftX(sled, 4));
    a.rival = a.luge[1];
    a.boom = gs::uploadMipped(vdp, boomArt());
    a.tree = gs::uploadMipped(vdp, treeArt());
    a.rock = gs::uploadMipped(vdp, rockArt());
    a.shadow = gs::uploadMipped(vdp, shadowArt());
    a.flake = gs::uploadMipped(vdp, flakeArt());

    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        const int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
    vdp.setFogColor(gs::rgb4(10, 12, 14));
}

}  // namespace luge
