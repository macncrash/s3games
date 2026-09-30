#include "game/art.h"

#include <initializer_list>

namespace fluteseven {
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

gs::Bitmap person(int coat, int shirt, int hair) {
    gs::Bitmap b(40, 58);
    b.ellipse(20.f, 9.f, 7.f, 7.2f, 4);
    b.ellipse(18.f, 8.f, 2.f, 2.f, 5);
    b.rect(14.f, 2.f, 12.f, 4.f, hair);
    b.rect(12.f, 16.f, 16.f, 18.f, coat);
    b.rect(16.f, 18.f, 8.f, 6.f, shirt);
    b.line(12.f, 20.f, 4.f, 28.f, coat, 3.f);
    b.line(28.f, 20.f, 36.f, 16.f, coat, 2.6f);
    b.rect(14.f, 34.f, 5.f, 16.f, 6);
    b.rect(21.f, 34.f, 5.f, 16.f, 6);
    b.rect(12.f, 49.f, 8.f, 4.f, 7);
    b.rect(21.f, 49.f, 8.f, 4.f, 7);
    return b;
}

gs::Bitmap fluteArt() {
    gs::Bitmap b(64, 12);
    b.rect(4.f, 4.f, 54.f, 4.f, 1);
    b.rect(4.f, 4.f, 54.f, 1.f, 2);
    b.ellipse(5.f, 6.f, 3.4f, 3.2f, 3);
    for (int i = 0; i < 7; i++) b.ellipse(14.f + float(i) * 6.2f, 6.f, 1.2f, 1.2f, 4);
    b.rect(56.f, 5.f, 5.f, 2.f, 2);
    return b;
}

gs::Bitmap staffArt() {
    gs::Bitmap b(248, 28);
    for (int i = 0; i < 5; i++) b.rect(2.f, 4.f + float(i) * 4.5f, 244.f, 1.f, 1);
    b.rect(6.f, 4.f, 2.f, 19.f, 2);
    b.rect(238.f, 4.f, 2.f, 19.f, 2);
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
    gs::Bitmap b(10, 18);
    b.poly({{5, 1}, {9, 9}, {5, 17}, {1, 9}}, 1);
    b.poly({{5, 5}, {7, 9}, {5, 13}, {3, 9}}, 2);
    return b;
}

gs::Bitmap archArt() {
    gs::Bitmap b(80, 70);
    b.rect(0, 18, 80, 52, 1);
    b.rect(8, 26, 64, 44, 2);
    b.ellipse(40.f, 26.f, 28.f, 16.f, 2);
    b.rect(36, 8, 8, 20, 3);
    b.rect(30, 4, 20, 6, 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 40);
    b.poly({{2, 6}, {12, 6}, {10, 14}, {4, 14}}, 1);
    b.rect(6, 14, 2, 16, 2);
    b.rect(2, 30, 10, 5, 3);
    b.ellipse(7.f, 4.f, 3.f, 2.f, 4);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(10, 14);
    b.rect(4, 0, 2, 8, 1);
    b.ellipse(5.f, 10.f, 4.f, 3.2f, 2);
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
           {0, gs::rgb4(11, 7, 3), gs::rgb4(15, 12, 6), gs::rgb4(6, 3, 1), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, gs::rgb4(2, 1, 0)});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(2, 5, 11), gs::rgb4(6, 10, 15), gs::rgb4(12, 8, 5), gs::rgb4(15, 13, 10), gs::rgb4(4, 2, 2),
            gs::rgb4(2, 2, 6), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_THEM,
           {0, gs::rgb4(8, 2, 3), gs::rgb4(13, 5, 5), gs::rgb4(10, 7, 4), gs::rgb4(14, 11, 8), gs::rgb4(3, 2, 2),
            gs::rgb4(3, 2, 4), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_HALL,
           {0, gs::rgb4(3, 3, 6), gs::rgb4(7, 8, 12), gs::rgb4(5, 5, 6), gs::rgb4(12, 10, 6), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_NOTE,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(15, 14, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});

    art.you = gs::uploadMipped(vdp, person(1, 2, 3));
    art.them = gs::uploadMipped(vdp, person(1, 2, 5));
    art.flute = gs::uploadMipped(vdp, fluteArt());
    art.staff = gs::uploadMipped(vdp, staffArt());
    art.note = gs::uploadMipped(vdp, noteArt());
    art.noteOn = gs::uploadMipped(vdp, noteOnArt());
    art.breath = gs::uploadMipped(vdp, breathArt());
    art.arch = gs::uploadMipped(vdp, archArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    loadFont(vdp, art);
}

}  // namespace fluteseven
