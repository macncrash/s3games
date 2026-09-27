#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace spanpurs {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void wheel(Bitmap& b, float cx, float cy, float r, int frame) {
    b.ellipse(cx, cy, r, r, 9);
    b.ellipse(cx, cy, r * 0.42f, r * 0.42f, 10);
    float a = frame ? 0.85f : 0.2f;
    b.line(cx - std::cos(a) * (r - 1.f), cy - std::sin(a) * (r - 1.f), cx + std::cos(a) * (r - 1.f),
           cy + std::sin(a) * (r - 1.f), 12, 1.15f);
}

// Side-on machines. Shared indices: 1 highlight, 2 body, 3 shade, 4 iron,
// 5 lamp, 6 glass, 7 stripe, 8 outline, 9 tyre, 10 hub, 11 stack, 12 spoke.
Bitmap jackArt(int frame) {
    Bitmap b(58, 40);
    wheel(b, 16, 32, 7, frame);
    wheel(b, 42, 32, 7, frame);
    b.rect(10, 24, 38, 7, 3);
    b.rect(12, 24, 34, 3, 2);
    b.rect(14, 26, 28, 2, 7);
    b.rect(22, 12, 18, 14, 2);
    b.rect(24, 14, 12, 7, 6);
    b.rect(25, 15, 5, 3, 12);
    b.rect(8, 18, 16, 8, 3);
    b.rect(10, 18, 12, 3, 2);
    b.rect(6, 8, 5, 16, 11);
    b.rect(6, 6, 5, 4, 4);
    b.rect(48, 20, 7, 5, 5);
    b.rect(50, 21, 3, 2, 12);
    b.rect(46, 23, 3, 3, 4);
    b.line(16, 32, 42, 32, 4, 1.4f);
    b.outline(8, false);
    return b;
}

Bitmap drumArt(int frame) {
    Bitmap b(54, 38);
    wheel(b, 14, 30, 8, frame);
    wheel(b, 40, 30, 8, frame);
    b.ellipse(28, 18, 16, 11, 2);
    b.ellipse(26, 16, 9, 6, 1);
    b.rect(12, 16, 32, 4, 7);
    b.ellipse(34, 10, 5, 4, 4);
    b.rect(38, 4, 4, 12, 11);
    b.rect(38, 3, 4, 3, 5);
    b.rect(8, 22, 38, 5, 3);
    b.rect(18, 14, 6, 5, 6);
    b.outline(8, false);
    return b;
}

Bitmap crawlArt(int frame) {
    Bitmap b(60, 34);
    b.rect(6, 22, 48, 8, 9);
    b.rect(8, 23, 44, 3, 3);
    for (int i = 0; i < 5; ++i) {
        float x = 10.f + float(i) * 8.f + (frame ? 3.f : 0.f);
        b.rect(x, 24, 3, 5, 10);
    }
    b.ellipse(12, 26, 4, 4, 4);
    b.ellipse(48, 26, 4, 4, 4);
    b.poly({{10, 14}, {46, 14}, {50, 24}, {8, 24}}, 2);
    b.poly({{18, 15}, {40, 15}, {42, 21}, {16, 21}}, 1);
    b.rect(34, 6, 16, 10, 3);
    b.rect(36, 8, 10, 5, 6);
    b.rect(44, 16, 8, 4, 5);
    b.rect(14, 4, 4, 12, 11);
    b.rect(12, 18, 8, 3, 7);
    b.outline(8, false);
    return b;
}

Bitmap wagonArt(int frame) {
    Bitmap b(64, 42);
    wheel(b, 16, 34, 6.5f, frame);
    wheel(b, 48, 34, 6.5f, frame);
    b.rect(6, 22, 52, 8, 4);
    b.rect(8, 16, 46, 8, 2);
    b.rect(10, 17, 42, 3, 1);
    b.rect(12, 8, 4, 14, 4);
    b.rect(46, 8, 4, 14, 4);
    b.rect(12, 6, 38, 4, 7);
    b.rect(14, 7, 34, 2, 1);
    b.rect(22, 18, 16, 5, 6);
    b.rect(52, 20, 6, 4, 5);
    b.rect(8, 24, 6, 3, 3);
    b.outline(8, false);
    return b;
}

