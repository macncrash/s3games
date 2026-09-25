#include "game/art.h"

#include <initializer_list>

namespace therm {
namespace {

using gs::Bitmap;

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(p * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(p * 16 + i++, 0);
    vdp.setColor(p * 16, 0);
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 15, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

void paintBag(Bitmap& b) {
    b.ellipse(30, 36, 24, 30, 3);
    b.ellipse(30, 32, 22, 22, 4);
    b.ellipse(30, 26, 16, 16, 5);
    b.ellipse(30, 18, 10, 10, 6);
    b.ellipse(22, 24, 5, 8, 9);
    b.line(30, 8, 30, 58, 8, 1.2f);
    b.line(16, 18, 20, 60, 8, 1.1f);
    b.line(44, 18, 40, 60, 8, 1.1f);
    b.line(8, 36, 18, 64, 8, 1.1f);
    b.line(52, 36, 42, 64, 8, 1.1f);
    b.poly({{20, 58}, {40, 58}, {36, 74}, {24, 74}}, 7);
    b.rect(26, 70, 8, 8, 1);
    b.ellipse(30, 8, 3.2f, 2.4f, 8);
    b.line(24, 74, 22, 82, 10, 1.2f);
    b.line(36, 74, 38, 82, 10, 1.2f);
    b.outline(15, false);
}

void paintBasket(Bitmap& b) {
    b.line(6, 2, 10, 10, 3, 1.3f);
    b.line(30, 2, 26, 10, 3, 1.3f);
    b.rect(7, 10, 22, 14, 1);
    b.rect(7, 10, 22, 3, 2);
    for (int y = 14; y < 23; y += 3) b.line(7, float(y), 29, float(y), 2, 1.f);
    for (int x = 11; x < 28; x += 4) b.line(float(x), 12, float(x), 24, 2, 1.f);
    b.ellipse(16, 12, 2.2f, 2.2f, 5);
    b.rect(13, 14, 7, 6, 4);
    b.rect(14, 10, 4, 2, 6);
    b.outline(15, false);
}

void paintFlame(Bitmap& b, int kind) {
    float h = 26.f - float(kind) * 6.f;
    float top = 30.f - h;
    b.poly({{10, 30}, {2, 30 - h * 0.45f}, {10, top}, {18, 30 - h * 0.45f}}, 4);
    b.poly({{10, 28}, {5, 28 - h * 0.4f}, {10, top + 4}, {15, 28 - h * 0.4f}}, 3);
    b.poly({{10, 26}, {7, 24 - h * 0.25f}, {10, top + 8}, {13, 24 - h * 0.25f}}, 2);
    b.ellipse(10, 24, 2.2f, 3.f, 1);
}

void paintTree(Bitmap& b) {
    b.rect(20, 52, 8, 27, 5);
    b.rect(21, 54, 6, 25, 4);
    b.ellipse(24, 40, 18, 22, 1);
    b.ellipse(24, 32, 14, 16, 2);
    b.ellipse(24, 22, 10, 14, 3);
    b.ellipse(16, 28, 4, 3, 3);
    b.outline(15, false);
}

void paintMark(Bitmap& b) {
    b.rect(0, 12, b.w, 18, 1);
    b.rect(0, 12, b.w, 4, 2);
    for (int x = 6; x < b.w - 2; x += 10) b.rect(x, 24, 2, 4, 2);
    float cx = b.w * 0.5f;
    b.ellipse(cx, 20, b.w * 0.34f, 8, 3);
    b.ellipse(cx, 20, b.w * 0.18f, 4.5f, 1);
    b.line(cx - 28, 14, cx + 28, 26, 4, 2.6f);
    b.line(cx + 28, 14, cx - 28, 26, 4, 2.6f);
    b.line(cx, 13, cx, 27, 5, 1.3f);
}

void paintFlag(Bitmap& b, bool out) {
    b.rect(4, 6, 3, b.h - 8, 2);
    b.rect(5, 6, 1, b.h - 8, 1);
    float tip = out ? 21.f : 15.f;
    b.poly({{7, 8}, {tip, 18}, {7, 30}}, 3);
    b.poly({{7, 13}, {tip - 3.f, 18}, {7, 25}}, 4);
    b.ellipse(5.5f, 5, 2.6f, 2.4f, 3);
    b.outline(15, false);
}

void paintCloud(Bitmap& b) {
    b.ellipse(24, 16, 18, 9, 1);
    b.ellipse(44, 14, 20, 10, 1);
    b.ellipse(34, 18, 16, 7, 2);
    b.ellipse(18, 18, 8, 5, 3);
}

void paintSun(Bitmap& b) {
    b.ellipse(20, 20, 10, 10, 1);
    b.ellipse(20, 20, 14, 14, 2);
    b.ellipse(20, 20, 8, 8, 1);
    b.line(20, 1, 20, 6, 3, 1.6f);
    b.line(20, 34, 20, 39, 3, 1.6f);
    b.line(1, 20, 6, 20, 3, 1.6f);
    b.line(34, 20, 39, 20, 3, 1.6f);
    b.line(6, 6, 10, 10, 3, 1.4f);
    b.line(30, 6, 34, 10, 3, 1.4f);
    b.line(6, 34, 10, 30, 3, 1.4f);
    b.line(30, 34, 34, 30, 3, 1.4f);
}

void paintBird(Bitmap& b, bool up) {
    float y = up ? 3.f : 7.f;
    b.line(1, 8, 10, y, 1, 1.6f);
    b.line(10, y, 19, 8, 1, 1.6f);
    b.ellipse(10, 8, 1.7f, 1.4f, 2);
}

void paintShadow(Bitmap& b) { b.ellipse(20, 6, 16, 4, 1); }

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    auto C = gs::rgb4;
    pal(vdp, PAL_HUD, {0, C(15, 14, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(3, 2, 4)});
    pal(vdp, PAL_ALERT, {0, C(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(4, 0, 1)});
    pal(vdp, PAL_WIN, {0, C(12, 15, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(1, 3, 1)});
    pal(vdp, PAL_BANNER, {0, C(15, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(4, 2, 1)});
    pal(vdp, PAL_BAG, {0, C(8, 1, 2), C(13, 2, 3), C(15, 4, 3), C(15, 8, 2), C(15, 12, 4), C(15, 14, 10),
                       C(12, 5, 2), C(6, 1, 2), C(15, 15, 12), C(7, 5, 3), 0, 0, 0, 0, C(2, 0, 1)});
    pal(vdp, PAL_BASKET, {0, C(11, 7, 3), C(6, 4, 1), C(8, 6, 4), C(3, 5, 8), C(13, 8, 5), C(12, 3, 2), 0, 0, 0, 0, 0,
                          0, 0, 0, C(2, 1, 0)});
    pal(vdp, PAL_FLAME, {0, C(15, 15, 12), C(15, 13, 2), C(15, 7, 1), C(11, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_TREE, {0, C(1, 4, 1), C(2, 7, 2), C(5, 11, 4), C(7, 4, 2), C(4, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        C(0, 2, 0)});
    pal(vdp, PAL_MARK, {0, C(3, 9, 3), C(2, 6, 2), C(15, 15, 13), C(14, 2, 2), C(15, 11, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        C(1, 2, 0)});
    pal(vdp, PAL_CLOUD, {0, C(15, 15, 15), C(12, 13, 14), C(9, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(6, 7, 9)});
    pal(vdp, PAL_FLAG, {0, C(12, 12, 13), C(5, 5, 6), C(14, 2, 2), C(15, 10, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        C(2, 1, 1)});
    pal(vdp, PAL_SUN, {0, C(15, 15, 8), C(15, 12, 3), C(15, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_BIRD, {0, C(4, 3, 3), C(9, 8, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    pal(vdp, PAL_TREE2, {0, C(1, 4, 3), C(2, 6, 4), C(4, 10, 6), C(6, 3, 2), C(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         C(0, 2, 1)});

    loadFont(vdp, art);

    Bitmap bag(kBagBmpW, kBagBmpH);
    paintBag(bag);
    art.bag = gs::uploadMipped(vdp, bag);
    Bitmap basket(kBasketBmpW, kBasketBmpH);
    paintBasket(basket);
    art.basket = gs::uploadMipped(vdp, basket);
    for (int i = 0; i < 3; i++) {
        Bitmap flame(20, 32);
        paintFlame(flame, i);
        art.flame[i] = gs::uploadMipped(vdp, flame);
    }
    Bitmap tree(kTreeBmpW, kTreeBmpH);
    paintTree(tree);
    art.tree = gs::uploadMipped(vdp, tree);
    Bitmap mark(128, 32);
    paintMark(mark);
    art.mark = gs::uploadMipped(vdp, mark);
    for (int i = 0; i < 2; i++) {
        Bitmap flag(kFlagBmpW, kFlagBmpH);
        paintFlag(flag, i == 0);
        art.flag[i] = gs::uploadMipped(vdp, flag);
    }
    Bitmap cloud(72, 28);
    paintCloud(cloud);
    art.cloud = gs::uploadMipped(vdp, cloud);
    Bitmap sun(40, 40);
    paintSun(sun);
    art.sun = gs::uploadMipped(vdp, sun);
    for (int i = 0; i < 2; i++) {
        Bitmap bird(20, 12);
        paintBird(bird, i == 0);
        art.bird[i] = gs::uploadMipped(vdp, bird);
    }
    Bitmap shadow(40, 12);
    paintShadow(shadow);
    art.shadow = gs::uploadMipped(vdp, shadow);

    art.therm = words(vdp, "THERM", 3);
    art.oneEnv = words(vdp, "ONE ENVELOPE", 2);
    art.theMark = words(vdp, "THE MARK", 2);
    art.theTrees = words(vdp, "THE TREES", 2);
    art.tooHot = words(vdp, "TOO HOT", 2);
    art.tooHard = words(vdp, "TOO HARD", 2);
    art.missed = words(vdp, "MISSED", 2);
    art.drifted = words(vdp, "DRIFTED", 2);
    art.theDay = words(vdp, "THE DAY", 2);
}

}  // namespace therm
