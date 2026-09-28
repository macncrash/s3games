#include "game/art.h"

#include <initializer_list>

namespace cranekilo {
namespace {

void textPal(gs::VDP& v, int pal, uint16_t ink) {
    v.setColor(pal * 16 + 1, ink);
    v.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

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

gs::Bitmap boomArt() {
    gs::Bitmap b(36, 96);
    for (int y = 4; y < 92; y++) {
        int inset = y / 14;
        b.rect(float(8 + inset), float(y), float(20 - inset * 2), 1, (y % 6 < 2) ? 2 : 1);
        b.set(10 + inset, y, 3);
        b.set(25 - inset, y, 3);
    }
    b.rect(6, 0, 24, 6, 4);
    b.rect(14, 88, 8, 8, 5);
    b.line(8, 8, 16, 90, 6, 1);
    b.line(27, 8, 19, 90, 6, 1);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(14, 28);
    b.rect(6, 0, 2, 14, 1);
    b.ellipse(7, 18, 5, 6, 2);
    b.ellipse(7, 17, 2, 3, 0);
    b.rect(4, 12, 6, 2, 3);
    return b;
}

gs::Bitmap dashArt() {
    gs::Bitmap b(120, 36);
    b.rect(0, 8, 120, 28, 2);
    b.rect(0, 8, 120, 4, 1);
    b.rect(46, 0, 28, 14, 4);
    b.rect(50, 3, 20, 7, 7);
    b.rect(8, 16, 22, 12, 3);
    b.rect(90, 16, 22, 12, 3);
    b.rect(54, 18, 12, 10, 6);
    b.rect(10, 28, 100, 4, 5);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 13, 13, 1);
    b.ellipse(14, 14, 9, 9, 2);
    b.ellipse(14, 14, 3, 3, 4);
    b.rect(13, 3, 2, 22, 3);
    b.rect(3, 13, 22, 2, 3);
    b.line(6, 6, 22, 22, 3, 1);
    b.line(6, 22, 22, 6, 3, 1);
    return b;
}

gs::Bitmap axleArt() {
    gs::Bitmap b(18, 10);
    b.rect(0, 3, 18, 4, 1);
    b.rect(2, 1, 4, 8, 2);
    b.rect(12, 1, 4, 8, 2);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(48, 32);
    b.rect(2, 10, 44, 22, 1);
    b.rect(0, 6, 48, 6, 2);
    b.rect(6, 16, 10, 12, 3);
    b.rect(22, 16, 8, 8, 4);
    b.rect(34, 18, 8, 10, 5);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 32);
    b.rect(3, 8, 2, 24, 1);
    b.rect(1, 1, 6, 7, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(8, 48);
    b.rect(3, 0, 2, 48, 1);
    b.rect(0, 0, 8, 4, 2);
    b.rect(1, 20, 6, 3, 3);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(48, 12);
    b.rect(0, 2, 48, 8, 1);
    b.rect(4, 4, 8, 4, 2);
    b.rect(16, 4, 6, 4, 3);
    b.rect(26, 4, 8, 4, 2);
    b.rect(38, 4, 6, 4, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 7));
    loadFont(vdp, art);

    setPal(vdp, PAL_CRANE,
           {0, gs::rgb4(15, 12, 2), gs::rgb4(11, 8, 1), gs::rgb4(3, 3, 4), gs::rgb4(8, 13, 15), gs::rgb4(2, 2, 3),
            gs::rgb4(14, 6, 2), gs::rgb4(6, 10, 12), gs::rgb4(13, 13, 12)});
    setPal(vdp, PAL_WHEEL,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 7), gs::rgb4(10, 9, 7), gs::rgb4(13, 12, 9)});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(7, 6, 5), gs::rgb4(11, 5, 3), gs::rgb4(4, 6, 8), gs::rgb4(12, 11, 8), gs::rgb4(3, 4, 3)});
    setPal(vdp, PAL_HOOK, {0, gs::rgb4(9, 9, 10), gs::rgb4(14, 11, 3), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(13, 13, 12), gs::rgb4(14, 3, 2), gs::rgb4(15, 13, 4)});

    auto roadPal = [&](int pal, uint16_t ground, uint16_t verge, uint16_t asphalt, uint16_t paint) {
        for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, asphalt);
        vdp.setColor(pal * 16 + 0, 0);
        vdp.setColor(pal * 16 + 1, ground);
        vdp.setColor(pal * 16 + 2, gs::rgb4(3, 4, 2));
        vdp.setColor(pal * 16 + 3, gs::rgb4(5, 5, 3));
        vdp.setColor(pal * 16 + 4, verge);
        vdp.setColor(pal * 16 + 5, gs::rgb4(4, 4, 3));
        vdp.setColor(pal * 16 + 6, asphalt);
        vdp.setColor(pal * 16 + 7, gs::rgb4(2, 2, 3));
        vdp.setColor(pal * 16 + 8, gs::rgb4(3, 3, 4));
        vdp.setColor(pal * 16 + 9, gs::rgb4(1, 1, 2));
        vdp.setColor(pal * 16 + 10, gs::rgb4(6, 5, 4));
        vdp.setColor(pal * 16 + 11, gs::rgb4(3, 4, 6));
        vdp.setColor(pal * 16 + 12, gs::rgb4(4, 6, 8));
        vdp.setColor(pal * 16 + 13, gs::rgb4(7, 8, 9));
        vdp.setColor(pal * 16 + 14, paint);
        vdp.setColor(pal * 16 + 15, gs::rgb4(8, 8, 7));
    };
    roadPal(PAL_ROAD, gs::rgb4(3, 5, 2), gs::rgb4(5, 5, 3), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 3));

    art.boom = gs::uploadMipped(vdp, boomArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.dash = gs::uploadMipped(vdp, dashArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.axle = gs::uploadMipped(vdp, axleArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    vdp.setFogColor(gs::rgb4(5, 6, 7));
}

}  // namespace cranekilo
