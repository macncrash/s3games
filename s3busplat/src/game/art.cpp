#include "game/art.h"

#include <initializer_list>

namespace busplat {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
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

gs::Bitmap busArt() {
    gs::Bitmap b(148, 42);
    b.rect(6, 14, 136, 18, 1);
    b.rect(8, 6, 118, 10, 2);
    b.rect(128, 8, 12, 8, 2);
    for (int i = 0; i < 6; i++) b.rect(12.f + i * 18.f, 8, 12, 7, 3);
    b.rect(122, 8, 8, 7, 3);
    b.rect(86, 16, 10, 16, 4);
    b.rect(88, 18, 6, 8, 5);
    b.rect(10, 18, 14, 6, 6);
    b.rect(4, 16, 6, 6, 7);
    b.rect(140, 18, 6, 8, 7);
    b.ellipse(28, 34, 7, 7, 8);
    b.ellipse(118, 34, 7, 7, 8);
    b.ellipse(28, 34, 3, 3, 9);
    b.ellipse(118, 34, 3, 3, 9);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 1);
    b.ellipse(8, 8, 3, 3, 2);
    b.rect(7, 2, 2, 12, 3);
    b.rect(2, 7, 12, 2, 3);
    return b;
}

gs::Bitmap platArt() {
    gs::Bitmap b(160, 52);
    b.rect(0, 16, 160, 22, 1);
    b.rect(0, 12, 160, 5, 2);
    b.rect(0, 38, 160, 14, 3);
    for (int i = 0; i < 9; i++) b.rect(6.f + i * 17.f, 20, 10, 12, (i & 1) ? 4 : 5);
    b.rect(0, 0, 5, 52, 6);
    b.rect(155, 0, 5, 52, 6);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(10, 22);
    b.rect(2, 0, 6, 22, 1);
    b.rect(3, 2, 4, 4, 4);
    b.rect(3, 12, 4, 4, 4);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(110, 36);
    b.rect(4, 12, 102, 14, 1);
    b.rect(8, 4, 80, 9, 2);
    for (int i = 0; i < 4; i++) b.rect(12.f + i * 18.f, 6, 12, 6, 3);
    b.rect(70, 14, 8, 12, 4);
    b.ellipse(22, 28, 5, 5, 5);
    b.ellipse(86, 28, 5, 5, 5);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 40);
    b.rect(5, 12, 2, 22, 1);
    b.ellipse(6, 7, 5, 5, 2);
    b.rect(2, 34, 8, 4, 3);
    return b;
}

gs::Bitmap signArt() {
    gs::TextStyle st{2, 1, 0, 0, 1};
    gs::Bitmap word = gs::textBitmap("BAY", st);
    gs::Bitmap b(word.w + 14, word.h + 12);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.rect(3, 3, float(b.w - 6), float(b.h - 6), 3);
    b.blit(word, 7, 6);
    return b;
}

gs::Bitmap shelterArt() {
    gs::Bitmap b(54, 28);
    b.rect(2, 8, 50, 4, 1);
    b.rect(6, 12, 3, 16, 2);
    b.rect(45, 12, 3, 16, 2);
    b.rect(10, 14, 32, 10, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 8));
    loadFont(vdp, art);

    setPal(vdp, PAL_BUS,
           {0, gs::rgb4(13, 11, 3), gs::rgb4(4, 5, 9), gs::rgb4(9, 13, 15), gs::rgb4(2, 2, 3), gs::rgb4(15, 14, 8),
            gs::rgb4(15, 12, 2), gs::rgb4(12, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 7)});
    setPal(vdp, PAL_PLAT,
           {0, gs::rgb4(7, 7, 8), gs::rgb4(12, 12, 11), gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 7), gs::rgb4(5, 6, 6),
            gs::rgb4(13, 11, 4)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(11, 3, 3), gs::rgb4(6, 2, 2), gs::rgb4(8, 12, 14), gs::rgb4(15, 12, 3), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_TOWN, {0, gs::rgb4(5, 5, 6), gs::rgb4(15, 13, 5), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 4, 8), gs::rgb4(15, 13, 3), gs::rgb4(15, 4, 3)});

    art.bus = gs::uploadMipped(vdp, busArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.plat = gs::uploadMipped(vdp, platArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.shelter = gs::uploadMipped(vdp, shelterArt());
}

}  // namespace busplat
