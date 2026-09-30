#include "art.h"

#include <cmath>
#include <initializer_list>

namespace kartgrass {
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

gs::Bitmap kartArt() {
    gs::Bitmap b(96, 40);
    b.poly({{8, 28}, {28, 22}, {62, 20}, {84, 26}, {86, 32}, {10, 34}}, 1);
    b.rect(18, 24, 46, 8, 2);
    b.poly({{48, 18}, {70, 14}, {78, 22}, {46, 24}}, 3);
    b.rect(58, 12, 10, 6, 4);
    b.ellipse(22, 32, 9, 9, 5);
    b.ellipse(74, 31, 9, 9, 5);
    b.ellipse(22, 32, 3, 3, 6);
    b.ellipse(74, 31, 3, 3, 6);
    b.rect(30, 16, 8, 6, 7);
    b.ellipse(34, 14, 4, 4, 8);
    b.line(12, 30, 8, 22, 9, 1.6f);
    b.outline(15, false);
    return b;
}

gs::Bitmap tyreArt(int phase) {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 1);
    b.ellipse(11, 11, 6, 6, 2);
    b.ellipse(11, 11, 2, 2, 3);
    float a0 = phase ? 0.4f : 0.0f;
    for (int i = 0; i < 4; i++) {
        float a = a0 + i * 1.5708f;
        b.line(11, 11, 11 + std::cos(a) * 6.5f, 11 + std::sin(a) * 6.5f, 4, 1.2f);
    }
    return b;
}

gs::Bitmap dirtArt() {
    gs::Bitmap b(32, 18);
    b.rect(0, 0, 32, 18, 1);
    b.rect(0, 0, 32, 3, 2);
    for (int x = 2; x < 30; x += 7) b.rect(x, 7, 4, 2, 3);
    b.rect(12, 12, 8, 2, 4);
    return b;
}

gs::Bitmap grassArt() {
    gs::Bitmap b(32, 18);
    b.rect(0, 5, 32, 13, 1);
    b.rect(0, 0, 32, 6, 2);
    for (int x = 1; x < 32; x += 3) {
        b.line(float(x), 7, float(x + ((x / 3) & 1 ? 1 : -1)), 0, 3, 1.1f);
        if ((x % 6) == 1) b.set(x, 1, 4);
    }
    b.rect(8, 11, 5, 2, 5);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(16, 14);
    b.line(4, 13, 3, 2, 1, 1.3f);
    b.line(8, 13, 9, 1, 2, 1.3f);
    b.line(12, 13, 11, 3, 1, 1.3f);
    b.set(3, 1, 3);
    b.set(9, 0, 3);
    return b;
}

gs::Bitmap rampArt() {
    gs::Bitmap b(30, 18);
    b.poly({{0, 17}, {29, 2}, {29, 17}}, 1);
    b.line(1, 16, 26, 4, 2, 1.5f);
    for (int i = 0; i < 4; i++) b.rect(3 + i * 6, 12 - i, 4, 2, 3);
    return b;
}

gs::Bitmap gapArt() {
    gs::Bitmap b(20, 32);
    b.rect(0, 0, 20, 32, 1);
    b.rect(2, 6, 16, 3, 2);
    b.line(3, 12, 16, 28, 3, 1.2f);
    b.rect(4, 20, 8, 2, 4);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(34, 50);
    b.rect(15, 30, 5, 18, 1);
    b.ellipse(17, 18, 13, 15, 2);
    b.ellipse(12, 16, 5, 4, 3);
    b.ellipse(21, 20, 4, 3, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(26, 42);
    b.rect(3, 3, 2, 36, 1);
    b.poly({{5, 4}, {22, 10}, {5, 16}}, 2);
    b.poly({{5, 16}, {20, 22}, {5, 26}}, 3);
    b.rect(2, 37, 5, 3, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(18, 10);
    b.ellipse(9, 6, 7, 3, 1);
    b.ellipse(5, 5, 2.4f, 1.6f, 2);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 9, 9, 1);
    b.ellipse(11, 11, 6, 6, 2);
    b.line(11, 11, 11, 5, 3, 1.4f);
    b.line(11, 11, 16, 13, 4, 1.3f);
    b.outline(15, false);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12), gs::rgb4(4, 5, 6)});
    setPal(vdp, PAL_KART, {0, gs::rgb4(12, 2, 2), gs::rgb4(14, 4, 3), gs::rgb4(15, 12, 3), gs::rgb4(8, 12, 14),
                           gs::rgb4(1, 1, 1), gs::rgb4(10, 10, 9), gs::rgb4(4, 5, 6), gs::rgb4(13, 9, 6),
                           gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_DIRT, {0, gs::rgb4(6, 4, 2), gs::rgb4(9, 7, 3), gs::rgb4(4, 3, 2), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(2, 7, 2), gs::rgb4(4, 11, 3), gs::rgb4(7, 14, 4), gs::rgb4(12, 15, 7),
                            gs::rgb4(1, 5, 2), gs::rgb4(5, 9, 3)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(5, 3, 1), gs::rgb4(1, 6, 2), gs::rgb4(3, 9, 3), gs::rgb4(8, 13, 4)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(9, 9, 8), gs::rgb4(14, 3, 2), gs::rgb4(15, 13, 3), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(9, 8, 5), gs::rgb4(13, 12, 8)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(5, 14, 5), gs::rgb4(12, 15, 8)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 11, 4)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 4), gs::rgb4(6, 10, 14)});
    setPal(vdp, PAL_GAP, {0, gs::rgb4(1, 2, 3), gs::rgb4(2, 3, 5), gs::rgb4(3, 4, 5), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_RAMP, {0, gs::rgb4(7, 5, 3), gs::rgb4(13, 10, 4), gs::rgb4(5, 4, 2)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(14, 10, 2), gs::rgb4(8, 5, 2), gs::rgb4(15, 14, 8), gs::rgb4(2, 2, 3)});

    art.kart = gs::uploadMipped(vdp, kartArt());
    art.tyre[0] = gs::uploadMipped(vdp, tyreArt(0));
    art.tyre[1] = gs::uploadMipped(vdp, tyreArt(1));
    art.dirt = gs::uploadMipped(vdp, dirtArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.ramp = gs::uploadMipped(vdp, rampArt());
    art.gap = gs::uploadMipped(vdp, gapArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.title = gs::uploadMipped(vdp, wordArt("KART", 3, 1));
    art.grassWord = gs::uploadMipped(vdp, wordArt("GRASS", 3, 2));
    art.stopped = gs::uploadMipped(vdp, wordArt("FULL STOP", 2, 1));
    art.missed = gs::uploadMipped(vdp, wordArt("MISSED", 2, 1));
    art.crew = gs::uploadMipped(vdp, wordArt("CREW", 2, 1));
    art.paused = gs::uploadMipped(vdp, wordArt("PAUSE", 2, 1));
    loadFont(vdp, art);
}

}  // namespace kartgrass
