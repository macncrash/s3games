#include "art.h"

#include <initializer_list>

namespace keellane {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap paintHull() {
    Bitmap b(40, 92);
    b.poly({{20, 2}, {32, 18}, {34, 78}, {28, 88}, {12, 88}, {6, 78}, {8, 18}}, 2);
    b.poly({{20, 10}, {28, 22}, {29, 74}, {11, 74}, {12, 22}}, 3);
    b.rect(16, 28, 8, 22, 4);
    b.rect(18, 32, 4, 6, 5);
    b.ellipse(20, 62, 4.f, 5.f, 6);
    b.line(20, 8, 20, 86, 7, 1.2f);
    b.outline(1, false);
    return b.cropToContent(1);
}

Bitmap paintSail() {
    Bitmap b(48, 56);
    b.poly({{6, 50}, {8, 6}, {42, 18}, {36, 50}}, 2);
    b.poly({{12, 46}, {13, 14}, {34, 22}, {30, 46}}, 3);
    b.line(8, 6, 8, 52, 4, 1.6f);
    b.line(10, 28, 38, 34, 5, 1.f);
    return b.cropToContent(0);
}

Bitmap paintBoom() {
    Bitmap b(44, 8);
    b.line(2, 4, 40, 4, 2, 2.2f);
    b.rect(38, 2, 4, 4, 3);
    return b;
}

Bitmap paintWake() {
    Bitmap b(28, 36);
    b.poly({{14, 2}, {22, 16}, {18, 34}, {10, 34}, {6, 16}}, 1);
    b.poly({{14, 10}, {18, 20}, {15, 32}, {13, 32}, {10, 20}}, 2);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(14, 26);
    b.rect(6, 12, 2, 12, 4);
    b.ellipse(7, 9, 5.5f, 6.f, 1);
    b.ellipse(6, 7, 2.4f, 2.f, 2);
    b.rect(5, 3, 4, 2, 3);
    return b;
}

Bitmap paintGate() {
    Bitmap b(18, 46);
    b.rect(7, 10, 4, 34, 2);
    b.poly({{11, 6}, {16, 14}, {11, 18}}, 1);
    b.rect(4, 40, 10, 3, 3);
    return b;
}

gs::Mipped word(gs::VDP& vdp, const char* s, int scale, int color) {
    gs::TextStyle st{scale, color, 0, 1, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(7, 9, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(1, 2, 4), gs::rgb4(15, 15, 13), gs::rgb4(12, 13, 14), gs::rgb4(3, 5, 8), gs::rgb4(8, 12, 14),
            gs::rgb4(6, 3, 2), gs::rgb4(9, 8, 6), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SAIL,
           {0, gs::rgb4(2, 3, 4), gs::rgb4(14, 4, 3), gs::rgb4(15, 8, 5), gs::rgb4(5, 4, 3), gs::rgb4(12, 3, 2), 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(15, 11, 2), gs::rgb4(15, 15, 12), gs::rgb4(12, 3, 2), gs::rgb4(3, 3, 4), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SIGN,
           {0, gs::rgb4(15, 14, 8), gs::rgb4(4, 6, 9), gs::rgb4(12, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_CREW,
           {0, gs::rgb4(2, 3, 5), gs::rgb4(13, 14, 15), gs::rgb4(2, 6, 10), gs::rgb4(9, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_LANE,
           {0, gs::rgb4(2, 7, 9), gs::rgb4(1, 4, 7), gs::rgb4(3, 8, 6), gs::rgb4(14, 13, 7), gs::rgb4(11, 9, 3),
            gs::rgb4(4, 9, 11), gs::rgb4(2, 6, 8), gs::rgb4(5, 8, 6), gs::rgb4(4, 5, 2), gs::rgb4(7, 8, 4),
            gs::rgb4(2, 8, 12), gs::rgb4(1, 5, 8), gs::rgb4(7, 13, 14), gs::rgb4(15, 14, 9), gs::rgb4(3, 7, 9)});
    vdp.setFogColor(gs::rgb4(6, 10, 13));

    art.hull = gs::uploadMipped(vdp, paintHull());
    art.sail = gs::uploadMipped(vdp, paintSail());
    art.boom = gs::uploadMipped(vdp, paintBoom());
    art.wake = gs::uploadMipped(vdp, paintWake());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.gate = gs::uploadMipped(vdp, paintGate());
    art.title = word(vdp, "KEEL LANE", 3, 1);
    art.held = word(vdp, "LANE HELD", 3, 1);
    art.left = word(vdp, "LEFT THE LANE", 2, 1);
    art.crew = word(vdp, "OTHER YACHT", 2, 1);
    art.stay = word(vdp, "STAY IN THE LANE", 1, 1);
    art.start = word(vdp, "START", 2, 1);
    loadFont(vdp, art);
}

}  // namespace keellane
