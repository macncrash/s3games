#include "game/art.h"

#include <initializer_list>

namespace palisade {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) vdp.setColor(pal * 16 + i++, c);
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(16, 72);
    b.poly({{8, 1}, {14, 16}, {12, 70}, {4, 70}, {2, 16}}, 2);
    b.poly({{8, 3}, {12, 16}, {10, 66}, {6, 66}, {4, 16}}, 1);
    b.line(8, 6, 8, 68, 3, 1);
    b.rect(5, 28, 6, 2, 4);
    b.rect(5, 48, 6, 2, 4);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(36, 78);
    b.rect(2, 4, 32, 72, 2);
    b.rect(4, 6, 28, 68, 1);
    for (int y = 10; y < 70; y += 8) b.line(5, float(y), 30, float(y), 3, 1);
    b.rect(2, 30, 32, 6, 5);
    b.rect(15, 32, 6, 4, 6);
    b.rect(0, 2, 4, 74, 4);
    b.rect(32, 2, 4, 74, 4);
    b.rect(14, 8, 8, 6, 6);
    return b;
}

gs::Bitmap guardArt(bool thrust) {
    gs::Bitmap b(40, 48);
    b.ellipse(16, 8, 5, 5, 3);
    b.rect(12, 14, 9, 14, 1);
    b.rect(13, 16, 7, 6, 2);
    b.rect(11, 28, 4, 14, 4);
    b.rect(17, 28, 4, 14, 4);
    b.rect(10, 41, 5, 3, 5);
    b.rect(17, 41, 5, 3, 5);
    if (thrust) {
        b.line(20, 20, 38, 16, 6, 2);
        b.poly({{36, 12}, {40, 16}, {36, 20}}, 7);
        b.rect(18, 18, 6, 3, 3);
    } else {
        b.line(22, 28, 28, 4, 6, 2);
        b.poly({{26, 2}, {32, 6}, {26, 8}}, 7);
    }
    b.rect(8, 16, 4, 3, 3);
    return b;
}

gs::Bitmap raiderArt() {
    gs::Bitmap b(28, 46);
    b.ellipse(14, 8, 5, 5, 3);
    b.rect(6, 4, 10, 3, 4);
    b.rect(10, 14, 9, 13, 1);
    b.rect(11, 16, 7, 5, 2);
    b.rect(10, 27, 4, 13, 5);
    b.rect(16, 27, 4, 13, 5);
    b.rect(9, 39, 5, 3, 6);
    b.rect(16, 39, 5, 3, 6);
    b.rect(6, 16, 4, 3, 3);
    return b;
}

gs::Bitmap axeArt() {
    gs::Bitmap b(16, 20);
    b.line(4, 18, 12, 2, 1, 2);
    b.poly({{10, 1}, {16, 6}, {11, 8}, {9, 3}}, 2);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(18, 12, 9, 9, 0);
    b.set(8, 10, 2);
    b.set(11, 16, 2);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(22, 16);
    b.rect(10, 0, 2, 16, 3);
    b.poly({{12, 1}, {21, 5}, {12, 10}}, 1);
    b.poly({{12, 3}, {18, 5}, {12, 8}}, 2);
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
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 10), gs::rgb4(15, 12, 4), gs::rgb4(8, 14, 6), gs::rgb4(15, 4, 3),
                          gs::rgb4(6, 8, 12), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(10, 7, 3), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 1), gs::rgb4(12, 9, 4),
                           gs::rgb4(8, 8, 9), gs::rgb4(14, 12, 6), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PLAYER, {0, gs::rgb4(8, 10, 6), gs::rgb4(4, 6, 8), gs::rgb4(13, 10, 7), gs::rgb4(5, 4, 3),
                             gs::rgb4(3, 2, 2), gs::rgb4(12, 10, 6), gs::rgb4(14, 13, 8), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RAIDER, {0, gs::rgb4(9, 3, 2), gs::rgb4(5, 2, 2), gs::rgb4(12, 8, 5), gs::rgb4(4, 3, 2),
                             gs::rgb4(3, 2, 1), gs::rgb4(2, 2, 2), gs::rgb4(7, 6, 5), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(11, 7, 3), gs::rgb4(14, 4, 2), gs::rgb4(9, 9, 10), gs::rgb4(5, 5, 6),
                           gs::rgb4(6, 4, 2), gs::rgb4(12, 11, 8), gs::rgb4(15, 13, 6), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 6, 2), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(14, 13, 9), gs::rgb4(11, 10, 7), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    vdp.setFogColor(gs::rgb4(2, 2, 4));

    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.guard = gs::uploadMipped(vdp, guardArt(false));
    art.guardThrust = gs::uploadMipped(vdp, guardArt(true));
    art.raider = gs::uploadMipped(vdp, raiderArt());
    art.axe = gs::uploadMipped(vdp, axeArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    loadFont(vdp, art);
}

}  // namespace palisade
