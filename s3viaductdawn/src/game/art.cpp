#include "game/art.h"

namespace viaduct {
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
        a.font[c - 32] = tiles.shared(px);
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap manBmp(int step) {
    gs::Bitmap b(18, 30);
    b.rect(6, 1, 6, 3, 8);
    b.ellipse(9, 7, 3.2f, 3.4f, 4);
    b.rect(7, 10, 4, 2, 5);
    b.rect(5, 12, 8, 9, 2);
    b.rect(6, 13, 6, 5, 3);
    b.rect(3, 13, 3, 8, 2);
    b.rect(12, 13, 3, 8, 6);
    int kick = step ? 2 : 0;
    b.rect(5, 21, 3, 7 - kick, 7);
    b.rect(10, 21 + kick, 3, 7 - kick, 7);
    b.rect(4, 27 - kick, 4, 2, 1);
    b.rect(9, 27, 4, 2, 1);
    b.set(7, 6, 9);
    b.set(11, 6, 9);
    return b;
}

gs::Bitmap basketBmp() {
    gs::Bitmap b(14, 12);
    b.rect(2, 3, 10, 7, 2);
    b.rect(3, 4, 8, 5, 3);
    b.rect(1, 2, 12, 2, 4);
    b.line(3, 4, 3, 9, 1, 1);
    b.line(10, 4, 10, 9, 1, 1);
    b.rect(4, 10, 6, 2, 1);
    return b;
}

gs::Bitmap flameBmp(int flick) {
    gs::Bitmap b(10, 16);
    b.ellipse(5, 10, 3.2f, 4.2f, 2);
    b.ellipse(5, 7, 2.2f, 4.f + flick, 3);
    b.ellipse(5, 5, 1.2f, 2.4f, 4);
    b.set(5, 3, 5);
    if (flick) b.set(6, 8, 4);
    return b;
}

gs::Bitmap caskBmp() {
    gs::Bitmap b(16, 18);
    b.ellipse(8, 9, 6.f, 7.f, 2);
    b.ellipse(8, 9, 4.2f, 5.5f, 3);
    b.rect(3, 4, 10, 2, 4);
    b.rect(3, 12, 10, 2, 4);
    b.rect(7, 1, 2, 3, 5);
    return b;
}

gs::Bitmap pierBmp() {
    gs::Bitmap b(16, 56);
    b.rect(2, 0, 12, 56, 2);
    b.rect(4, 2, 8, 52, 3);
    for (int y = 6; y < 54; y += 8) b.rect(3, y, 10, 1, 1);
    b.rect(1, 0, 14, 3, 4);
    return b;
}

gs::Bitmap spanBmp() {
    gs::Bitmap b(56, 28);
    b.rect(0, 0, 56, 8, 2);
    b.rect(0, 8, 8, 20, 3);
    b.rect(48, 8, 8, 20, 3);
    b.ellipse(28, 22, 18.f, 14.f, 2);
    b.ellipse(28, 24, 14.f, 12.f, 0);
    for (int x = 0; x < 56; x += 8) b.rect(x, 0, 1, 8, 1);
    return b;
}

gs::Bitmap sparkBmp() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(1, 1, 1);
    b.set(2, 1, 2);
    b.set(3, 1, 1);
    b.set(2, 2, 1);
    b.set(0, 2, 2);
    b.set(4, 2, 2);
    return b;
}

