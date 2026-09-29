#include "game/art.h"

#include <cmath>

namespace orcharddawn {
namespace {

uint16_t C(int r, int g, int b) { return gs::rgb4(r, g, b); }

void setPal(gs::VDP& vdp, int pal, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{2, 1, 0, 15, 1};
    uint8_t bar[64];
    for (int i = 0; i < 64; i++) bar[i] = 2;
    a.bar = tiles.shared(bar);
    vdp.loadTile(a.bar, bar);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        if (g) {
            for (int y = 0; y < 7; y++)
                for (int x = 0; x < 5; x++)
                    if (g[y * 5 + x]) {
                        px[y * 8 + x + 1] = 1;
                        if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                    }
        }
        int t = tiles.shared(px);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap treeBmp() {
    gs::Bitmap b(40, 72);
    b.rect(17, 36, 6, 36, 4);
    b.rect(18, 36, 2, 36, 5);
    b.ellipse(20, 28, 16, 18, 2);
    b.ellipse(14, 22, 8, 7, 3);
    b.ellipse(26, 20, 7, 6, 1);
    b.ellipse(20, 16, 6, 5, 6);
    b.set(12, 18, 7);
    b.set(28, 26, 7);
    return b;
}

gs::Bitmap manBmp(int step) {
    gs::Bitmap b(22, 40);
    b.ellipse(11, 6, 4, 4, 3);
    b.rect(8, 11, 7, 12, 4);
    b.rect(9, 12, 5, 4, 5);
    b.rect(6, 13, 3, 8, 4);
    b.rect(14, 13, 3, 8, 4);
    int ly = step ? 24 : 26;
    int ry = step ? 26 : 24;
    b.rect(8, ly, 3, 14, 2);
    b.rect(12, ry, 3, 14, 2);
    b.rect(7, 37, 5, 2, 6);
    b.rect(12, 37, 5, 2, 6);
    return b;
}

gs::Bitmap potBmp() {
    gs::Bitmap b(16, 16);
    b.poly({{3, 4}, {13, 4}, {12, 14}, {4, 14}}, 2);
    b.rect(2, 3, 12, 2, 3);
    b.rect(6, 1, 4, 3, 4);
    return b;
}

gs::Bitmap flameBmp(int flick) {
    gs::Bitmap b(12, 20);
    b.ellipse(6, 12, 4, 6, 2);
    b.ellipse(6, 8, 3, flick ? 6 : 5, 3);
    b.ellipse(6, 6, 1, 3, 4);
    return b;
}

gs::Bitmap appleBmp() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 7, 5, 4, 2);
    b.ellipse(6, 6, 3, 2, 3);
    b.line(6, 2, 7, 5, 4, 1);
    b.set(8, 2, 1);
    return b;
}

gs::Bitmap leafBmp() {
    gs::Bitmap b(10, 8);
    b.ellipse(5, 4, 4, 2, 2);
    b.line(1, 4, 9, 4, 3, 1);
    return b;
}

gs::Bitmap sparkBmp() {
    gs::Bitmap b(4, 4);
    b.set(1, 1, 1);
    b.set(2, 1, 1);
    b.set(1, 2, 2);
    b.set(2, 2, 2);
    return b;
}

gs::Bitmap shadeBmp() {
    gs::Bitmap b(16, 4);
    b.ellipse(8, 2, 7, 1.5f, 1);
    return b;
}

gs::Bitmap moonBmp() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(11, 7, 4, 4, 0);
    return b;
}

gs::Bitmap sunBmp() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 6, 6, 2);
    b.ellipse(10, 10, 3, 3, 3);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785f;
        b.line(10 + std::cos(a) * 7, 10 + std::sin(a) * 7, 10 + std::cos(a) * 9, 10 + std::sin(a) * 9, 1, 1);
    }
    return b;
}

