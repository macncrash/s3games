#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace cliffkilo {
namespace {

void setPal(gs::VDP& v, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) {
        if (i < 16) v.setColor(pal * 16 + i, c);
        i++;
    }
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp, 1);
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
        art.font[c - 32] = t;
    }
}

gs::Bitmap muleArt() {
    gs::Bitmap b(88, 64);
    b.rect(10, 28, 68, 22, 2);
    b.rect(14, 24, 60, 10, 1);
    b.rect(18, 18, 22, 12, 3);
    b.rect(22, 20, 10, 6, 4);
    b.rect(48, 32, 16, 8, 5);
    b.rect(8, 46, 14, 12, 6);
    b.rect(66, 46, 14, 12, 6);
    b.rect(12, 50, 6, 6, 7);
    b.rect(70, 50, 6, 6, 7);
    b.line(16, 28, 72, 28, 8, 1);
    b.rect(36, 8, 6, 18, 3);
    b.rect(32, 6, 14, 5, 5);
    return b;
}

gs::Bitmap panArt() {
    gs::Bitmap b(40, 36);
    b.rect(6, 8, 28, 22, 1);
    b.rect(8, 10, 24, 16, 2);
    b.line(6, 8, 20, 2, 3, 2);
    b.line(34, 8, 20, 2, 3, 2);
    b.rect(10, 26, 20, 4, 4);
    b.ellipse(14, 18, 3, 3, 5);
    b.ellipse(26, 18, 3, 3, 5);
    return b;
}

gs::Bitmap sheaveArt() {
    gs::Bitmap b(64, 64);
    b.ellipse(32, 32, 28, 28, 1);
    b.ellipse(32, 32, 20, 20, 2);
    b.ellipse(32, 32, 8, 8, 3);
    b.ellipse(32, 32, 3, 3, 4);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785398f;
        float c = std::cos(a), s = std::sin(a);
        b.line(32 + c * 8, 32 + s * 8, 32 + c * 22, 32 + s * 22, 5, 2);
    }
    b.ellipse(32, 32, 28, 28, 0);
    b.ellipse(32, 32, 26, 26, 1);
    return b;
}

gs::Bitmap hubArt() {
    gs::Bitmap b(28, 16);
    b.rect(2, 4, 24, 8, 1);
    b.rect(0, 6, 6, 4, 2);
    b.rect(22, 6, 6, 4, 2);
    b.rect(10, 2, 8, 12, 3);
    return b;
}

gs::Bitmap cragArt() {
    gs::Bitmap b(48, 80);
    b.poly({{8, 78}, {18, 20}, {28, 36}, {40, 8}, {46, 78}}, 1);
    b.poly({{14, 78}, {22, 34}, {30, 48}, {38, 22}, {42, 78}}, 2);
    b.rect(16, 60, 10, 8, 3);
    b.rect(30, 50, 6, 6, 4);
    return b;
}

gs::Bitmap scrubArt() {
    gs::Bitmap b(36, 28);
    b.ellipse(10, 16, 8, 6, 1);
    b.ellipse(20, 12, 10, 8, 2);
    b.ellipse(28, 18, 7, 5, 1);
    b.rect(16, 18, 3, 8, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 72);
    b.rect(4, 4, 4, 66, 1);
    b.rect(2, 0, 8, 6, 2);
    b.rect(3, 64, 6, 8, 3);
    return b;
}

gs::Bitmap ribbonArt() {
    gs::Bitmap b(80, 16);
    b.rect(0, 4, 80, 8, 1);
    b.rect(0, 6, 80, 3, 2);
    for (int x = 6; x < 76; x += 14) b.rect(float(x), 4, 4, 8, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int p = 0; p < gs::NUM_PALETTES; p++)
        for (int i = 0; i < 16; i++) vdp.setColor(p * 16 + i, 0);

    auto ink = [&](int pal, uint16_t c) {
        vdp.setColor(pal * 16 + 1, c);
        vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
    };
    ink(PAL_HUD, gs::rgb4(14, 14, 13));
    ink(PAL_AMBER, gs::rgb4(15, 12, 3));
    ink(PAL_BAD, gs::rgb4(15, 4, 3));
    ink(PAL_GOOD, gs::rgb4(6, 14, 7));
    loadFont(vdp, art);

    setPal(vdp, PAL_MULE,
           {0, gs::rgb4(8, 7, 5), gs::rgb4(5, 5, 4), gs::rgb4(11, 9, 6), gs::rgb4(6, 10, 12),
            gs::rgb4(14, 8, 2), gs::rgb4(2, 2, 3), gs::rgb4(9, 9, 8), gs::rgb4(13, 12, 9)});
    setPal(vdp, PAL_PAN,
           {0, gs::rgb4(10, 6, 3), gs::rgb4(13, 9, 4), gs::rgb4(4, 3, 2), gs::rgb4(7, 5, 3), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_WHEEL,
           {0, gs::rgb4(3, 3, 4), gs::rgb4(7, 7, 8), gs::rgb4(12, 11, 8), gs::rgb4(15, 13, 6), gs::rgb4(10, 9, 7)});
    setPal(vdp, PAL_ROCK,
           {0, gs::rgb4(7, 6, 5), gs::rgb4(5, 4, 4), gs::rgb4(9, 8, 6), gs::rgb4(4, 5, 3),
            gs::rgb4(3, 6, 3), gs::rgb4(2, 4, 2), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(13, 13, 11), gs::rgb4(14, 4, 3), gs::rgb4(15, 12, 3), gs::rgb4(4, 4, 4)});

    auto roadPal = [&](int pal) {
        for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, gs::rgb4(5, 4, 3));
        vdp.setColor(pal * 16 + 0, 0);
        vdp.setColor(pal * 16 + 1, gs::rgb4(6, 5, 3));
        vdp.setColor(pal * 16 + 2, gs::rgb4(4, 3, 2));
        vdp.setColor(pal * 16 + 3, gs::rgb4(8, 7, 5));
        vdp.setColor(pal * 16 + 4, gs::rgb4(7, 6, 3));
        vdp.setColor(pal * 16 + 5, gs::rgb4(5, 4, 2));
        vdp.setColor(pal * 16 + 6, gs::rgb4(6, 5, 4));
        vdp.setColor(pal * 16 + 7, gs::rgb4(4, 4, 3));
        vdp.setColor(pal * 16 + 8, gs::rgb4(8, 7, 6));
        vdp.setColor(pal * 16 + 9, gs::rgb4(3, 3, 3));
        vdp.setColor(pal * 16 + 10, gs::rgb4(9, 8, 6));
        vdp.setColor(pal * 16 + 11, gs::rgb4(2, 6, 8));
        vdp.setColor(pal * 16 + 12, gs::rgb4(1, 4, 7));
        vdp.setColor(pal * 16 + 13, gs::rgb4(3, 8, 9));
        vdp.setColor(pal * 16 + 14, gs::rgb4(12, 10, 4));
        vdp.setColor(pal * 16 + 15, gs::rgb4(9, 8, 7));
    };
    roadPal(PAL_ROAD);
    vdp.setFogColor(gs::rgb4(8, 7, 6));

    art.mule = gs::uploadMipped(vdp, muleArt());
    art.pan = gs::uploadMipped(vdp, panArt());
    art.sheave = gs::uploadMipped(vdp, sheaveArt());
    art.hub = gs::uploadMipped(vdp, hubArt());
    art.crag = gs::uploadMipped(vdp, cragArt());
    art.scrub = gs::uploadMipped(vdp, scrubArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.ribbon = gs::uploadMipped(vdp, ribbonArt());
}

}  // namespace cliffkilo
