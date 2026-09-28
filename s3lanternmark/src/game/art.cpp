#include "game/art.h"

#include <algorithm>

namespace lanternmark {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

gs::Bitmap lampArt() {
    gs::Bitmap b(36, 52);
    b.line(18, 1, 18, 6, 6, 1.2f);
    b.poly({{10, 6}, {26, 6}, {29, 12}, {7, 12}}, 13);
    b.rect(9, 10, 18, 3, 3);
    b.ellipse(18, 28, 13, 14, 1);
    b.ellipse(23, 28, 5, 12, 2);
    b.ellipse(18, 29, 5, 6, 5);
    b.line(8, 22, 28, 22, 2, 1);
    b.line(6, 28, 30, 28, 2, 1);
    b.line(8, 34, 28, 34, 2, 1);
    b.poly({{9, 40}, {27, 40}, {23, 46}, {13, 46}}, 13);
    b.rect(13, 45, 10, 2, 3);
    b.line(18, 46, 18, 51, 4, 1.1f);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 8, 8, 0);
    b.ellipse(14, 14, 5.5f, 5.5f, 2);
    b.ellipse(14, 14, 3.2f, 3.2f, 0);
    b.rect(13, 2, 2, 3, 3);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(12, 16);
    if (frame == 0) {
        b.poly({{6, 1}, {11, 11}, {1, 11}}, 1);
        b.ellipse(6, 11, 4, 3, 1);
        b.poly({{6, 5}, {9, 13}, {3, 13}}, 2);
    } else {
        b.poly({{7, 2}, {11, 12}, {2, 11}}, 1);
        b.ellipse(6, 12, 4, 3, 1);
        b.ellipse(6, 10, 2, 3, 2);
    }
    return b;
}

gs::Bitmap glowArt() {
    gs::Bitmap b(48, 48);
    b.ellipse(24, 24, 22, 20, 1);
    b.ellipse(24, 24, 12, 12, 2);
    b.ellipse(24, 24, 5, 5, 3);
    return b;
}

gs::Bitmap wickArt() {
    gs::Bitmap b(12, 10);
    b.poly({{6, 0}, {11, 5}, {6, 9}, {1, 5}}, 1);
    b.poly({{6, 3}, {8, 5}, {6, 8}, {4, 5}}, 2);
    return b;
}

gs::Bitmap cordArt() {
    gs::Bitmap b(4, 16);
    b.rect(1, 0, 2, 16, 1);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(320, 8);
    b.rect(0, 2, 320, 4, 1);
    b.rect(0, 1, 320, 1, 2);
    for (int i = 0; i < 6; i++) b.rect(float(18 + i * 52), 0, 6, 8, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 80);
    b.rect(3, 0, 4, 80, 1);
    b.rect(1, 0, 2, 80, 2);
    b.rect(2, 70, 6, 8, 3);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(40, 40);
    b.ellipse(18, 20, 14, 14, 1);
    b.ellipse(14, 16, 3, 2, 2);
    b.ellipse(20, 24, 4, 2, 2);
    b.ellipse(28, 18, 11, 13, 0);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(7, 7);
    b.line(3, 0, 3, 6, 1, 1);
    b.line(0, 3, 6, 3, 1, 1);
    b.set(3, 3, 2);
    return b;
}

