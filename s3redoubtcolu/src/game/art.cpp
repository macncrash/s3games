#include "game/art.h"

#include <initializer_list>
#include <string>

namespace redoubt {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

Bitmap lorryArt(bool lead) {
    Bitmap b(40, 52);
    b.rect(8, 2, 24, 14, 3);
    b.rect(11, 4, 18, 7, lead ? 6 : 5);
    b.rect(6, 16, 28, 18, 2);
    b.rect(10, 19, 8, 6, 4);
    b.rect(22, 19, 8, 6, 4);
    b.rect(4, 34, 32, 8, 1);
    b.ellipse(12, 44, 5, 5, 7);
    b.ellipse(28, 44, 5, 5, 7);
    b.ellipse(12, 44, 2, 2, 8);
    b.ellipse(28, 44, 2, 2, 8);
    b.rect(16, 36, 8, 3, lead ? 9 : 4);
    b.outline(8, false);
    return b;
}

Bitmap earthArt() {
    Bitmap b(96, 36);
    b.poly({{2, 34}, {10, 10}, {86, 10}, {94, 34}}, 2);
    b.rect(18, 14, 60, 8, 3);
    b.rect(24, 16, 48, 4, 1);
    b.rect(8, 22, 14, 8, 4);
    b.rect(74, 22, 14, 8, 4);
    b.rect(40, 6, 16, 8, 5);
    return b;
}

Bitmap gunArt() {
    Bitmap b(18, 48);
    b.rect(6, 4, 6, 36, 2);
    b.rect(7, 0, 4, 8, 1);
    b.rect(3, 34, 12, 8, 3);
    b.rect(4, 38, 10, 6, 4);
    b.ellipse(9, 42, 3, 3, 5);
    return b;
}

Bitmap treeArt() {
    Bitmap b(22, 48);
    b.ellipse(11, 10, 8, 10, 2);
    b.ellipse(11, 18, 7, 9, 3);
    b.rect(9, 26, 4, 20, 4);
    return b;
}

Bitmap burstArt() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 9, 7, 1);
    b.ellipse(10, 10, 5, 4, 2);
    b.ellipse(10, 9, 2, 2, 3);
    return b;
}

Bitmap reticleArt() {
    Bitmap b(16, 16);
    b.rect(7, 0, 2, 5, 1);
    b.rect(7, 11, 2, 5, 1);
    b.rect(0, 7, 5, 2, 1);
    b.rect(11, 7, 5, 2, 1);
    b.rect(7, 7, 2, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 0)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 15, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 3, 1)});
    setPal(vdp, PAL_LORRY, {0, gs::rgb4(3, 4, 2), gs::rgb4(5, 6, 3), gs::rgb4(7, 8, 4), gs::rgb4(8, 7, 3),
                            gs::rgb4(10, 12, 13), gs::rgb4(4, 8, 12), gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 2),
                            gs::rgb4(12, 10, 4)});
    setPal(vdp, PAL_LEAD, {0, gs::rgb4(4, 4, 3), gs::rgb4(6, 6, 4), gs::rgb4(8, 8, 5), gs::rgb4(9, 8, 4),
                           gs::rgb4(11, 13, 14), gs::rgb4(12, 4, 3), gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 2),
                           gs::rgb4(14, 12, 3)});
    setPal(vdp, PAL_EARTH, {0, gs::rgb4(2, 2, 1), gs::rgb4(7, 5, 2), gs::rgb4(9, 7, 3), gs::rgb4(5, 6, 3),
                            gs::rgb4(4, 4, 3)});
    setPal(vdp, PAL_GUN, {0, gs::rgb4(6, 7, 7), gs::rgb4(4, 5, 5), gs::rgb4(3, 3, 3), gs::rgb4(8, 7, 4),
                          gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(2, 4, 1), gs::rgb4(3, 6, 2), gs::rgb4(4, 8, 3), gs::rgb4(5, 4, 2)});
    setPal(vdp, PAL_BURST, {0, gs::rgb4(15, 10, 3), gs::rgb4(14, 6, 2), gs::rgb4(15, 15, 8), gs::rgb4(8, 8, 8)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(4, 6, 2), gs::rgb4(3, 5, 2), gs::rgb4(6, 7, 3), gs::rgb4(6, 5, 3), gs::rgb4(5, 4, 2),
            gs::rgb4(4, 4, 4), gs::rgb4(3, 3, 3), gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 5), gs::rgb4(8, 8, 7),
            gs::rgb4(2, 3, 6), gs::rgb4(3, 4, 7), gs::rgb4(5, 7, 10), gs::rgb4(13, 12, 5), gs::rgb4(6, 6, 5)});
    vdp.setFogColor(gs::rgb4(8, 8, 9));
    loadFont(vdp, art);
    art.lorry = gs::uploadMipped(vdp, lorryArt(false));
    art.lead = gs::uploadMipped(vdp, lorryArt(true));
    art.earth = gs::uploadMipped(vdp, earthArt());
    art.gun = gs::uploadMipped(vdp, gunArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.burst = gs::uploadMipped(vdp, burstArt());
    art.reticle = gs::uploadMipped(vdp, reticleArt());
}

}  // namespace redoubt