Bitmap drayArt(int frame) {
    Bitmap b(56, 48);
    wheel(b, 14, 38, 6, frame);
    wheel(b, 40, 38, 9, frame);
    b.rect(8, 28, 40, 7, 3);
    b.rect(18, 12, 26, 18, 2);
    b.rect(20, 14, 22, 4, 1);
    b.rect(22, 20, 18, 3, 7);
    b.rect(22, 26, 8, 4, 6);
    b.rect(6, 22, 16, 8, 4);
    b.rect(8, 20, 10, 4, 3);
    b.rect(20, 4, 5, 10, 11);
    b.rect(20, 2, 5, 4, 5);
    b.rect(44, 24, 6, 5, 5);
    b.rect(16, 30, 4, 3, 12);
    b.outline(8, false);
    return b;
}

Bitmap plankArt() {
    Bitmap b(52, 16);
    b.rect(1, 3, 50, 10, 5);
    b.rect(1, 3, 50, 3, 1);
    b.rect(1, 11, 50, 2, 6);
    for (int x = 6; x < 48; x += 8) b.rect(float(x), 5, 1, 7, 6);
    b.rect(4, 8, 3, 3, 7);
    b.rect(44, 8, 3, 3, 7);
    b.outline(8, false);
    return b;
}

Bitmap trussArt() {
    Bitmap b(44, 22);
    b.rect(0, 1, 44, 3, 2);
    b.rect(0, 17, 44, 3, 3);
    b.line(2, 4, 42, 17, 1, 1.4f);
    b.line(42, 4, 2, 17, 4, 1.3f);
    b.rect(20, 8, 4, 5, 7);
    return b;
}

Bitmap ropeArt() {
    Bitmap b(6, 32);
    b.rect(2, 0, 2, 32, 2);
    b.rect(1, 0, 1, 32, 1);
    b.rect(3, 4, 1, 24, 3);
    return b;
}

Bitmap linkArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3.1f, 2.3f, 5);
    b.ellipse(4, 4, 1.5f, 1.0f, 0);
    b.rect(3, 1, 2, 2, 4);
    return b;
}

Bitmap towerArt() {
    Bitmap b(36, 108);
    b.poly({{6, 10}, {30, 10}, {34, 104}, {2, 104}}, 2);
    b.rect(8, 14, 20, 86, 3);
    b.rect(4, 6, 28, 8, 1);
    b.rect(10, 2, 16, 6, 4);
    for (int y = 22; y < 96; y += 14) {
        b.rect(4, float(y), 28, 3, 7);
        b.rect(15, float(y), 6, 10, 1);
    }
    b.poly({{2, 104}, {12, 78}, {16, 104}}, 4);
    b.poly({{34, 104}, {24, 78}, {20, 104}}, 4);
    b.outline(8, false);
    return b;
}

Bitmap cliffArt() {
    Bitmap b(70, 120);
    b.poly({{0, 34}, {16, 16}, {34, 28}, {52, 10}, {70, 26}, {70, 119}, {0, 119}}, 2);
    b.poly({{0, 48}, {24, 32}, {48, 44}, {70, 30}, {70, 78}, {0, 86}}, 1);
    b.rect(0, 24, 70, 5, 5);
    for (int y = 56; y < 112; y += 16) b.line(4, float(y), 62, float(y + 8), 3, 1.3f);
    b.rect(18, 18, 3, 14, 4);
    b.rect(46, 14, 3, 16, 4);
    b.outline(8, false);
    return b;
}

