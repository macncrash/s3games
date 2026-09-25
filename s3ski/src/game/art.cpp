#include "game/art.h"

#include <cmath>
#include <cstdint>

namespace ski {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void ink(gs::VDP& vdp, int pal, uint16_t main, uint16_t shadow) {
    setPal(vdp, pal, {0, main});
    vdp.setColor(pal * 16 + 15, shadow);
}

gs::Bitmap skierArt(bool carve) {
    gs::Bitmap b(48, 60);
    const float dx = carve ? -7.f : 0.f;
    b.line(13.f + dx * 0.15f, 57.f, 20.f + dx * 0.35f, 44.f, 7, 2.8f);
    b.line(35.f + dx * 0.15f, 57.f, 28.f + dx * 0.35f, 44.f, 7, 2.8f);
    b.line(13.f + dx * 0.15f, 57.f, 18.f + dx * 0.15f, 54.f, 4, 1.5f);
    b.line(35.f + dx * 0.15f, 57.f, 32.f + dx * 0.15f, 54.f, 4, 1.5f);
    b.rect(19.f + dx * 0.45f, 40.f, 5.f, 5.f, 11);
    b.rect(25.f + dx * 0.45f, 40.f, 5.f, 5.f, 11);
    b.poly({{20.f + dx * 0.5f, 41.f}, {24.f + dx * 0.5f, 41.f}, {25.f + dx * 0.75f, 29.f}, {21.f + dx * 0.75f, 29.f}}, 3);
    b.poly({{25.f + dx * 0.5f, 41.f}, {30.f + dx * 0.5f, 41.f}, {31.f + dx * 0.75f, 29.f}, {26.f + dx * 0.75f, 29.f}}, 3);
    b.rect(18.f + dx * 0.8f, 28.f, 14.f, 4.f, 8);
    b.poly({{11.f + dx, 30.f}, {37.f + dx, 30.f}, {39.f + dx, 18.f}, {9.f + dx, 18.f}}, 1);
    b.rect(22.f + dx, 18.f, 4.f, 12.f, 2);
    b.line(12.f + dx, 22.f, 4.f + dx * 0.2f, 34.f, 1, 3.2f);
    b.line(36.f + dx, 22.f, 44.f + dx * 0.2f, 34.f, 1, 3.2f);
    b.ellipse(4.f + dx * 0.2f, 35.f, 2.3f, 2.3f, 8);
    b.ellipse(44.f + dx * 0.2f, 35.f, 2.3f, 2.3f, 8);
    b.line(4.f + dx * 0.2f, 36.f, 1.f, 50.f, 9, 1.6f);
    b.line(44.f + dx * 0.2f, 36.f, 47.f, 50.f, 9, 1.6f);
    b.line(20.f + dx, 19.f, 8.f, 28.f, 2, 2.2f);
    b.ellipse(24.f + dx, 12.f, 8.2f, 8.4f, 5);
    b.ellipse(21.f + dx, 9.f, 2.4f, 1.6f, 4);
    b.rect(16.f + dx, 13.f, 16.f, 4.f, 8);
    b.rect(18.f + dx, 14.f, 5.f, 2.f, 6);
    b.rect(26.f + dx, 14.f, 5.f, 2.f, 10);
    b.outline(8, false);
    return b;
}

gs::Bitmap fallArt() {
    gs::Bitmap b(64, 32);
    b.line(6, 26, 58, 22, 7, 2.6f);
    b.line(8, 29, 60, 26, 7, 2.4f);
    b.ellipse(18, 16, 8, 7, 5);
    b.ellipse(16, 14, 2.2f, 1.4f, 4);
    b.rect(14, 17, 10, 3, 8);
    b.rect(16, 18, 3, 1, 6);
    b.ellipse(34, 18, 12, 7, 1);
    b.rect(30, 16, 4, 6, 2);
    b.ellipse(50, 20, 8, 5, 3);
    b.line(22, 20, 10, 12, 1, 2.4f);
    b.line(40, 16, 48, 8, 9, 1.4f);
    b.ellipse(10, 12, 2, 2, 8);
    b.outline(8, false);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(kPoleW, 64);
    b.rect(6, 4, 3, 56, 1);
    b.rect(6, 4, 1, 56, 4);
    for (int y = 12; y < 58; y += 8) b.rect(6, y, 3, 4, 5);
    b.rect(5, 58, 5, 3, 5);
    b.poly({{10, 6}, {26, 15}, {10, 26}}, 2);
    b.poly({{10, 10}, {21, 15}, {10, 22}}, 3);
    b.rect(6, 2, 3, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(40, 56);
    b.rect(17, 40, 6, 13, 4);
    b.rect(14, 51, 12, 3, 5);
    b.poly({{20, 46}, {3, 40}, {37, 40}}, 2);
    b.poly({{20, 34}, {6, 28}, {34, 28}}, 1);
    b.poly({{20, 24}, {9, 16}, {31, 16}}, 2);
    b.poly({{20, 14}, {13, 6}, {27, 6}}, 3);
    b.poly({{20, 42}, {10, 36}, {20, 32}}, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap rockArt() {
    gs::Bitmap b(28, 18);
    b.poly({{2, 16}, {8, 8}, {14, 5}, {22, 9}, {26, 16}}, 1);
    b.poly({{6, 16}, {10, 10}, {16, 8}, {18, 16}}, 2);
    b.line(8, 9, 12, 7, 3, 1.2f);
    b.outline(3, false);
    return b;
}

gs::Bitmap manArt() {
    gs::Bitmap b(28, 36);
    b.ellipse(14, 26, 9, 8, 1);
    b.ellipse(14, 24, 6, 5, 2);
    b.ellipse(14, 14, 6.2f, 6.2f, 1);
    b.ellipse(12, 12, 2, 1.4f, 2);
    b.rect(10, 3, 8, 5, 3);
    b.rect(8, 7, 12, 2, 3);
    b.rect(17, 14, 5, 1, 4);
    b.rect(18, 16, 2, 2, 5);
    b.line(6, 22, 1, 16, 6, 1.6f);
    b.line(22, 22, 27, 16, 6, 1.6f);
    b.set(11, 13, 3);
    b.set(16, 13, 3);
    b.outline(3, false);
    return b;
}

gs::Bitmap mountArt() {
    gs::Bitmap b(80, 44);
    b.poly({{2, 43}, {28, 8}, {52, 43}}, 1);
    b.poly({{36, 43}, {58, 14}, {78, 43}}, 2);
    b.poly({{18, 20}, {28, 8}, {38, 20}}, 3);
    b.poly({{50, 24}, {58, 14}, {66, 24}}, 4);
    b.line(28, 8, 30, 16, 3, 1.2f);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(32, 32);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785398f;
        float c = std::cos(a), s = std::sin(a);
        b.line(16 + c * 8, 16 + s * 8, 16 + c * 15, 16 + s * 15, 1, 1.6f);
    }
    b.ellipse(16, 16, 8, 8, 1);
    b.ellipse(14, 14, 3.2f, 3.2f, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(40, 18);
    b.ellipse(14, 10, 10, 6, 1);
    b.ellipse(24, 9, 12, 7, 1);
    b.ellipse(20, 12, 8, 4, 2);
    b.ellipse(10, 11, 4, 3, 2);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(48, 18);
    for (int y = 2; y < 16; y++) {
        for (int x = 1; x < 47; x++) {
            bool on = ((x / 6) ^ (y / 6)) & 1;
            b.set(x, y, on ? 2 : 3);
        }
    }
    b.rect(0, 0, 48, 2, 1);
    b.rect(0, 16, 48, 2, 1);
    b.rect(0, 2, 2, 14, 1);
    b.rect(46, 2, 2, 14, 1);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 9, 6, 5, 1);
    b.ellipse(5, 8, 3, 2.4f, 2);
    b.ellipse(11, 7, 3.2f, 2.6f, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 10);
    b.ellipse(14, 5, 12, 3.4f, 1);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 2, 4);
    ink(vdp, PAL_HUD, gs::rgb4(15, 15, 15), shadow);
    ink(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(4, 2, 0));
    ink(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(4, 0, 0));
    ink(vdp, PAL_GREEN, gs::rgb4(6, 15, 7), gs::rgb4(0, 3, 1));
    vdp.setColor(PAL_GOLD * 16 + 2, gs::rgb4(6, 3, 0));
    vdp.setColor(PAL_GREEN * 16 + 2, gs::rgb4(0, 5, 1));

    setPal(vdp, PAL_SKIER,
           {0, gs::rgb4(14, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(2, 3, 8), gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15),
            gs::rgb4(15, 10, 2), gs::rgb4(15, 13, 2), gs::rgb4(1, 1, 2), gs::rgb4(10, 11, 12), gs::rgb4(15, 14, 8),
            gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_GATE_R,
           {0, gs::rgb4(12, 1, 1), gs::rgb4(15, 3, 2), gs::rgb4(9, 0, 0), gs::rgb4(15, 15, 15), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_GATE_B,
           {0, gs::rgb4(1, 3, 11), gs::rgb4(3, 7, 15), gs::rgb4(1, 2, 7), gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 3)});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(3, 11, 4), gs::rgb4(2, 8, 3), gs::rgb4(6, 14, 6), gs::rgb4(8, 5, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(8, 9, 10), gs::rgb4(5, 6, 7), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_MAN,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), gs::rgb4(2, 2, 3), gs::rgb4(15, 8, 2), gs::rgb4(14, 3, 2),
            gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15)});
    setPal(vdp, PAL_BANNER,
           {0, gs::rgb4(2, 2, 4), gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(15, 14, 12), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_MOUNT,
           {0, gs::rgb4(6, 8, 11), gs::rgb4(5, 7, 10), gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 12)});

