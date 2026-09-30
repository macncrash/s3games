#include "game/art.h"

#include <initializer_list>

namespace drumbell {
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
    gs::Bitmap b(80, 44);
    b.ellipse(40.f, 16.f, 32.f, 11.f, 3);
    b.ellipse(40.f, 15.f, 22.f, 6.f, 5);
    b.ellipse(34.f, 13.f, 8.f, 2.4f, 6);
    b.rect(8.f, 16.f, 64.f, 18.f, 2);
    b.rect(10.f, 18.f, 60.f, 3.f, 1);
    b.rect(8.f, 16.f, 64.f, 3.f, 4);
    b.rect(8.f, 30.f, 64.f, 3.f, 4);
    b.rect(14.f, 34.f, 52.f, 5.f, 7);
    b.rect(18.f, 38.f, 4.f, 5.f, 4);
    b.rect(58.f, 38.f, 4.f, 5.f, 4);
    return b;
}

gs::Bitmap headArt() {
    gs::Bitmap b(22, 16);
    b.ellipse(11.f, 8.f, 9.f, 6.f, 1);
    b.ellipse(11.f, 7.5f, 5.f, 3.f, 2);
    return b;
}

gs::Bitmap headOnArt() {
    gs::Bitmap b(22, 16);
    b.ellipse(11.f, 8.f, 9.f, 6.f, 1);
    b.ellipse(11.f, 7.5f, 5.5f, 3.2f, 3);
    b.ellipse(9.f, 6.5f, 2.f, 1.2f, 4);
    return b;
}

gs::Bitmap stickUpArt() {
    gs::Bitmap b(46, 26);
    b.line(4.f, 22.f, 38.f, 6.f, 1, 2.2f);
    b.ellipse(40.f, 5.f, 3.4f, 3.f, 2);
    return b;
}

gs::Bitmap stickDownArt() {
    gs::Bitmap b(46, 26);
    b.line(6.f, 5.f, 36.f, 20.f, 1, 2.2f);
    b.ellipse(38.f, 21.f, 3.6f, 2.6f, 2);
    return b;
}

gs::Bitmap playerArt() {
    gs::Bitmap b(40, 62);
    b.ellipse(20.f, 7.f, 8.f, 3.f, 2);
    b.ellipse(20.f, 12.f, 6.f, 6.f, 3);
    b.ellipse(18.f, 11.f, 1.6f, 1.6f, 4);
    b.rect(14.f, 17.f, 12.f, 3.f, 5);
    b.rect(11.f, 20.f, 18.f, 16.f, 1);
    b.rect(14.f, 22.f, 12.f, 5.f, 6);
    b.line(11.f, 22.f, 3.f, 34.f, 3, 3.f);
    b.line(29.f, 22.f, 37.f, 14.f, 3, 3.f);
    b.rect(13.f, 36.f, 6.f, 16.f, 7);
    b.rect(21.f, 36.f, 6.f, 16.f, 7);
    b.rect(11.f, 50.f, 9.f, 4.f, 8);
    b.rect(21.f, 50.f, 9.f, 4.f, 8);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 32);
    b.rect(12, 1, 4, 5, 1);
    b.poly({{8, 8}, {20, 8}, {25, 26}, {3, 26}}, 2);
    b.poly({{10, 10}, {18, 10}, {21, 22}, {7, 22}}, 3);
    b.rect(2, 25, 24, 4, 4);
    b.ellipse(14.f, 18.f, 2.2f, 3.f, 5);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(8, 10);
    b.rect(3, 0, 2, 4, 1);
    b.ellipse(4.f, 7.f, 3.f, 2.6f, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 14);
    b.ellipse(6.f, 7.f, 5.f, 5.f, 1);
    b.ellipse(6.f, 6.5f, 2.2f, 2.2f, 2);
    return b;
}

gs::Bitmap curtainArt() {
    gs::Bitmap b(26, 96);
    b.rect(0, 0, 26, 96, 1);
    b.rect(2, 0, 5, 96, 2);
    b.rect(16, 0, 5, 96, 3);
    for (int y = 6; y < 92; y += 14) b.rect(8, y, 7, 5, 2);
    return b;
}

gs::Bitmap beaterArt() {
    gs::Bitmap b(10, 18);
    b.poly({{5, 1}, {9, 13}, {1, 13}}, 1);
    b.rect(2, 13, 6, 3, 2);
    return b;
}

gs::Bitmap stoolArt() {
    gs::Bitmap b(30, 16);
    b.rect(2, 1, 26, 4, 1);
    b.line(7.f, 5.f, 4.f, 15.f, 2, 2.f);
    b.line(23.f, 5.f, 26.f, 15.f, 2, 2.f);
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
    vdp.setColor(PAL_GOLD * 16 + 2, gs::rgb4(15, 14, 8));
    vdp.setColor(PAL_GOLD * 16 + 3, gs::rgb4(15, 15, 12));
    vdp.setColor(PAL_GOLD * 16 + 4, gs::rgb4(15, 15, 14));
    ink(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(4, 0, 1));
    vdp.setColor(PAL_RED * 16 + 2, gs::rgb4(15, 12, 8));
    ink(vdp, PAL_GREEN, gs::rgb4(6, 15, 8), gs::rgb4(0, 3, 1));
    vdp.setColor(PAL_GREEN * 16 + 2, gs::rgb4(12, 15, 10));
    vdp.setColor(PAL_GREEN * 16 + 3, gs::rgb4(14, 15, 12));
    vdp.setColor(PAL_GREEN * 16 + 4, gs::rgb4(15, 15, 14));
    ink(vdp, PAL_DIM, gs::rgb4(8, 8, 9), gs::rgb4(1, 1, 2));
    vdp.setColor(PAL_DIM * 16 + 2, gs::rgb4(12, 12, 13));

    setPal(vdp, PAL_DRUM,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(8, 2, 2), gs::rgb4(14, 6, 3), gs::rgb4(13, 11, 7), gs::rgb4(15, 14, 11),
            gs::rgb4(15, 15, 13), gs::rgb4(5, 3, 2), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(3, 5, 10), gs::rgb4(6, 3, 2), gs::rgb4(13, 9, 6), gs::rgb4(15, 13, 11), gs::rgb4(2, 1, 1),
            gs::rgb4(11, 8, 4), gs::rgb4(2, 2, 5), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(8, 7, 5), gs::rgb4(14, 12, 4), gs::rgb4(15, 14, 8), gs::rgb4(10, 8, 2), gs::rgb4(6, 5, 2), 0,
            0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(9, 2, 3), gs::rgb4(13, 3, 4), gs::rgb4(5, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            gs::rgb4(2, 0, 1)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 12), gs::rgb4(11, 9, 6), gs::rgb4(5, 4, 3), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, gs::rgb4(2, 1, 1)});

    art.drum = gs::uploadMipped(vdp, drumArt());
    art.head = gs::uploadMipped(vdp, headArt());
    art.headOn = gs::uploadMipped(vdp, headOnArt());
    art.stickUp = gs::uploadMipped(vdp, stickUpArt());
    art.stickDown = gs::uploadMipped(vdp, stickDownArt());
    art.player = gs::uploadMipped(vdp, playerArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadMipped(vdp, clapperArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.curtain = gs::uploadMipped(vdp, curtainArt());
    art.beater = gs::uploadMipped(vdp, beaterArt());
    art.stool = gs::uploadMipped(vdp, stoolArt());
    loadFont(vdp, art);
}

}  // namespace drumbell