gs::Bitmap moonBmp() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6.f, 6.f, 2);
    b.ellipse(9, 6, 4.5f, 4.5f, 0);
    b.set(4, 5, 3);
    b.set(5, 9, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[16] = {0, C(15, 14, 11), C(8, 8, 8)};
    uint16_t stone[16] = {};
    stone[1] = C(3, 3, 4);
    stone[2] = C(7, 7, 8);
    stone[3] = C(9, 9, 10);
    stone[4] = C(5, 5, 6);
    uint16_t man[16] = {};
    man[1] = C(2, 2, 2);
    man[2] = C(4, 5, 7);
    man[3] = C(6, 8, 11);
    man[4] = C(12, 9, 7);
    man[5] = C(8, 4, 3);
    man[6] = C(14, 10, 4);
    man[7] = C(3, 3, 4);
    man[8] = C(2, 2, 3);
    man[9] = C(1, 1, 1);
    uint16_t fire[16] = {};
    fire[2] = C(12, 3, 1);
    fire[3] = C(15, 8, 1);
    fire[4] = C(15, 13, 4);
    fire[5] = C(15, 15, 12);
    uint16_t ember[16] = {};
    ember[2] = C(6, 1, 1);
    ember[3] = C(10, 3, 1);
    ember[4] = C(13, 6, 2);
    ember[5] = C(14, 10, 4);
    uint16_t iron[16] = {};
    iron[1] = C(2, 2, 3);
    iron[2] = C(5, 5, 6);
    iron[3] = C(8, 8, 9);
    iron[4] = C(11, 10, 8);
    uint16_t cask[16] = {};
    cask[2] = C(6, 3, 1);
    cask[3] = C(9, 5, 2);
    cask[4] = C(4, 3, 2);
    cask[5] = C(12, 10, 4);
    uint16_t moon[16] = {};
    moon[2] = C(13, 13, 11);
    moon[3] = C(8, 8, 7);
    uint16_t gold[16] = {};
    gold[1] = C(15, 12, 4);
    gold[15] = C(4, 2, 0);
    uint16_t water[16] = {};
    water[11] = C(1, 3, 6);
    water[12] = C(2, 5, 8);
    water[13] = C(6, 10, 12);
    uint16_t alert[16] = {};
    alert[2] = C(10, 1, 1);
    alert[3] = C(14, 4, 2);
    alert[4] = C(15, 10, 3);
    alert[5] = C(15, 14, 8);
    uint16_t spark[16] = {};
    spark[1] = C(15, 12, 5);
    spark[2] = C(15, 15, 12);
    uint16_t pier[16] = {};
    pier[1] = C(2, 2, 3);
    pier[2] = C(6, 6, 7);
    pier[3] = C(8, 8, 9);
    pier[4] = C(4, 4, 5);
    uint16_t hint[16] = {};
    hint[1] = C(11, 14, 12);
    hint[15] = C(1, 2, 2);

    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_STONE, stone);
    setPal(vdp, PAL_MAN, man);
    setPal(vdp, PAL_FIRE, fire);
    setPal(vdp, PAL_EMBER, ember);
    setPal(vdp, PAL_IRON, iron);
    setPal(vdp, PAL_CASK, cask);
    setPal(vdp, PAL_MOON, moon);
    setPal(vdp, PAL_GOLD, gold);
    setPal(vdp, PAL_WATER, water);
    setPal(vdp, PAL_ALERT, alert);
    setPal(vdp, PAL_SPARK, spark);
    setPal(vdp, PAL_PIER, pier);
    setPal(vdp, PAL_HINT, hint);
    vdp.setFogColor(C(1, 1, 3));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);

    art.man[0] = gs::uploadMipped(vdp, manBmp(0));
    art.man[1] = gs::uploadMipped(vdp, manBmp(1));
    art.basket = gs::uploadMipped(vdp, basketBmp());
    art.flame[0] = gs::uploadMipped(vdp, flameBmp(0));
    art.flame[1] = gs::uploadMipped(vdp, flameBmp(1));
    art.cask = gs::uploadMipped(vdp, caskBmp());
    art.pier = gs::uploadMipped(vdp, pierBmp());
    art.span = gs::uploadMipped(vdp, spanBmp());
    art.spark = gs::uploadMipped(vdp, sparkBmp());
    art.moon = gs::uploadMipped(vdp, moonBmp());

    uint8_t deck[64] = {};
    uint8_t joint[64] = {};
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = ((x + y) & 3) == 0 ? 1 : 2;
            if (y == 0) c = 4;
            if (y > 5) c = 3;
            deck[y * 8 + x] = uint8_t(c);
            joint[y * 8 + x] = uint8_t(x == 0 ? 1 : c);
        }
    art.deck = tiles.shared(deck);
    art.joint = tiles.shared(joint);
}

}  // namespace viaduct