    // Packed piste. Indices match the road generator: 6/7 corduroy, 9 tracks, banks in 1/2/14.
    const uint16_t snow[16] = {0,
                               gs::rgb4(12, 14, 15),
                               gs::rgb4(9, 12, 14),
                               gs::rgb4(7, 10, 12),
                               gs::rgb4(14, 15, 15),
                               gs::rgb4(11, 13, 15),
                               gs::rgb4(15, 15, 15),
                               gs::rgb4(12, 14, 15),
                               gs::rgb4(8, 11, 13),
                               gs::rgb4(10, 13, 15),
                               gs::rgb4(15, 15, 15),
                               gs::rgb4(13, 15, 15),
                               gs::rgb4(11, 13, 15),
                               gs::rgb4(14, 15, 15),
                               gs::rgb4(15, 15, 15),
                               gs::rgb4(13, 15, 15)};
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, snow[i]);

    vdp.setFogColor(gs::rgb4(10, 13, 15));
    loadFont(vdp, art);
    art.skier = gs::uploadMipped(vdp, skierArt(false));
    art.carve = gs::uploadMipped(vdp, skierArt(true));
    art.fall = gs::uploadMipped(vdp, fallArt());
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.man = gs::uploadMipped(vdp, manArt());
    art.mount = gs::uploadMipped(vdp, mountArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 SKI", {3, 1, 2, 0, 1}));
}

}  // namespace ski
