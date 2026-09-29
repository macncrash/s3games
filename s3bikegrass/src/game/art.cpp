#include "art.h"

#include <cmath>
#include <initializer_list>

namespace bikegrass {
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

gs::Bitmap bikeArt() {
    gs::Bitmap b(88, 52);
    b.line(16, 38, 44, 20, 1, 2.4f);
    b.line(44, 20, 72, 36, 1, 2.4f);
    b.line(44, 20, 40, 38, 1, 2.0f);
    b.line(28, 38, 58, 38, 2, 2.2f);
    b.rect(40, 12, 4, 10, 3);
    b.rect(34, 10, 16, 3, 3);
    b.rect(46, 8, 6, 4, 6);
    b.ellipse(20, 40, 11, 11, 4);
    b.ellipse(68, 38, 11, 11, 4);
    b.ellipse(20, 40, 3, 3, 5);
    b.ellipse(68, 38, 3, 3, 5);
    b.line(62, 36, 78, 28, 6, 1.6f);
    b.ellipse(50, 16, 4, 5, 7);
    b.rect(47, 20, 6, 8, 7);
    b.outline(15, false);
    return b;
}

gs::Bitmap wheelArt(int phase) {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 11, 11, 1);
    b.ellipse(13, 13, 8, 8, 2);
    b.ellipse(13, 13, 2.2f, 2.2f, 3);
    float a0 = phase ? 0.45f : 0.05f;
    for (int i = 0; i < 5; i++) {
        float a = a0 + i * 1.2566f;
        b.line(13, 13, 13 + std::cos(a) * 8.f, 13 + std::sin(a) * 8.f, 4, 1.1f);
    }
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785f;
        b.set(int(13 + std::cos(a) * 10.f), int(13 + std::sin(a) * 10.f), 5);
    }
    return b;
}

gs::Bitmap dirtArt() {
    gs::Bitmap b(32, 20);
    b.rect(0, 0, 32, 20, 1);
    for (int x = 1; x < 30; x += 6) b.rect(x, 4, 3, 2, 2);
    b.rect(0, 0, 32, 3, 3);
    b.rect(4, 12, 6, 2, 4);
    b.rect(18, 9, 5, 2, 4);
    return b;
}

gs::Bitmap grassArt() {
    gs::Bitmap b(32, 18);
    b.rect(0, 4, 32, 14, 1);
    b.rect(0, 0, 32, 5, 2);
    for (int x = 1; x < 32; x += 4) {
        b.line(float(x), 6, float(x + (x & 2 ? 1 : -1)), 0, 3, 1.1f);
        b.set(x, 1, 4);
    }
    b.rect(6, 10, 4, 2, 5);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(14, 12);
    b.line(3, 11, 3, 2, 1, 1.2f);
    b.line(7, 11, 8, 1, 2, 1.2f);
    b.line(11, 11, 10, 3, 1, 1.2f);
    b.set(3, 1, 3);
    b.set(8, 0, 3);
    return b;
}

gs::Bitmap rampArt() {
    gs::Bitmap b(28, 16);
    b.poly({{0, 15}, {27, 1}, {27, 15}}, 1);
    b.line(2, 14, 24, 3, 2, 1.4f);
    for (int i = 0; i < 4; i++) b.rect(4 + i * 6, 10 - i, 3, 2, 3);
    return b;
}

gs::Bitmap pitArt() {
    gs::Bitmap b(24, 28);
    b.rect(0, 0, 24, 28, 1);
    b.rect(2, 4, 20, 3, 2);
    b.line(4, 8, 18, 24, 3, 1.2f);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(36, 48);
    b.rect(16, 28, 5, 18, 1);
    b.ellipse(18, 18, 14, 14, 2);
    b.ellipse(14, 16, 6, 5, 3);
    b.ellipse(22, 20, 4, 3, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(28, 40);
    b.rect(4, 4, 2, 34, 1);
    b.poly({{6, 5}, {24, 11}, {6, 17}}, 2);
    b.rect(7, 7, 8, 3, 3);
    b.rect(3, 36, 5, 3, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 6, 6, 3, 1);
    b.ellipse(5, 5, 2, 1.5f, 2);
    return b;
}

gs::Bitmap wordArt(const char* s, int scale, int color) {
    gs::TextStyle st{scale, color, 0, 0, 1};
    gs::Bitmap t = gs::textBitmap(s, st);
    gs::Bitmap b(t.w + 8, t.h + 8);
    b.blit(t, 4, 4);
    b.outline(15, false);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12), gs::rgb4(5, 6, 5)});
    setPal(vdp, PAL_BIKE, {0, gs::rgb4(3, 4, 5), gs::rgb4(9, 9, 8), gs::rgb4(12, 5, 2), gs::rgb4(1, 1, 1),
                           gs::rgb4(11, 11, 10), gs::rgb4(14, 12, 3), gs::rgb4(13, 8, 5)});
    setPal(vdp, PAL_DIRT, {0, gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(2, 7, 2), gs::rgb4(3, 10, 3), gs::rgb4(6, 13, 4), gs::rgb4(10, 14, 6),
                            gs::rgb4(1, 5, 1), gs::rgb4(4, 8, 3)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(5, 3, 2), gs::rgb4(1, 6, 2), gs::rgb4(3, 9, 3), gs::rgb4(8, 12, 4)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(8, 8, 7), gs::rgb4(14, 12, 3), gs::rgb4(12, 3, 2), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(8, 7, 4), gs::rgb4(12, 11, 7)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(6, 14, 5), gs::rgb4(12, 15, 9)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 11, 5)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(14, 13, 6), gs::rgb4(7, 11, 14)});
    setPal(vdp, PAL_PIT, {0, gs::rgb4(1, 2, 3), gs::rgb4(2, 4, 5), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_RAMP, {0, gs::rgb4(7, 5, 3), gs::rgb4(12, 10, 4), gs::rgb4(5, 4, 2)});

    art.bike = gs::uploadMipped(vdp, bikeArt());
    art.wheel[0] = gs::uploadMipped(vdp, wheelArt(0));
    art.wheel[1] = gs::uploadMipped(vdp, wheelArt(1));
    art.dirt = gs::uploadMipped(vdp, dirtArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.ramp = gs::uploadMipped(vdp, rampArt());
    art.pit = gs::uploadMipped(vdp, pitArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.title = gs::uploadMipped(vdp, wordArt("BIKE", 3, 1));
    art.grassWord = gs::uploadMipped(vdp, wordArt("GRASS", 3, 2));
    art.stopped = gs::uploadMipped(vdp, wordArt("FULL STOP", 2, 1));
    art.missed = gs::uploadMipped(vdp, wordArt("MISSED", 2, 1));
    art.paused = gs::uploadMipped(vdp, wordArt("PAUSE", 2, 1));
    loadFont(vdp, art);
}

}  // namespace bikegrass
