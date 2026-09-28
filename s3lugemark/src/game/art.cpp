#include "game/art.h"

#include <initializer_list>
#include <string>

namespace luge {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap sledArt() {
    Bitmap b(96, 128);
    b.poly({{18, 8}, {78, 8}, {86, 118}, {10, 118}}, 2);
    b.poly({{28, 16}, {68, 16}, {74, 108}, {22, 108}}, 3);
    b.rect(14, 20, 8, 96, 4);
    b.rect(74, 20, 8, 96, 4);
    b.rect(16, 22, 4, 90, 5);
    b.rect(76, 22, 4, 90, 5);
    b.ellipse(48, 46, 16, 18, 1);
    b.ellipse(48, 44, 10, 12, 6);
    b.rect(40, 70, 16, 22, 7);
    b.rect(44, 62, 8, 10, 8);
    b.line(30, 30, 66, 30, 5, 2);
    b.line(26, 100, 70, 100, 5, 2);
    b.outline(5, false);
    return b;
}

Bitmap riderArt() {
    Bitmap b(48, 56);
    b.ellipse(24, 14, 10, 10, 2);
    b.ellipse(24, 13, 7, 7, 3);
    b.rect(20, 12, 3, 2, 1);
    b.rect(26, 12, 3, 2, 1);
    b.poly({{12, 28}, {36, 28}, {40, 52}, {8, 52}}, 4);
    b.poly({{16, 30}, {32, 30}, {34, 48}, {14, 48}}, 5);
    b.outline(8, false);
    return b;
}

Bitmap sprayArt() {
    Bitmap b(40, 28);
    b.ellipse(12, 16, 8, 5, 2);
    b.ellipse(22, 12, 10, 6, 1);
    b.ellipse(30, 18, 7, 4, 3);
    return b;
}

Bitmap flagArt() {
    Bitmap b(40, 72);
    b.rect(6, 8, 3, 62, 4);
    b.poly({{9, 10}, {34, 18}, {9, 30}}, 2);
    b.poly({{11, 13}, {28, 18}, {11, 26}}, 3);
    b.outline(5, false);
    return b;
}

Bitmap treeArt() {
    Bitmap b(48, 72);
    b.poly({{24, 4}, {44, 40}, {4, 40}}, 2);
    b.poly({{24, 18}, {40, 52}, {8, 52}}, 1);
    b.poly({{24, 32}, {36, 64}, {12, 64}}, 3);
    b.rect(21, 58, 6, 12, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintArt() {
    Bitmap b(32, 16);
    b.rect(0, 0, 32, 16, 1);
    b.rect(0, 0, 32, 3, 2);
    b.rect(0, 13, 32, 3, 2);
    for (int i = 0; i < 4; i++) b.poly({{float(i * 8), 4}, {float(i * 8 + 6), 8}, {float(i * 8), 12}}, 3);
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t ink = gs::rgb4(15, 15, 15);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(10, 12, 14), gs::rgb4(15, 14, 8), gs::rgb4(15, 5, 3), gs::rgb4(6, 14, 8),
                          gs::rgb4(8, 10, 14), gs::rgb4(15, 12, 4), gs::rgb4(4, 5, 7), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ICE, {0, gs::rgb4(14, 15, 15), gs::rgb4(9, 12, 14), gs::rgb4(12, 14, 15), gs::rgb4(6, 7, 8),
                          gs::rgb4(15, 15, 15), gs::rgb4(7, 9, 12), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 13, 3), gs::rgb4(12, 2, 2), gs::rgb4(15, 15, 12), gs::rgb4(2, 2, 2), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RIDER, {0, gs::rgb4(15, 4, 3), gs::rgb4(12, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(3, 3, 4),
                            gs::rgb4(14, 14, 15), gs::rgb4(15, 12, 8), gs::rgb4(6, 4, 3), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0,
                            0, shadow});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(4, 8, 14), gs::rgb4(15, 14, 4), gs::rgb4(12, 10, 2), gs::rgb4(5, 4, 3),
                           gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(4, 8, 5), gs::rgb4(2, 5, 3), gs::rgb4(6, 10, 6), gs::rgb4(5, 4, 2), gs::rgb4(1, 1, 1),
                           0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    // Road chip indices: 1-3 snow, 4-5 verge, 6-7 ice sheet, 8 grit, 9 runners,
    // 14 glassy streak, 15 sparkle. Banks use GROUND_SNOWWALL.
    const uint16_t field[16] = {
        0,
        gs::rgb4(13, 14, 15), gs::rgb4(11, 12, 14), gs::rgb4(9, 10, 12),
        gs::rgb4(8, 10, 12), gs::rgb4(6, 8, 11),
        gs::rgb4(12, 14, 15), gs::rgb4(9, 12, 14),
        gs::rgb4(7, 8, 9), gs::rgb4(14, 15, 15),
        gs::rgb4(10, 12, 13),
        gs::rgb4(6, 8, 12), gs::rgb4(8, 10, 14), gs::rgb4(10, 12, 15),
        gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);
    vdp.setFogColor(gs::rgb4(10, 12, 14));

    art.sled = gs::uploadMipped(vdp, sledArt());
    art.rider = gs::uploadMipped(vdp, riderArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.paint = gs::uploadMipped(vdp, paintArt());
    loadFont(vdp, art);
}

}  // namespace luge
