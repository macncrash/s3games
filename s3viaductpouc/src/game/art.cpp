#include "game/art.h"

namespace pouch {
namespace {

void stoneNoise(gs::Bitmap& b, int c0, int c1) {
    uint32_t n = 0x9E3779B9u;
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            n = n * 1664525u + 1013904223u;
            int p = b.get(x, y);
            if (!p) continue;
            b.set(x, y, (n >> 28) > 10 ? c1 : c0);
            if (((x / 4) + (y / 5)) % 7 == 0) b.set(x, y, 3);
            if (y % 5 == 0) b.set(x, y, 1);
        }
    }
}

gs::Bitmap runnerFrame(int step) {
    gs::Bitmap b(16, 28);
    b.ellipse(8, 6, 4.2f, 4.0f, 3);
    b.rect(5, 3, 7, 3, 4);
    b.rect(5, 10, 7, 9, 1);
    b.rect(6, 11, 5, 4, 2);
    b.rect(5, 18, 7, 2, 6);
    b.rect(step ? 4 : 6, 12, 3, 7, 1);
    int lx = step ? 5 : 7;
    int rx = step ? 9 : 7;
    b.rect(lx, 20, 3, 7, 5);
    b.rect(rx, 20, 3, 6, 5);
    b.rect(lx - 1, 26, 4, 2, 5);
    b.rect(rx - 1, 25, 4, 2, 5);
    b.set(6, 6, 7);
    b.set(10, 6, 7);
    return b;
}

gs::Bitmap pouchArt() {
    gs::Bitmap b(14, 12);
    b.rect(1, 2, 12, 9, 1);
    b.rect(2, 3, 10, 7, 2);
    b.rect(4, 1, 6, 3, 1);
    b.rect(6, 0, 2, 4, 3);
    b.rect(3, 6, 8, 1, 4);
    b.rect(5, 5, 4, 3, 3);
    return b;
}

gs::Bitmap deckArt() {
    gs::Bitmap b(32, 14);
    b.rect(0, 0, 32, 14, 2);
    stoneNoise(b, 2, 5);
    b.rect(0, 0, 32, 3, 5);
    b.rect(0, 11, 32, 3, 3);
    for (int x = 0; x < 32; x += 8) b.rect(x, 3, 1, 8, 3);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(18, 78);
    b.rect(2, 0, 14, 78, 2);
    stoneNoise(b, 2, 5);
    b.rect(0, 0, 18, 4, 5);
    // Arch opening punched by leaving a curve clear.
    for (int y = 8; y < 70; y++) {
        float ny = (y - 8) / 62.0f;
        int half = int(7.0f * (1.0f - ny * ny));
        for (int x = 9 - half; x <= 9 + half; x++) b.set(x, y, 0);
    }
    b.rect(4, 70, 10, 8, 3);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 9, 9, 1);
    b.ellipse(14, 9, 7, 7, 0);
    b.set(7, 8, 2);
    b.set(8, 14, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(14, 9, 10, 6, 1);
    b.ellipse(24, 8, 12, 6, 1);
    b.ellipse(30, 10, 7, 4, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 16);
    b.rect(4, 6, 2, 10, 3);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2.2f, 2);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(18, 14);
    b.rect(1, 0, 2, 14, 3);
    b.poly({{3, 1}, {16, 4}, {3, 8}}, 1);
    b.poly({{4, 3}, {12, 4}, {4, 6}}, 2);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(12, 6);
    b.line(0, 3, 5, 1, 1, 1);
    b.line(5, 1, 11, 4, 1, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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

void paintPals(gs::VDP& v) {
    for (int i = 0; i < gs::NUM_PALETTES * 16; i++) v.setColor(i, 0);
    auto put = [&](int pal, int i, int r, int g, int b) { v.setColor(pal * 16 + i, gs::rgb4(r, g, b)); };
    // 1 stone
    put(1, 1, 9, 9, 8);
    put(1, 2, 6, 6, 7);
    put(1, 3, 3, 3, 4);
    put(1, 4, 4, 6, 4);
    put(1, 5, 11, 11, 10);
    // 2 courier
    put(2, 1, 2, 3, 7);
    put(2, 2, 4, 6, 11);
    put(2, 3, 13, 9, 6);
    put(2, 4, 4, 2, 1);
    put(2, 5, 2, 2, 2);
    put(2, 6, 10, 8, 3);
    put(2, 7, 1, 1, 2);
    // 3 pouch
    put(3, 1, 8, 4, 1);
    put(3, 2, 4, 2, 1);
    put(3, 3, 14, 11, 4);
    put(3, 4, 13, 12, 9);
    // 4 night trim / flag
    put(4, 1, 10, 2, 2);
    put(4, 2, 14, 12, 6);
    put(4, 3, 5, 5, 6);
    // 5 moon, cloud, gull
    put(5, 1, 13, 13, 12);
    put(5, 2, 8, 8, 9);
    // 6 lamp
    put(6, 1, 15, 13, 5);
    put(6, 2, 15, 15, 12);
    put(6, 3, 4, 4, 5);
    // 7 HUD
    put(7, 1, 15, 14, 12);
    put(7, 15, 2, 2, 4);
    put(8, 1, 15, 6, 3);
    put(8, 15, 3, 1, 1);
    put(9, 1, 8, 14, 8);
    put(9, 15, 1, 3, 2);
    v.setFogColor(gs::rgb4(2, 3, 6));
}

}  // namespace

void makeArt(gs::VDP& vdp, Art& art) {
    paintPals(vdp);
    loadFont(vdp, art);
    art.runner[0] = gs::uploadImage(vdp, runnerFrame(0));
    art.runner[1] = gs::uploadImage(vdp, runnerFrame(1));
    art.pouch = gs::uploadImage(vdp, pouchArt());
    art.deck = gs::uploadImage(vdp, deckArt());
    art.pier = gs::uploadImage(vdp, pierArt());
    art.moon = gs::uploadImage(vdp, moonArt());
    art.cloud = gs::uploadImage(vdp, cloudArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
    art.flag = gs::uploadImage(vdp, flagArt());
    art.gull = gs::uploadImage(vdp, gullArt());
}

}  // namespace pouch
