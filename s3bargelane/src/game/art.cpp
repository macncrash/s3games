#include "art.h"

#include <initializer_list>

namespace bargelane {
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
    Bitmap b(96, 36);
    b.poly({{48, 2}, {90, 14}, {84, 32}, {12, 32}, {6, 14}}, 2);
    b.poly({{48, 6}, {80, 16}, {76, 28}, {20, 28}, {16, 16}}, 3);
    b.rect(30, 18, 36, 8, 4);
    b.rect(44, 8, 8, 6, 5);
    b.outline(1, false);
    return b.cropToContent(1);
}

Bitmap paintCabin() {
    Bitmap b(48, 28);
    b.rect(6, 10, 36, 16, 2);
    b.rect(10, 4, 28, 10, 3);
    b.rect(14, 6, 8, 6, 4);
    b.rect(26, 6, 8, 6, 4);
    b.rect(22, 18, 4, 8, 5);
    b.outline(1, false);
    return b.cropToContent(1);
}

Bitmap paintStack() {
    Bitmap b(16, 28);
    b.rect(5, 8, 6, 18, 2);
    b.rect(3, 4, 10, 6, 3);
    b.ellipse(8, 3, 3.f, 2.f, 4);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(14, 26);
    b.rect(6, 14, 2, 10, 3);
    b.ellipse(7, 10, 5.f, 6.f, 1);
    b.ellipse(7, 8, 3.f, 2.5f, 2);
    return b;
}

Bitmap paintReed() {
    Bitmap b(22, 30);
    for (int i = 0; i < 5; i++) {
        float x = 3.f + i * 4.f;
        b.line(x, 28, x + ((i & 1) ? 2.f : -2.f), 4, (i & 1) ? 2 : 1, 1.4f);
    }
    b.ellipse(11, 26, 8.f, 3.f, 3);
    return b;
}

Bitmap paintMill() {
    Bitmap b(40, 48);
    b.rect(16, 18, 10, 28, 2);
    b.poly({{16, 18}, {21, 8}, {26, 18}}, 3);
    b.line(21, 16, 4, 8, 1, 1.5f);
    b.line(21, 16, 36, 6, 1, 1.5f);
    b.line(21, 16, 8, 28, 1, 1.5f);
    b.line(21, 16, 34, 26, 1, 1.5f);
    b.rect(19, 30, 4, 6, 4);
    return b;
}

Bitmap paintGate() {
    Bitmap b(70, 36);
    b.rect(4, 8, 4, 26, 2);
    b.rect(62, 8, 4, 26, 2);
    b.rect(4, 6, 62, 5, 1);
    b.rect(18, 12, 34, 8, 3);
    return b;
}

Bitmap paintWake() {
    Bitmap b(40, 16);
    b.ellipse(20, 8, 16.f, 5.f, 1);
    b.ellipse(20, 8, 8.f, 2.5f, 2);
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
    const uint16_t ink = gs::rgb4(15, 15, 13);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(6, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 4, 3), gs::rgb4(9, 6, 3), gs::rgb4(12, 9, 4), gs::rgb4(3, 3, 4), 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(14, 4, 3), gs::rgb4(15, 14, 8), gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(6, 9, 4), gs::rgb4(4, 7, 3), gs::rgb4(8, 7, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 13, 6), gs::rgb4(6, 5, 3), gs::rgb4(12, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_STACK, {0, gs::rgb4(3, 3, 3), gs::rgb4(7, 7, 7), gs::rgb4(4, 4, 4), gs::rgb4(12, 12, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_CANAL,
           {0, gs::rgb4(5, 8, 4), gs::rgb4(3, 6, 3), gs::rgb4(7, 8, 4), gs::rgb4(8, 7, 4), gs::rgb4(6, 6, 3),
            gs::rgb4(3, 7, 9), gs::rgb4(2, 5, 8), gs::rgb4(9, 8, 6), gs::rgb4(4, 4, 3), gs::rgb4(10, 9, 6),
            gs::rgb4(3, 8, 10), gs::rgb4(2, 6, 9), gs::rgb4(8, 12, 13), gs::rgb4(14, 13, 8), gs::rgb4(4, 9, 10)});
    vdp.setFogColor(gs::rgb4(6, 8, 10));

    art.hull = gs::uploadMipped(vdp, paintHull());
    art.cabin = gs::uploadMipped(vdp, paintCabin());
    art.stack = gs::uploadMipped(vdp, paintStack());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.mill = gs::uploadMipped(vdp, paintMill());
    art.gate = gs::uploadMipped(vdp, paintGate());
    art.wake = gs::uploadMipped(vdp, paintWake());
    art.title = word(vdp, "BARGE LANE", 3, 1);
    art.held = word(vdp, "LANE HELD", 3, 1);
    art.left = word(vdp, "LEFT THE LANE", 2, 1);
    art.missed = word(vdp, "MISSED THE END", 2, 1);
    art.stay = word(vdp, "STAY IN THE LANE", 1, 1);
    art.start = word(vdp, "START", 2, 1);
    loadFont(vdp, art);
}

}  // namespace bargelane