Bitmap lipArt() {
    Bitmap b(48, 18);
    b.rect(0, 2, 48, 12, 2);
    b.rect(0, 2, 48, 4, 1);
    b.rect(0, 12, 48, 4, 3);
    for (int x = 6; x < 44; x += 10) b.rect(float(x), 7, 2, 5, 6);
    b.outline(8, false);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(64, 22);
    b.ellipse(18, 13, 14, 7, 1);
    b.ellipse(34, 11, 16, 8, 1);
    b.ellipse(50, 13, 12, 6, 1);
    b.ellipse(32, 10, 9, 5, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(26, 26);
    b.ellipse(13, 13, 11, 11, 5);
    b.ellipse(13, 13, 6, 6, 4);
    b.ellipse(10, 10, 3, 2, 7);
    return b;
}

Bitmap birdArt() {
    Bitmap b(14, 8);
    b.line(1, 6, 7, 2, 3, 1.3f);
    b.line(7, 2, 13, 6, 3, 1.3f);
    return b;
}

Bitmap flameArt(int frame) {
    Bitmap b(10, 14);
    b.poly({{5, 1}, {9, 8}, {7, 7}, {8, 13}, {2, 13}, {4, 7}, {1, 8}}, frame ? 6 : 5);
    b.poly({{5, 5}, {7, 11}, {3, 11}}, 1);
    return b;
}

Bitmap dustArt() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 3, 3);
    b.ellipse(4, 4, 2, 1.5f, 1);
    return b;
}

Bitmap chevArt() {
    Bitmap b(14, 12);
    b.poly({{7, 11}, {1, 3}, {4, 3}, {4, 1}, {10, 1}, {10, 3}, {13, 3}}, 1);
    return b;
}

