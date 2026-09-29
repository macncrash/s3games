#include "art.h"

#include <initializer_list>

namespace rickshawturn {
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
    Bitmap b(76, 50);
    b.poly({{6, 22}, {70, 22}, {64, 32}, {12, 32}}, 2);
    b.rect(12, 30, 52, 14, 3);
    b.rect(16, 33, 12, 7, 4);
    b.rect(44, 33, 12, 7, 4);
    b.rect(34, 34, 8, 8, 5);
    b.poly({{10, 22}, {38, 4}, {66, 22}}, 6);
    b.line(38, 4, 38, 22, 1, 1.2f);
    b.rect(30, 7, 16, 6, 7);
    b.rect(8, 18, 6, 4, 8);
    b.outline(1, false);
    return b.cropToContent(1);
}

Bitmap paintWheel() {
    Bitmap b(24, 24);
    b.ellipse(12, 12, 11.f, 11.f, 1);
    b.ellipse(12, 12, 7.5f, 7.5f, 2);
    b.ellipse(12, 12, 2.2f, 2.2f, 3);
    b.line(12, 2, 12, 22, 3, 1.f);
    b.line(2, 12, 22, 12, 3, 1.f);
    b.line(4, 4, 20, 20, 3, 1.f);
    b.line(20, 4, 4, 20, 3, 1.f);
    return b;
}

Bitmap paintRider() {
    Bitmap b(22, 30);
    b.ellipse(11, 6, 4.2f, 4.2f, 1);
    b.rect(8, 10, 6, 11, 2);
    b.line(8, 14, 2, 22, 2, 1.5f);
    b.line(14, 14, 20, 20, 2, 1.5f);
    b.line(9, 21, 6, 28, 3, 1.4f);
    b.line(13, 21, 16, 28, 3, 1.4f);
    b.rect(7, 4, 8, 2, 4);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(14, 42);
    b.rect(6, 14, 2, 26, 2);
    b.ellipse(7, 8, 5.f, 4.5f, 1);
    b.rect(2, 13, 10, 2, 3);
    return b;
}

Bitmap paintChevron() {
    Bitmap b(28, 22);
    b.poly({{2, 4}, {16, 11}, {2, 18}}, 1);
    b.poly({{10, 4}, {24, 11}, {10, 18}}, 2);
    b.outline(3, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(20, 36);
    b.rect(8, 10, 4, 24, 2);
    b.rect(3, 4, 14, 10, 1);
    b.rect(6, 7, 8, 4, 3);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(36, 48);
    b.rect(4, 8, 3, 38, 2);
    b.poly({{7, 8}, {32, 16}, {7, 24}}, 1);
    b.poly({{7, 24}, {28, 30}, {7, 36}}, 3);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(52, 12);
    b.ellipse(26, 6, 22.f, 4.f, 1);
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
           {0, gs::rgb4(2, 1, 1), gs::rgb4(13, 4, 2), gs::rgb4(15, 8, 3), gs::rgb4(7, 11, 14), gs::rgb4(3, 2, 3),
            gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 10), gs::rgb4(12, 10, 6)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(1, 1, 1), gs::rgb4(7, 7, 8), gs::rgb4(13, 11, 6)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(14, 12, 3), gs::rgb4(5, 4, 3), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 11, 2), gs::rgb4(14, 6, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), gs::rgb4(8, 2, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(7, 15, 7), gs::rgb4(1, 5, 2)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 4), gs::rgb4(8, 8, 7), gs::rgb4(3, 3, 3), gs::rgb4(3, 3, 2),
            gs::rgb4(2, 2, 2), gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 5), gs::rgb4(7, 6, 4), gs::rgb4(4, 3, 2),
            gs::rgb4(2, 3, 4), gs::rgb4(3, 4, 5), gs::rgb4(5, 6, 7), gs::rgb4(14, 12, 3), gs::rgb4(5, 5, 6)});
    vdp.setFogColor(gs::rgb4(9, 6, 4));

    art.cab = gs::uploadMipped(vdp, paintCab());
    art.wheel = gs::uploadMipped(vdp, paintWheel());
    art.rider = gs::uploadMipped(vdp, paintRider());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.chevron = gs::uploadMipped(vdp, paintChevron());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.title = word(vdp, "RICKSHAW TURN", 2, 1);
    art.job = word(vdp, "THREE TURNS  NO TIP", 1, 1);
    art.made = word(vdp, "THREE TURNS", 2, 1);
    art.tipped = word(vdp, "TIPPED", 3, 1);
    art.start = word(vdp, "START", 2, 1);
    loadFont(vdp, art);
}

}  // namespace rickshawturn
