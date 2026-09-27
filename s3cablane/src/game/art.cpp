#include "art.h"

namespace cablane {
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

Bitmap paintCab() {
    Bitmap b(52, 64);
    b.rect(10, 16, 32, 36, 1);
    b.rect(14, 8, 24, 12, 2);
    b.rect(16, 4, 20, 6, 8);
    b.rect(18, 1, 16, 4, 7);
    b.rect(12, 20, 10, 8, 4);
    b.rect(30, 20, 10, 8, 4);
    b.rect(22, 22, 8, 6, 5);
    b.rect(8, 46, 8, 12, 3);
    b.rect(36, 46, 8, 12, 3);
    b.rect(11, 50, 4, 6, 6);
    b.rect(37, 50, 4, 6, 6);
    b.rect(6, 40, 6, 4, 9);
    b.rect(40, 40, 6, 4, 9);
    b.rect(14, 34, 24, 4, 3);
    b.rect(18, 38, 6, 3, 6);
    b.rect(28, 38, 6, 3, 6);
    b.outline(15, false);
    return b;
}

Bitmap paintShade() {
    Bitmap b(40, 14);
    b.ellipse(20, 7, 18, 5, 1);
    return b;
}

Bitmap paintBlock(int kind) {
    Bitmap b(36, 56);
    int wall = kind == 0 ? 1 : kind == 1 ? 2 : 4;
    int win = kind == 2 ? 6 : 3;
    b.rect(2, 10, 32, 46, wall);
    b.poly({{2, 10}, {18, 1}, {34, 10}}, 5);
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 3; c++) {
            int lit = ((r + c + kind) % 3) == 0 ? win : 7;
            b.rect(6 + c * 9, 16 + r * 9, 6, 6, lit);
        }
    }
    b.rect(14, 46, 8, 10, 8);
    return b;
}

Bitmap paintPark() {
    Bitmap b(28, 18);
    b.rect(2, 4, 24, 10, 1);
    b.rect(4, 5, 7, 4, 4);
    b.rect(17, 5, 7, 4, 4);
    b.ellipse(7, 14, 3, 3, 3);
    b.ellipse(21, 14, 3, 3, 3);
    b.rect(10, 2, 8, 3, 2);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(14, 40);
    b.rect(6, 10, 2, 28, 2);
    b.ellipse(7, 6, 5, 4, 1);
    b.ellipse(7, 6, 2.4f, 1.8f, 3);
    b.rect(2, 36, 10, 3, 2);
    return b;
}

Bitmap paintGate() {
    Bitmap b(64, 28);
    b.rect(2, 8, 60, 16, 1);
    b.rect(0, 4, 4, 24, 3);
    b.rect(60, 4, 4, 24, 3);
    b.rect(8, 12, 8, 8, 2);
    b.rect(20, 12, 8, 8, 4);
    b.rect(32, 12, 8, 8, 2);
    b.rect(44, 12, 8, 8, 4);
    return b;
}

Bitmap paintMeter() {
    Bitmap b(18, 22);
    b.rect(7, 8, 4, 12, 2);
    b.rect(2, 2, 14, 8, 1);
    b.rect(4, 4, 10, 4, 3);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 13), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(15, 13, 2), gs::rgb4(12, 9, 1), gs::rgb4(1, 1, 1), gs::rgb4(6, 10, 13), gs::rgb4(3, 5, 7),
            gs::rgb4(14, 14, 13), gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2), gs::rgb4(13, 2, 2), 0, 0, 0, 0, 0,
            gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_BLOCK,
           {0, gs::rgb4(11, 6, 4), gs::rgb4(8, 8, 9), gs::rgb4(15, 13, 6), gs::rgb4(6, 7, 8), gs::rgb4(5, 3, 3),
            gs::rgb4(14, 12, 8), gs::rgb4(2, 2, 4), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_PARK,
           {0, gs::rgb4(4, 8, 12), gs::rgb4(13, 12, 10), gs::rgb4(1, 1, 1), gs::rgb4(8, 12, 14)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 8), gs::rgb4(4, 4, 5), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 4), gs::rgb4(14, 14, 13)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 4, 1)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(7, 7, 7), gs::rgb4(5, 5, 5), gs::rgb4(9, 9, 8), gs::rgb4(4, 4, 4), gs::rgb4(3, 3, 3),
            gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 6), gs::rgb4(8, 7, 5), gs::rgb4(4, 4, 3),
            gs::rgb4(2, 3, 5), gs::rgb4(3, 4, 6), gs::rgb4(5, 7, 9), gs::rgb4(14, 12, 3), gs::rgb4(5, 5, 6)});

    art.cab = gs::uploadMipped(vdp, paintCab());
    art.shade = gs::uploadMipped(vdp, paintShade());
    for (int i = 0; i < 3; i++) art.block[i] = gs::uploadMipped(vdp, paintBlock(i));
    art.park = gs::uploadMipped(vdp, paintPark());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.gate = gs::uploadMipped(vdp, paintGate());
    art.meter = gs::uploadMipped(vdp, paintMeter());
    art.title = words(vdp, "CAB LANE", 3, 1);
    art.stay = words(vdp, "STAY IN THE LANE", 1, 1);
    art.held = words(vdp, "LANE HELD", 2, 1);
    art.left = words(vdp, "LEFT THE LANE", 2, 1);
    art.start = words(vdp, "START", 2, 1);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(6, 5, 8));
}

}  // namespace cablane
