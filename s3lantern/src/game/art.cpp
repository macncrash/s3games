#include "game/art.h"

#include <algorithm>

namespace lantern {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

void lampPal(gs::VDP& vdp, int pal, int pr, int pg, int pb, int hr, int hg, int hb) {
    auto C = gs::rgb4;
    auto lo = [](int v, int d) { return std::max(0, v - d); };
    auto hi = [](int v) { return std::min(15, v); };
    uint16_t c[16] = {};
    c[1] = C(pr, pg, pb);
    c[2] = C(lo(pr, 5), lo(pg, 5), lo(pb, 4));
    c[3] = C(7, 4, 2);
    c[4] = C(9, 3, 2);
    c[5] = C(hr, hg, hb);
    c[6] = C(5, 4, 3);
    c[7] = C(hi(pr * 2 / 5), hi(pg * 2 / 5), hi(pb * 2 / 5));
    c[8] = C(hi(pr * 3 / 4 + 1), hi(pg * 3 / 4 + 1), hi(pb * 3 / 4));
    c[9] = C(hi((pr + hr) / 2 + 2), hi((pg + hg) / 2 + 2), hi((pb + hb) / 2 + 1));
    c[10] = C(hr, hg, hb);
    c[11] = C(15, 15, 12);
    c[12] = C(2, 1, 1);
    c[13] = C(12, 10, 6);
    c[15] = C(1, 1, 2);
    setPal(vdp, pal, c);
}

void mark(gs::Bitmap& b, int kind) {
    if (kind == 0) {
        b.ellipse(20, 20, 3.2f, 3.2f, 12);
        b.ellipse(20, 20, 1.6f, 1.6f, 1);
    } else if (kind == 1) {
        b.rect(19, 16, 2, 8, 12);
    } else if (kind == 2) {
        b.ellipse(16, 20, 1.6f, 1.6f, 12);
        b.ellipse(24, 20, 1.6f, 1.6f, 12);
    } else if (kind == 3) {
        b.rect(14, 19, 12, 2, 12);
    } else if (kind == 4) {
        b.poly({{20, 16}, {24, 20}, {20, 24}, {16, 20}}, 12);
    } else {
        b.line(16, 17, 24, 24, 12, 1.3f);
        b.line(24, 17, 16, 24, 12, 1.3f);
    }
}

gs::Bitmap lantern(int kind) {
    gs::Bitmap b(40, 60);
    b.line(20, 0, 20, 6, 6, 1.2f);
    b.poly({{11, 6}, {29, 6}, {32, 13}, {8, 13}}, 13);
    b.rect(10, 11, 20, 3, 3);
    b.ellipse(20, 32, 15, 16, 1);
    b.ellipse(26, 32, 6, 14, 2);
    b.ellipse(20, 34, 6, 7, 5);
    b.line(9, 26, 31, 26, 2, 1);
    b.line(7, 32, 33, 32, 2, 1);
    b.line(9, 38, 31, 38, 2, 1);
    b.line(13, 44, 27, 44, 2, 1);
    mark(b, kind);
    b.poly({{10, 46}, {30, 46}, {26, 51}, {14, 51}}, 13);
    b.rect(14, 50, 12, 2, 3);
    b.line(20, 52, 20, 58, 4, 1.2f);
    b.line(20, 55, 16, 59, 4, 1);
    b.line(20, 55, 24, 59, 4, 1);
    b.set(20, 54, 13);
    return b;
}

gs::Bitmap glowArt() {
    gs::Bitmap b(64, 64);
    b.ellipse(32, 32, 30, 28, 7);
    b.ellipse(32, 32, 18, 18, 8);
    b.ellipse(32, 32, 8, 8, 9);
    return b;
}

gs::Bitmap flameArt(int frame) {
    gs::Bitmap b(14, 18);
    if (frame == 0) {
        b.poly({{7, 1}, {12, 12}, {2, 12}}, 10);
        b.ellipse(7, 12, 5, 4, 10);
        b.poly({{7, 5}, {10, 14}, {4, 14}}, 11);
    } else {
        b.poly({{8, 2}, {12, 13}, {3, 12}}, 10);
        b.ellipse(7, 13, 4, 3, 10);
        b.ellipse(7, 11, 2, 3, 11);
    }
    return b;
}

gs::Bitmap wickArt() {
    gs::Bitmap b(12, 10);
    b.poly({{6, 0}, {11, 6}, {6, 9}, {1, 6}}, 11);
    b.poly({{6, 3}, {8, 6}, {6, 8}, {4, 6}}, 5);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(48, 48);
    b.ellipse(22, 24, 16, 16, 1);
    b.ellipse(16, 20, 3, 2, 2);
    b.ellipse(24, 28, 4, 2, 2);
    b.ellipse(26, 18, 2, 2, 2);
    b.ellipse(34, 22, 12, 14, 0);
    return b;
}

gs::Bitmap mothArt(bool up) {
    gs::Bitmap b(22, 14);
    if (up) {
        b.ellipse(6, 5, 5, 3, 5);
        b.ellipse(16, 5, 5, 3, 5);
    } else {
        b.ellipse(6, 9, 5, 3, 5);
        b.ellipse(16, 9, 5, 3, 5);
    }
    b.ellipse(11, 7, 2, 4, 6);
    b.set(10, 5, 9);
    b.set(12, 5, 9);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(5, 5);
    for (int i = 0; i < 5; i++) b.set(2, i, 9);
    for (int i = 0; i < 5; i++) b.set(i, 2, 9);
    b.set(2, 2, 1);
    return b;
}

gs::Bitmap cordArt() {
    gs::Bitmap b(2, 16);
    b.rect(0, 0, 2, 16, 11);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(304, 10);
    b.rect(0, 3, 304, 4, 3);
    b.rect(0, 7, 304, 2, 11);
    for (int x = 6; x < 300; x += 28) b.rect(float(x), 2, 3, 7, 11);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 100);
    b.rect(1, 0, 10, 8, 3);
    b.rect(3, 6, 6, 94, 3);
    b.rect(3, 6, 2, 94, 11);
    return b;
}

