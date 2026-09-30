#include "game/art.h"

#include <initializer_list>

namespace heli {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
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

gs::Bitmap heliArt() {
    gs::Bitmap b(72, 36);
    b.rect(18, 14, 34, 12, 2);
    b.rect(20, 16, 28, 4, 3);
    b.ellipse(46, 18, 10, 8, 4);
    b.ellipse(48, 18, 5, 4, 5);
    b.rect(8, 17, 14, 5, 2);
    b.rect(2, 15, 8, 3, 6);
    b.rect(4, 12, 3, 4, 6);
    b.rect(30, 8, 3, 8, 1);
    b.rect(22, 26, 3, 6, 1);
    b.rect(42, 26, 3, 6, 1);
    b.rect(16, 31, 34, 2, 7);
    b.rect(14, 18, 4, 3, 8);
    b.rect(52, 20, 8, 2, 2);
    return b;
}

gs::Bitmap rotorArt() {
    gs::Bitmap b(70, 8);
    b.rect(2, 3, 66, 2, 1);
    b.rect(32, 1, 6, 6, 2);
    b.ellipse(8, 4, 5, 2, 3);
    b.ellipse(62, 4, 5, 2, 3);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(88, 22);
    b.rect(2, 4, 84, 14, 2);
    b.rect(6, 7, 76, 8, 3);
    b.rect(38, 6, 6, 12, 1);
    b.rect(28, 6, 26, 3, 1);
    b.rect(28, 15, 26, 3, 1);
    b.rect(0, 2, 6, 18, 4);
    b.rect(82, 2, 6, 18, 4);
    return b;
}

gs::Bitmap groundArt() {
    gs::Bitmap b(320, 40);
    b.rect(0, 0, 320, 40, 2);
    b.rect(0, 0, 320, 6, 3);
    for (int i = 0; i < 16; i++) b.rect(i * 20 + 4, 14, 10, 4, 4);
    for (int i = 0; i < 10; i++) b.ellipse(i * 34 + 8, 28, 12, 4, 5);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(48, 20);
    b.ellipse(16, 12, 14, 7, 1);
    b.ellipse(30, 11, 14, 8, 1);
    b.ellipse(22, 8, 10, 6, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 8, 8, 1);
    b.ellipse(14, 14, 5, 5, 2);
    b.rect(13, 1, 2, 5, 1);
    b.rect(13, 22, 2, 5, 1);
    b.rect(1, 13, 5, 2, 1);
    b.rect(22, 13, 5, 2, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(24, 40);
    b.rect(10, 22, 4, 16, 2);
    b.ellipse(12, 14, 10, 12, 3);
    b.ellipse(12, 12, 6, 6, 4);
    return b;
}

gs::Bitmap sockArt() {
    gs::Bitmap b(28, 20);
    b.rect(2, 2, 3, 16, 1);
    b.poly({{5, 6}, {24, 10}, {5, 14}}, 2);
    b.poly({{6, 7}, {18, 10}, {6, 13}}, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {gs::rgb4(0, 0, 0), gs::rgb4(15, 15, 15), gs::rgb4(15, 14, 6), gs::rgb4(15, 6, 4),
                          gs::rgb4(8, 14, 8), gs::rgb4(6, 8, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_GROUND, {0, gs::rgb4(4, 8, 3), gs::rgb4(3, 7, 2), gs::rgb4(5, 10, 3), gs::rgb4(2, 5, 2),
                             gs::rgb4(4, 6, 3)});
    setPal(vdp, PAL_HELI, {0, gs::rgb4(12, 12, 13), gs::rgb4(8, 9, 7), gs::rgb4(11, 12, 9), gs::rgb4(4, 8, 12),
                           gs::rgb4(10, 14, 15), gs::rgb4(6, 6, 6), gs::rgb4(3, 3, 3), gs::rgb4(14, 4, 3)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 15, 8), gs::rgb4(12, 12, 12), gs::rgb4(4, 4, 5), gs::rgb4(15, 8, 2)});
    setPal(vdp, PAL_CLOUD, {0, gs::rgb4(14, 14, 15), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(6, 4, 2), gs::rgb4(5, 3, 1), gs::rgb4(3, 8, 3), gs::rgb4(5, 11, 4)});

    loadFont(vdp, art);
    art.heli = gs::uploadMipped(vdp, heliArt());
    art.rotor = gs::uploadMipped(vdp, rotorArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    art.ground = gs::uploadMipped(vdp, groundArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.sock = gs::uploadMipped(vdp, sockArt());
    vdp.setFogColor(gs::rgb4(8, 10, 14));
}

}  // namespace heli
