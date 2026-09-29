#include "art.h"

#include <initializer_list>

namespace mailvanlane {
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

Bitmap paintVan() {
    Bitmap b(88, 56);
    b.poly({{10, 22}, {78, 22}, {82, 48}, {6, 48}}, 2);
    b.rect(14, 8, 60, 18, 3);
    b.rect(18, 11, 16, 10, 4);
    b.rect(54, 11, 16, 10, 4);
    b.rect(16, 28, 56, 16, 5);
    b.rect(42, 28, 3, 16, 1);
    b.rect(20, 32, 16, 8, 6);
    b.rect(52, 32, 16, 8, 6);
    b.rect(8, 24, 72, 4, 7);
    b.rect(36, 14, 16, 8, 7);
    b.outline(1, false);
    return b.cropToContent(1);
}

Bitmap paintWheel() {
    Bitmap b(18, 18);
    b.ellipse(9, 9, 8.f, 8.f, 1);
    b.ellipse(9, 9, 4.f, 4.f, 2);
    b.ellipse(9, 9, 1.6f, 1.6f, 3);
    return b;
}

Bitmap paintShade() {
    Bitmap b(70, 16);
    b.ellipse(35, 8, 30.f, 5.f, 1);
    return b;
}

Bitmap paintBox() {
    Bitmap b(18, 32);
    b.rect(8, 14, 2, 16, 3);
    b.rect(3, 6, 12, 12, 1);
    b.rect(5, 8, 8, 6, 2);
    b.rect(3, 4, 12, 3, 4);
    return b;
}

Bitmap paintHouse() {
    Bitmap b(48, 52);
    b.poly({{4, 22}, {24, 6}, {44, 22}}, 2);
    b.rect(8, 22, 32, 26, 3);
    b.rect(12, 28, 8, 8, 4);
    b.rect(28, 28, 8, 8, 4);
    b.rect(20, 34, 8, 14, 5);
    b.rect(22, 16, 4, 6, 6);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(14, 40);
    b.rect(6, 12, 2, 26, 2);
    b.rect(2, 4, 10, 8, 1);
    b.ellipse(7, 8, 3.f, 2.f, 3);
    return b;
}

Bitmap paintGate() {
    Bitmap b(78, 40);
    b.rect(4, 8, 5, 30, 2);
    b.rect(69, 8, 5, 30, 2);
    b.rect(4, 6, 70, 6, 1);
    b.rect(22, 14, 34, 10, 3);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(20, 28);
    b.rect(3, 4, 2, 22, 2);
    b.poly({{5, 5}, {17, 9}, {5, 14}}, 1);
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
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(5, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(14, 13, 11), gs::rgb4(11, 10, 9), gs::rgb4(6, 9, 12), gs::rgb4(12, 3, 3),
            gs::rgb4(8, 2, 2), gs::rgb4(13, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(3, 6, 12), gs::rgb4(8, 12, 15), gs::rgb4(4, 4, 4), gs::rgb4(12, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_HOUSE,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(8, 4, 3), gs::rgb4(13, 11, 8), gs::rgb4(6, 10, 13), gs::rgb4(6, 4, 3),
            gs::rgb4(9, 9, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 7), gs::rgb4(4, 4, 5), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 13, 5), gs::rgb4(5, 5, 6), gs::rgb4(12, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(7, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(8, 8, 7), gs::rgb4(6, 6, 5), gs::rgb4(4, 7, 3), gs::rgb4(9, 8, 6), gs::rgb4(6, 5, 4),
            gs::rgb4(5, 5, 5), gs::rgb4(4, 4, 4), gs::rgb4(7, 7, 6), gs::rgb4(8, 7, 5), gs::rgb4(4, 4, 3),
            gs::rgb4(2, 3, 5), gs::rgb4(3, 4, 6), gs::rgb4(5, 7, 9), gs::rgb4(14, 14, 12), gs::rgb4(7, 7, 6)});
    vdp.setFogColor(gs::rgb4(6, 7, 10));

    art.van = gs::uploadMipped(vdp, paintVan());
    art.wheel = gs::uploadMipped(vdp, paintWheel());
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.box = gs::uploadMipped(vdp, paintBox());
    art.house = gs::uploadMipped(vdp, paintHouse());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.gate = gs::uploadMipped(vdp, paintGate());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.title = word(vdp, "MAIL VAN", 3, 1);
    art.held = word(vdp, "LANE HELD", 3, 1);
    art.left = word(vdp, "LEFT THE LANE", 2, 1);
    art.missed = word(vdp, "MISSED THE END", 2, 1);
    art.stay = word(vdp, "STAY IN THE LANE", 1, 1);
    art.start = word(vdp, "START", 2, 1);
    loadFont(vdp, art);
}

}  // namespace mailvanlane