gs::Bitmap flyArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2, 2, 1.6f, 1.6f, 11);
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(36, 12);
    b.ellipse(18, 6, 16, 4, 1);
    return b;
}

gs::Bitmap paneArt() {
    gs::Bitmap b(12, 10);
    b.rect(0, 0, 12, 10, 7);
    b.rect(3, 2, 6, 5, 1);
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(320, 64);
    for (int i = 0; i < 4; i++) {
        float x = float(i * 80);
        b.poly({{x, 28}, {x + 40, 6}, {x + 80, 28}}, 4);
    }
    b.rect(0, 24, 320, 6, 3);
    b.rect(0, 30, 320, 30, 8);
    b.rect(0, 58, 320, 6, 3);
    const int wx[5] = {24, 88, 152, 216, 280};
    for (int x : wx) {
        b.rect(float(x), 40, 18, 14, 10);
        b.rect(float(x + 3), 43, 12, 8, 7);
        b.rect(float(x + 8), 43, 2, 8, 10);
    }
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

void paintStreet(gs::VDP& vdp, gs::TileAlloc& tiles) {
    uint8_t cob[64];
    uint8_t curb[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            bool grout = y == 0 || x == 0 || x == 7;
            int c = grout ? 3 : ((x * 3 + y) % 7 == 0 ? 4 : (x < 4 ? 1 : 2));
            cob[y * 8 + x] = uint8_t(c);
            curb[y * 8 + x] = uint8_t(y < 3 ? 5 : c);
        }
    int tc = tiles.shared(cob);
    int tu = tiles.shared(curb);
    for (int y = 20; y < 32; y++)
        for (int x = 0; x < 64; x++) vdp.B.set(x, y, gs::entry(tc, PAL_YARD));
    for (int x = 0; x < 64; x++) vdp.B.set(x, 19, gs::entry(tu, PAL_YARD));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto C = gs::rgb4;
    uint16_t hud[16] = {};
    hud[1] = C(15, 14, 12);
    hud[15] = C(1, 1, 3);
    setPal(vdp, PAL_HUD, hud);

    uint16_t dark[16] = {};
    dark[1] = C(7, 6, 5);
    dark[2] = C(4, 3, 3);
    dark[3] = C(5, 3, 2);
    dark[4] = C(4, 2, 2);
    dark[5] = C(3, 3, 3);
    dark[6] = C(4, 3, 2);
    dark[7] = dark[8] = dark[9] = C(2, 2, 3);
    dark[10] = dark[11] = C(3, 3, 3);
    dark[12] = C(2, 1, 1);
    dark[13] = C(6, 5, 4);
    dark[15] = C(1, 1, 2);
    setPal(vdp, PAL_DARK, dark);

    lampPal(vdp, PAL_L0, 15, 12, 4, 15, 15, 10);
    lampPal(vdp, PAL_L1, 15, 6, 6, 15, 13, 12);
    lampPal(vdp, PAL_L2, 6, 14, 7, 14, 15, 12);
    lampPal(vdp, PAL_L3, 5, 8, 15, 12, 14, 15);
    lampPal(vdp, PAL_L4, 12, 6, 15, 15, 12, 15);
    lampPal(vdp, PAL_L5, 15, 9, 2, 15, 14, 8);

    uint16_t scene[16] = {};
    scene[1] = C(15, 14, 9);
    scene[2] = C(11, 9, 6);
    scene[3] = C(9, 6, 3);
    scene[4] = C(5, 2, 2);
    scene[5] = C(12, 12, 9);
    scene[6] = C(4, 3, 2);
    scene[7] = C(15, 11, 4);
    scene[8] = C(3, 3, 6);
    scene[9] = C(15, 15, 13);
    scene[10] = C(2, 2, 4);
    scene[11] = C(3, 2, 1);
    scene[15] = C(1, 1, 2);
    setPal(vdp, PAL_SCENE, scene);

    uint16_t yard[16] = {};
    yard[1] = C(6, 6, 7);
    yard[2] = C(4, 4, 5);
    yard[3] = C(2, 2, 3);
    yard[4] = C(3, 5, 3);
    yard[5] = C(8, 7, 6);
    setPal(vdp, PAL_YARD, yard);

    uint16_t no[16] = {};
    no[1] = C(15, 3, 3);
    no[2] = C(10, 1, 1);
    no[3] = C(6, 2, 2);
    no[4] = C(8, 2, 2);
    no[5] = C(15, 14, 12);
    no[6] = C(5, 2, 2);
    no[7] = C(10, 1, 1);
    no[8] = C(14, 3, 2);
    no[9] = C(15, 8, 6);
    no[10] = C(15, 6, 3);
    no[11] = C(15, 15, 12);
    no[12] = C(3, 0, 0);
    no[13] = C(15, 10, 8);
    no[15] = C(2, 0, 0);
    setPal(vdp, PAL_NO, no);

    uint16_t gold[16] = {};
    gold[1] = C(15, 12, 4);
    gold[15] = C(3, 1, 0);
    setPal(vdp, PAL_GOLD, gold);
    uint16_t rose[16] = {};
    rose[1] = C(15, 7, 6);
    rose[15] = C(3, 0, 1);
    setPal(vdp, PAL_ROSE, rose);
    uint16_t jade[16] = {};
    jade[1] = C(8, 15, 8);
    jade[15] = C(0, 2, 1);
    setPal(vdp, PAL_JADE, jade);
    uint16_t dim[16] = {};
    dim[1] = C(9, 10, 13);
    dim[15] = C(1, 1, 3);
    setPal(vdp, PAL_DIM, dim);

    vdp.setFogColor(C(1, 1, 4));
    vdp.A.enabled = true;
    vdp.B.enabled = true;

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    paintStreet(vdp, tiles);
    gs::bitmapToPlane(tiles, vdp.A, 0, 11, houseArt(), PAL_SCENE);

    for (int i = 0; i < 6; i++) art.lamp[i] = gs::uploadMipped(vdp, lantern(i));
    art.glow = gs::uploadMipped(vdp, glowArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.wick = gs::uploadMipped(vdp, wickArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.moth[0] = gs::uploadMipped(vdp, mothArt(true));
    art.moth[1] = gs::uploadMipped(vdp, mothArt(false));
    art.star = gs::uploadMipped(vdp, starArt());
    art.cord = gs::uploadMipped(vdp, cordArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.fly = gs::uploadMipped(vdp, flyArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.pane = gs::uploadMipped(vdp, paneArt());
}

}  // namespace lantern
