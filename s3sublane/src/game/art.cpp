#include "art.h"

namespace sublane {
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

Bitmap paintSub() {
    Bitmap b(68, 52);
    b.ellipse(34, 30, 28, 12, 1);
    b.ellipse(34, 28, 22, 7, 8);
    b.rect(18, 24, 32, 6, 2);
    b.rect(26, 12, 16, 16, 3);
    b.rect(30, 16, 8, 5, 4);
    b.rect(31, 4, 3, 10, 9);
    b.rect(28, 2, 9, 3, 9);
    b.ellipse(12, 34, 5, 4, 7);
    b.ellipse(56, 34, 5, 4, 7);
    b.rect(8, 32, 4, 3, 2);
    b.rect(56, 32, 4, 3, 2);
    b.rect(6, 26, 4, 3, 5);
    b.rect(58, 26, 4, 3, 6);
    b.rect(32, 36, 4, 6, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintShade() {
    Bitmap b(52, 10);
    b.ellipse(26, 5, 24, 3, 1);
    return b;
}

Bitmap paintScrew() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.line(2, 8, 14, 8, 2, 1.2f);
    b.line(8, 2, 8, 14, 2, 1.2f);
    return b;
}

Bitmap paintRock(int kind) {
    Bitmap b(36, 64);
    int body = kind == 0 ? 1 : 2;
    b.poly({{6, 60}, {2, 28}, {10, 8}, {22, 2}, {32, 18}, {34, 48}, {24, 62}}, body);
    b.poly({{10, 40}, {14, 16}, {24, 12}, {28, 36}, {18, 48}}, 3);
    b.rect(12, 54, 12, 6, 4);
    b.rect(8, 20, 5, 4, 5);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(14, 36);
    b.rect(6, 10, 2, 22, 1);
    b.ellipse(7, 8, 5, 5, 2);
    b.ellipse(7, 7, 2, 2, 3);
    b.rect(4, 30, 6, 3, 4);
    return b;
}

Bitmap paintDock() {
    Bitmap b(84, 40);
    b.rect(2, 8, 6, 28, 1);
    b.rect(76, 8, 6, 28, 1);
    b.rect(2, 6, 80, 6, 2);
    b.rect(14, 16, 8, 6, 3);
    b.rect(30, 16, 8, 6, 4);
    b.rect(46, 16, 8, 6, 3);
    b.rect(62, 16, 8, 6, 4);
    b.rect(20, 28, 44, 4, 5);
    return b;
}

Bitmap paintBubble() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 3, 3, 1);
    b.set(4, 3, 2);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(12, 15, 14), gs::rgb4(1, 3, 5)});
    setPal(vdp, PAL_SUB,
           {0, gs::rgb4(2, 7, 6), gs::rgb4(1, 3, 4), gs::rgb4(3, 9, 8), gs::rgb4(8, 14, 15), gs::rgb4(15, 3, 3),
            gs::rgb4(3, 14, 5), gs::rgb4(12, 11, 5), gs::rgb4(5, 12, 10), gs::rgb4(6, 7, 8), 0, 0, 0, 0, 0,
            gs::rgb4(1, 2, 3)});
    setPal(vdp, PAL_ROCK,
           {0, gs::rgb4(4, 5, 6), gs::rgb4(3, 4, 5), gs::rgb4(6, 7, 8), gs::rgb4(2, 2, 3), gs::rgb4(8, 9, 7)});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(7, 8, 6), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 12), gs::rgb4(3, 4, 4)});
    setPal(vdp, PAL_DOCK,
           {0, gs::rgb4(5, 6, 7), gs::rgb4(8, 10, 11), gs::rgb4(14, 12, 3), gs::rgb4(3, 12, 10), gs::rgb4(12, 14, 15),
            gs::rgb4(2, 4, 5)});
    setPal(vdp, PAL_BUBBLE, {0, gs::rgb4(9, 13, 14), gs::rgb4(14, 15, 15)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 4), gs::rgb4(4, 1, 2)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(6, 15, 10), gs::rgb4(1, 4, 3)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(1, 3, 5), gs::rgb4(1, 2, 4), gs::rgb4(2, 4, 6), gs::rgb4(10, 12, 6), gs::rgb4(6, 9, 4),
            gs::rgb4(2, 6, 8), gs::rgb4(1, 4, 7), gs::rgb4(3, 5, 6), gs::rgb4(4, 6, 7), gs::rgb4(2, 3, 4),
            gs::rgb4(1, 5, 8), gs::rgb4(2, 7, 10), gs::rgb4(3, 9, 12), gs::rgb4(14, 13, 6), gs::rgb4(8, 12, 14)});

    art.sub = gs::uploadMipped(vdp, paintSub());
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.screw = gs::uploadMipped(vdp, paintScrew());
    art.rock[0] = gs::uploadMipped(vdp, paintRock(0));
    art.rock[1] = gs::uploadMipped(vdp, paintRock(1));
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.dock = gs::uploadMipped(vdp, paintDock());
    art.bubble = gs::uploadMipped(vdp, paintBubble());
    art.title = words(vdp, "SUB LANE", 3, 1);
    art.stay = words(vdp, "STAY IN THE LANE", 1, 1);
    art.held = words(vdp, "LANE HELD", 2, 1);
    art.left = words(vdp, "LEFT THE LANE", 2, 1);
    art.start = words(vdp, "START", 2, 1);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(1, 3, 6));
}

}  // namespace sublane
