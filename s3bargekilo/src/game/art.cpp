#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace bkilo {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 15) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap bargeArt() {
    gs::Bitmap b(96, 36);
    b.poly({{4.f, 22.f}, {86.f, 18.f}, {94.f, 24.f}, {88.f, 32.f}, {8.f, 32.f}}, 2);
    b.poly({{10.f, 24.f}, {84.f, 22.f}, {86.f, 32.f}, {12.f, 32.f}}, 3);
    b.rect(8, 14, 74, 8, 1);
    b.rect(18, 8, 28, 8, 4);
    b.rect(20, 10, 6, 4, 6);
    b.rect(28, 10, 6, 4, 6);
    b.rect(36, 10, 6, 4, 6);
    b.rect(50, 6, 16, 10, 5);
    b.rect(54, 2, 4, 6, 7);
    b.poly({{6.f, 14.f}, {14.f, 10.f}, {6.f, 18.f}}, 8);
    b.rect(70, 16, 10, 3, 9);
    gs::TextStyle st{1, 10, 0, 0, 0};
    b.blit(gs::textBitmap("NELL", st), 22, 24);
    b.outline(15, false);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(72, 28);
    b.poly({{4.f, 16.f}, {64.f, 14.f}, {70.f, 18.f}, {64.f, 26.f}, {6.f, 26.f}}, 1);
    b.rect(10, 10, 46, 6, 2);
    b.rect(16, 6, 18, 6, 3);
    b.rect(40, 4, 10, 8, 4);
    b.rect(42, 1, 3, 4, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap wheelArt(int frame) {
    gs::Bitmap b(40, 40);
    b.ellipse(20, 20, 16, 16, 1);
    b.ellipse(20, 20, 12, 12, 2);
    b.ellipse(20, 20, 4, 4, 3);
    float a0 = frame * 0.785f;
    for (int s = 0; s < 4; s++) {
        float a = a0 + s * 1.5708f;
        float c = std::cos(a), sn = std::sin(a);
        b.line(20 - c * 4, 20 - sn * 4, 20 + c * 14, 20 + sn * 14, 4, 2.f);
    }
    b.ellipse(20, 20, 17, 17, 5);
    return b;
}

gs::Bitmap millArt() {
    gs::Bitmap b(48, 56);
    b.rect(6, 20, 36, 36, 1);
    b.poly({{2.f, 20.f}, {24.f, 4.f}, {46.f, 20.f}}, 2);
    b.rect(18, 36, 10, 20, 3);
    b.rect(10, 26, 8, 8, 4);
    b.rect(30, 26, 8, 8, 4);
    b.rect(22, 6, 4, 8, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap craneArt() {
    gs::Bitmap b(28, 64);
    b.rect(4, 8, 8, 56, 1);
    b.rect(8, 6, 16, 6, 2);
    b.rect(20, 10, 4, 18, 1);
    b.rect(2, 58, 14, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap cartArt() {
    gs::Bitmap b(52, 24);
    b.rect(4, 4, 40, 10, 1);
    b.poly({{4.f, 4.f}, {16.f, 4.f}, {12.f, 0.f}, {4.f, 0.f}}, 2);
    b.rect(8, 6, 8, 5, 3);
    b.rect(36, 14, 8, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(16, 28);
    b.line(4, 26, 3, 4, 1, 1.5f);
    b.line(8, 26, 9, 2, 2, 1.5f);
    b.line(12, 26, 13, 8, 1, 1.5f);
    b.set(3, 3, 3);
    b.set(9, 1, 3);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(36, 44);
    b.rect(15, 26, 6, 16, 3);
    b.ellipse(18, 18, 14, 14, 1);
    b.ellipse(12, 16, 6, 7, 2);
    b.outline(4, false);
    return b;
}

gs::Bitmap houseArt() {
    gs::Bitmap b(40, 32);
    b.rect(6, 14, 28, 16, 1);
    b.poly({{2.f, 14.f}, {20.f, 2.f}, {38.f, 14.f}}, 2);
    b.rect(16, 20, 8, 10, 3);
    b.rect(9, 18, 6, 5, 4);
    b.rect(26, 18, 6, 5, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap waterArt() {
    gs::Bitmap b(32, 16);
    b.rect(0, 0, 32, 16, 1);
    for (int x = 0; x < 32; x += 8) b.rect(x, 4, 5, 2, 2);
    for (int x = 4; x < 32; x += 8) b.rect(x, 10, 5, 2, 3);
    return b;
}

gs::Bitmap bankArt() {
    gs::Bitmap b(48, 20);
    b.rect(0, 8, 48, 12, 1);
    b.poly({{0.f, 10.f}, {8.f, 4.f}, {18.f, 9.f}, {30.f, 2.f}, {42.f, 8.f}, {48.f, 6.f}, {48.f, 20.f}, {0.f, 20.f}}, 2);
    for (int x = 3; x < 46; x += 7) b.rect(x, 6, 2, 5, 3);
    return b;
}

gs::Bitmap bannerArt() {
    gs::TextStyle st{2, 1, 0, 0, 1};
    gs::Bitmap word = gs::textBitmap("1 KM", st);
    gs::Bitmap b(word.w + 12, word.h + 10);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.rect(2, 2, float(b.w - 4), float(b.h - 4), 3);
    b.blit(word, 6, 5);
    b.outline(4, false);
    return b;
}

gs::Bitmap gullArt(bool up) {
    gs::Bitmap b(20, 10);
    if (up) {
        b.line(1, 8, 8, 3, 1, 1.4f);
        b.line(8, 3, 18, 7, 1, 1.4f);
    } else {
        b.line(1, 3, 8, 6, 1, 1.4f);
        b.line(8, 6, 18, 2, 1, 1.4f);
    }
    b.set(9, 5, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(10, 7, 8, 4, 1);
    b.ellipse(18, 6, 8, 5, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 5, 5, 1);
    b.line(8, 0, 8, 3, 2, 1.f);
    b.line(8, 13, 8, 16, 2, 1.f);
    b.line(0, 8, 3, 8, 2, 1.f);
    b.line(13, 8, 16, 8, 2, 1.f);
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
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 8));

    setPal(vdp, PAL_BOAT,
           {0, gs::rgb4(12, 8, 4), gs::rgb4(6, 3, 2), gs::rgb4(9, 5, 3), gs::rgb4(14, 13, 10), gs::rgb4(4, 5, 7),
            gs::rgb4(8, 12, 14), gs::rgb4(3, 3, 3), gs::rgb4(13, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(15, 14, 8),
            gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(5, 6, 8), gs::rgb4(8, 9, 11), gs::rgb4(12, 6, 4), gs::rgb4(4, 4, 5), gs::rgb4(10, 10, 8),
            gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_MILL,
           {0, gs::rgb4(9, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 5), gs::rgb4(3, 3, 3), gs::rgb4(14, 12, 8),
            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_CRANE,
           {0, gs::rgb4(7, 8, 9), gs::rgb4(11, 12, 12), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3), gs::rgb4(14, 10, 3)});
    setPal(vdp, PAL_CART,
           {0, gs::rgb4(8, 4, 2), gs::rgb4(12, 7, 3), gs::rgb4(6, 10, 14), gs::rgb4(3, 3, 3), gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_WATER,
           {0, gs::rgb4(2, 6, 10), gs::rgb4(4, 9, 13), gs::rgb4(7, 12, 14), gs::rgb4(1, 4, 7)});
    setPal(vdp, PAL_BANK,
           {0, gs::rgb4(5, 8, 3), gs::rgb4(7, 11, 4), gs::rgb4(10, 12, 5), gs::rgb4(4, 6, 2)});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(3, 8, 3), gs::rgb4(6, 11, 4), gs::rgb4(6, 4, 2), gs::rgb4(2, 4, 2)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 13), gs::rgb4(14, 12, 6), gs::rgb4(8, 10, 12)});
    setPal(vdp, PAL_BANNER,
           {0, gs::rgb4(15, 14, 8), gs::rgb4(10, 2, 2), gs::rgb4(14, 12, 6), gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(14, 14, 12), gs::rgb4(8, 6, 3)});

    art.barge = gs::uploadMipped(vdp, bargeArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    for (int i = 0; i < 4; i++) art.wheel[i] = gs::uploadMipped(vdp, wheelArt(i));
    art.mill = gs::uploadMipped(vdp, millArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.house = gs::uploadMipped(vdp, houseArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(true));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(false));
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    loadFont(vdp, art);
}

}  // namespace bkilo
