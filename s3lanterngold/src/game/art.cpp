#include "game/art.h"

namespace lanterngold {
namespace {

constexpr uint16_t C(int r, int g, int b) { return gs::rgb4(r, g, b); }

void setPal(gs::VDP& vdp, int pal, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

gs::Bitmap lampArt() {
    gs::Bitmap b(20, 36);
    b.rect(8, 0, 4, 4, 2);
    b.ellipse(10, 14, 8, 9, 3);
    b.ellipse(10, 14, 5, 6, 1);
    b.rect(7, 22, 6, 3, 4);
    b.ellipse(10, 28, 6, 5, 5);
    b.ellipse(10, 28, 3, 3, 6);
    b.rect(9, 32, 2, 3, 2);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(8, 12);
    b.ellipse(4, 7, 3, 4, 1);
    b.ellipse(4, 6, 2, 3, 2);
    if (frame == 0) b.set(4, 2, 3);
    else b.set(3, 3, 3);
    return b;
}

gs::Bitmap wickArt() {
    gs::Bitmap b(10, 8);
    b.poly({{1, 7}, {5, 1}, {9, 7}}, 1);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(12, 8, 6, 6, 0);
    b.set(5, 7, 2);
    b.set(6, 11, 2);
    return b;
}

gs::Bitmap cordArt() {
    gs::Bitmap b(2, 8);
    b.rect(0, 0, 2, 8, 1);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(320, 6);
    b.rect(0, 2, 320, 3, 1);
    b.rect(0, 1, 320, 1, 2);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(8, 80);
    b.rect(2, 0, 4, 80, 1);
    b.rect(1, 0, 1, 80, 2);
    return b;
}

gs::Bitmap mothArt(bool up) {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 6, 2, 3, 1);
    if (up) {
        b.poly({{6, 5}, {1, 1}, {5, 6}}, 2);
        b.poly({{10, 5}, {15, 1}, {11, 6}}, 2);
    } else {
        b.poly({{6, 6}, {1, 8}, {5, 7}}, 2);
        b.poly({{10, 6}, {15, 8}, {11, 7}}, 2);
    }
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    uint8_t px[64];
    for (int c = 32; c < 128; c++) {
        const uint8_t* g = gs::glyph(char(c));
        for (int i = 0; i < 64; i++) px[i] = 0;
        if (g) {
            for (int y = 0; y < 7; y++) {
                for (int x = 0; x < 5; x++) {
                    if (g[y] & (1 << (4 - x))) px[y * 8 + x] = 1;
                }
            }
        }
        int t = tiles.shared(px);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    uint16_t z[16] = {};
    setPal(vdp, PAL_HUD, z);
    uint16_t hud[16] = {};
    hud[1] = C(15, 14, 10);
    hud[15] = C(1, 1, 2);
    setPal(vdp, PAL_HUD, hud);

    uint16_t dark[16] = {};
    dark[1] = C(10, 8, 4);
    dark[2] = C(6, 5, 3);
    dark[3] = C(4, 3, 3);
    dark[4] = C(9, 7, 3);
    dark[5] = C(5, 4, 3);
    dark[6] = C(3, 2, 2);
    setPal(vdp, PAL_DARK, dark);

    uint16_t cream[16] = {};
    cream[1] = C(15, 14, 11);
    cream[2] = C(15, 12, 6);
    cream[3] = C(15, 15, 13);
    cream[4] = C(12, 10, 7);
    cream[5] = C(8, 7, 5);
    cream[6] = C(6, 5, 4);
    setPal(vdp, PAL_CREAM, cream);

    uint16_t gold[16] = {};
    gold[1] = C(15, 13, 4);
    gold[2] = C(15, 10, 2);
    gold[3] = C(15, 15, 8);
    gold[4] = C(15, 11, 1);
    gold[5] = C(10, 6, 1);
    gold[6] = C(6, 3, 1);
    setPal(vdp, PAL_GOLD, gold);

    uint16_t scene[16] = {};
    scene[1] = C(14, 13, 9);
    scene[2] = C(8, 7, 5);
    scene[3] = C(3, 3, 6);
    setPal(vdp, PAL_SCENE, scene);

    uint16_t rose[16] = {};
    rose[1] = C(15, 6, 6);
    setPal(vdp, PAL_ROSE, rose);
    uint16_t jade[16] = {};
    jade[1] = C(7, 15, 8);
    setPal(vdp, PAL_JADE, jade);
    uint16_t dim[16] = {};
    dim[1] = C(8, 9, 12);
    setPal(vdp, PAL_DIM, dim);
    uint16_t ink[16] = {};
    ink[1] = C(15, 15, 14);
    setPal(vdp, PAL_INK, ink);

    vdp.setFogColor(C(1, 1, 4));
    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);

    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.wick = gs::uploadMipped(vdp, wickArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.cord = gs::uploadMipped(vdp, cordArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.moth[0] = gs::uploadMipped(vdp, mothArt(true));
    art.moth[1] = gs::uploadMipped(vdp, mothArt(false));
}

}  // namespace lanterngold
