#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace rail {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 3));
}

void wheel(gs::Bitmap& b, float cx, float cy, float phase) {
    b.ellipse(cx, cy, 8, 8, 7);
    b.ellipse(cx, cy, 3, 3, 1);
    for (int k = 0; k < 3; k++) {
        float a = phase + k * 1.0472f;
        b.line(cx, cy, cx + std::cos(a) * 7.f, cy + std::sin(a) * 7.f, 8, 1.f);
    }
}

gs::Bitmap locoArt(int phase) {
    gs::Bitmap b(144, 70);
    b.rect(4, 52, 132, 5, 1);
    b.rect(4, 52, 132, 2, 12);
    b.rect(8, 28, 70, 26, 2);
    b.rect(8, 28, 70, 4, 11);
    b.ellipse(28, 29, 6, 3, 1);
    b.ellipse(50, 29, 6, 3, 1);
    b.line(12, 40, 74, 40, 3, 1);
    b.rect(76, 14, 36, 40, 2);
    b.rect(76, 14, 36, 4, 11);
    b.rect(80, 20, 26, 14, 4);
    b.rect(82, 22, 10, 8, 9);
    b.rect(100, 34, 8, 18, 3);
    b.rect(104, 42, 2, 2, 8);
    b.rect(82, 36, 16, 8, 5);
    b.blit(gs::textBitmap("S3", {1, 1, 0, 0, 1}), 84, 37);
    b.ellipse(98, 12, 3, 3, 12);
    b.rect(112, 32, 22, 22, 2);
    b.rect(112, 32, 22, 3, 11);
    b.poly({{132, 34}, {143, 44}, {132, 48}}, 3);
    b.rect(134, 42, 8, 6, 10);
    b.rect(136, 44, 3, 2, 9);
    b.rect(118, 52, 25, 6, 6);
    b.rect(122, 57, 3, 4, 1);
    b.rect(136, 57, 3, 4, 1);
    b.rect(16, 54, 40, 8, 7);
    b.rect(96, 54, 34, 8, 7);
    float p = phase * 1.1f;
    wheel(b, 26, 60, p);
    wheel(b, 46, 60, p);
    wheel(b, 104, 60, p);
    wheel(b, 120, 60, p);
    float rod = 60.f + std::sin(p) * 4.f;
    b.line(26, rod, 46, rod, 8, 2);
    b.line(104, rod, 120, rod, 8, 2);
    b.line(10, 26, 74, 26, 8, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap stationArt(const char* name) {
    gs::Bitmap b(BOX_W, 72);
    b.rect(0, 52, BOX_W, 18, 10);
    b.rect(0, 52, BOX_W, 2, 9);
    b.rect(0, 68, BOX_W, 3, 9);
    b.rect(0, 52, 3, 18, 9);
    b.rect(BOX_W - 3, 52, 3, 18, 9);
    for (int x = 10; x < BOX_W - 4; x += 16) b.rect(x, 58, 2, 7, 9);
    b.rect(6, 42, BOX_W - 12, 10, 7);
    b.rect(6, 42, BOX_W - 12, 2, 8);
    b.rect(30, 22, 4, 22, 6);
    b.rect(86, 22, 4, 22, 6);
    b.poly({{24, 24}, {60, 6}, {96, 24}}, 3);
    b.rect(26, 20, 68, 4, 4);
    b.rect(36, 26, 48, 14, 5);
    gs::Bitmap label = gs::textBitmap(name, {1, 1, 0, 0, 1});
    b.blit(label, 36 + (48 - label.w) / 2, 29);
    b.rect(46, 40, 28, 3, 2);
    b.rect(16, 28, 2, 14, 1);
    b.ellipse(17, 26, 3, 3, 9);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(32, 24);
    b.rect(0, 2, 32, 20, 13);
    b.rect(0, 5, 32, 14, 12);
    for (int x = 1; x < 32; x += 8) b.rect(x, 7, 5, 9, 11);
    b.rect(0, 6, 32, 2, 10);
    b.rect(0, 14, 32, 2, 10);
    b.rect(0, 6, 32, 1, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(36, 58);
    b.rect(16, 32, 5, 24, 5);
    b.ellipse(18, 22, 14, 16, 2);
    b.ellipse(12, 18, 8, 8, 3);
    b.ellipse(24, 16, 6, 6, 4);
    return b;
}

gs::Bitmap hillArt() {
    gs::Bitmap b(120, 46);
    b.poly({{0, 45}, {24, 26}, {48, 34}, {78, 10}, {104, 28}, {119, 45}}, 6);
    b.poly({{46, 45}, {78, 18}, {108, 45}}, 7);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(56, 24);
    b.ellipse(16, 14, 14, 8, 9);
    b.ellipse(32, 12, 16, 9, 8);
    b.ellipse(46, 14, 10, 7, 8);
    return b;
}

gs::Bitmap signalArt() {
    gs::Bitmap b(20, 58);
    b.rect(9, 18, 3, 40, 1);
    b.rect(3, 6, 14, 16, 1);
    b.rect(5, 9, 10, 10, 6);
    b.rect(1, 24, 18, 3, 3);
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(16, 44);
    b.rect(7, 14, 3, 30, 1);
    b.rect(1, 2, 14, 14, 9);
    b.line(4, 12, 8, 5, 1, 1);
    b.line(8, 5, 12, 12, 1, 1);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 6, 3);
    b.ellipse(8, 8, 4, 3, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 9, 9, 4);
    b.ellipse(11, 11, 4, 4, 5);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(14, 64);
    b.rect(6, 8, 2, 56, 1);
    b.rect(0, 8, 14, 2, 1);
    b.rect(1, 10, 2, 2, 10);
    b.rect(11, 10, 2, 2, 10);
    return b;
}

gs::Bitmap panelArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 7, 1);
    b.rect(0, 7, 8, 1, 2);
    return b;
}

