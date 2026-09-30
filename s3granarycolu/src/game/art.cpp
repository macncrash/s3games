#include "game/art.h"

#include <string>

namespace granary {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; ++i) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 1, 1));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap cartArt() {
    gs::Bitmap b(44, 30);
    b.rect(6, 6, 30, 12, 2);
    b.rect(8, 8, 12, 6, 4);
    b.rect(22, 8, 12, 6, 5);
    b.rect(4, 16, 34, 4, 3);
    b.ellipse(12, 22, 4, 4, 8);
    b.ellipse(32, 22, 4, 4, 8);
    b.rect(10, 4, 8, 4, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap bikeArt() {
    gs::Bitmap b(36, 28);
    b.ellipse(8, 18, 6, 6, 2);
    b.ellipse(26, 18, 6, 6, 2);
    b.ellipse(8, 18, 2, 2, 4);
    b.ellipse(26, 18, 2, 2, 4);
    b.line(8, 18, 18, 8, 3, 2);
    b.line(18, 8, 26, 18, 3, 2);
    b.line(14, 10, 22, 10, 1, 2);
    b.rect(16, 4, 3, 6, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap wagonArt() {
    gs::Bitmap b(52, 40);
    b.rect(8, 4, 36, 16, 2);
    b.rect(10, 6, 32, 4, 1);
    for (int x = 12; x < 40; x += 8) b.ellipse(x, 14, 3, 4, 6);
    b.rect(6, 18, 40, 6, 3);
    b.ellipse(14, 28, 5, 5, 8);
    b.ellipse(38, 28, 5, 5, 8);
    b.rect(4, 20, 4, 10, 4);
    b.rect(44, 20, 4, 10, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap barnArt() {
    gs::Bitmap b(48, 64);
    b.poly({{4, 28}, {24, 6}, {44, 28}}, 3);
    b.rect(6, 28, 36, 32, 2);
    b.rect(18, 40, 12, 20, 5);
    b.rect(10, 34, 8, 8, 4);
    b.rect(30, 34, 8, 8, 4);
    b.rect(22, 8, 4, 10, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap siloArt() {
    gs::Bitmap b(28, 72);
    b.ellipse(14, 10, 10, 8, 2);
    b.rect(4, 12, 20, 52, 1);
    b.rect(6, 14, 4, 48, 3);
    for (int y = 20; y < 60; y += 10) b.rect(4, y, 20, 2, 4);
    b.rect(2, 60, 24, 8, 5);
    b.outline(15, false);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(22, 28);
    b.ellipse(11, 16, 8, 10, 2);
    b.ellipse(11, 8, 5, 4, 1);
    b.line(7, 8, 15, 8, 4, 1);
    b.rect(9, 18, 4, 3, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap wheatArt() {
    gs::Bitmap b(24, 40);
    b.rect(11, 16, 3, 20, 4);
    for (int i = 0; i < 5; ++i) {
        b.ellipse(12, 6 + i * 3, 3, 2, 1);
        b.ellipse(8, 8 + i * 3, 2, 2, 2);
        b.ellipse(16, 8 + i * 3, 2, 2, 3);
    }
    b.outline(15, false);
    return b;
}

gs::Bitmap chuteArt() {
    gs::Bitmap b(36, 16);
    b.rect(2, 4, 32, 8, 2);
    b.rect(4, 6, 28, 2, 1);
    b.rect(2, 10, 32, 2, 3);
    b.rect(30, 2, 4, 12, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(18, 12);
    b.ellipse(9, 6, 7, 4, 1);
    b.ellipse(5, 5, 3, 2, 2);
    b.ellipse(12, 7, 3, 2, 3);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(26, 8);
    b.ellipse(13, 4, 11, 3, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(15, 14, 10));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_ALERT, gs::rgb4(14, 3, 2));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 13, 4));

    setPal(vdp, PAL_CART,
           {0, gs::rgb4(14, 12, 8), gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(4, 6, 9), gs::rgb4(8, 10, 13),
            gs::rgb4(12, 6, 2), gs::rgb4(3, 2, 1), gs::rgb4(2, 2, 2), gs::rgb4(8, 6, 3), gs::rgb4(1, 1, 1),
            gs::rgb4(13, 9, 4), gs::rgb4(7, 5, 2), gs::rgb4(15, 13, 8), gs::rgb4(5, 4, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BIKE,
           {0, gs::rgb4(13, 13, 12), gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(1, 1, 1), gs::rgb4(12, 3, 2),
            gs::rgb4(8, 8, 8), gs::rgb4(10, 9, 7), gs::rgb4(2, 2, 2), gs::rgb4(14, 8, 3), gs::rgb4(5, 5, 5),
            gs::rgb4(9, 4, 2), gs::rgb4(4, 4, 4), gs::rgb4(15, 14, 10), gs::rgb4(7, 6, 5), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WAGON,
           {0, gs::rgb4(14, 12, 4), gs::rgb4(9, 6, 2), gs::rgb4(5, 3, 1), gs::rgb4(3, 2, 1), gs::rgb4(8, 5, 2),
            gs::rgb4(13, 10, 3), gs::rgb4(7, 5, 2), gs::rgb4(1, 1, 1), gs::rgb4(11, 8, 3), gs::rgb4(4, 3, 2),
            gs::rgb4(15, 13, 6), gs::rgb4(6, 4, 2), gs::rgb4(12, 7, 2), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BARN,
           {0, gs::rgb4(12, 5, 3), gs::rgb4(9, 3, 2), gs::rgb4(6, 2, 1), gs::rgb4(4, 6, 8), gs::rgb4(2, 2, 2),
            gs::rgb4(8, 7, 5), gs::rgb4(14, 10, 4), gs::rgb4(5, 3, 2), gs::rgb4(11, 6, 3), gs::rgb4(7, 4, 2),
            gs::rgb4(13, 8, 4), gs::rgb4(3, 2, 1), gs::rgb4(15, 12, 6), gs::rgb4(10, 7, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_SACK,
           {0, gs::rgb4(13, 10, 4), gs::rgb4(10, 7, 2), gs::rgb4(7, 5, 2), gs::rgb4(4, 3, 1), gs::rgb4(14, 11, 5),
            gs::rgb4(8, 6, 2), gs::rgb4(12, 8, 3), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), gs::rgb4(11, 8, 3),
            gs::rgb4(6, 4, 2), gs::rgb4(5, 3, 1), gs::rgb4(15, 13, 7), gs::rgb4(9, 6, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WHEAT,
           {0, gs::rgb4(14, 12, 3), gs::rgb4(11, 9, 2), gs::rgb4(8, 7, 2), gs::rgb4(6, 5, 2), gs::rgb4(4, 4, 1),
            gs::rgb4(13, 10, 3), gs::rgb4(9, 8, 3), gs::rgb4(3, 3, 1), gs::rgb4(15, 13, 5), gs::rgb4(7, 6, 2),
            gs::rgb4(5, 4, 1), gs::rgb4(12, 11, 4), gs::rgb4(10, 8, 2), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(13, 11, 7), gs::rgb4(10, 8, 5), gs::rgb4(7, 6, 4), gs::rgb4(15, 13, 8), gs::rgb4(5, 4, 3),
            gs::rgb4(12, 10, 6), gs::rgb4(8, 7, 4), gs::rgb4(3, 3, 2), gs::rgb4(14, 12, 8), gs::rgb4(6, 5, 3),
            gs::rgb4(4, 4, 3), gs::rgb4(15, 14, 10), gs::rgb4(9, 8, 5), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_SILO,
           {0, gs::rgb4(12, 12, 11), gs::rgb4(9, 9, 8), gs::rgb4(14, 14, 12), gs::rgb4(6, 6, 5), gs::rgb4(4, 4, 3),
            gs::rgb4(8, 7, 5), gs::rgb4(11, 10, 8), gs::rgb4(3, 3, 2), gs::rgb4(15, 15, 13), gs::rgb4(7, 7, 6),
            gs::rgb4(5, 5, 4), gs::rgb4(13, 12, 9), gs::rgb4(2, 2, 2), gs::rgb4(10, 9, 7), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CHUTE,
           {0, gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 4), gs::rgb4(3, 3, 2), gs::rgb4(11, 6, 2), gs::rgb4(7, 4, 2),
            gs::rgb4(13, 12, 8), gs::rgb4(2, 2, 1), gs::rgb4(9, 8, 6), gs::rgb4(4, 4, 3), gs::rgb4(12, 8, 3),
            gs::rgb4(6, 5, 3), gs::rgb4(14, 10, 4), gs::rgb4(1, 1, 1), gs::rgb4(10, 9, 7), gs::rgb4(1, 1, 1)});

    int r = PAL_ROAD * 16;
    vdp.setColor(r + 0, 0);
    vdp.setColor(r + 1, gs::rgb4(8, 7, 3));
    vdp.setColor(r + 2, gs::rgb4(5, 4, 2));
    vdp.setColor(r + 3, gs::rgb4(10, 8, 4));
    vdp.setColor(r + 4, gs::rgb4(7, 6, 3));
    vdp.setColor(r + 5, gs::rgb4(4, 3, 2));
    vdp.setColor(r + 6, gs::rgb4(3, 3, 2));
    vdp.setColor(r + 7, gs::rgb4(6, 5, 4));
    vdp.setColor(r + 8, gs::rgb4(9, 7, 4));
    vdp.setColor(r + 9, gs::rgb4(2, 2, 1));
    vdp.setColor(r + 10, gs::rgb4(4, 4, 3));
    vdp.setColor(r + 11, gs::rgb4(6, 7, 3));
    vdp.setColor(r + 12, gs::rgb4(9, 10, 4));
    vdp.setColor(r + 13, gs::rgb4(12, 11, 6));
    vdp.setColor(r + 14, gs::rgb4(14, 12, 5));
    vdp.setColor(r + 15, gs::rgb4(5, 4, 3));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.bike = gs::uploadMipped(vdp, bikeArt());
    art.wagon = gs::uploadMipped(vdp, wagonArt());
    art.barn = gs::uploadMipped(vdp, barnArt());
    art.silo = gs::uploadMipped(vdp, siloArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.wheat = gs::uploadMipped(vdp, wheatArt());
    art.chute = gs::uploadMipped(vdp, chuteArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
}

}  // namespace granary
