#include "game/art.h"

#include <cmath>

namespace luge {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap lugeArt(int lean) {
    Bitmap b(72, 96);
    const float dx = lean * 7.f;
    b.ellipse(36 + dx * 0.2f, 78, 22, 7, 3);
    b.poly({{18 + dx * 0.15f, 70}, {54 + dx * 0.15f, 70}, {58, 86}, {14, 86}}, 4);
    b.poly({{22 + dx * 0.2f, 66}, {50 + dx * 0.2f, 66}, {48, 78}, {24, 78}}, 2);
    b.line(16, 84, 20 + dx, 28, 5, 2.2f);
    b.line(56, 84, 52 + dx, 28, 5, 2.2f);
    b.ellipse(36 + dx, 30, 11, 12, 6);
    b.ellipse(36 + dx, 29, 7, 6, 7);
    b.ellipse(33 + dx, 28, 2, 2, 1);
    b.ellipse(40 + dx, 28, 2, 2, 1);
    b.poly({{24 + dx * 0.6f, 40}, {48 + dx * 0.6f, 40}, {52 + dx * 0.3f, 64}, {20 + dx * 0.3f, 64}}, 8);
    b.line(26 + dx, 44, 46 + dx, 44, 9, 1.4f);
    b.outline(1, false);
    return b;
}

Bitmap wheelArt(bool clock, bool sheave) {
    Bitmap b(48, 48);
    b.ellipse(24, 24, 20, 20, sheave ? 4 : 2);
    b.ellipse(24, 24, 15, 15, 1);
    if (clock) {
        b.ellipse(24, 24, 12, 12, 8);
        b.line(24, 24, 24, 14, 3, 1.6f);
        b.line(24, 24, 32, 26, 5, 1.6f);
        b.ellipse(24, 24, 2, 2, 3);
    } else if (sheave) {
        b.ellipse(24, 24, 11, 11, 3);
        b.ellipse(24, 24, 5, 5, 1);
    } else {
        for (int i = 0; i < 6; i++) {
            float a = float(i) * 1.0472f;
            b.line(24, 24, 24 + std::cos(a) * 14.f, 24 + std::sin(a) * 14.f, 5, 1.5f);
        }
        b.ellipse(24, 24, 3, 3, 6);
    }
    b.outline(1, false);
    return b;
}

Bitmap ghostArt() {
    Bitmap b(40, 56);
    b.ellipse(20, 46, 12, 4, 2);
    b.line(12, 48, 16, 18, 3, 1.6f);
    b.line(28, 48, 24, 18, 3, 1.6f);
    b.ellipse(20, 16, 6, 6, 4);
    b.poly({{14, 24}, {26, 24}, {28, 40}, {12, 40}}, 3);
    return b;
}

Bitmap puffArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 6, 4, 1);
    b.ellipse(6, 8, 3, 2, 2);
    b.ellipse(11, 7, 2, 2, 3);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        const int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 12, 6), gs::rgb4(8, 10, 12), gs::rgb4(15, 6, 4),
                          gs::rgb4(4, 14, 8), gs::rgb4(6, 8, 12), 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_ICE, {0, gs::rgb4(12, 14, 15), gs::rgb4(7, 10, 13), gs::rgb4(3, 5, 8)});
    setPal(vdp, PAL_LUGE, {0, gs::rgb4(2, 2, 3), gs::rgb4(12, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(14, 4, 3),
                           gs::rgb4(9, 9, 10), gs::rgb4(15, 8, 3), gs::rgb4(14, 12, 8), gs::rgb4(4, 6, 10),
                           gs::rgb4(8, 10, 13)});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(4, 5, 6), gs::rgb4(10, 11, 12), gs::rgb4(6, 7, 8), gs::rgb4(13, 14, 15),
                            gs::rgb4(8, 9, 10), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(3, 3, 4), gs::rgb4(12, 10, 4), gs::rgb4(15, 13, 6), gs::rgb4(8, 6, 2),
                            gs::rgb4(15, 15, 12), gs::rgb4(6, 5, 3), 0, gs::rgb4(14, 12, 8)});
    setPal(vdp, PAL_CART, {0, gs::rgb4(3, 3, 3), gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 4), gs::rgb4(12, 11, 8),
                           gs::rgb4(14, 8, 2), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_SHEAVE, {0, gs::rgb4(2, 3, 4), gs::rgb4(6, 8, 9), gs::rgb4(10, 12, 13), gs::rgb4(4, 6, 7),
                             gs::rgb4(14, 15, 15)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 15, 15), gs::rgb4(10, 12, 14), gs::rgb4(7, 9, 12)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(11, 13, 14), gs::rgb4(8, 10, 12), gs::rgb4(6, 8, 10), gs::rgb4(13, 14, 15),
            gs::rgb4(9, 11, 12), gs::rgb4(12, 14, 15), gs::rgb4(7, 9, 11), gs::rgb4(15, 15, 15),
            gs::rgb4(10, 13, 15), gs::rgb4(5, 7, 9), gs::rgb4(4, 8, 12), gs::rgb4(3, 6, 10),
            gs::rgb4(6, 10, 13), gs::rgb4(14, 15, 15), gs::rgb4(9, 12, 14)});

    art.luge[0] = gs::uploadMipped(vdp, lugeArt(-1));
    art.luge[1] = gs::uploadMipped(vdp, lugeArt(0));
    art.luge[2] = gs::uploadMipped(vdp, lugeArt(1));
    art.wheel = gs::uploadMipped(vdp, wheelArt(false, false));
    art.clock = gs::uploadMipped(vdp, wheelArt(true, false));
    art.sheave = gs::uploadMipped(vdp, wheelArt(false, true));
    art.ghost = gs::uploadMipped(vdp, ghostArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(8, 10, 12));
}

}  // namespace luge
