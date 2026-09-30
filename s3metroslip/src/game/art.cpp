#include "art.h"

#include <cstdint>
#include <initializer_list>

namespace metroslip {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    uint16_t c[16] = {};
    int i = 0;
    for (uint16_t v : cs)
        if (i < 16) c[i++] = v;
    for (int k = 0; k < 16; k++) vdp.setColor(pal * 16 + k, c[k]);
}

gs::Bitmap paintHull() {
    gs::Bitmap b(64, 96);
    b.poly({{32, 6}, {46, 28}, {44, 78}, {32, 90}, {20, 78}, {18, 28}}, 3);
    b.poly({{32, 14}, {40, 30}, {38, 70}, {32, 80}, {26, 70}, {24, 30}}, 1);
    b.rect(28, 34, 8, 22, 2);
    b.rect(30, 38, 4, 8, 4);
    b.ellipse(32, 22, 4, 3, 5);
    b.line(22, 72, 42, 72, 6, 1.4f);
    b.outline(12, false);
    return b;
}

gs::Bitmap paintWake() {
    gs::Bitmap b(28, 18);
    b.line(14, 2, 4, 14, 1, 1.4f);
    b.line(14, 2, 24, 14, 1, 1.4f);
    b.line(14, 6, 8, 16, 2, 1.1f);
    b.line(14, 6, 20, 16, 2, 1.1f);
    return b;
}

gs::Bitmap paintPile() {
    gs::Bitmap b(10, 28);
    b.rect(4, 4, 2, 20, 1);
    b.ellipse(5, 5, 3, 3, 2);
    b.rect(2, 22, 6, 3, 3);
    return b;
}

gs::Bitmap paintLamp() {
    gs::Bitmap b(14, 26);
    b.rect(6, 10, 2, 12, 1);
    b.ellipse(7, 8, 4, 4, 2);
    b.ellipse(7, 8, 2, 2, 3);
    return b;
}

gs::Bitmap paintFlag() {
    gs::Bitmap b(22, 30);
    b.rect(3, 6, 2, 20, 1);
    b.poly({{5, 6}, {18, 11}, {5, 16}}, 2);
    return b;
}

gs::Bitmap paintDash() {
    gs::Bitmap b(16, 6);
    b.rect(0, 1, 16, 4, 1);
    return b;
}

gs::Bitmap paintFender() {
    gs::Bitmap b(18, 36);
    b.rect(4, 2, 10, 32, 1);
    b.rect(6, 6, 6, 8, 2);
    b.rect(6, 20, 6, 8, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap paintGull(bool up) {
    gs::Bitmap b(26, 12);
    b.ellipse(13, 7, 3, 2, 1);
    float tip = up ? 2.f : 10.f;
    b.line(13, 6, 2, tip, 1, 1.2f);
    b.line(13, 6, 24, tip, 1, 1.2f);
    return b;
}

gs::Bitmap paintDot() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

gs::Bitmap paintPin() {
    gs::Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

gs::Mipped word(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st{scale, 1, 14, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
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

void waterPal(gs::VDP& vdp, int pal, bool slip) {
    uint16_t c[16] = {};
    c[1] = slip ? gs::rgb4(5, 9, 12) : gs::rgb4(4, 6, 8);
    c[2] = slip ? gs::rgb4(3, 7, 10) : gs::rgb4(3, 5, 7);
    c[3] = gs::rgb4(6, 8, 9);
    c[4] = slip ? gs::rgb4(12, 11, 6) : gs::rgb4(8, 8, 7);
    c[5] = gs::rgb4(6, 6, 5);
    c[6] = slip ? gs::rgb4(8, 12, 14) : gs::rgb4(5, 8, 10);
    c[7] = gs::rgb4(3, 6, 8);
    c[11] = gs::rgb4(3, 8, 12);
    c[12] = gs::rgb4(2, 6, 10);
    c[13] = gs::rgb4(8, 13, 15);
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t dim = gs::rgb4(8, 9, 10);
    const uint16_t line = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, dim, gs::rgb4(12, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(14, 14, 12), gs::rgb4(6, 11, 15), gs::rgb4(3, 5, 7), gs::rgb4(14, 12, 5), gs::rgb4(15, 8, 3),
            gs::rgb4(12, 14, 15), gs::rgb4(2, 3, 4), 0, 0, 0, 0, gs::rgb4(1, 1, 1), 0, line, line});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(12, 15, 15), gs::rgb4(6, 10, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_PILE, {0, gs::rgb4(8, 7, 6), gs::rgb4(14, 12, 4), gs::rgb4(4, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(10, 7, 3), gs::rgb4(14, 4, 3), gs::rgb4(14, 13, 8), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 15, 12), gs::rgb4(12, 3, 2), gs::rgb4(15, 13, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 5, 6), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_TIDE, {0, gs::rgb4(6, 12, 15), gs::rgb4(14, 10, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_MAP, {0, gs::rgb4(8, 12, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, line, line});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(10, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 1), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1), line});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 1), line});
    waterPal(vdp, PAL_WATER, false);
    waterPal(vdp, PAL_SLIP, true);
    waterPal(vdp, PAL_FIELD, false);

    loadFont(vdp, art);
    art.hull = gs::uploadMipped(vdp, paintHull());
    art.wake = gs::uploadMipped(vdp, paintWake());
    art.pile = gs::uploadMipped(vdp, paintPile());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.dash = gs::uploadMipped(vdp, paintDash());
    art.fender = gs::uploadMipped(vdp, paintFender());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.dot = gs::uploadMipped(vdp, paintDot());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.title = word(vdp, "METRO SLIP", 3);
    art.berthed = word(vdp, "BERTHED", 3);
    art.inSlip = word(vdp, "IN THE SLIP", 2);
    art.missed = word(vdp, "MISSED THE END", 2);
    art.tide = word(vdp, "TIDE TURNED", 2);
    art.legFail = word(vdp, "LEG FAILS", 2);
    art.paused = word(vdp, "PAUSED", 3);
}

}  // namespace metroslip
