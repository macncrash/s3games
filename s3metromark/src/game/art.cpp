#include "art.h"

namespace metromark {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap paintMetro() {
    Bitmap b(96, 40);
    b.rect(4, 10, 88, 22, 1);
    b.rect(8, 12, 80, 8, 4);
    b.rect(10, 13, 12, 6, 5);
    b.rect(26, 13, 12, 6, 5);
    b.rect(42, 13, 12, 6, 5);
    b.rect(58, 13, 12, 6, 5);
    b.rect(74, 13, 12, 6, 5);
    b.rect(6, 24, 10, 6, 2);
    b.rect(80, 24, 10, 6, 2);
    b.rect(18, 30, 14, 6, 3);
    b.rect(64, 30, 14, 6, 3);
    b.rect(22, 33, 6, 4, 6);
    b.rect(68, 33, 6, 4, 6);
    b.rect(36, 4, 24, 7, 8);
    b.rect(44, 1, 8, 4, 9);
    b.rect(40, 22, 16, 4, 10);
    b.outline(15, false);
    return b;
}

Bitmap paintShade() {
    Bitmap b(70, 12);
    b.ellipse(35, 6, 32, 4, 1);
    return b;
}

Bitmap paintPant() {
    Bitmap b(16, 14);
    b.line(2, 12, 8, 2, 1, 1.3f);
    b.line(14, 12, 8, 2, 1, 1.3f);
    b.rect(1, 1, 14, 2, 2);
    return b;
}

Bitmap paintArch() {
    Bitmap b(28, 56);
    b.rect(2, 18, 6, 36, 1);
    b.rect(20, 18, 6, 36, 1);
    b.rect(4, 8, 20, 8, 2);
    b.ellipse(14, 16, 10, 10, 2);
    b.rect(8, 16, 12, 10, 0);
    b.rect(10, 28, 8, 16, 3);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(10, 28);
    b.rect(4, 8, 2, 18, 1);
    b.ellipse(5, 6, 4, 4, 2);
    b.rect(3, 4, 4, 3, 3);
    return b;
}

Bitmap paintBench() {
    Bitmap b(36, 18);
    b.rect(2, 6, 32, 4, 1);
    b.rect(4, 10, 3, 7, 2);
    b.rect(29, 10, 3, 7, 2);
    b.rect(16, 10, 3, 6, 2);
    return b;
}

Bitmap paintPlat() {
    Bitmap b(48, 16);
    b.rect(0, 4, 48, 10, 1);
    b.rect(0, 2, 48, 3, 2);
    b.rect(0, 12, 48, 2, 3);
    return b;
}

Bitmap paintStripe() {
    Bitmap b(48, 6);
    b.rect(0, 1, 48, 3, 1);
    return b;
}

Bitmap paintMark() {
    Bitmap b(28, 36);
    b.poly({{14, 2}, {26, 16}, {14, 30}, {2, 16}}, 1);
    b.poly({{14, 7}, {21, 16}, {14, 25}, {7, 16}}, 2);
    b.rect(12, 28, 4, 6, 3);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int color) {
    gs::TextStyle st{scale, color, 2, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_METRO,
           {0, gs::rgb4(12, 12, 13), gs::rgb4(4, 5, 7), gs::rgb4(2, 2, 3), gs::rgb4(6, 10, 13), gs::rgb4(3, 6, 9),
            gs::rgb4(1, 1, 1), gs::rgb4(15, 12, 3), gs::rgb4(14, 14, 15), gs::rgb4(8, 8, 9), gs::rgb4(15, 8, 2),
            0, 0, 0, 0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_PLAT,
           {0, gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 10), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 13, 2), gs::rgb4(8, 4, 1), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 5, 6), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_ARCH, {0, gs::rgb4(4, 4, 6), gs::rgb4(6, 6, 8), gs::rgb4(3, 3, 5)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(6, 15, 8), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(1, 1, 2), gs::rgb4(2, 2, 2),
            gs::rgb4(4, 4, 5), gs::rgb4(6, 5, 3), gs::rgb4(2, 3, 3), gs::rgb4(1, 2, 2), gs::rgb4(7, 6, 4),
            gs::rgb4(3, 4, 4), gs::rgb4(1, 1, 3), gs::rgb4(2, 2, 4), gs::rgb4(14, 12, 3), gs::rgb4(9, 8, 2)});

    art.metro = gs::uploadMipped(vdp, paintMetro());
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.pant = gs::uploadMipped(vdp, paintPant());
    art.arch = gs::uploadMipped(vdp, paintArch());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.bench = gs::uploadMipped(vdp, paintBench());
    art.plat = gs::uploadMipped(vdp, paintPlat());
    art.stripe = gs::uploadMipped(vdp, paintStripe());
    art.mark = gs::uploadMipped(vdp, paintMark());
    art.title = words(vdp, "METRO MARK", 3, 1);
    art.crew = words(vdp, "THE CLOCK IS THE OTHER CREW", 1, 1);
    art.onmark = words(vdp, "ON THE MARK", 2, 1);
    art.missed = words(vdp, "MISSED THE MARK", 2, 1);
    art.start = words(vdp, "START", 2, 1);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(2, 2, 4));
}

}  // namespace metromark