gs::Image words(gs::VDP& vdp, const char* s, int scale) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {scale, 1, 0, 15, 1}));
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 96; c++) {
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
        art.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_DIM, gs::rgb4(10, 12, 14));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(4, 15, 7));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));

    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 6, 11), gs::rgb4(1, 3, 6), gs::rgb4(10, 14, 15), gs::rgb4(14, 13, 9),
            gs::rgb4(13, 2, 2), gs::rgb4(2, 2, 3), gs::rgb4(9, 9, 10), gs::rgb4(15, 15, 15), gs::rgb4(15, 14, 4),
            gs::rgb4(1, 2, 4), gs::rgb4(14, 11, 2), 0, 0, 0});

    setPal(vdp, PAL_STATION,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(6, 3, 2), gs::rgb4(8, 2, 2), gs::rgb4(12, 5, 3), gs::rgb4(14, 12, 8),
            gs::rgb4(3, 2, 2), gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 4), gs::rgb4(15, 13, 2), gs::rgb4(11, 8, 2),
            gs::rgb4(7, 9, 11), 0, 0, 0, 0});

    // 1 pole, 2-4 foliage, 5 trunk, 6-7 hill, 8-9 cloud, 10 rail, 11 tie, 12-13 ballast
    setPal(vdp, PAL_SCENERY,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(1, 5, 2), gs::rgb4(3, 8, 3), gs::rgb4(5, 11, 4), gs::rgb4(6, 4, 2),
            gs::rgb4(4, 6, 4), gs::rgb4(6, 8, 5), gs::rgb4(14, 14, 15), gs::rgb4(10, 11, 12), gs::rgb4(8, 8, 9),
            gs::rgb4(6, 4, 2), gs::rgb4(7, 6, 5), gs::rgb4(4, 4, 3), 0, 0});

    uint16_t post = gs::rgb4(2, 2, 3), postHi = gs::rgb4(6, 6, 7), arm = gs::rgb4(12, 10, 3);
    setPal(vdp, PAL_GO, {0, post, postHi, arm, 0, 0, gs::rgb4(2, 14, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_STOP, {0, post, postHi, arm, 0, 0, gs::rgb4(15, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0});

    setPal(vdp, PAL_FX,
           {0, gs::rgb4(15, 15, 15), 0, gs::rgb4(9, 9, 10), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 11), 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0});
    setPal(vdp, PAL_PANEL, {0, gs::rgb4(1, 2, 5), gs::rgb4(10, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    loadFont(vdp, art);
    art.loco[0] = gs::uploadImage(vdp, locoArt(0));
    art.loco[1] = gs::uploadImage(vdp, locoArt(1));
    const char* names[4] = {"MILL", "PIER", "FORK", "TERM"};
    for (int i = 0; i < 4; i++) art.stop[i] = gs::uploadImage(vdp, stationArt(names[i]));
    art.rail = gs::uploadImage(vdp, railArt());
    art.tree = gs::uploadImage(vdp, treeArt());
    art.hill = gs::uploadImage(vdp, hillArt());
    art.cloud = gs::uploadImage(vdp, cloudArt());
    art.signal = gs::uploadImage(vdp, signalArt());
    art.board = gs::uploadImage(vdp, boardArt());
    art.puff = gs::uploadImage(vdp, puffArt());
    art.sun = gs::uploadImage(vdp, sunArt());
    art.pole = gs::uploadImage(vdp, poleArt());
    art.panel = gs::uploadImage(vdp, panelArt());
    art.logo = words(vdp, "S3 RAIL", 4);
    art.tag = words(vdp, "LATE COSTS THE RUN", 2);
    art.banOn = words(vdp, "ON TIME", 3);
    art.banLate = words(vdp, "LATE", 3);
    art.banMade = words(vdp, "RUN MADE", 3);
    art.banRan = words(vdp, "RAN THE BOX", 3);
}

}  // namespace rail
