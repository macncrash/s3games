#include "game/art.h"

namespace lotdawn {
namespace {

uint16_t C(int r, int g, int b) { return gs::rgb4(r, g, b); }

void setPal(gs::VDP& vdp, int pal, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
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
    b.rect(5, 1, 6, 2, 8);
    b.ellipse(8, 6, 3.1f, 3.2f, 4);
    b.rect(6, 9, 4, 2, 5);
    b.rect(4, 11, 8, 8, 2);
    b.rect(5, 12, 6, 5, 3);
    b.rect(2, 12, 3, 7, 2);
    b.rect(11, 12, 3, 7, 2);
    int kick = step ? 2 : 0;
    b.rect(5, 19, 3, 7 - kick, 6);
    b.rect(9, 19 + kick, 3, 7 - kick, 6);
    b.rect(4, 25 - kick, 4, 2, 1);
    b.rect(8, 25, 4, 2, 1);
    b.set(6, 5, 7);
    b.set(10, 5, 7);
    return b;
}

gs::Bitmap potBmp() {
    gs::Bitmap b(12, 14);
    b.rect(2, 5, 8, 7, 2);
    b.rect(1, 4, 10, 2, 3);
    b.rect(3, 7, 6, 3, 4);
    b.rect(3, 12, 2, 2, 1);
    b.rect(7, 12, 2, 2, 1);
    return b;
}

gs::Bitmap flameBmp(int flick) {
    gs::Bitmap b(10, 16);
    b.ellipse(5, 11, 3.1f, 4.f, 2);
    b.ellipse(5, 8, 2.1f, 3.8f, 3);
    b.ellipse(5 + flick, 5, 1.1f, 2.4f, 4);
    b.set(5, 2, 5);
    return b;
}

gs::Bitmap carBmp(int kind) {
    gs::Bitmap b(36, 16);
    int body = kind ? 3 : 2;
    b.rect(2, 8, 32, 5, body);
    b.poly({{6, 8}, {10, 3}, {24, 3}, {28, 8}}, 4);
    b.rect(11, 4, 6, 3, 5);
    b.rect(18, 4, 5, 3, 5);
    b.ellipse(9, 13, 3.2f, 3.2f, 1);
    b.ellipse(27, 13, 3.2f, 3.2f, 1);
    b.ellipse(9, 13, 1.4f, 1.4f, 6);
    b.ellipse(27, 13, 1.4f, 1.4f, 6);
    b.rect(31, 9, 3, 2, 7);
    b.rect(1, 9, 2, 2, 8);
    return b;
}

gs::Bitmap drumBmp() {
    gs::Bitmap b(16, 20);
    b.ellipse(8, 5, 6, 3, 3);
    b.rect(2, 5, 12, 11, 2);
    b.ellipse(8, 16, 6, 3, 4);
    b.rect(5, 8, 6, 4, 5);
    b.rect(4, 1, 2, 4, 1);
    b.rect(10, 1, 2, 4, 1);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(10, 28);
    b.rect(4, 8, 2, 18, 2);
    b.rect(2, 2, 6, 6, 3);
    b.rect(3, 3, 4, 3, 4);
    b.rect(3, 25, 4, 2, 1);
    return b;
}

gs::Bitmap sparkBmp() {
    gs::Bitmap b(4, 4);
    b.set(1, 0, 1);
    b.set(2, 1, 2);
    b.set(1, 1, 1);
    b.set(0, 1, 2);
    b.set(1, 2, 1);
    return b;
}

gs::Bitmap moonBmp() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 2);
    b.ellipse(9, 5, 4.2f, 4.2f, 0);
    b.set(4, 5, 3);
    return b;
}