Bitmap pennantArt() {
    Bitmap b(16, 20);
    b.rect(2, 1, 2, 18, 4);
    b.poly({{4, 3}, {14, 7}, {4, 12}}, 2);
    b.poly({{4, 4}, {10, 7}, {4, 10}}, 7);
    b.outline(8, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y) {
            for (int x = 0; x < 5; ++x) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t out = gs::rgb4(1, 1, 2);

    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 9, 10), gs::rgb4(14, 12, 9), gs::rgb4(12, 4, 3), gs::rgb4(6, 13, 7),
                          gs::rgb4(14, 11, 4), gs::rgb4(5, 6, 8), 0, 0, 0, 0, 0, 0, 0, shadow});

    setPal(vdp, PAL_IRON, {0, gs::rgb4(12, 13, 14), gs::rgb4(7, 8, 10), gs::rgb4(3, 4, 5), gs::rgb4(10, 6, 3),
                           gs::rgb4(12, 8, 4), gs::rgb4(7, 4, 2), gs::rgb4(14, 12, 6), out, gs::rgb4(4, 6, 3),
                           gs::rgb4(4, 5, 6), 0, 0, 0, 0, shadow});

    auto machine = [&](int pal, uint16_t hi, uint16_t body, uint16_t shade, uint16_t iron, uint16_t lamp, uint16_t glass,
                        uint16_t stripe, uint16_t hub, uint16_t stack) {
        setPal(vdp, pal, {0, hi, body, shade, iron, lamp, glass, stripe, out, gs::rgb4(2, 2, 2), hub, stack,
                          gs::rgb4(15, 14, 10), 0, 0, shadow});
    };
    machine(PAL_JACK, gs::rgb4(15, 13, 8), gs::rgb4(14, 8, 2), gs::rgb4(8, 4, 1), gs::rgb4(6, 6, 7),
            gs::rgb4(15, 15, 6), gs::rgb4(12, 14, 15), gs::rgb4(12, 3, 2), gs::rgb4(8, 8, 8), gs::rgb4(4, 4, 5));
    machine(PAL_DRUM, gs::rgb4(15, 12, 8), gs::rgb4(12, 4, 3), gs::rgb4(6, 2, 2), gs::rgb4(8, 6, 4),
            gs::rgb4(15, 14, 10), gs::rgb4(10, 12, 12), gs::rgb4(14, 10, 4), gs::rgb4(9, 7, 4), gs::rgb4(5, 4, 4));
    machine(PAL_CRAWL, gs::rgb4(12, 14, 8), gs::rgb4(5, 8, 3), gs::rgb4(2, 4, 2), gs::rgb4(6, 6, 6),
            gs::rgb4(14, 12, 4), gs::rgb4(8, 12, 10), gs::rgb4(10, 8, 2), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4));
    machine(PAL_WAGON, gs::rgb4(12, 14, 15), gs::rgb4(4, 7, 12), gs::rgb4(2, 3, 6), gs::rgb4(8, 6, 4),
            gs::rgb4(14, 12, 6), gs::rgb4(10, 12, 14), gs::rgb4(12, 8, 3), gs::rgb4(8, 8, 8), gs::rgb4(5, 5, 6));
    machine(PAL_DRAY, gs::rgb4(14, 13, 8), gs::rgb4(3, 9, 5), gs::rgb4(1, 4, 3), gs::rgb4(6, 6, 7),
            gs::rgb4(15, 14, 6), gs::rgb4(10, 14, 12), gs::rgb4(12, 10, 3), gs::rgb4(8, 7, 4), gs::rgb4(4, 4, 5));
    machine(PAL_DEAD, gs::rgb4(10, 10, 11), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 6),
            gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 6), gs::rgb4(4, 4, 5), gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 4));

    setPal(vdp, PAL_ROCK, {0, gs::rgb4(10, 11, 11), gs::rgb4(6, 7, 8), gs::rgb4(3, 4, 5), gs::rgb4(4, 7, 4),
                           gs::rgb4(8, 8, 9), gs::rgb4(2, 3, 3), gs::rgb4(6, 7, 4), out, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(13, 11, 7), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(15, 14, 8),
                           gs::rgb4(10, 10, 11), gs::rgb4(3, 2, 2), gs::rgb4(12, 9, 5), out, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 12, 6), gs::rgb4(8, 7, 5), gs::rgb4(15, 14, 8),
                         gs::rgb4(15, 15, 8), gs::rgb4(14, 6, 2), gs::rgb4(15, 10, 4), gs::rgb4(4, 2, 1),
                         gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 8), gs::rgb4(15, 14, 8), gs::rgb4(15, 15, 15), 0, 0, shadow});
    setPal(vdp, PAL_MIST, {0, gs::rgb4(14, 13, 12), gs::rgb4(9, 8, 8), gs::rgb4(3, 3, 4), gs::rgb4(15, 14, 6),
                           gs::rgb4(14, 8, 3), gs::rgb4(12, 11, 10), gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 3), 0, 0, 0,
                           0, 0, 0, 0, shadow});

    auto textPal = [&](int pal, uint16_t c) { setPal(vdp, pal, {0, c, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow}); };
    textPal(PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(PAL_ALERT, gs::rgb4(15, 5, 3));
    textPal(PAL_GOOD, gs::rgb4(8, 15, 7));
    textPal(PAL_TITLE, gs::rgb4(15, 13, 6));

    art.jack[0] = gs::uploadMipped(vdp, jackArt(0));
    art.jack[1] = gs::uploadMipped(vdp, jackArt(1));
    art.drum[0] = gs::uploadMipped(vdp, drumArt(0));
    art.drum[1] = gs::uploadMipped(vdp, drumArt(1));
    art.crawl[0] = gs::uploadMipped(vdp, crawlArt(0));
    art.crawl[1] = gs::uploadMipped(vdp, crawlArt(1));
    art.wagon[0] = gs::uploadMipped(vdp, wagonArt(0));
    art.wagon[1] = gs::uploadMipped(vdp, wagonArt(1));
    art.dray[0] = gs::uploadMipped(vdp, drayArt(0));
    art.dray[1] = gs::uploadMipped(vdp, drayArt(1));
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.truss = gs::uploadMipped(vdp, trussArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.cliff = gs::uploadMipped(vdp, cliffArt());
    art.lip = gs::uploadMipped(vdp, lipArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.bird = gs::uploadMipped(vdp, birdArt());
    art.flame[0] = gs::uploadMipped(vdp, flameArt(0));
    art.flame[1] = gs::uploadMipped(vdp, flameArt(1));
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());

    gs::TextStyle big{3, 1, 0, 15, 1};
    art.title = gs::uploadMipped(vdp, gs::textBitmap("PURSUIT", big));
    art.last = gs::uploadMipped(vdp, gs::textBitmap("LAST", big));
    art.seized = gs::uploadMipped(vdp, gs::textBitmap("SEIZED", big));
    art.paused = gs::uploadMipped(vdp, gs::textBitmap("PAUSED", big));
    loadFont(vdp, art);
}

}  // namespace spanpurs
