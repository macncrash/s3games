#include "game/art.h"

#include <initializer_list>

namespace cabplat {
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

gs::Bitmap cabArt() {
    gs::Bitmap b(96, 40);
    b.rect(4, 16, 88, 16, 1);
    b.poly({{18, 16}, {28, 4}, {62, 4}, {74, 16}}, 2);
    b.rect(30, 6, 14, 9, 3);
    b.rect(48, 6, 14, 9, 3);
    b.rect(8, 18, 16, 10, 4);
    b.rect(70, 18, 16, 8, 5);
    b.ellipse(22, 32, 7, 7, 6);
    b.ellipse(74, 32, 7, 7, 6);
    b.ellipse(22, 32, 3, 3, 7);
    b.ellipse(74, 32, 3, 3, 7);
    b.rect(40, 18, 3, 14, 8);
    b.rect(0, 22, 6, 4, 5);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 4, 4, 2);
    b.rect(8, 2, 2, 14, 3);
    b.rect(2, 8, 14, 2, 3);
    return b;
}

gs::Bitmap platArt() {
    gs::Bitmap b(120, 48);
    b.rect(0, 10, 120, 28, 1);
    b.rect(0, 6, 120, 6, 2);
    b.rect(0, 38, 120, 10, 3);
    for (int i = 0; i < 8; i++) b.rect(4.f + i * 14.f, 16, 8, 16, (i & 1) ? 4 : 5);
    b.rect(0, 0, 4, 48, 6);
    b.rect(116, 0, 4, 48, 6);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(8, 20);
    b.rect(1, 0, 6, 20, 1);
    b.rect(2, 2, 4, 4, 2);
    b.rect(2, 10, 4, 4, 2);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(72, 32);
    b.rect(4, 14, 64, 12, 1);
    b.poly({{14, 14}, {22, 4}, {48, 4}, {56, 14}}, 2);
    b.rect(24, 6, 10, 7, 3);
    b.rect(38, 6, 10, 7, 3);
    b.ellipse(16, 26, 5, 5, 4);
    b.ellipse(54, 26, 5, 5, 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 36);
    b.rect(5, 10, 2, 22, 1);
    b.ellipse(6, 6, 5, 5, 2);
    b.rect(2, 30, 8, 4, 3);
    return b;
}

gs::Bitmap signArt() {
    gs::TextStyle st{2, 1, 0, 0, 1};
    gs::Bitmap word = gs::textBitmap("PLAT", st);
    gs::Bitmap b(word.w + 12, word.h + 10);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.rect(3, 3, float(b.w - 6), float(b.h - 6), 3);
    b.blit(word, 6, 5);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 8));
    loadFont(vdp, art);

    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(14, 11, 2), gs::rgb4(12, 9, 2), gs::rgb4(8, 13, 15), gs::rgb4(4, 3, 2), gs::rgb4(15, 6, 2),
            gs::rgb4(2, 2, 2), gs::rgb4(9, 9, 8), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_PLAT,
           {0, gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 10), gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 6),
            gs::rgb4(13, 12, 4)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(10, 3, 3), gs::rgb4(13, 5, 4), gs::rgb4(6, 10, 12), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_TOWN, {0, gs::rgb4(5, 5, 6), gs::rgb4(15, 13, 5), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(2, 2, 2), gs::rgb4(3, 5, 8), gs::rgb4(14, 12, 3), gs::rgb4(15, 4, 3)});

    art.cab = gs::uploadMipped(vdp, cabArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.plat = gs::uploadMipped(vdp, platArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.sign = gs::uploadMipped(vdp, signArt());
}

}  // namespace cabplat
