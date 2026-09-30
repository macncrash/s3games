#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace flutechime {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void ink(gs::VDP& vdp, int pal, uint16_t main, uint16_t shadow) {
    setPal(vdp, pal, {0, main});
    vdp.setColor(pal * 16 + 15, shadow);
}

gs::Bitmap playerArt() {
    gs::Bitmap b(40, 56);
    b.ellipse(20.f, 8.f, 6.5f, 7.f, 3);
    b.ellipse(18.f, 7.f, 2.f, 2.f, 4);
    b.rect(14.f, 14.f, 12.f, 3.f, 5);
    b.rect(12.f, 17.f, 16.f, 16.f, 1);
    b.rect(14.f, 19.f, 12.f, 4.f, 2);
    b.line(12.f, 20.f, 4.f, 28.f, 3, 2.6f);
    b.line(28.f, 20.f, 36.f, 16.f, 3, 2.4f);
    b.rect(14.f, 33.f, 5.f, 14.f, 6);
    b.rect(21.f, 33.f, 5.f, 14.f, 6);
    b.rect(12.f, 46.f, 8.f, 4.f, 7);
    b.rect(20.f, 46.f, 8.f, 4.f, 7);
    return b;
}

gs::Bitmap fluteArt() {
    gs::Bitmap b(64, 12);
    b.rect(4.f, 4.f, 54.f, 4.f, 1);
    b.rect(4.f, 4.f, 54.f, 1.f, 2);
    b.ellipse(6.f, 6.f, 3.2f, 3.2f, 3);
    for (int i = 0; i < 7; i++) b.ellipse(16.f + float(i) * 6.f, 6.f, 1.2f, 1.2f, 4);
    b.rect(56.f, 5.f, 5.f, 2.f, 2);
    return b;
}

gs::Bitmap standArt() {
    gs::Bitmap b(36, 40);
    b.poly({{3, 4}, {33, 4}, {29, 16}, {7, 16}}, 1);
    b.rect(6.f, 6.f, 24.f, 8.f, 2);
    b.rect(16.f, 16.f, 4.f, 14.f, 3);
    b.line(18.f, 28.f, 6.f, 38.f, 3, 2.f);
    b.line(18.f, 28.f, 30.f, 38.f, 3, 2.f);
    return b;
}

gs::Bitmap staffArt() {
    gs::Bitmap b(248, 28);
    for (int i = 0; i < 5; i++) b.rect(2.f, 4.f + float(i) * 4.f, 244.f, 1.f, 1);
    b.rect(6.f, 4.f, 2.f, 17.f, 2);
    b.rect(238.f, 4.f, 2.f, 17.f, 2);
    return b;
}

gs::Bitmap noteArt() {
    gs::Bitmap b(12, 20);
    b.ellipse(4.5f, 14.f, 3.6f, 2.6f, 1);
    b.rect(7.f, 3.f, 2.f, 12.f, 1);
    b.rect(7.f, 3.f, 4.f, 2.f, 1);
    return b;
}

gs::Bitmap noteOnArt() {
    gs::Bitmap b(12, 20);
    b.ellipse(4.5f, 14.f, 3.8f, 2.8f, 2);
    b.ellipse(4.5f, 14.f, 1.6f, 1.1f, 1);
    b.rect(7.f, 2.f, 2.f, 13.f, 1);
    b.rect(7.f, 2.f, 4.f, 2.f, 2);
    return b;
}

gs::Bitmap breathArt() {
    gs::Bitmap b(10, 16);
    b.poly({{5, 1}, {9, 8}, {5, 15}, {1, 8}}, 1);
    b.poly({{5, 4}, {7, 8}, {5, 12}, {3, 8}}, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 36);
    b.poly({{2, 6}, {12, 6}, {10, 13}, {4, 13}}, 1);
    b.rect(6, 13, 2, 14, 2);
    b.rect(2, 27, 10, 4, 3);
    b.ellipse(7.f, 4.f, 2.6f, 2.f, 4);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(44, 44);
    b.ellipse(22.f, 22.f, 21.f, 21.f, 1);
    b.ellipse(22.f, 22.f, 17.f, 17.f, 2);
    b.ellipse(22.f, 22.f, 2.f, 2.f, 3);
    for (int i = 0; i < 12; i++) {
        float a = float(i) * 0.5236f - 1.5708f;
        int x = int(22.f + std::cos(a) * 14.f);
        int y = int(22.f + std::sin(a) * 14.f);
        b.rect(float(x), float(y), 2.f, 2.f, i % 3 == 0 ? 4 : 3);
    }
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(3, 12);
    b.rect(1.f, 0.f, 1.f, 12.f, 1);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(18, 22);
    b.poly({{9, 1}, {16, 14}, {2, 14}}, 1);
    b.rect(2.f, 14.f, 14.f, 3.f, 2);
    b.ellipse(9.f, 19.f, 2.2f, 2.2f, 3);
    b.rect(8.f, 0.f, 2.f, 3.f, 4);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    ink(vdp, PAL_HUD, gs::rgb4(15, 15, 14), gs::rgb4(1, 2, 4));
    ink(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(4, 3, 0));
    ink(vdp, PAL_DIM, gs::rgb4(9, 9, 11), gs::rgb4(1, 1, 3));
    ink(vdp, PAL_BAD, gs::rgb4(15, 4, 4), gs::rgb4(4, 0, 1));
    ink(vdp, PAL_CREAM, gs::rgb4(15, 14, 11), gs::rgb4(5, 4, 3));
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(10, 6, 2), gs::rgb4(14, 10, 5), gs::rgb4(6, 3, 1), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(3, 5, 11), gs::rgb4(7, 10, 15), gs::rgb4(13, 9, 6), gs::rgb4(15, 13, 11), gs::rgb4(4, 2, 2),
            gs::rgb4(2, 2, 6), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(2, 3, 7), gs::rgb4(8, 10, 14), gs::rgb4(13, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            gs::rgb4(1, 1, 3)});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(15, 12, 5), gs::rgb4(7, 6, 5), gs::rgb4(3, 3, 3), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(8, 6, 2), gs::rgb4(14, 12, 8), gs::rgb4(3, 2, 1), gs::rgb4(15, 14, 6), gs::rgb4(10, 8, 4), 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 0)});

    art.player = gs::uploadMipped(vdp, playerArt());
    art.flute = gs::uploadMipped(vdp, fluteArt());
    art.stand = gs::uploadMipped(vdp, standArt());
    art.staff = gs::uploadMipped(vdp, staffArt());
    art.note = gs::uploadMipped(vdp, noteArt());
    art.noteOn = gs::uploadMipped(vdp, noteOnArt());
    art.breath = gs::uploadMipped(vdp, breathArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.hand = gs::uploadMipped(vdp, handArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    loadFont(vdp, art);
}

}  // namespace flutechime