void solid(uint8_t* t, int c) {
    for (int i = 0; i < 64; i++) t[i] = uint8_t(c);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {C(0, 0, 0), C(14, 13, 9), C(7, 6, 5), C(3, 3, 3), C(15, 12, 3),
                            C(15, 7, 2), C(2, 2, 3),  C(12, 5, 2), C(5, 7, 3),  C(3, 4, 7),
                            C(10, 10, 12), C(15, 14, 8), C(8, 6, 3), C(15, 15, 11), C(4, 4, 4), C(1, 1, 2)};
    const uint16_t lot[] = {C(0, 0, 0), C(2, 2, 2), C(3, 3, 3), C(5, 5, 4), C(14, 12, 3),
                            C(1, 1, 1),  C(4, 4, 4), C(6, 6, 5), C(8, 7, 4),  C(2, 2, 3),
                            C(9, 8, 5),  C(1, 1, 2), C(7, 6, 3), C(12, 10, 4), C(3, 3, 2), C(0, 0, 1)};
    const uint16_t car[] = {C(0, 0, 0), C(1, 1, 1), C(2, 4, 8), C(8, 2, 2), C(6, 8, 10),
                            C(10, 12, 14), C(4, 4, 4), C(14, 10, 2), C(12, 12, 8), C(3, 3, 4),
                            C(5, 2, 2), C(1, 2, 4), C(9, 9, 8), C(7, 3, 2), C(2, 2, 2), C(0, 0, 0)};
    const uint16_t man[] = {C(0, 0, 0), C(2, 1, 1), C(1, 2, 5), C(15, 10, 2), C(12, 8, 5),
                            C(10, 6, 4), C(2, 2, 3), C(1, 1, 1), C(3, 3, 4), C(8, 5, 3),
                            C(14, 12, 6), C(4, 3, 2), C(6, 4, 2), C(9, 7, 4), C(15, 14, 8), C(0, 0, 0)};
    const uint16_t fire[] = {C(0, 0, 0), C(6, 1, 0), C(12, 3, 0), C(15, 8, 1), C(15, 13, 3),
                             C(15, 15, 8), C(8, 2, 0), C(4, 1, 0), C(14, 6, 1), C(10, 4, 0),
                             C(15, 10, 2), C(7, 3, 0), C(13, 12, 4), C(3, 1, 0), C(15, 14, 6), C(0, 0, 0)};
    const uint16_t ember[] = {C(0, 0, 0), C(4, 1, 0), C(8, 2, 0), C(12, 4, 1), C(14, 8, 2),
                              C(15, 12, 4), C(6, 2, 0), C(2, 1, 0), C(10, 3, 0), C(7, 2, 1),
                              C(13, 6, 1), C(5, 1, 0), C(11, 5, 1), C(3, 1, 0), C(14, 10, 3), C(0, 0, 0)};
    const uint16_t iron[] = {C(0, 0, 0), C(2, 2, 2), C(5, 5, 5), C(8, 8, 7), C(4, 4, 4),
                             C(10, 6, 2), C(3, 3, 3), C(12, 10, 6), C(6, 6, 6), C(1, 1, 1),
                             C(9, 8, 6), C(7, 5, 3), C(11, 11, 10), C(4, 3, 2), C(13, 12, 8), C(0, 0, 0)};
    const uint16_t drum[] = {C(0, 0, 0), C(3, 2, 1), C(6, 4, 1), C(10, 7, 2), C(4, 3, 1),
                             C(14, 10, 2), C(8, 5, 1), C(2, 1, 0), C(12, 8, 2), C(5, 3, 1),
                             C(9, 6, 2), C(7, 4, 1), C(13, 9, 3), C(1, 1, 0), C(15, 12, 4), C(0, 0, 0)};
    const uint16_t lamp[] = {C(0, 0, 0), C(3, 3, 3), C(6, 6, 6), C(8, 7, 4), C(15, 14, 6),
                             C(12, 11, 5), C(2, 2, 2), C(10, 9, 6), C(4, 4, 3), C(14, 12, 4),
                             C(5, 5, 4), C(1, 1, 1), C(9, 8, 5), C(7, 6, 3), C(15, 15, 10), C(0, 0, 0)};
    const uint16_t smoke[] = {C(0, 0, 0), C(6, 6, 6), C(8, 8, 8), C(4, 4, 4), C(10, 10, 9),
                              C(3, 3, 3), C(7, 7, 6), C(5, 5, 5), C(9, 9, 8), C(2, 2, 2),
                              C(11, 11, 10), C(1, 1, 1), C(12, 11, 8), C(6, 5, 4), C(8, 7, 6), C(0, 0, 0)};
    const uint16_t moon[] = {C(0, 0, 0), C(8, 8, 10), C(14, 14, 12), C(10, 10, 8), C(6, 6, 8),
                             C(12, 12, 10), C(4, 4, 6), C(15, 15, 13), C(9, 9, 11), C(3, 3, 5),
                             C(7, 7, 8), C(11, 11, 12), C(13, 13, 11), C(5, 5, 6), C(2, 2, 4), C(0, 0, 0)};
    const uint16_t sign[] = {C(0, 0, 0), C(1, 3, 6), C(2, 5, 9), C(12, 10, 3), C(15, 13, 4),
                             C(4, 6, 8), C(8, 7, 3), C(3, 4, 6), C(10, 8, 2), C(6, 8, 10),
                             C(14, 12, 6), C(1, 2, 3), C(9, 9, 6), C(5, 4, 2), C(13, 11, 5), C(0, 0, 0)};
    const uint16_t gold[] = {C(0, 0, 0), C(15, 13, 4), C(12, 9, 2), C(8, 6, 1), C(15, 15, 8),
                             C(10, 7, 2), C(6, 4, 1), C(14, 11, 3), C(9, 8, 3), C(13, 10, 4),
                             C(7, 5, 1), C(11, 8, 2), C(15, 14, 6), C(5, 3, 1), C(14, 12, 5), C(1, 1, 0)};
    const uint16_t alert[] = {C(0, 0, 0), C(15, 3, 2), C(10, 1, 1), C(6, 1, 1), C(15, 8, 3),
                              C(12, 2, 1), C(4, 0, 0), C(15, 12, 6), C(8, 1, 1), C(14, 5, 2),
                              C(3, 0, 0), C(11, 3, 1), C(15, 6, 2), C(7, 2, 1), C(15, 15, 8), C(1, 0, 0)};
    const uint16_t pip[] = {C(0, 0, 0), C(9, 10, 8), C(5, 6, 5), C(3, 4, 3), C(12, 13, 8),
                            C(7, 8, 6), C(2, 3, 2), C(14, 14, 10), C(6, 7, 5), C(4, 5, 4),
                            C(11, 12, 8), C(1, 2, 1), C(8, 9, 6), C(10, 10, 7), C(13, 13, 9), C(2, 2, 1)};
    const uint16_t vest[] = {C(0, 0, 0), C(2, 2, 2), C(1, 2, 4), C(15, 9, 1), C(13, 8, 5),
                             C(11, 7, 4), C(3, 3, 4), C(1, 1, 2), C(4, 3, 2), C(14, 11, 3),
                             C(8, 6, 2), C(6, 4, 2), C(15, 12, 4), C(5, 4, 3), C(10, 8, 3), C(0, 0, 0)};

    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_LOT, lot);
    setPal(vdp, PAL_CAR, car);
    setPal(vdp, PAL_MAN, man);
    setPal(vdp, PAL_FIRE, fire);
    setPal(vdp, PAL_EMBER, ember);
    setPal(vdp, PAL_IRON, iron);
    setPal(vdp, PAL_DRUM, drum);
    setPal(vdp, PAL_LAMP, lamp);
    setPal(vdp, PAL_SMOKE, smoke);
    setPal(vdp, PAL_MOON, moon);
    setPal(vdp, PAL_SIGN, sign);
    setPal(vdp, PAL_GOLD, gold);
    setPal(vdp, PAL_ALERT, alert);
    setPal(vdp, PAL_PIP, pip);
    setPal(vdp, PAL_VEST, vest);
    vdp.setFogColor(C(2, 2, 4));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);

    uint8_t asph[64], stall[64], curb[64], dark[64];
    solid(asph, 2);
    for (int i = 0; i < 64; i++) {
        if ((i * 17 + 3) % 11 == 0) asph[i] = 3;
        if ((i * 5 + 1) % 13 == 0) asph[i] = 1;
    }
    solid(stall, 2);
    for (int y = 0; y < 8; y++) stall[y * 8 + 3] = 4;
    solid(curb, 6);
    for (int x = 0; x < 8; x++) curb[x] = 8;
    solid(dark, 1);
    art.asphalt = tiles.shared(asph);
    art.stall = tiles.shared(stall);
    art.curb = tiles.shared(curb);
    art.dark = tiles.shared(dark);
    vdp.loadTile(art.asphalt, asph);
    vdp.loadTile(art.stall, stall);
    vdp.loadTile(art.curb, curb);
    vdp.loadTile(art.dark, dark);

    art.man[0] = gs::uploadMipped(vdp, manBmp(0));
    art.man[1] = gs::uploadMipped(vdp, manBmp(1));
    art.pot = gs::uploadMipped(vdp, potBmp());
    art.flame[0] = gs::uploadMipped(vdp, flameBmp(0));
    art.flame[1] = gs::uploadMipped(vdp, flameBmp(-1));
    art.car[0] = gs::uploadMipped(vdp, carBmp(0));
    art.car[1] = gs::uploadMipped(vdp, carBmp(1));
    art.drum = gs::uploadMipped(vdp, drumBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.spark = gs::uploadMipped(vdp, sparkBmp());
    art.moon = gs::uploadMipped(vdp, moonBmp());
}

}  // namespace lotdawn
