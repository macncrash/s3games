#include "game/art.h"

#include <cstdint>

namespace culvertdoor {
namespace {

void putPal(gs::VDP& v, int pal, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) v.setColor(pal * 16 + i, i < n ? c[i] : 0);
}

void loadFont(gs::VDP& v, Art& art) {
    gs::TileAlloc tiles(v, 1);
    for (int ch = 0; ch < 96; ch++) {
        const uint8_t* g = gs::glyph(char(32 + ch));
        uint8_t px[64] = {};
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (g[y * 5 + x]) px[y * 8 + (x + 1)] = 1;
            }
        }
        art.font[ch] = tiles.shared(px);
    }
}

// Looking out the pipe. The leaf sprite covers the night slit until a surge walks it.
void paintPipe(gs::VDP& vdp) {
    gs::Bitmap b(320, 224);
    for (int y = 0; y < 224; y++) {
        for (int x = 0; x < 320; x++) {
            float nx = (float(x) - 160.f) / 148.f;
            float ny = (float(y) - 112.f) / 100.f;
            float r2 = nx * nx + ny * ny;
            int c = 1;
            if (r2 < 1.f) {
                int ring = int((1.f - r2) * 9.f);
                c = (ring & 1) ? 3 : 2;
                if (((x / 8 + ring) % 7) == 0) c = 4;
                if (r2 > 0.86f) c = 5;
                if (y > 168 && r2 < 0.92f) c = 6;
            }
            b.set(x, y, c);
        }
    }
    // Mouth of the pipe: night and a strip of road beyond the leaf.
    b.rect(118, 46, 84, 132, 8);
    b.rect(126, 150, 68, 18, 9);
    for (int i = 0; i < 5; i++) b.rect(130 + i * 12, 156, 4, 8, 10);
    // Seep stains and a lamp bolted to the crown.
    for (int y = 20; y < 70; y += 3) b.set(160 + (y % 5) - 2, y, 7);
    b.ellipse(160, 28, 10, 5, 11);
    b.rect(158, 32, 4, 10, 11);
    gs::TileAlloc tiles(vdp, 200);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, b, PAL_PIPE);
    vdp.B.enabled = true;
    vdp.A.enabled = false;
    vdp.A.clear();
}

gs::Bitmap leafBmp() {
    gs::Bitmap b(88, 136);
    b.rect(2, 2, 84, 132, 1);
    b.rect(6, 6, 76, 124, 2);
    for (int y = 10; y < 126; y += 16) b.rect(8, y, 72, 3, 3);
    b.line(14, 18, 74, 118, 4, 4.f);
    b.line(74, 18, 14, 118, 4, 3.f);
    b.ellipse(44, 70, 14, 14, 5);
    b.ellipse(44, 70, 7, 7, 6);
    b.rect(40, 40, 8, 60, 5);
    for (int i = 0; i < 6; i++) {
        b.ellipse(10, 16 + i * 20, 3, 3, 6);
        b.ellipse(78, 16 + i * 20, 3, 3, 6);
    }
    b.rect(0, 0, 88, 4, 3);
    b.rect(0, 132, 88, 4, 3);
    return b;
}

gs::Bitmap shoulderBmp() {
    gs::Bitmap b(40, 28);
    b.ellipse(12, 14, 10, 10, 1);
    b.rect(14, 8, 22, 12, 2);
    b.rect(28, 6, 10, 8, 3);
    b.rect(8, 18, 16, 6, 4);
    return b;
}

gs::Bitmap figureBmp() {
    gs::Bitmap b(20, 36);
    b.ellipse(10, 6, 4, 4, 1);
    b.rect(6, 10, 8, 14, 2);
    b.rect(3, 12, 4, 10, 3);
    b.rect(13, 12, 4, 10, 3);
    b.rect(6, 24, 3, 10, 4);
    b.rect(11, 24, 3, 10, 4);
    return b;
}

gs::Bitmap chevBmp() {
    gs::Bitmap b(18, 16);
    b.line(2, 2, 14, 8, 1, 2.f);
    b.line(2, 14, 14, 8, 1, 2.f);
    b.line(6, 2, 16, 8, 2, 1.5f);
    b.line(6, 14, 16, 8, 2, 1.5f);
    return b;
}

gs::Bitmap sheetBmp() {
    gs::Bitmap b(160, 16);
    b.rect(0, 4, 160, 12, 1);
    b.rect(0, 0, 160, 4, 2);
    for (int x = 0; x < 160; x += 9) b.set(x, 2, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 13, 9), gs::rgb4(6, 5, 4)};
    const uint16_t pipe[] = {0,
                             gs::rgb4(1, 1, 2),
                             gs::rgb4(4, 4, 4),
                             gs::rgb4(6, 6, 5),
                             gs::rgb4(8, 8, 7),
                             gs::rgb4(3, 3, 3),
                             gs::rgb4(3, 4, 4),
                             gs::rgb4(2, 5, 3),
                             gs::rgb4(1, 2, 4),
                             gs::rgb4(3, 3, 2),
                             gs::rgb4(8, 8, 6),
                             gs::rgb4(12, 10, 4)};
    const uint16_t door[] = {0, gs::rgb4(5, 6, 6), gs::rgb4(7, 8, 8), gs::rgb4(9, 9, 8), gs::rgb4(3, 4, 4),
                             gs::rgb4(10, 7, 3), gs::rgb4(14, 12, 6)};
    const uint16_t body[] = {0, gs::rgb4(10, 8, 6), gs::rgb4(6, 6, 5), gs::rgb4(4, 4, 3), gs::rgb4(3, 3, 4)};
    const uint16_t water[] = {0, gs::rgb4(2, 6, 8), gs::rgb4(6, 11, 12), gs::rgb4(10, 14, 14)};
    const uint16_t warn[] = {0, gs::rgb4(15, 8, 3), gs::rgb4(15, 14, 6)};
    const uint16_t fig[] = {0, gs::rgb4(11, 8, 6), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2)};
    putPal(vdp, PAL_HUD, hud, 3);
    putPal(vdp, PAL_PIPE, pipe, 12);
    putPal(vdp, PAL_DOOR, door, 7);
    putPal(vdp, PAL_BODY, body, 5);
    putPal(vdp, PAL_WATER, water, 4);
    putPal(vdp, PAL_WARN, warn, 3);
    putPal(vdp, PAL_FIG, fig, 5);
    vdp.setFogColor(gs::rgb4(1, 2, 3));
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.lineBackdrop[y] = gs::rgb4(1, 1, 2);

    paintPipe(vdp);
    art.leaf = gs::uploadMipped(vdp, leafBmp());
    art.shoulder = gs::uploadMipped(vdp, shoulderBmp());
    art.figure = gs::uploadMipped(vdp, figureBmp());
    art.chev = gs::uploadMipped(vdp, chevBmp());
    art.sheet = gs::uploadMipped(vdp, sheetBmp());
    loadFont(vdp, art);
}

}  // namespace culvertdoor
