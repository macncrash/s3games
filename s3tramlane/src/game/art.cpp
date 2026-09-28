#include "art.h"

namespace tramlane {
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

Bitmap paintTram() {
    Bitmap b(72, 48);
    b.rect(6, 16, 60, 22, 1);
    b.rect(8, 18, 56, 8, 4);
    b.rect(10, 19, 10, 6, 5);
    b.rect(24, 19, 10, 6, 5);
    b.rect(38, 19, 10, 6, 5);
    b.rect(52, 19, 10, 6, 5);
    b.rect(4, 28, 6, 8, 2);
    b.rect(62, 28, 6, 8, 2);
    b.rect(14, 36, 10, 6, 3);
    b.rect(48, 36, 10, 6, 3);
    b.rect(18, 40, 4, 4, 6);
    b.rect(50, 40, 4, 4, 6);
    b.rect(22, 12, 28, 5, 8);
    b.rect(30, 8, 12, 5, 7);
    b.rect(34, 2, 2, 8, 9);
    b.rect(28, 1, 14, 3, 9);
    b.rect(30, 30, 12, 4, 10);
    b.outline(15, false);
    return b;
}

Bitmap paintShade() {
    Bitmap b(56, 12);
    b.ellipse(28, 6, 26, 4, 1);
    return b;
}

Bitmap paintPant() {
    Bitmap b(18, 16);
    b.line(2, 14, 9, 2, 1, 1.4f);
    b.line(16, 14, 9, 2, 1, 1.4f);
    b.rect(1, 1, 16, 2, 2);
    return b;
}

Bitmap paintStop(int kind) {
    Bitmap b(40, 52);
    int wall = kind == 0 ? 1 : 2;
    b.rect(4, 14, 32, 34, wall);
    b.poly({{2, 14}, {20, 2}, {38, 14}}, 3);
    b.rect(8, 20, 10, 8, 4);
    b.rect(22, 20, 10, 8, 5);
    b.rect(14, 34, 12, 14, 6);
    b.rect(6, 46, 28, 3, 7);
    return b;
}

Bitmap paintPole() {
    Bitmap b(10, 48);
    b.rect(4, 4, 2, 42, 1);
    b.rect(1, 2, 8, 4, 2);
    b.rect(2, 44, 6, 3, 1);
    return b;
}

Bitmap paintWire() {
    Bitmap b(48, 6);
    b.rect(0, 2, 48, 2, 1);
    return b;
}

Bitmap paintGate() {
    Bitmap b(70, 32);
    b.rect(2, 6, 66, 8, 1);
    b.rect(0, 4, 4, 26, 2);
    b.rect(66, 4, 4, 26, 2);
    b.rect(10, 16, 50, 10, 3);
    b.rect(14, 18, 8, 6, 4);
    b.rect(26, 18, 8, 6, 5);
    b.rect(38, 18, 8, 6, 4);
    b.rect(50, 18, 8, 6, 5);
    return b;
}

Bitmap paintPost() {
    Bitmap b(16, 28);
    b.rect(6, 10, 4, 16, 2);
    b.rect(1, 2, 14, 10, 1);
    b.rect(3, 4, 10, 6, 3);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 3, 4)});
    setPal(vdp, PAL_TRAM,
           {0, gs::rgb4(14, 3, 3), gs::rgb4(8, 1, 1), gs::rgb4(2, 2, 2), gs::rgb4(10, 13, 15), gs::rgb4(4, 7, 9),
            gs::rgb4(1, 1, 1), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12), gs::rgb4(6, 6, 7), gs::rgb4(15, 12, 3),
            0, 0, 0, 0, gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_STOP,
           {0, gs::rgb4(9, 8, 7), gs::rgb4(6, 8, 10), gs::rgb4(5, 4, 3), gs::rgb4(13, 12, 8), gs::rgb4(4, 6, 8),
            gs::rgb4(3, 2, 2), gs::rgb4(4, 4, 4)});
    setPal(vdp, PAL_WIRE, {0, gs::rgb4(12, 12, 11), gs::rgb4(7, 7, 8)});
    setPal(vdp, PAL_POLE, {0, gs::rgb4(5, 5, 6), gs::rgb4(10, 9, 4)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 11, 2), gs::rgb4(3, 3, 4), gs::rgb4(14, 14, 12), gs::rgb4(8, 3, 3),
                           gs::rgb4(2, 6, 10)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(7, 15, 8), gs::rgb4(1, 4, 2)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 4), gs::rgb4(8, 8, 7), gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2),
            gs::rgb4(5, 5, 4), gs::rgb4(7, 6, 4), gs::rgb4(3, 4, 3), gs::rgb4(2, 3, 2), gs::rgb4(9, 8, 6),
            gs::rgb4(4, 5, 4), gs::rgb4(1, 2, 3), gs::rgb4(2, 3, 4), gs::rgb4(14, 13, 4), gs::rgb4(11, 10, 3)});

    art.tram = gs::uploadMipped(vdp, paintTram());
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.pant = gs::uploadMipped(vdp, paintPant());
    art.stop[0] = gs::uploadMipped(vdp, paintStop(0));
    art.stop[1] = gs::uploadMipped(vdp, paintStop(1));
    art.pole = gs::uploadMipped(vdp, paintPole());
    art.wire = gs::uploadMipped(vdp, paintWire());
    art.gate = gs::uploadMipped(vdp, paintGate());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.title = words(vdp, "TRAM LANE", 3, 1);
    art.stay = words(vdp, "STAY IN THE LANE", 1, 1);
    art.held = words(vdp, "LANE HELD", 2, 1);
    art.left = words(vdp, "LEFT THE LANE", 2, 1);
    art.start = words(vdp, "START", 2, 1);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(5, 6, 8));
}

}  // namespace tramlane
