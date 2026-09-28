#include "game/art.h"

#include <string>

namespace bunkerpace {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap sentry(int step) {
    gs::Bitmap b(28, 64);
    b.ellipse(14, 8, 7, 6, 4);
    b.rect(8, 4, 12, 4, 5);
    b.rect(7, 7, 14, 3, 3);
    b.set(11, 10, 6);
    b.set(17, 10, 6);
    b.rect(12, 13, 4, 2, 2);
    b.poly({{6, 16}, {22, 16}, {24, 38}, {4, 38}}, 1);
    b.rect(12, 18, 4, 14, 2);
    b.rect(18, 20, 8, 4, 7);
    b.line(22, 22, 26, 28, 7, 2.f);
    if (step == 0) {
        b.rect(7, 38, 6, 18, 2);
        b.rect(15, 38, 6, 14, 3);
        b.rect(6, 54, 8, 4, 8);
        b.rect(14, 50, 8, 4, 8);
    } else {
        b.rect(7, 38, 6, 14, 3);
        b.rect(15, 38, 6, 18, 2);
        b.rect(6, 50, 8, 4, 8);
        b.rect(14, 54, 8, 4, 8);
    }
    b.outline(9, false);
    return b;
}

gs::Bitmap downed() {
    gs::Bitmap b(70, 28);
    b.ellipse(12, 16, 6, 5, 4);
    b.rect(6, 8, 12, 4, 5);
    b.poly({{18, 10}, {62, 14}, {64, 24}, {16, 22}}, 1);
    b.rect(48, 16, 10, 5, 7);
    b.rect(58, 18, 8, 3, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap pillar() {
    gs::Bitmap b(48, 96);
    b.rect(0, 0, 48, 96, 1);
    for (int y = 0; y < 96; y += 8) {
        b.rect(0, y, 48, 1, 2);
        int shift = ((y / 8) & 1) ? 10 : 0;
        for (int x = shift; x < 48; x += 16) b.rect(x, y, 1, 8, 3);
    }
    for (int y = 4; y < 90; y += 16) b.rect(8, y, 10, 6, 4);
    b.rect(0, 0, 4, 96, 5);
    b.outline(3, false);
    return b;
}

gs::Bitmap lintel() {
    gs::Bitmap b(80, 16);
    b.rect(0, 2, 80, 12, 1);
    b.rect(0, 2, 80, 3, 2);
    b.rect(0, 11, 80, 3, 3);
    for (int x = 4; x < 76; x += 10) b.rect(x, 6, 2, 6, 4);
    return b;
}

gs::Bitmap bag() {
    gs::Bitmap b(36, 22);
    b.ellipse(18, 12, 16, 8, 1);
    b.ellipse(12, 11, 8, 6, 2);
    b.line(8, 8, 28, 16, 3, 1.2f);
    b.outline(4, false);
    return b;
}

gs::Bitmap lamp() {
    gs::Bitmap b(12, 16);
    b.rect(5, 0, 2, 4, 3);
    b.poly({{2, 5}, {10, 5}, {8, 12}, {4, 12}}, 1);
    b.ellipse(6, 9, 2, 2, 2);
    return b;
}

gs::Bitmap bead() {
    gs::Bitmap b(10, 10);
    b.rect(4, 0, 2, 10, 1);
    b.rect(0, 4, 10, 2, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

gs::Bitmap flash() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 1);
    b.ellipse(10, 10, 4, 4, 2);
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
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 15, 14), gs::rgb4(6, 7, 6), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 10), gs::rgb4(6, 4, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 10, 8), gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 15, 7), gs::rgb4(13, 15, 12), gs::rgb4(1, 4, 1)});
    setPal(vdp, PAL_CONCRETE, {0, gs::rgb4(7, 7, 6), gs::rgb4(10, 10, 9), gs::rgb4(4, 4, 4), gs::rgb4(5, 5, 4),
                               gs::rgb4(3, 3, 3), gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_FIGURE, {0, gs::rgb4(8, 7, 5), gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 3), gs::rgb4(12, 9, 6),
                             gs::rgb4(4, 5, 3), gs::rgb4(1, 1, 1), gs::rgb4(10, 8, 4), gs::rgb4(2, 2, 1),
                             gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_BAG, {0, gs::rgb4(9, 8, 4), gs::rgb4(6, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 10, 2), gs::rgb4(8, 8, 6)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(2, 3, 6), gs::rgb4(4, 5, 8), gs::rgb4(1, 1, 2)});

    loadFont(vdp, art);
    art.sentry[0] = gs::uploadMipped(vdp, sentry(0));
    art.sentry[1] = gs::uploadMipped(vdp, sentry(1));
    art.downed = gs::uploadMipped(vdp, downed());
    art.pillar = gs::uploadMipped(vdp, pillar());
    art.lintel = gs::uploadMipped(vdp, lintel());
    art.bag = gs::uploadMipped(vdp, bag());
    art.lamp = gs::uploadMipped(vdp, lamp());
    art.bead = gs::uploadMipped(vdp, bead());
    art.flash = gs::uploadMipped(vdp, flash());
}

}  // namespace bunkerpace
