#include "art.h"

#include <initializer_list>

namespace rickshawlane {
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

Bitmap paintCab() {
    Bitmap b(72, 48);
    b.poly({{8, 18}, {64, 18}, {60, 28}, {12, 28}}, 2);
    b.rect(10, 26, 52, 16, 3);
    b.rect(16, 30, 14, 8, 4);
    b.rect(42, 30, 14, 8, 4);
    b.rect(32, 32, 8, 10, 5);
    b.poly({{14, 18}, {36, 6}, {58, 18}}, 6);
    b.line(36, 6, 36, 18, 1, 1.2f);
    b.rect(30, 8, 12, 6, 7);
    b.outline(1, false);
    return b.cropToContent(1);
}

Bitmap paintWheel() {
    Bitmap b(22, 22);
    b.ellipse(11, 11, 10.f, 10.f, 1);
    b.ellipse(11, 11, 7.f, 7.f, 2);
    b.ellipse(11, 11, 2.f, 2.f, 3);
    b.line(11, 2, 11, 20, 3, 1.f);
    b.line(2, 11, 20, 11, 3, 1.f);
    return b;
}

Bitmap paintRider() {
    Bitmap b(20, 28);
    b.ellipse(10, 6, 4.f, 4.f, 1);
    b.rect(7, 10, 6, 10, 2);
    b.line(7, 14, 2, 20, 2, 1.4f);
    b.line(13, 14, 18, 20, 2, 1.4f);
    b.line(8, 20, 5, 26, 3, 1.3f);
    b.line(12, 20, 15, 26, 3, 1.3f);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(16, 40);
    b.rect(7, 12, 2, 26, 2);
    b.ellipse(8, 8, 5.f, 4.f, 1);
    b.rect(3, 12, 10, 2, 3);
    return b;
}

Bitmap paintStall() {
    Bitmap b(36, 32);
    b.poly({{2, 14}, {18, 4}, {34, 14}}, 1);
    b.rect(6, 14, 24, 14, 2);
    b.rect(10, 18, 6, 6, 3);
    b.rect(20, 18, 6, 6, 4);
    b.rect(4, 26, 28, 3, 5);
    return b;
}

Bitmap paintArch() {
    Bitmap b(80, 40);
    b.rect(4, 10, 6, 28, 2);
    b.rect(70, 10, 6, 28, 2);
    b.rect(4, 6, 72, 8, 1);
    b.rect(22, 16, 36, 8, 3);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(48, 12);
    b.ellipse(24, 6, 20.f, 4.f, 1);
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
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(12, 3, 2), gs::rgb4(14, 6, 3), gs::rgb4(8, 12, 14), gs::rgb4(3, 3, 4),
            gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 8), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_STALL,
           {0, gs::rgb4(13, 5, 3), gs::rgb4(10, 8, 5), gs::rgb4(15, 13, 6), gs::rgb4(6, 10, 8), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 7), gs::rgb4(5, 5, 6), gs::rgb4(9, 9, 8)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 12, 3), gs::rgb4(4, 3, 2), gs::rgb4(14, 4, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 8), gs::rgb4(1, 4, 1)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(7, 7, 7), gs::rgb4(5, 5, 5), gs::rgb4(9, 9, 8), gs::rgb4(4, 4, 4), gs::rgb4(3, 3, 3),
            gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 6), gs::rgb4(8, 7, 5), gs::rgb4(4, 4, 3),
            gs::rgb4(2, 3, 5), gs::rgb4(3, 4, 6), gs::rgb4(5, 7, 9), gs::rgb4(14, 12, 3), gs::rgb4(5, 5, 6)});
    vdp.setFogColor(gs::rgb4(10, 7, 5));

    art.cab = gs::uploadMipped(vdp, paintCab());
    art.wheel = gs::uploadMipped(vdp, paintWheel());
    art.rider = gs::uploadMipped(vdp, paintRider());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.stall = gs::uploadMipped(vdp, paintStall());
    art.arch = gs::uploadMipped(vdp, paintArch());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.title = word(vdp, "RICKSHAW LANE", 2, 1);
    art.held = word(vdp, "LANE HELD", 3, 1);
    art.left = word(vdp, "LEFT THE LANE", 2, 1);
    art.missed = word(vdp, "MISSED THE END", 2, 1);
    art.stay = word(vdp, "STAY IN THE LANE", 1, 1);
    art.start = word(vdp, "START", 2, 1);
    loadFont(vdp, art);
}

}  // namespace rickshawlane
