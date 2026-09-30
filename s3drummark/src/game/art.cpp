#include "game/art.h"

#include <initializer_list>

namespace drummark {
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

gs::Bitmap drumArt() {
    gs::Bitmap b(72, 40);
    b.ellipse(36.f, 14.f, 28.f, 10.f, 4);
    b.ellipse(36.f, 13.f, 20.f, 6.f, 5);
    b.rect(8.f, 14.f, 56.f, 18.f, 2);
    b.rect(10.f, 16.f, 52.f, 4.f, 1);
    b.rect(8.f, 14.f, 56.f, 3.f, 3);
    b.rect(8.f, 28.f, 56.f, 3.f, 3);
    b.rect(12.f, 32.f, 48.f, 4.f, 6);
    b.ellipse(20.f, 34.f, 3.f, 2.f, 3);
    b.ellipse(52.f, 34.f, 3.f, 2.f, 3);
    return b;
}

gs::Bitmap stickUpArt() {
    gs::Bitmap b(48, 28);
    b.line(6.f, 24.f, 40.f, 6.f, 1, 2.4f);
    b.ellipse(42.f, 5.f, 3.2f, 3.2f, 2);
    return b;
}

gs::Bitmap stickDownArt() {
    gs::Bitmap b(48, 28);
    b.line(8.f, 6.f, 38.f, 22.f, 1, 2.4f);
    b.ellipse(40.f, 23.f, 3.4f, 2.6f, 2);
    return b;
}

gs::Bitmap playerArt() {
    gs::Bitmap b(36, 58);
    b.ellipse(18.f, 8.f, 6.f, 6.5f, 3);
    b.ellipse(16.f, 7.f, 2.f, 2.f, 4);
    b.rect(12.f, 13.f, 12.f, 3.f, 5);
    b.rect(10.f, 16.f, 16.f, 18.f, 1);
    b.rect(12.f, 18.f, 12.f, 4.f, 2);
    b.line(10.f, 18.f, 2.f, 30.f, 3, 3.f);
    b.line(26.f, 18.f, 33.f, 12.f, 3, 3.f);
    b.rect(12.f, 34.f, 6.f, 16.f, 6);
    b.rect(19.f, 34.f, 6.f, 16.f, 6);
    b.rect(10.f, 48.f, 8.f, 4.f, 7);
    b.rect(19.f, 48.f, 8.f, 4.f, 7);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(18, 48);
    b.poly({{3, 10}, {15, 10}, {12, 18}, {6, 18}}, 1);
    b.rect(8, 18, 2, 22, 2);
    b.rect(4, 38, 10, 6, 3);
    b.ellipse(9.f, 8.f, 3.f, 2.f, 4);
    return b;
}

gs::Bitmap curtainArt() {
    gs::Bitmap b(28, 90);
    b.rect(0, 0, 28, 90, 1);
    b.rect(2, 0, 6, 90, 2);
    b.rect(16, 0, 6, 90, 3);
    for (int y = 8; y < 88; y += 12) b.rect(8, y, 8, 4, 2);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(12, 12);
    b.rect(2, 2, 8, 8, 1);
    b.rect(4, 4, 4, 4, 2);
    return b;
}

gs::Bitmap pipOnArt() {
    gs::Bitmap b(12, 12);
    b.rect(1, 1, 10, 10, 1);
    b.rect(3, 3, 6, 6, 2);
    return b;
}

gs::Bitmap beaterArt() {
    gs::Bitmap b(10, 16);
    b.poly({{5, 1}, {9, 12}, {1, 12}}, 1);
    b.rect(2, 12, 6, 3, 2);
    return b;
}

gs::Bitmap cardArt() {
    gs::Bitmap b(72, 40);
    b.rect(1, 1, 70, 38, 1);
    b.rect(4, 4, 64, 32, 2);
    b.rect(8, 10, 36, 4, 3);
    b.rect(8, 20, 24, 3, 4);
    return b;
}

gs::Bitmap stampArt() {
    gs::Bitmap b(44, 16);
    b.rect(1, 1, 42, 14, 1);
    b.rect(3, 3, 38, 10, 2);
    return b;
}

gs::Bitmap stoolArt() {
    gs::Bitmap b(28, 16);
    b.rect(2, 1, 24, 4, 1);
    b.line(6.f, 5.f, 4.f, 15.f, 2, 2.f);
    b.line(22.f, 5.f, 24.f, 15.f, 2, 2.f);
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
    ink(vdp, PAL_HUD, gs::rgb4(15, 15, 14), gs::rgb4(2, 1, 3));
    ink(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(4, 2, 0));
    ink(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(4, 0, 1));
    ink(vdp, PAL_GREEN, gs::rgb4(6, 15, 8), gs::rgb4(0, 3, 1));

    setPal(vdp, PAL_DRUM,
           {0, gs::rgb4(10, 2, 2), gs::rgb4(14, 3, 3), gs::rgb4(13, 12, 9), gs::rgb4(14, 13, 10),
            gs::rgb4(15, 15, 13), gs::rgb4(6, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(2, 4, 9), gs::rgb4(4, 7, 13), gs::rgb4(12, 8, 5), gs::rgb4(15, 13, 10), gs::rgb4(3, 2, 2),
            gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_STAGE,
           {0, gs::rgb4(8, 1, 3), gs::rgb4(12, 2, 4), gs::rgb4(5, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            gs::rgb4(2, 0, 1)});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(15, 13, 6), gs::rgb4(8, 7, 6), gs::rgb4(4, 3, 3), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(12, 10, 8), gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            gs::rgb4(2, 1, 1)});

    art.drum = gs::uploadMipped(vdp, drumArt());
    art.stickUp = gs::uploadMipped(vdp, stickUpArt());
    art.stickDown = gs::uploadMipped(vdp, stickDownArt());
    art.player = gs::uploadMipped(vdp, playerArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.curtain = gs::uploadMipped(vdp, curtainArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.pipOn = gs::uploadMipped(vdp, pipOnArt());
    art.beater = gs::uploadMipped(vdp, beaterArt());
    art.card = gs::uploadMipped(vdp, cardArt());
    art.stamp = gs::uploadMipped(vdp, stampArt());
    art.stool = gs::uploadMipped(vdp, stoolArt());
    loadFont(vdp, art);
}

}  // namespace drummark
