#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace boccetape {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
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
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

void disc(gs::Bitmap& b, float cx, float cy, float r, int c) {
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
            if (dx * dx + dy * dy <= r * r) b.set(x, y, c);
        }
    }
}

gs::Bitmap bowlBmp() {
    gs::Bitmap b(24, 24);
    disc(b, 12.f, 12.f, 11.f, 1);
    disc(b, 12.f, 12.f, 9.2f, 2);
    disc(b, 12.f, 13.2f, 7.4f, 3);
    disc(b, 9.2f, 8.6f, 2.4f, 4);
    disc(b, 12.f, 12.f, 2.1f, 5);
    return b;
}

gs::Bitmap pallinoBmp() {
    gs::Bitmap b(16, 16);
    disc(b, 8.f, 8.f, 6.4f, 1);
    disc(b, 8.f, 8.f, 5.f, 2);
    disc(b, 6.4f, 6.2f, 1.6f, 3);
    return b;
}

gs::Bitmap ringBmp() {
    gs::Bitmap b(32, 32);
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 32; x++) {
            float dx = x + 0.5f - 16.f, dy = y + 0.5f - 16.f;
            float d = std::sqrt(dx * dx + dy * dy);
            if (d > 14.2f && d < 15.6f) b.set(x, y, 1);
            else if (d > 11.4f && d < 12.4f) b.set(x, y, 2);
        }
    }
    b.set(16, 4, 3);
    b.set(16, 27, 3);
    return b;
}

gs::Bitmap slipBmp() {
    gs::Bitmap b(22, 14);
    b.rect(0, 0, 22, 14, 1);
    b.rect(1, 1, 20, 12, 2);
    b.rect(2, 3, 12, 1, 3);
    b.rect(2, 6, 16, 1, 4);
    b.rect(2, 9, 10, 1, 4);
    return b;
}

gs::Bitmap slotBmp() {
    gs::Bitmap b(26, 10);
    b.rect(0, 0, 26, 10, 1);
    b.rect(1, 1, 24, 8, 2);
    b.rect(2, 6, 22, 2, 3);
    return b;
}

gs::Bitmap shadowBmp() {
    gs::Bitmap b(18, 8);
    disc(b, 9.f, 4.f, 8.f, 1);
    return b;
}

gs::Bitmap treeBmp() {
    gs::Bitmap b(18, 40);
    b.rect(7, 22, 4, 16, 1);
    b.rect(8, 22, 2, 14, 2);
    disc(b, 9.f, 14.f, 8.f, 3);
    disc(b, 7.f, 12.f, 5.f, 4);
    disc(b, 11.f, 16.f, 3.5f, 5);
    return b;
}

gs::Bitmap pineBmp() {
    gs::Bitmap b(28, 22);
    b.rect(13, 14, 3, 8, 1);
    for (int y = 0; y < 16; y++) {
        int w = 4 + y;
        b.rect(14 - w / 2, y, w, 1, (y & 2) ? 3 : 2);
    }
    return b;
}

gs::Bitmap playerBmp() {
    gs::Bitmap b(16, 28);
    disc(b, 8.f, 5.f, 4.f, 1);
    b.rect(5, 9, 6, 8, 2);
    b.rect(4, 10, 2, 6, 3);
    b.rect(10, 10, 2, 6, 3);
    b.rect(5, 17, 2, 9, 4);
    b.rect(9, 17, 2, 9, 4);
    b.rect(4, 25, 4, 2, 5);
    b.rect(8, 25, 4, 2, 5);
    return b;
}

void tileSolid(uint8_t* px, int c, int speckle, int salt) {
    for (int i = 0; i < 64; i++) px[i] = uint8_t(c);
    px[salt % 64] = uint8_t(speckle);
    px[(salt * 3 + 11) % 64] = uint8_t(speckle);
    px[(salt * 5 + 27) % 64] = uint8_t(c == speckle ? c : speckle);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 12), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_PAPER, {gs::rgb4(0, 0, 0), gs::rgb4(6, 4, 2), gs::rgb4(14, 12, 8), gs::rgb4(10, 3, 2),
                            gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_RED, {gs::rgb4(0, 0, 0), gs::rgb4(6, 1, 1), gs::rgb4(12, 2, 2), gs::rgb4(15, 5, 3),
                          gs::rgb4(15, 12, 10), gs::rgb4(8, 6, 4)});
    setPal(vdp, PAL_PALE, {gs::rgb4(0, 0, 0), gs::rgb4(10, 9, 6), gs::rgb4(15, 14, 10), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_GREEN, {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 8), gs::rgb4(12, 10, 4), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_WOOD, {gs::rgb4(0, 0, 0), gs::rgb4(6, 3, 1), gs::rgb4(10, 6, 2), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_DIRT, {gs::rgb4(0, 0, 0), gs::rgb4(10, 8, 4), gs::rgb4(8, 6, 3), gs::rgb4(12, 10, 6)});
    setPal(vdp, PAL_TREE, {gs::rgb4(0, 0, 0), gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(1, 6, 2),
                           gs::rgb4(3, 9, 3), gs::rgb4(1, 4, 1)});
    setPal(vdp, PAL_AIM, {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 12), gs::rgb4(15, 8, 2)});
    setPal(vdp, PAL_GOLD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 12, 3), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_PLAYER, {gs::rgb4(0, 0, 0), gs::rgb4(13, 9, 6), gs::rgb4(2, 4, 9), gs::rgb4(12, 10, 8),
                             gs::rgb4(1, 2, 5), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_SLOT, {gs::rgb4(0, 0, 0), gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 1), gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_SHADE, {gs::rgb4(0, 0, 0), gs::rgb4(0, 0, 0)});
    setPal(vdp, PAL_SKY, {gs::rgb4(0, 0, 0), gs::rgb4(8, 12, 15)});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);

    uint8_t px[64];
    tileSolid(px, 4, 3, 3);
    art.grass = tiles.alloc(1);
    vdp.loadTile(art.grass, px);
    tileSolid(px, 1, 3, 9);
    art.dirt = tiles.alloc(1);
    vdp.loadTile(art.dirt, px);
    tileSolid(px, 2, 3, 5);
    art.rail = tiles.alloc(1);
    vdp.loadTile(art.rail, px);

    art.bowl = gs::uploadMipped(vdp, bowlBmp());
    art.pallino = gs::uploadMipped(vdp, pallinoBmp());
    art.ring = gs::uploadMipped(vdp, ringBmp());
    art.slip = gs::uploadMipped(vdp, slipBmp());
    art.slot = gs::uploadMipped(vdp, slotBmp());
    art.shadow = gs::uploadMipped(vdp, shadowBmp());
    art.tree = gs::uploadMipped(vdp, treeBmp());
    art.pine = gs::uploadMipped(vdp, pineBmp());
    art.player = gs::uploadMipped(vdp, playerBmp());
}

}  // namespace boccetape
