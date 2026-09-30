#include "art.h"

namespace buslane {
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

Bitmap paintBus() {
    Bitmap b(64, 52);
    b.rect(6, 8, 52, 34, 1);
    b.rect(8, 10, 48, 8, 2);
    b.rect(10, 11, 12, 6, 3);
    b.rect(26, 11, 12, 6, 3);
    b.rect(42, 11, 12, 6, 3);
    b.rect(8, 22, 48, 6, 4);
    b.rect(10, 30, 10, 8, 5);
    b.rect(44, 30, 10, 8, 5);
    b.rect(28, 28, 8, 6, 6);
    b.rect(4, 18, 3, 16, 7);
    b.rect(57, 18, 3, 16, 7);
    b.rect(12, 40, 12, 8, 8);
    b.rect(40, 40, 12, 8, 8);
    b.rect(14, 44, 8, 4, 9);
    b.rect(42, 44, 8, 4, 9);
    b.rect(22, 4, 20, 5, 4);
    b.rect(26, 1, 12, 4, 6);
    b.outline(10, false);
    return b;
}

Bitmap paintShade() {
    Bitmap b(52, 10);
    b.ellipse(26, 5, 24, 3, 1);
    return b;
}

Bitmap paintStripe() {
    Bitmap b(48, 6);
    b.rect(0, 2, 48, 2, 1);
    return b;
}

Bitmap paintShelter() {
    Bitmap b(44, 48);
    b.rect(2, 10, 40, 4, 1);
    b.rect(4, 14, 3, 30, 2);
    b.rect(37, 14, 3, 30, 2);
    b.rect(8, 18, 28, 16, 3);
    b.rect(10, 20, 10, 12, 4);
    b.rect(22, 20, 12, 12, 5);
    b.rect(6, 42, 32, 4, 2);
    b.rect(16, 6, 12, 5, 6);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(12, 46);
    b.rect(5, 8, 2, 36, 1);
    b.rect(2, 2, 8, 7, 2);
    b.rect(3, 3, 6, 4, 3);
    b.rect(3, 42, 6, 3, 1);
    return b;
}

Bitmap paintCar() {
    Bitmap b(36, 22);
    b.rect(2, 6, 32, 12, 1);
    b.rect(6, 2, 22, 6, 2);
    b.rect(8, 3, 8, 4, 3);
    b.rect(18, 3, 8, 4, 3);
    b.rect(4, 14, 6, 5, 4);
    b.rect(26, 14, 6, 5, 4);
    return b;
}

Bitmap paintCone() {
    Bitmap b(14, 18);
    b.poly({{7, 1}, {2, 16}, {12, 16}}, 1);
    b.rect(4, 8, 6, 2, 2);
    b.rect(3, 12, 8, 2, 2);
    return b;
}

Bitmap paintDepot() {
    Bitmap b(80, 40);
    b.rect(2, 10, 8, 28, 1);
    b.rect(70, 10, 8, 28, 1);
    b.rect(2, 6, 76, 8, 2);
    b.rect(18, 16, 44, 10, 3);
    b.rect(28, 18, 24, 6, 4);
    b.rect(8, 34, 6, 4, 5);
    b.rect(66, 34, 6, 4, 5);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 12), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_BUS,
           {0, gs::rgb4(15, 12, 1), gs::rgb4(12, 8, 1), gs::rgb4(6, 9, 12), gs::rgb4(2, 2, 2), gs::rgb4(15, 3, 2),
            gs::rgb4(14, 14, 12), gs::rgb4(4, 4, 5), gs::rgb4(1, 1, 1), gs::rgb4(8, 8, 8), gs::rgb4(6, 4, 1), 0, 0, 0,
            0, gs::rgb4(4, 3, 0)});
    setPal(vdp, PAL_SHELTER,
           {0, gs::rgb4(8, 9, 10), gs::rgb4(4, 4, 5), gs::rgb4(10, 12, 14), gs::rgb4(6, 8, 10), gs::rgb4(13, 12, 8),
            gs::rgb4(15, 10, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 5, 6), gs::rgb4(10, 10, 8), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_CAR, {0, gs::rgb4(3, 5, 9), gs::rgb4(6, 8, 12), gs::rgb4(12, 14, 15), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_DEPOT, {0, gs::rgb4(6, 5, 4), gs::rgb4(9, 7, 4), gs::rgb4(12, 10, 6), gs::rgb4(15, 13, 4),
                            gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), gs::rgb4(5, 1, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 4, 1)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(5, 5, 5), gs::rgb4(3, 3, 3), gs::rgb4(7, 7, 6), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1),
            gs::rgb4(4, 4, 4), gs::rgb4(6, 5, 3), gs::rgb4(3, 4, 2), gs::rgb4(2, 3, 2), gs::rgb4(8, 7, 5),
            gs::rgb4(4, 5, 3), gs::rgb4(1, 2, 2), gs::rgb4(2, 3, 3), gs::rgb4(14, 12, 2), gs::rgb4(10, 8, 1)});

    art.bus = gs::uploadMipped(vdp, paintBus());
    art.shade = gs::uploadMipped(vdp, paintShade());
    art.stripe = gs::uploadMipped(vdp, paintStripe());
    art.shelter = gs::uploadMipped(vdp, paintShelter());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.car = gs::uploadMipped(vdp, paintCar());
    art.cone = gs::uploadMipped(vdp, paintCone());
    art.depot = gs::uploadMipped(vdp, paintDepot());
    art.title = words(vdp, "BUS LANE", 3, 1);
    art.stay = words(vdp, "STAY IN THE LANE", 1, 1);
    art.held = words(vdp, "LANE HELD", 2, 1);
    art.left = words(vdp, "LEFT THE LANE", 2, 1);
    art.start = words(vdp, "START", 2, 1);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(6, 6, 7));
}

}  // namespace buslane
