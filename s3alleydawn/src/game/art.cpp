#include "game/art.h"

namespace alleydawn {
namespace {

uint16_t C(int r, int g, int b) { return gs::rgb4(r, g, b); }

void setPal(gs::VDP& vdp, int pal, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

void px8(uint8_t* t, int x, int y, int c) {
    if (x >= 0 && y >= 0 && x < 8 && y < 8) t[y * 8 + x] = uint8_t(c);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{2, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int tile = tiles.shared(px);
        vdp.loadTile(tile, px);
        a.font[c - 32] = tile;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap manBmp(int step) {
    gs::Bitmap b(16, 28);
    b.ellipse(8, 5, 3.2f, 3.4f, 4);
    b.rect(6, 8, 4, 2, 5);
    b.rect(5, 10, 6, 9, 2);
    b.rect(6, 11, 4, 6, 3);
    b.rect(3, 11, 3, 7, 2);
    b.rect(10, 11, 3, 7, 2);
    int kick = step ? 2 : 0;
    b.rect(5, 19, 3, 8 - kick, 6);
    b.rect(9, 19 + kick, 3, 8 - kick, 6);
    b.rect(4, 26 - kick, 4, 2, 1);
    b.rect(8, 26, 4, 2, 1);
    b.set(7, 4, 7);
    b.set(10, 4, 7);
    return b;
}

gs::Bitmap potBmp() {
    gs::Bitmap b(14, 16);
    b.rect(2, 6, 10, 8, 2);
    b.rect(1, 5, 12, 2, 3);
    b.rect(3, 8, 8, 4, 4);
    b.rect(4, 14, 2, 2, 1);
    b.rect(8, 14, 2, 2, 1);
    b.ellipse(7, 4, 2.2f, 2.2f, 5);
    return b;
}

gs::Bitmap flameBmp(int flick) {
    gs::Bitmap b(10, 16);
    b.ellipse(5, 11, 3.2f, 4.2f, 2);
    b.ellipse(5, 8, 2.2f, 4.f, 3);
    b.ellipse(5 + flick, 5, 1.2f, 2.6f, 4);
    b.set(5, 2, 5);
    return b;
}

gs::Bitmap tarpBmp() {
    gs::Bitmap b(22, 12);
    b.poly({{1, 2}, {20, 1}, {21, 10}, {2, 11}}, 2);
    b.line(3, 3, 18, 9, 3, 1.2f);
    b.line(4, 9, 16, 3, 4, 1.f);
    b.rect(0, 1, 3, 3, 5);
    return b;
}

gs::Bitmap canBmp(int roll) {
    gs::Bitmap b(16, 18);
    b.ellipse(8, 9, 6, 7, 2);
    b.rect(3, 4, 10, 10, 3);
    b.ellipse(8, 5, 4, 2, 4);
    b.rect(6, 8, 4, 5, roll ? 5 : 1);
    b.rect(2, 14, 3, 3, 6);
    b.rect(11, 14, 3, 3, 6);
    return b;
}

gs::Bitmap sparkBmp() {
    gs::Bitmap b(4, 4);
    b.set(1, 0, 1);
    b.set(2, 1, 1);
    b.set(1, 1, 2);
    b.set(0, 1, 1);
    b.set(1, 2, 1);
    return b;
}

gs::Bitmap moonBmp() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 2);
    b.ellipse(9, 6, 4, 4, 0);
    b.set(4, 5, 3);
    b.set(5, 9, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {C(0, 0, 0), C(14, 13, 10), C(8, 7, 6), C(4, 3, 3), C(15, 12, 4),
                            C(15, 6, 2),  C(2, 2, 3),  C(12, 4, 2), C(6, 8, 4),  C(3, 5, 8),
                            C(10, 10, 12), C(15, 14, 8), C(9, 6, 3), C(15, 15, 12), C(5, 4, 4), C(1, 1, 2)};
    const uint16_t brick[] = {C(0, 0, 0), C(6, 3, 2), C(8, 4, 3), C(4, 2, 2), C(10, 6, 4),
                              C(3, 2, 2),  C(5, 4, 3), C(2, 1, 1), C(7, 5, 3), C(9, 5, 3),
                              C(1, 1, 1),  C(12, 7, 4), C(8, 8, 7), C(14, 12, 6), C(2, 2, 3), C(0, 0, 0)};
    const uint16_t wet[] = {C(0, 0, 0), C(2, 2, 3), C(3, 3, 4), C(4, 4, 5), C(1, 1, 2),
                            C(6, 6, 7),  C(5, 5, 4), C(8, 7, 5), C(2, 3, 4), C(1, 2, 3),
                            C(9, 8, 6),  C(3, 4, 5), C(7, 6, 5), C(10, 9, 7), C(4, 5, 6), C(0, 0, 0)};
    const uint16_t man[] = {C(0, 0, 0), C(2, 2, 2), C(3, 4, 6), C(5, 7, 9), C(12, 8, 6),
                            C(8, 5, 4),  C(2, 2, 4), C(1, 1, 1), C(14, 12, 8), C(6, 8, 4),
                            C(4, 3, 3),  C(9, 9, 10), C(7, 6, 5), C(15, 10, 4), C(1, 1, 2), C(0, 0, 0)};
    const uint16_t fire[] = {C(0, 0, 0), C(8, 2, 0), C(14, 5, 0), C(15, 10, 1), C(15, 14, 4),
                             C(15, 15, 10), C(6, 1, 0), C(12, 3, 0), C(10, 8, 2), C(4, 1, 0),
                             C(15, 8, 2), C(9, 4, 1), C(7, 7, 2), C(14, 12, 6), C(3, 1, 0), C(0, 0, 0)};
    const uint16_t ember[] = {C(0, 0, 0), C(10, 3, 0), C(14, 6, 1), C(15, 10, 2), C(8, 2, 0),
                              C(4, 1, 0),  C(12, 4, 1), C(6, 2, 1), C(15, 12, 4), C(3, 1, 1),
                              C(9, 5, 2),  C(7, 3, 1), C(11, 7, 2), C(2, 1, 0), C(13, 8, 3), C(0, 0, 0)};
    const uint16_t iron[] = {C(0, 0, 0), C(3, 3, 4), C(6, 6, 7), C(8, 8, 9), C(10, 10, 11),
                             C(4, 4, 5),  C(2, 2, 3), C(12, 11, 8), C(5, 5, 6), C(7, 7, 8),
                             C(1, 1, 2),  C(9, 8, 6), C(11, 11, 12), C(13, 12, 8), C(4, 5, 6), C(0, 0, 0)};
    const uint16_t tarp[] = {C(0, 0, 0), C(1, 3, 2), C(2, 6, 4), C(3, 8, 5), C(1, 4, 3),
                             C(8, 7, 3),  C(4, 9, 5), C(0, 2, 1), C(5, 10, 6), C(2, 5, 3),
                             C(6, 8, 4),  C(3, 5, 3), C(7, 9, 4), C(9, 10, 5), C(1, 2, 1), C(0, 0, 0)};
    const uint16_t can[] = {C(0, 0, 0), C(4, 5, 3), C(6, 7, 4), C(8, 9, 5), C(10, 11, 6),
                            C(3, 4, 2),  C(2, 2, 2), C(12, 10, 4), C(5, 6, 3), C(7, 8, 4),
                             C(1, 2, 1), C(9, 8, 4), C(11, 10, 6), C(13, 12, 6), C(4, 4, 3), C(0, 0, 0)};
    const uint16_t smoke[] = {C(0, 0, 0), C(4, 4, 5), C(6, 6, 7), C(8, 8, 8), C(3, 3, 4),
                              C(5, 5, 6),  C(7, 7, 7), C(2, 2, 3), C(9, 9, 9), C(1, 1, 2),
                              C(10, 10, 10), C(6, 6, 5), C(8, 7, 6), C(4, 4, 4), C(3, 3, 3), C(0, 0, 0)};
    const uint16_t moon[] = {C(0, 0, 0), C(10, 10, 12), C(14, 14, 15), C(8, 8, 10), C(6, 6, 8),
                             C(12, 12, 14), C(4, 4, 6), C(15, 15, 15), C(9, 9, 11), C(7, 7, 9),
                             C(3, 3, 5), C(11, 11, 13), C(13, 13, 14), C(5, 5, 7), C(2, 2, 4), C(0, 0, 0)};
    const uint16_t sun[] = {C(0, 0, 0), C(15, 10, 3), C(15, 13, 5), C(15, 15, 8), C(12, 7, 2),
                            C(14, 8, 2), C(10, 5, 1), C(15, 12, 6), C(8, 4, 1), C(13, 9, 3),
                            C(15, 14, 10), C(11, 6, 2), C(9, 6, 2), C(14, 11, 4), C(7, 3, 1), C(0, 0, 0)};
    const uint16_t gold[] = {C(0, 0, 0), C(12, 9, 2), C(15, 12, 3), C(15, 14, 6), C(8, 6, 1),
                             C(14, 10, 2), C(10, 7, 1), C(15, 15, 8), C(6, 4, 1), C(13, 11, 4),
                             C(9, 8, 2), C(11, 8, 2), C(14, 13, 5), C(7, 5, 1), C(15, 11, 3), C(1, 1, 0)};
    const uint16_t alert[] = {C(0, 0, 0), C(12, 2, 1), C(15, 4, 2), C(15, 8, 3), C(8, 1, 1),
                              C(14, 6, 2), C(6, 1, 1), C(15, 12, 4), C(10, 2, 1), C(4, 0, 0),
                              C(15, 10, 3), C(9, 3, 1), C(13, 5, 2), C(7, 1, 1), C(15, 14, 8), C(1, 0, 0)};
    const uint16_t pip[] = {C(0, 0, 0), C(6, 5, 3), C(10, 8, 4), C(3, 3, 2), C(8, 7, 4),
                            C(4, 4, 3),  C(12, 10, 5), C(2, 2, 2), C(14, 12, 6), C(5, 4, 2),
                            C(9, 7, 3), C(7, 6, 3), C(11, 9, 4), C(1, 1, 1), C(13, 11, 5), C(0, 0, 0)};
    const uint16_t lamp[] = {C(0, 0, 0), C(12, 10, 4), C(15, 13, 6), C(8, 6, 2), C(14, 12, 5),
                             C(4, 3, 1),  C(10, 8, 3), C(6, 5, 2), C(15, 14, 8), C(3, 2, 1),
                             C(11, 9, 4), C(7, 6, 2), C(13, 11, 5), C(2, 2, 1), C(9, 7, 3), C(1, 1, 0)};

    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_BRICK, brick);
    setPal(vdp, PAL_WET, wet);
    setPal(vdp, PAL_MAN, man);
    setPal(vdp, PAL_FIRE, fire);
    setPal(vdp, PAL_EMBER, ember);
    setPal(vdp, PAL_IRON, iron);
    setPal(vdp, PAL_TARP, tarp);
    setPal(vdp, PAL_CAN, can);
    setPal(vdp, PAL_SMOKE, smoke);
    setPal(vdp, PAL_MOON, moon);
    setPal(vdp, PAL_SUN, sun);
    setPal(vdp, PAL_GOLD, gold);
    setPal(vdp, PAL_ALERT, alert);
    setPal(vdp, PAL_PIP, pip);
    setPal(vdp, PAL_LAMP, lamp);
    vdp.setFogColor(C(2, 2, 4));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);

    auto tileOf = [&](auto paint) {
        uint8_t px[64] = {};
        paint(px);
        int t = tiles.shared(px);
        vdp.loadTile(t, px);
        return t;
    };
    art.brick = tileOf([](uint8_t* px) {
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) {
                bool mortar = y == 0 || y == 7 || ((x + ((y < 4) ? 0 : 4)) % 8) == 0;
                px8(px, x, y, mortar ? 3 : ((x + y) % 5 == 0 ? 4 : 2));
            }
    });
    art.mortar = tileOf([](uint8_t* px) {
        for (int i = 0; i < 64; i++) px[i] = 3;
        for (int x = 0; x < 8; x++) px8(px, x, 3, 5);
    });
    art.wet = tileOf([](uint8_t* px) {
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) px8(px, x, y, ((x + y * 3) % 7 == 0) ? 6 : 2);
    });
    art.stripe = tileOf([](uint8_t* px) {
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) px8(px, x, y, (x == 0 || x == 7) ? 1 : 4);
    });
    art.dark = tileOf([](uint8_t* px) {
        for (int i = 0; i < 64; i++) px[i] = 4;
    });

    art.man[0] = gs::uploadMipped(vdp, manBmp(0));
    art.man[1] = gs::uploadMipped(vdp, manBmp(1));
    art.pot = gs::uploadMipped(vdp, potBmp());
    art.flame[0] = gs::uploadMipped(vdp, flameBmp(0));
    art.flame[1] = gs::uploadMipped(vdp, flameBmp(1));
    art.tarp = gs::uploadMipped(vdp, tarpBmp());
    art.can[0] = gs::uploadMipped(vdp, canBmp(0));
    art.can[1] = gs::uploadMipped(vdp, canBmp(1));
    art.spark = gs::uploadMipped(vdp, sparkBmp());
    art.moon = gs::uploadMipped(vdp, moonBmp());
}

}  // namespace alleydawn
