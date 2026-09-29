#include "art.h"

#include <cmath>
#include <initializer_list>

namespace kilo {
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

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap cabArt() {
    gs::Bitmap b(48, 56);
    b.ellipse(10, 44, 8, 8, 1);
    b.ellipse(38, 44, 8, 8, 1);
    b.ellipse(10, 44, 3, 3, 2);
    b.ellipse(38, 44, 3, 3, 2);
    b.rect(8, 28, 32, 12, 3);
    b.rect(12, 22, 24, 8, 4);
    b.rect(16, 24, 6, 4, 5);
    b.rect(26, 24, 6, 4, 5);
    b.poly({{6.f, 22.f}, {24.f, 6.f}, {42.f, 22.f}}, 6);
    b.rect(22, 8, 4, 10, 7);
    b.rect(14, 36, 20, 4, 8);
    b.line(10, 36, 10, 48, 2, 1.4f);
    b.line(38, 36, 38, 48, 2, 1.4f);
    b.outline(9, false);
    return b;
}

gs::Bitmap pullerArt() {
    gs::Bitmap b(20, 36);
    b.ellipse(10, 6, 4, 4, 1);
    b.rect(7, 10, 6, 12, 2);
    b.line(8, 14, 2, 22, 1, 1.6f);
    b.line(12, 14, 18, 22, 1, 1.6f);
    b.line(8, 22, 5, 34, 3, 1.8f);
    b.line(12, 22, 15, 34, 3, 1.8f);
    b.rect(2, 20, 16, 2, 4);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 16, 14, 14, 1);
    b.ellipse(16, 16, 10, 10, 2);
    b.ellipse(16, 16, 3, 3, 3);
    for (int i = 0; i < 6; i++) {
        float a = i * 1.0472f;
        float c = std::cos(a), s = std::sin(a);
        b.line(16 - c * 3, 16 - s * 3, 16 + c * 12, 16 + s * 12, 4, 1.3f);
    }
    b.ellipse(16, 16, 14, 14, 5);
    return b;
}

gs::Bitmap cartArt() {
    gs::Bitmap b(40, 28);
    b.rect(4, 6, 32, 12, 1);
    b.poly({{4.f, 6.f}, {20.f, 1.f}, {36.f, 6.f}}, 2);
    b.ellipse(10, 22, 5, 5, 3);
    b.ellipse(30, 22, 5, 5, 3);
    b.rect(8, 9, 8, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap stallArt() {
    gs::Bitmap b(28, 32);
    b.rect(4, 14, 20, 16, 1);
    b.poly({{2.f, 14.f}, {14.f, 4.f}, {26.f, 14.f}}, 2);
    b.rect(10, 20, 8, 10, 3);
    b.rect(6, 16, 5, 4, 4);
    b.rect(17, 16, 5, 4, 4);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(24, 36);
    b.rect(10, 18, 4, 16, 1);
    b.ellipse(12, 12, 10, 10, 2);
    b.ellipse(8, 14, 5, 4, 3);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 8));

    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 8), gs::rgb4(13, 8, 3), gs::rgb4(10, 4, 2), gs::rgb4(6, 10, 13),
            gs::rgb4(14, 3, 2), gs::rgb4(12, 10, 3), gs::rgb4(9, 6, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(7, 7, 8), gs::rgb4(6, 8, 11), gs::rgb4(4, 6, 9), gs::rgb4(10, 12, 14),
            gs::rgb4(3, 5, 8), gs::rgb4(12, 11, 6), gs::rgb4(5, 6, 7), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_WHEEL,
           {0, gs::rgb4(3, 3, 3), gs::rgb4(9, 8, 7), gs::rgb4(12, 10, 4), gs::rgb4(14, 13, 11), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CART,
           {0, gs::rgb4(10, 6, 3), gs::rgb4(13, 8, 4), gs::rgb4(2, 2, 2), gs::rgb4(14, 12, 6), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_STALL,
           {0, gs::rgb4(12, 10, 7), gs::rgb4(13, 4, 3), gs::rgb4(6, 4, 3), gs::rgb4(8, 12, 14)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(6, 4, 2), gs::rgb4(2, 8, 3), gs::rgb4(4, 11, 4)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 13, 6)});

    // Road bank used by the road generator (indices documented on the chip).
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, gs::rgb4(4, 6, 3));
    vdp.setColor(PAL_ROAD * 16 + 1, gs::rgb4(5, 9, 3));
    vdp.setColor(PAL_ROAD * 16 + 2, gs::rgb4(3, 7, 2));
    vdp.setColor(PAL_ROAD * 16 + 3, gs::rgb4(7, 8, 4));
    vdp.setColor(PAL_ROAD * 16 + 4, gs::rgb4(8, 7, 4));
    vdp.setColor(PAL_ROAD * 16 + 5, gs::rgb4(6, 5, 3));
    vdp.setColor(PAL_ROAD * 16 + 6, gs::rgb4(5, 5, 5));
    vdp.setColor(PAL_ROAD * 16 + 7, gs::rgb4(7, 7, 7));
    vdp.setColor(PAL_ROAD * 16 + 8, gs::rgb4(9, 8, 6));
    vdp.setColor(PAL_ROAD * 16 + 9, gs::rgb4(4, 4, 4));
    vdp.setColor(PAL_ROAD * 16 + 10, gs::rgb4(10, 9, 6));
    vdp.setColor(PAL_ROAD * 16 + 14, gs::rgb4(14, 12, 4));
    vdp.setColor(PAL_ROAD * 16 + 15, gs::rgb4(8, 8, 8));

    art.cab = gs::uploadMipped(vdp, cabArt());
    art.rival = gs::uploadMipped(vdp, cabArt());
    art.puller = gs::uploadMipped(vdp, pullerArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.stall = gs::uploadMipped(vdp, stallArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    loadFont(vdp, art);
}

}  // namespace kilo
