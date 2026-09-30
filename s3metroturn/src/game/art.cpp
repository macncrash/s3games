#include "game/art.h"

#include <initializer_list>

namespace metroturn {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
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

void paintCar(gs::Bitmap& b) {
    b.rect(4, 18, 116, 28, 2);
    b.rect(8, 8, 104, 14, 3);
    b.rect(2, 22, 10, 18, 4);
    b.rect(112, 22, 10, 18, 5);
    for (int i = 0; i < 4; i++) b.rect(18 + i * 24, 12, 16, 10, 6);
    b.rect(8, 34, 108, 4, 7);
    b.rect(14, 40, 96, 3, 1);
    b.ellipse(28, 48, 8, 8, 8);
    b.ellipse(28, 48, 3, 3, 9);
    b.ellipse(96, 48, 8, 8, 8);
    b.ellipse(96, 48, 3, 3, 9);
    b.rect(54, 20, 3, 22, 1);
    b.rect(6, 24, 4, 4, 10);
}

void paintLamp(gs::Bitmap& b) {
    b.rect(6, 8, 4, 22, 2);
    b.ellipse(8, 6, 5, 5, 3);
    b.rect(2, 28, 12, 3, 1);
}

void paintChev(gs::Bitmap& b) {
    b.line(4, 4, 18, 14, 2, 3.f);
    b.line(4, 24, 18, 14, 2, 3.f);
    b.line(12, 4, 26, 14, 3, 2.f);
    b.line(12, 24, 26, 14, 3, 2.f);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 3), gs::rgb4(15, 12, 3), gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_TRAIN,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(11, 12, 13), gs::rgb4(13, 14, 15), gs::rgb4(4, 6, 8), gs::rgb4(14, 3, 2),
            gs::rgb4(6, 10, 14), gs::rgb4(14, 11, 2), gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 9), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9), gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(2, 2, 2), gs::rgb4(15, 12, 2), gs::rgb4(15, 6, 1)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(3, 3, 4), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 2),
            gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 5), gs::rgb4(7, 6, 2),
            gs::rgb4(2, 3, 4), gs::rgb4(3, 4, 5), gs::rgb4(4, 5, 6), gs::rgb4(14, 12, 3), gs::rgb4(7, 7, 8)});
    vdp.setFogColor(gs::rgb4(1, 1, 2));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);

    gs::Bitmap car(128, 56);
    paintCar(car);
    art.car = gs::uploadMipped(vdp, car);

    gs::Bitmap lamp(16, 32);
    paintLamp(lamp);
    art.lamp = gs::uploadMipped(vdp, lamp);

    gs::Bitmap chev(32, 28);
    paintChev(chev);
    art.chev = gs::uploadMipped(vdp, chev);
}

}  // namespace metroturn
