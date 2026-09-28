#include "game/art.h"

#include <cmath>

namespace tramkilo {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap tramArt() {
    gs::Bitmap b(96, 48);
    b.rect(6, 14, 84, 22, 1);
    b.rect(10, 8, 70, 10, 1);
    b.rect(8, 16, 80, 6, 2);
    b.rect(14, 10, 14, 8, 4);
    b.rect(32, 10, 14, 8, 4);
    b.rect(50, 10, 14, 8, 4);
    b.rect(68, 10, 10, 8, 4);
    b.rect(18, 24, 12, 8, 4);
    b.rect(40, 22, 16, 12, 8);
    b.rect(66, 24, 12, 8, 4);
    b.rect(4, 34, 88, 4, 3);
    b.ellipse(22, 38, 7, 7, 5);
    b.ellipse(22, 38, 3, 3, 6);
    b.ellipse(74, 38, 7, 7, 5);
    b.ellipse(74, 38, 3, 3, 6);
    b.line(28, 6, 48, 2, 7, 1.5f);
    b.line(48, 2, 68, 6, 7, 1.5f);
    b.rect(46, 1, 4, 4, 6);
    b.outline(5, false);
    return b;
}

gs::Bitmap wheelArt(int kind) {
    gs::Bitmap b(40, 40);
    int tyre = 1, hub = 2, spoke = 3, rim = 4;
    b.ellipse(20, 20, 16, 16, tyre);
    b.ellipse(20, 20, 12, 12, rim);
    b.ellipse(20, 20, 9, 9, 0);
    for (int i = 0; i < 6; i++) {
        float a = float(i) * 1.0472f;
        float x = 20 + std::cos(a) * 8.f;
        float y = 20 + std::sin(a) * 8.f;
        b.line(20, 20, x, y, spoke, 1.4f);
    }
    b.ellipse(20, 20, 4, 4, hub);
    if (kind == 1) {
        b.rect(6, 4, 28, 6, 5);
        b.rect(10, 0, 20, 5, 6);
    } else if (kind == 2) {
        b.rect(4, 30, 32, 5, 5);
        b.rect(17, 2, 6, 10, 6);
    }
    b.outline(7, false);
    return b;
}

gs::Bitmap brickArt() {
    gs::Bitmap b(40, 56);
    b.rect(2, 10, 36, 44, 1);
    b.poly({{2, 10}, {20, 2}, {38, 10}}, 2);
    for (int y = 16; y < 48; y += 12)
        for (int x = 6; x < 34; x += 10) b.rect(float(x), float(y), 6, 7, 3);
    b.rect(16, 38, 8, 14, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap stopArt() {
    gs::Bitmap b(28, 48);
    b.rect(12, 16, 4, 30, 1);
    b.rect(4, 4, 20, 14, 2);
    b.rect(7, 7, 14, 8, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(8, 64);
    b.rect(3, 0, 2, 64, 1);
    b.rect(0, 4, 8, 2, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 20);
    b.rect(6, 8, 4, 12, 1);
    b.ellipse(8, 6, 6, 5, 2);
    b.ellipse(8, 6, 3, 2, 3);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(32, 14);
    b.ellipse(10, 8, 8, 5, 1);
    b.ellipse(20, 7, 9, 6, 1);
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

    setPal(vdp, PAL_TRAM,
           {0, gs::rgb4(14, 13, 9), gs::rgb4(13, 3, 3), gs::rgb4(8, 2, 2), gs::rgb4(6, 10, 13), gs::rgb4(2, 2, 2),
            gs::rgb4(14, 12, 4), gs::rgb4(7, 8, 9), gs::rgb4(12, 14, 15)});
    setPal(vdp, PAL_LORRY,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 8), gs::rgb4(12, 12, 11), gs::rgb4(5, 5, 6), gs::rgb4(10, 4, 2),
            gs::rgb4(13, 8, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(3, 3, 3), gs::rgb4(11, 10, 6), gs::rgb4(14, 13, 8), gs::rgb4(6, 6, 5), gs::rgb4(4, 6, 8),
            gs::rgb4(12, 12, 10), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_OTHER,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(9, 3, 3), gs::rgb4(13, 12, 10), gs::rgb4(6, 7, 8), gs::rgb4(5, 5, 6),
            gs::rgb4(14, 12, 5), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BRICK,
           {0, gs::rgb4(10, 5, 3), gs::rgb4(6, 3, 3), gs::rgb4(12, 11, 6), gs::rgb4(4, 3, 3), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_STOP,
           {0, gs::rgb4(5, 5, 5), gs::rgb4(13, 3, 2), gs::rgb4(15, 14, 12), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_POLE, {0, gs::rgb4(4, 5, 6), gs::rgb4(8, 9, 10)});
    setPal(vdp, PAL_WIRE, {0, gs::rgb4(14, 14, 15), gs::rgb4(15, 14, 6), gs::rgb4(12, 12, 8)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(3, 7, 4), gs::rgb4(14, 12, 3), gs::rgb4(15, 15, 14)});

    art.tram = gs::uploadMipped(vdp, tramArt());
    art.wheel[0] = gs::uploadMipped(vdp, wheelArt(0));
    art.wheel[1] = gs::uploadMipped(vdp, wheelArt(1));
    art.wheel[2] = gs::uploadMipped(vdp, wheelArt(2));
    art.brick = gs::uploadMipped(vdp, brickArt());
    art.stop = gs::uploadMipped(vdp, stopArt());
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    loadFont(vdp, art);
}

}  // namespace tramkilo
