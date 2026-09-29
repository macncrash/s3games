#include "art.h"

namespace headerlane {
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

Bitmap paintHull() {
    Bitmap b(48, 28);
    b.poly({{4, 6}, {44, 6}, {40, 22}, {8, 22}}, 1);
    b.poly({{10, 8}, {38, 8}, {36, 16}, {12, 16}}, 2);
    b.rect(20, 2, 8, 8, 3);
    b.rect(22, 4, 4, 4, 4);
    b.ellipse(14, 20, 4, 3, 5);
    b.ellipse(34, 20, 4, 3, 5);
    b.rect(6, 18, 36, 3, 6);
    b.outline(15, false);
    return b;
}

Bitmap paintSail() {
    Bitmap b(36, 52);
    b.poly({{18, 2}, {32, 48}, {6, 46}}, 1);
    b.poly({{18, 6}, {28, 44}, {12, 42}}, 2);
    b.line(18, 2, 18, 50, 3, 1.2f);
    b.line(8, 44, 30, 46, 4, 1.f);
    b.outline(15, false);
    return b;
}

Bitmap paintJib() {
    Bitmap b(22, 36);
    b.poly({{4, 2}, {18, 32}, {3, 30}}, 1);
    b.poly({{6, 6}, {15, 28}, {5, 26}}, 2);
    b.line(4, 2, 4, 32, 3, 1.f);
    return b;
}

Bitmap paintShade() {
    Bitmap b(40, 12);
    b.ellipse(20, 6, 16, 4, 1);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(16, 28);
    b.ellipse(8, 10, 6, 7, 1);
    b.rect(7, 16, 2, 8, 2);
    b.ellipse(8, 10, 2, 2, 3);
    b.rect(4, 23, 8, 3, 4);
    return b;
}

Bitmap paintMark() {
    Bitmap b(28, 48);
    b.rect(12, 16, 4, 28, 2);
    b.poly({{14, 4}, {26, 18}, {14, 18}}, 1);
    b.rect(6, 40, 16, 4, 3);
    b.ellipse(14, 44, 6, 3, 4);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(22, 18);
    b.rect(2, 2, 2, 14, 2);
    b.poly({{4, 3}, {20, 7}, {4, 12}}, 1);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(1, 3, 6)});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(3, 8, 12), gs::rgb4(8, 5, 2), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 2),
            gs::rgb4(12, 4, 3), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_SAIL,
           {0, gs::rgb4(15, 15, 13), gs::rgb4(13, 14, 15), gs::rgb4(6, 4, 2), gs::rgb4(9, 8, 6), 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, gs::rgb4(2, 3, 4)});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(15, 5, 2), gs::rgb4(4, 4, 4), gs::rgb4(15, 14, 8), gs::rgb4(2, 6, 4)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 12, 2), gs::rgb4(5, 4, 3), gs::rgb4(2, 5, 8), gs::rgb4(8, 10, 12)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 14, 6), gs::rgb4(2, 4, 8), gs::rgb4(12, 13, 14)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(4, 1, 2)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 9), gs::rgb4(1, 4, 3)});
    setPal(vdp, PAL_SEA,
           {0, gs::rgb4(2, 6, 8), gs::rgb4(1, 4, 7), gs::rgb4(3, 8, 9), gs::rgb4(4, 9, 8), gs::rgb4(2, 7, 7),
            gs::rgb4(1, 5, 8), gs::rgb4(2, 8, 10), gs::rgb4(1, 6, 9), gs::rgb4(6, 10, 11), gs::rgb4(3, 7, 9),
            gs::rgb4(5, 9, 10), gs::rgb4(1, 5, 9), gs::rgb4(2, 7, 11), gs::rgb4(4, 10, 13), gs::rgb4(8, 13, 14),
            gs::rgb4(3, 8, 10)});

    art.hull = gs::uploadMipped(vdp, paintHull());
    art.sail = gs::uploadMipped(vdp, paintSail());
    art.jib = gs::uploadMipped(vdp, paintJib());
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.mark = gs::uploadMipped(vdp, paintMark());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.title = words(vdp, "HEADER LANE", 2, 1);
    art.sub = words(vdp, "TAKE THE HEADER", 1, 1);
    art.took = words(vdp, "LANE HELD", 2, 1);
    art.missed = words(vdp, "LOST THE LEG", 2, 1);
    art.start = words(vdp, "START", 2, 1);
    art.header = words(vdp, "HEADER", 2, 1);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(6, 9, 12));
}

}  // namespace headerlane