gs::Bitmap mothArt(bool up) {
    gs::Bitmap b(20, 12);
    if (up) {
        b.ellipse(5, 4, 4, 2.5f, 1);
        b.ellipse(15, 4, 4, 2.5f, 1);
    } else {
        b.ellipse(5, 8, 4, 2.5f, 1);
        b.ellipse(15, 8, 4, 2.5f, 1);
    }
    b.ellipse(10, 6, 2, 3, 2);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

void paintYard(gs::VDP& vdp, gs::TileAlloc& tiles) {
    uint8_t cob[64];
    uint8_t wall[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            bool grout = (y % 4 == 0) || (x % 4 == 0);
            cob[y * 8 + x] = uint8_t(grout ? 3 : ((x + y) & 1 ? 1 : 2));
            wall[y * 8 + x] = uint8_t(y == 0 ? 4 : (x % 7 == 0 ? 5 : 6));
        }
    int tc = tiles.shared(cob);
    int tw = tiles.shared(wall);
    for (int y = 18; y < 22; y++)
        for (int x = 0; x < 64; x++) vdp.B.set(x, y, gs::entry(tw, PAL_YARD));
    for (int y = 22; y < 32; y++)
        for (int x = 0; x < 64; x++) vdp.B.set(x, y, gs::entry(tc, PAL_YARD));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto C = gs::rgb4;
    uint16_t hud[16] = {};
    hud[1] = C(15, 14, 11);
    hud[15] = C(1, 1, 3);
    setPal(vdp, PAL_HUD, hud);

    uint16_t dark[16] = {};
    dark[1] = C(6, 5, 5);
    dark[2] = C(3, 3, 4);
    dark[3] = C(5, 3, 2);
    dark[4] = C(4, 2, 2);
    dark[5] = C(2, 2, 3);
    dark[6] = C(4, 3, 2);
    dark[13] = C(5, 4, 3);
    setPal(vdp, PAL_DARK, dark);

    auto lampPal = [&](int pal, int r, int g, int b) {
        uint16_t c[16] = {};
        c[1] = C(r, g, b);
        c[2] = C(std::max(0, r - 5), std::max(0, g - 5), std::max(0, b - 4));
        c[3] = C(6, 4, 2);
        c[4] = C(8, 3, 2);
        c[5] = C(std::min(15, r + 2), std::min(15, g + 3), std::min(15, b + 2));
        c[6] = C(4, 3, 2);
        c[13] = C(10, 8, 5);
        setPal(vdp, pal, c);
    };
    lampPal(PAL_L0, 14, 10, 3);
    lampPal(PAL_L1, 14, 5, 5);
    lampPal(PAL_L2, 5, 13, 6);
    lampPal(PAL_L3, 6, 8, 15);

    uint16_t scene[16] = {};
    scene[1] = C(12, 12, 10);
    scene[2] = C(7, 7, 8);
    scene[3] = C(5, 4, 3);
    setPal(vdp, PAL_SCENE, scene);

    uint16_t gold[16] = {};
    gold[1] = C(15, 12, 3);
    gold[2] = C(15, 15, 10);
    gold[3] = C(10, 7, 1);
    gold[4] = C(15, 8, 2);
    gold[5] = C(15, 14, 6);
    setPal(vdp, PAL_GOLD, gold);

    uint16_t rose[16] = {};
    rose[1] = C(15, 6, 7);
    setPal(vdp, PAL_ROSE, rose);
    uint16_t jade[16] = {};
    jade[1] = C(6, 15, 9);
    setPal(vdp, PAL_JADE, jade);
    uint16_t dim[16] = {};
    dim[1] = C(8, 8, 9);
    setPal(vdp, PAL_DIM, dim);

    uint16_t yard[16] = {};
    yard[1] = C(4, 4, 5);
    yard[2] = C(3, 3, 4);
    yard[3] = C(2, 2, 3);
    yard[4] = C(6, 5, 4);
    yard[5] = C(3, 2, 2);
    yard[6] = C(5, 4, 4);
    setPal(vdp, PAL_YARD, yard);

    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.wick = gs::uploadMipped(vdp, wickArt());
    art.cord = gs::uploadMipped(vdp, cordArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.moth[0] = gs::uploadMipped(vdp, mothArt(true));
    art.moth[1] = gs::uploadMipped(vdp, mothArt(false));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    vdp.B.clear();
    paintYard(vdp, tiles);
    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.HUD.enabled = true;
    vdp.setFogColor(C(1, 1, 3));
}

}  // namespace lanternmark