gs::Bitmap starBmp() {
    gs::Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(2, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {C(0, 0, 0), C(14, 14, 13), C(2, 3, 4), C(8, 8, 6), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                            C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                            C(0, 0, 0), C(4, 4, 3)};
    const uint16_t grass[] = {C(0, 0, 0), C(2, 6, 2), C(3, 8, 3), C(4, 10, 3), C(1, 4, 1), C(6, 8, 3), C(8, 7, 3),
                              C(5, 4, 2), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                              C(0, 0, 0), C(0, 0, 0)};
    const uint16_t bark[] = {C(0, 0, 0), C(6, 4, 2), C(8, 5, 2), C(4, 3, 1), C(5, 3, 1), C(10, 7, 4), C(3, 2, 1),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0)};
    const uint16_t leaf[] = {C(0, 0, 0), C(2, 8, 2), C(1, 5, 1), C(4, 11, 3), C(3, 2, 1), C(6, 4, 2), C(8, 12, 4),
                             C(12, 3, 2), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0)};
    const uint16_t man[] = {C(0, 0, 0), C(3, 3, 4), C(2, 2, 3), C(12, 9, 6), C(4, 5, 8), C(8, 8, 10), C(2, 2, 2),
                            C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                            C(0, 0, 0), C(0, 0, 0)};
    const uint16_t fire[] = {C(0, 0, 0), C(15, 14, 4), C(15, 8, 1), C(15, 12, 2), C(15, 15, 10), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0)};
    const uint16_t apple[] = {C(0, 0, 0), C(2, 8, 2), C(12, 2, 2), C(15, 5, 3), C(4, 3, 1), C(0, 0, 0), C(0, 0, 0),
                              C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                              C(0, 0, 0), C(0, 0, 0)};
    const uint16_t wind[] = {C(0, 0, 0), C(8, 12, 5), C(5, 9, 3), C(3, 6, 2), C(10, 10, 6), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0)};
    const uint16_t iron[] = {C(0, 0, 0), C(6, 6, 7), C(4, 4, 5), C(9, 9, 10), C(3, 3, 3), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0)};
    const uint16_t moon[] = {C(0, 0, 0), C(13, 13, 12), C(8, 8, 10), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0)};
    const uint16_t sun[] = {C(0, 0, 0), C(15, 12, 3), C(15, 8, 2), C(15, 15, 8), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                            C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                            C(0, 0, 0), C(0, 0, 0)};
    const uint16_t gold[] = {C(0, 0, 0), C(15, 13, 4), C(10, 8, 2), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                             C(0, 0, 0), C(15, 15, 8)};
    const uint16_t alert[] = {C(0, 0, 0), C(15, 3, 2), C(8, 1, 1), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                              C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                              C(0, 0, 0), C(6, 1, 1)};
    const uint16_t ember[] = {C(0, 0, 0), C(12, 4, 1), C(8, 2, 1), C(15, 8, 2), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                              C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                              C(0, 0, 0), C(0, 0, 0)};
    const uint16_t smoke[] = {C(0, 0, 0), C(6, 6, 7), C(4, 4, 5), C(8, 8, 8), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                              C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                              C(0, 0, 0), C(0, 0, 0)};
    const uint16_t pip[] = {C(0, 0, 0), C(4, 4, 3), C(10, 9, 4), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                            C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0),
                            C(0, 0, 0), C(0, 0, 0)};
    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_GRASS, grass);
    setPal(vdp, PAL_BARK, bark);
    setPal(vdp, PAL_LEAF, leaf);
    setPal(vdp, PAL_MAN, man);
    setPal(vdp, PAL_FIRE, fire);
    setPal(vdp, PAL_APPLE, apple);
    setPal(vdp, PAL_WIND, wind);
    setPal(vdp, PAL_IRON, iron);
    setPal(vdp, PAL_MOON, moon);
    setPal(vdp, PAL_SUN, sun);
    setPal(vdp, PAL_GOLD, gold);
    setPal(vdp, PAL_ALERT, alert);
    setPal(vdp, PAL_EMBER, ember);
    setPal(vdp, PAL_SMOKE, smoke);
    setPal(vdp, PAL_PIP, pip);
    vdp.setFogColor(C(6, 5, 8));
    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.tree = gs::uploadMipped(vdp, treeBmp());
    art.man[0] = gs::uploadMipped(vdp, manBmp(0));
    art.man[1] = gs::uploadMipped(vdp, manBmp(1));
    art.pot = gs::uploadMipped(vdp, potBmp());
    art.flame[0] = gs::uploadMipped(vdp, flameBmp(0));
    art.flame[1] = gs::uploadMipped(vdp, flameBmp(1));
    art.apple = gs::uploadMipped(vdp, appleBmp());
    art.leaf = gs::uploadMipped(vdp, leafBmp());
    art.spark = gs::uploadMipped(vdp, sparkBmp());
    art.shade = gs::uploadMipped(vdp, shadeBmp());
    art.moon = gs::uploadMipped(vdp, moonBmp());
    art.sun = gs::uploadMipped(vdp, sunBmp());
    art.star = gs::uploadMipped(vdp, starBmp());
}

}  // namespace orcharddawn
