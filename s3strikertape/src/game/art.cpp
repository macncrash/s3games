#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <initializer_list>

namespace strikertape {
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

gs::Bitmap towerBmp() {
    gs::Bitmap b(36, 168);
    b.rect(0, 0, 36, 168, 1);
    b.rect(4, 8, 28, 152, 2);
    b.rect(2, 4, 32, 6, 3);
    b.rect(14, 0, 8, 8, 3);
    for (int i = 0; i < kBandN; i++) {
        float mid = (kBand[i].p0 + kBand[i].p1) * 0.5f;
        int y = int((1.f - mid) * 140.f) + 12;
        int ink = 4 + i;
        b.rect(6, y - 6, 24, 12, ink);
        b.rect(6, y - 1, 24, 2, 8);
    }
    b.rect(8, 148, 20, 8, 9);
    return b;
}

gs::Bitmap puckBmp() {
    gs::Bitmap b(16, 10);
    b.rect(1, 1, 14, 8, 1);
    b.rect(2, 2, 12, 6, 2);
    b.rect(3, 3, 6, 2, 3);
    return b;
}

gs::Bitmap bellBmp() {
    gs::Bitmap b(28, 24);
    disc(b, 14.f, 10.f, 10.f, 1);
    b.rect(4, 10, 20, 8, 2);
    b.rect(10, 16, 8, 4, 3);
    disc(b, 14.f, 21.f, 2.4f, 4);
    disc(b, 10.f, 8.f, 2.f, 5);
    return b;
}

gs::Bitmap malletBmp() {
    gs::Bitmap b(40, 14);
    b.rect(0, 4, 28, 4, 1);
    b.rect(24, 1, 14, 12, 2);
    b.rect(26, 2, 10, 3, 3);
    return b;
}

gs::Bitmap manBmp() {
    gs::Bitmap b(20, 40);
    disc(b, 10.f, 6.f, 5.f, 1);
    b.rect(6, 11, 8, 12, 2);
    b.rect(3, 12, 3, 8, 3);
    b.rect(14, 12, 3, 8, 3);
    b.rect(6, 22, 3, 12, 4);
    b.rect(11, 22, 3, 12, 4);
    b.rect(5, 33, 5, 3, 5);
    b.rect(11, 33, 5, 3, 5);
    return b;
}

gs::Bitmap slipBmp() {
    gs::Bitmap b(22, 14);
    b.rect(0, 0, 22, 14, 1);
    b.rect(1, 1, 20, 12, 2);
    b.rect(3, 4, 12, 1, 3);
    b.rect(3, 7, 16, 1, 4);
    b.rect(3, 10, 9, 1, 4);
    return b;
}

gs::Bitmap slotBmp() {
    gs::Bitmap b(28, 12);
    b.rect(0, 0, 28, 12, 1);
    b.rect(1, 1, 26, 10, 2);
    b.rect(2, 8, 24, 2, 3);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(10, 14);
    b.rect(4, 0, 2, 4, 1);
    disc(b, 5.f, 9.f, 4.f, 2);
    disc(b, 5.f, 9.f, 2.f, 3);
    return b;
}

void tileSolid(uint8_t* px, int c, int speckle, int salt) {
    for (int i = 0; i < 64; i++) px[i] = uint8_t(c);
    px[salt % 64] = uint8_t(speckle);
    px[(salt * 3 + 11) % 64] = uint8_t(speckle);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 14, 12), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_WOOD, {gs::rgb4(0, 0, 0), gs::rgb4(6, 3, 1), gs::rgb4(3, 2, 1), gs::rgb4(12, 9, 3),
                           gs::rgb4(14, 12, 4), gs::rgb4(15, 11, 3), gs::rgb4(8, 10, 12), gs::rgb4(12, 4, 3),
                           gs::rgb4(15, 14, 8), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_BRASS, {gs::rgb4(0, 0, 0), gs::rgb4(10, 7, 2), gs::rgb4(14, 11, 3), gs::rgb4(6, 4, 1),
                            gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_PUCK, {gs::rgb4(0, 0, 0), gs::rgb4(8, 1, 1), gs::rgb4(14, 2, 2), gs::rgb4(15, 8, 6)});
    setPal(vdp, PAL_MAN, {gs::rgb4(0, 0, 0), gs::rgb4(13, 9, 6), gs::rgb4(2, 3, 8), gs::rgb4(12, 8, 4),
                          gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BELL, {gs::rgb4(0, 0, 0), gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_GOLD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 12, 2), gs::rgb4(8, 5, 1)});
    setPal(vdp, PAL_DING, {gs::rgb4(0, 0, 0), gs::rgb4(12, 12, 14)});
    setPal(vdp, PAL_TIN, {gs::rgb4(0, 0, 0), gs::rgb4(6, 10, 8)});
    setPal(vdp, PAL_PAPER, {gs::rgb4(0, 0, 0), gs::rgb4(8, 6, 3), gs::rgb4(14, 12, 8), gs::rgb4(10, 3, 2),
                            gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_RED, {gs::rgb4(0, 0, 0), gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_NIGHT, {gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 4), gs::rgb4(3, 2, 1), gs::rgb4(15, 13, 6),
                            gs::rgb4(8, 6, 2)});

    art.tower = gs::uploadMipped(vdp, towerBmp());
    art.puck = gs::uploadMipped(vdp, puckBmp());
    art.bell = gs::uploadMipped(vdp, bellBmp());
    art.mallet = gs::uploadMipped(vdp, malletBmp());
    art.man = gs::uploadMipped(vdp, manBmp());
    art.slip = gs::uploadMipped(vdp, slipBmp());
    art.slot = gs::uploadMipped(vdp, slotBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, art, tiles);
    uint8_t sky[64], dirt[64];
    tileSolid(sky, 1, 2, 3);
    tileSolid(dirt, 2, 3, 9);
    art.sky = tiles.shared(sky);
    art.dirt = tiles.shared(dirt);
}

}  // namespace strikertape
