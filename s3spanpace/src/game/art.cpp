#include "game/art.h"

#include <initializer_list>

namespace spanpace {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) vdp.setColor(pal * 16 + i++, c);
}

gs::Bitmap coatArt(int stride) {
    gs::Bitmap b(22, 40);
    b.rect(8, 2, 6, 6, 4);
    b.rect(9, 1, 4, 2, 5);
    b.set(10, 4, 8);
    b.set(12, 4, 8);
    b.rect(7, 8, 8, 16, 1);
    b.rect(6, 10, 3, 10, 2);
    b.rect(13, 10, 3, 10, 2);
    b.rect(8, 22, 6, 4, 6);
    int lx = stride ? 6 : 8;
    int rx = stride ? 13 : 10;
    b.rect(lx, 26, 3, 12, 3);
    b.rect(rx, 26, 3, 12, 7);
    b.rect(lx - 1, 36, 5, 3, 9);
    b.rect(rx - 1, 36, 5, 3, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap fallenArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(10, 7, 5, 4, 4);
    b.rect(14, 5, 16, 6, 1);
    b.rect(28, 6, 8, 4, 3);
    b.rect(16, 11, 6, 3, 9);
    b.rect(24, 11, 6, 3, 9);
    b.outline(10, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(28, 72);
    b.poly({{4, 70}, {10, 8}, {18, 8}, {24, 70}}, 1);
    b.rect(11, 4, 6, 8, 2);
    b.rect(8, 18, 12, 3, 3);
    b.rect(7, 36, 14, 3, 3);
    b.rect(6, 54, 16, 3, 4);
    b.rect(12, 0, 4, 6, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 8, 2, 18, 2);
    b.ellipse(5, 6, 4, 4, 1);
    b.rect(2, 24, 6, 3, 3);
    return b;
}

gs::Bitmap linkArt() {
    gs::Bitmap b(8, 6);
    b.rect(1, 2, 6, 2, 1);
    b.set(0, 2, 2);
    b.set(7, 2, 2);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(14, 18);
    b.poly({{7, 1}, {12, 10}, {2, 10}}, 1);
    b.rect(3, 10, 8, 3, 3);
    b.ellipse(7, 15, 5, 2, 2);
    return b;
}

gs::Bitmap gullArt(int fr) {
    gs::Bitmap b(24, 10);
    if (fr == 0) {
        b.poly({{12, 6}, {1, 2}, {8, 6}}, 1);
        b.poly({{12, 6}, {23, 2}, {16, 6}}, 1);
    } else {
        b.poly({{12, 5}, {2, 8}, {8, 6}}, 2);
        b.poly({{12, 5}, {22, 8}, {16, 6}}, 2);
    }
    b.ellipse(12, 6, 2, 2, 1);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(18, 18);
    b.rect(8, 1, 2, 5, 1);
    b.rect(8, 12, 2, 5, 1);
    b.rect(1, 8, 5, 2, 1);
    b.rect(12, 8, 5, 2, 1);
    b.ellipse(9, 9, 2, 2, 2);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(16, 16);
    b.poly({{8, 0}, {10, 6}, {16, 8}, {10, 10}, {8, 16}, {6, 10}, {0, 8}, {6, 6}}, 1);
    b.ellipse(8, 8, 2, 2, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(20, 12);
    b.ellipse(6, 7, 5, 3, 1);
    b.ellipse(13, 6, 4, 3, 2);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 1);
    b.ellipse(14, 8, 6, 6, 0);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(40, 14);
    b.ellipse(12, 8, 8, 4, 1);
    b.ellipse(22, 6, 9, 5, 2);
    b.ellipse(32, 8, 6, 3, 1);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(16, 8);
    b.rect(0, 2, 16, 2, 1);
    b.rect(2, 4, 2, 4, 2);
    b.rect(12, 4, 2, 4, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 14, 15), gs::rgb4(9, 10, 12), gs::rgb4(3, 4, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 10), gs::rgb4(6, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 10, 7), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 15, 10), gs::rgb4(13, 15, 14), gs::rgb4(1, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(11, 12, 14), gs::rgb4(6, 7, 9), gs::rgb4(14, 13, 8), gs::rgb4(4, 5, 6),
                            gs::rgb4(15, 14, 6), gs::rgb4(2, 2, 3), gs::rgb4(8, 8, 9), gs::rgb4(3, 4, 5), 0, 0, 0, 0, 0,
                            0, 0, ink});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(4, 6, 9), gs::rgb4(2, 3, 5), gs::rgb4(1, 2, 3), gs::rgb4(12, 9, 6),
                           gs::rgb4(3, 2, 2), gs::rgb4(8, 7, 5), gs::rgb4(6, 5, 4), gs::rgb4(14, 12, 8),
                           gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 3), gs::rgb4(9, 8, 7), gs::rgb4(5, 6, 8), 0, 0, ink});
    setPal(vdp, PAL_HOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12), gs::rgb4(5, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(8, 15, 12), gs::rgb4(4, 12, 9), gs::rgb4(1, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(12, 6, 3), gs::rgb4(6, 8, 10), gs::rgb4(3, 5, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 10, 3), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 14, 10), gs::rgb4(10, 11, 14), gs::rgb4(6, 7, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, ink});
    setPal(vdp, PAL_CABLE, {0, gs::rgb4(13, 13, 14), gs::rgb4(7, 7, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    const uint16_t road[16] = {0,
                               gs::rgb4(5, 6, 7),
                               gs::rgb4(8, 9, 10),
                               gs::rgb4(3, 4, 5),
                               gs::rgb4(12, 11, 6),
                               gs::rgb4(2, 5, 8),
                               gs::rgb4(3, 7, 10),
                               gs::rgb4(1, 3, 5),
                               gs::rgb4(4, 5, 6),
                               gs::rgb4(9, 10, 11),
                               gs::rgb4(6, 7, 8),
                               gs::rgb4(2, 3, 4),
                               gs::rgb4(14, 13, 8),
                               gs::rgb4(1, 2, 3),
                               gs::rgb4(7, 8, 9),
                               ink};
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    art.coat[0] = gs::uploadMipped(vdp, coatArt(0));
    art.coat[1] = gs::uploadMipped(vdp, coatArt(1));
    art.fallen = gs::uploadMipped(vdp, fallenArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(0));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(1));
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    loadFont(vdp, art);
}

}  // namespace spanpace
