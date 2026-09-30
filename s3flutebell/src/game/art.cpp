#include "game/art.h"

#include <initializer_list>

namespace flutebell {
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
    gs::Bitmap b(48, 64);
    b.ellipse(22.f, 9.f, 7.f, 7.5f, 3);
    b.ellipse(20.f, 8.f, 2.2f, 2.2f, 4);
    b.rect(16.f, 15.f, 12.f, 3.f, 5);
    b.rect(14.f, 18.f, 18.f, 20.f, 1);
    b.rect(16.f, 20.f, 14.f, 5.f, 2);
    b.line(14.f, 22.f, 4.f, 30.f, 3, 3.2f);
    b.line(32.f, 22.f, 44.f, 18.f, 3, 3.f);
    b.rect(16.f, 38.f, 6.f, 18.f, 6);
    b.rect(24.f, 38.f, 6.f, 18.f, 6);
    b.rect(14.f, 54.f, 9.f, 4.f, 7);
    b.rect(24.f, 54.f, 9.f, 4.f, 7);
    return b;
}

gs::Bitmap fluteArt() {
    gs::Bitmap b(56, 12);
    b.rect(2.f, 4.f, 50.f, 4.f, 1);
    b.rect(2.f, 4.f, 50.f, 1.f, 2);
    b.ellipse(4.f, 6.f, 3.f, 3.2f, 3);
    for (int i = 0; i < 6; i++) b.ellipse(14.f + float(i) * 6.f, 6.f, 1.3f, 1.3f, 4);
    b.rect(50.f, 5.f, 4.f, 2.f, 2);
    return b;
}

gs::Bitmap standArt() {
    gs::Bitmap b(40, 48);
    b.poly({{4, 6}, {36, 6}, {32, 22}, {8, 22}}, 1);
    b.rect(6.f, 8.f, 28.f, 10.f, 2);
    b.rect(18.f, 22.f, 4.f, 18.f, 3);
    b.line(20.f, 36.f, 6.f, 46.f, 3, 2.f);
    b.line(20.f, 36.f, 34.f, 46.f, 3, 2.f);
    return b;
}

gs::Bitmap staffArt() {
    gs::Bitmap b(220, 36);
    for (int i = 0; i < 5; i++) b.rect(4.f, 6.f + float(i) * 5.f, 212.f, 1.f, 1);
    b.rect(8.f, 6.f, 2.f, 21.f, 2);
    b.rect(206.f, 6.f, 2.f, 21.f, 2);
    return b;
}

gs::Bitmap noteArt() {
    gs::Bitmap b(14, 22);
    b.ellipse(5.f, 16.f, 4.f, 3.f, 1);
    b.rect(8.f, 4.f, 2.f, 13.f, 1);
    b.rect(8.f, 4.f, 5.f, 2.f, 1);
    return b;
}

gs::Bitmap noteOnArt() {
    gs::Bitmap b(14, 22);
    b.ellipse(5.f, 16.f, 4.2f, 3.2f, 2);
    b.ellipse(5.f, 16.f, 2.f, 1.4f, 1);
    b.rect(8.f, 3.f, 2.f, 14.f, 1);
    b.rect(8.f, 3.f, 5.f, 2.f, 2);
    return b;
}

gs::Bitmap breathArt() {
    gs::Bitmap b(10, 16);
    b.poly({{5, 1}, {9, 8}, {5, 15}, {1, 8}}, 1);
    b.poly({{5, 4}, {7, 8}, {5, 12}, {3, 8}}, 2);
    return b;
}

gs::Bitmap windowArt() {
    gs::Bitmap b(36, 80);
    b.rect(0, 0, 36, 80, 1);
    b.rect(4, 4, 28, 72, 2);
    b.rect(16, 4, 2, 72, 1);
    b.rect(4, 36, 28, 2, 1);
    b.rect(8, 10, 6, 10, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 44);
    b.poly({{2, 8}, {14, 8}, {11, 16}, {5, 16}}, 1);
    b.rect(7, 16, 2, 18, 2);
    b.rect(3, 34, 10, 5, 3);
    b.ellipse(8.f, 6.f, 3.f, 2.2f, 4);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(40, 36);
    b.ellipse(20, 22, 16, 12, 3);
    b.ellipse(20, 20, 12, 8, 1);
    b.ellipse(16, 16, 4, 3, 2);
    b.rect(17, 4, 6, 8, 3);
    b.rect(14, 2, 12, 4, 4);
    b.rect(8, 30, 24, 3, 4);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(8, 16);
    b.rect(3, 0, 2, 8, 4);
    b.ellipse(4, 12, 3, 3, 3);
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
    ink(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(4, 3, 0));
    ink(vdp, PAL_RED, gs::rgb4(15, 5, 4), gs::rgb4(4, 0, 1));
    ink(vdp, PAL_GREEN, gs::rgb4(5, 15, 9), gs::rgb4(0, 3, 2));

    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(10, 6, 2), gs::rgb4(14, 10, 4), gs::rgb4(6, 3, 1), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_PLAYER,
           {0, gs::rgb4(3, 5, 10), gs::rgb4(6, 9, 14), gs::rgb4(13, 9, 6), gs::rgb4(15, 13, 11), gs::rgb4(4, 2, 2),
            gs::rgb4(2, 2, 5), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_NIGHT,
           {0, gs::rgb4(2, 3, 6), gs::rgb4(6, 9, 13), gs::rgb4(12, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            gs::rgb4(1, 1, 3)});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(15, 13, 6), gs::rgb4(7, 6, 5), gs::rgb4(3, 3, 3), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_NOTE,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10), gs::rgb4(11, 8, 2), gs::rgb4(6, 4, 1), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, gs::rgb4(3, 2, 0)});

    art.player = gs::uploadMipped(vdp, playerArt());
    art.flute = gs::uploadMipped(vdp, fluteArt());
    art.stand = gs::uploadMipped(vdp, standArt());
    art.staff = gs::uploadMipped(vdp, staffArt());
    art.note = gs::uploadMipped(vdp, noteArt());
    art.noteOn = gs::uploadMipped(vdp, noteOnArt());
    art.breath = gs::uploadMipped(vdp, breathArt());
    art.window = gs::uploadMipped(vdp, windowArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clapper = gs::uploadMipped(vdp, clapperArt());
    loadFont(vdp, art);
}

}  // namespace flutebell
