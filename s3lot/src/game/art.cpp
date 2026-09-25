#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

namespace lot {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void ring(Bitmap& b, float cx, float cy, float r0, float r1, int c) {
    int x0 = int(std::floor(cx - r1)) - 1, x1 = int(std::ceil(cx + r1)) + 1;
    int y0 = int(std::floor(cy - r1)) - 1, y1 = int(std::ceil(cy + r1)) + 1;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            float d = std::hypot(x + 0.5f - cx, y + 0.5f - cy);
            if (d <= r1 && d >= r0) b.set(x, y, c);
        }
}

Bitmap kidFront() {
    Bitmap b(40, 48);
    b.ellipse(20, 11, 11, 7, 1);
    b.rect(9, 12, 22, 5, 1);
    b.rect(7, 16, 26, 3, 2);
    b.rect(11, 17, 3, 5, 5);
    b.rect(26, 17, 3, 5, 5);
    b.ellipse(20, 24, 8, 8, 3);
    b.rect(15, 22, 3, 3, 13);
    b.rect(23, 22, 3, 3, 13);
    b.rect(16, 23, 1, 1, 12);
    b.rect(24, 23, 1, 1, 12);
    b.rect(17, 28, 6, 1, 14);
    b.rect(10, 32, 20, 9, 6);
    b.rect(10, 32, 20, 3, 8);
    b.rect(5, 32, 5, 10, 3);
    b.rect(30, 32, 5, 10, 3);
    b.rect(5, 40, 5, 3, 4);
    b.rect(30, 40, 5, 3, 4);
    b.ellipse(32, 41, 3, 3, 11);
    b.rect(13, 41, 6, 5, 9);
    b.rect(21, 41, 6, 5, 9);
    b.rect(12, 45, 8, 3, 11);
    b.rect(21, 45, 8, 3, 11);
    b.rect(12, 47, 8, 1, 12);
    b.rect(21, 47, 8, 1, 12);
    b.outline(15, false);
    return b;
}

Bitmap kidBack() {
    Bitmap b(40, 48);
    b.ellipse(20, 11, 11, 7, 1);
    b.rect(10, 13, 20, 6, 2);
    b.rect(12, 16, 16, 4, 5);
    b.rect(16, 20, 8, 4, 3);
    b.rect(10, 24, 20, 14, 6);
    b.rect(14, 27, 12, 3, 8);
    b.rect(18, 32, 4, 6, 2);
    b.rect(5, 26, 5, 12, 6);
    b.rect(30, 26, 5, 12, 6);
    b.rect(5, 36, 5, 4, 4);
    b.rect(30, 36, 5, 4, 4);
    b.rect(13, 38, 6, 6, 9);
    b.rect(21, 38, 6, 6, 9);
    b.rect(12, 44, 8, 3, 11);
    b.rect(21, 44, 8, 3, 11);
    b.outline(15, false);
    return b;
}

Bitmap kidSide() {
    Bitmap b(40, 48);
    b.ellipse(22, 11, 10, 6, 1);
    b.rect(14, 12, 16, 5, 1);
    b.rect(12, 16, 18, 3, 2);
    b.ellipse(24, 23, 7, 7, 3);
    b.rect(26, 22, 2, 2, 13);
    b.rect(27, 23, 1, 1, 12);
    b.rect(16, 28, 14, 10, 6);
    b.rect(16, 28, 14, 3, 8);
    b.rect(12, 30, 5, 8, 3);
    b.rect(28, 32, 6, 4, 3);
    b.ellipse(34, 36, 3, 3, 11);
    b.rect(18, 38, 6, 6, 9);
    b.rect(22, 40, 5, 5, 10);
    b.rect(17, 44, 9, 3, 11);
    b.rect(23, 45, 7, 3, 11);
    b.outline(15, false);
    return b;
}

Bitmap targetUp() {
    Bitmap b(48, 64);
    b.rect(22, 36, 5, 24, 6);
    b.rect(20, 58, 9, 4, 7);
    b.rect(6, 12, 36, 36, 2);
    b.rect(8, 14, 32, 32, 1);
    ring(b, 24, 30, 10, 14, 3);
    ring(b, 24, 30, 8, 10, 4);
    ring(b, 24, 30, 4, 7, 1);
    b.ellipse(24, 30, 4, 4, 5);
    b.set(24, 30, 9);
    b.set(12, 18, 8);
    b.set(36, 18, 8);
    b.set(12, 42, 8);
    b.set(36, 42, 8);
    b.outline(15, false);
    return b;
}

Bitmap targetDown() {
    Bitmap b(48, 64);
    b.rect(22, 44, 5, 14, 6);
    b.rect(20, 56, 9, 4, 7);
    b.ellipse(30, 48, 16, 9, 2);
    b.ellipse(30, 48, 14, 7, 1);
    ring(b, 30, 48, 5, 8, 3);
    b.ellipse(30, 48, 3, 2, 5);
    b.line(14, 40, 22, 46, 7, 2);
    b.outline(15, false);
    return b;
}

Bitmap gatePost() {
    Bitmap b(16, 68);
    b.rect(4, 6, 8, 56, 1);
    b.rect(5, 6, 3, 56, 2);
    b.rect(2, 58, 12, 8, 7);
    b.rect(3, 4, 10, 6, 3);
    for (int y = 14; y < 54; y += 12) b.rect(6, y, 4, 2, 3);
    b.outline(15, false);
    return b;
}

Bitmap gateBar() {
    Bitmap b(76, 12);
    b.rect(2, 2, 72, 8, 4);
    for (int x = 2; x < 74; x += 8) b.rect(float(x), 2, 4, 8, 6);
    b.rect(2, 2, 72, 2, 5);
    b.outline(15, false);
    return b;
}

Bitmap ballArt() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(5, 5, 2, 2, 2);
    b.line(3, 9, 11, 5, 3, 1.2f);
    return b;
}

Bitmap puffArt() {
    Bitmap b(20, 20);
    b.ellipse(10, 11, 8, 6, 2);
    b.ellipse(8, 9, 5, 4, 1);
    b.ellipse(13, 8, 3, 3, 3);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(24, 10);
    b.ellipse(12, 5, 10, 4, 1);
    return b;
}

Bitmap dotArt() {
    Bitmap b(6, 6);
    b.ellipse(3, 3, 2, 2, 1);
    return b;
}

Bitmap drumArt() {
    Bitmap b(32, 38);
    b.ellipse(16, 8, 12, 5, 2);
    b.rect(4, 8, 24, 22, 1);
    b.ellipse(16, 30, 12, 5, 3);
    b.rect(4, 14, 24, 4, 4);
    b.rect(4, 22, 24, 3, 5);
    b.ellipse(16, 8, 8, 3, 2);
    b.outline(15, false);
    return b;
}

Bitmap crateArt() {
    Bitmap b(34, 34);
    b.rect(4, 8, 26, 22, 1);
    b.rect(6, 10, 22, 18, 2);
    b.line(6, 10, 28, 28, 3, 2);
    b.line(28, 10, 6, 28, 3, 2);
    b.rect(4, 8, 26, 4, 4);
    b.outline(15, false);
    return b;
}

Bitmap tireArt() {
    Bitmap b(28, 28);
    ring(b, 14, 14, 6, 12, 1);
    ring(b, 14, 14, 5, 7, 2);
    b.rect(13, 4, 2, 4, 3);
    return b;
}

Bitmap weedArt() {
    Bitmap b(18, 24);
    b.line(9, 22, 4, 6, 1, 1.4f);
    b.line(9, 22, 9, 3, 2, 1.6f);
    b.line(9, 22, 15, 7, 1, 1.4f);
    b.line(9, 16, 2, 12, 3, 1.2f);
    b.line(9, 14, 16, 11, 3, 1.2f);
    return b;
}

Bitmap lampArt() {
    Bitmap b(20, 58);
    b.rect(8, 16, 4, 38, 1);
    b.rect(6, 50, 8, 4, 2);
    b.poly({{2, 16}, {10, 4}, {18, 16}}, 3);
    b.poly({{5, 15}, {10, 7}, {15, 15}}, 4);
    b.outline(15, false);
    return b;
}

Bitmap sunArt() {
    Bitmap b(22, 22);
    b.ellipse(11, 11, 8, 8, 1);
    b.ellipse(11, 11, 5, 5, 2);
    for (int i = 0; i < 8; i++) {
        float a = i * 0.785398f;
        b.line(11 + std::cos(a) * 6, 11 + std::sin(a) * 6, 11 + std::cos(a) * 10, 11 + std::sin(a) * 10, 1, 1.4f);
    }
    return b;
}

Bitmap starArt() {
    Bitmap b(7, 7);
    b.line(3, 0, 3, 6, 1, 1);
    b.line(0, 3, 6, 3, 1, 1);
    b.set(1, 1, 2);
    b.set(5, 1, 2);
    b.set(1, 5, 2);
    b.set(5, 5, 2);
    return b;
}

void fillGrass(uint8_t* px, int seed) {
    for (int i = 0; i < 64; i++) px[i] = 1;
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            uint32_t h = uint32_t(x * 17 + seed) * 0x8da6b343u ^ uint32_t(y * 13 + seed * 3) * 0xd8163841u;
            h ^= h >> 13;
            if ((h % 9) == 0) px[y * 8 + x] = 2;
            else if ((h % 11) == 0) px[y * 8 + x] = 3;
            else if ((h % 17) == 0) px[y * 8 + x] = 4;
        }
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    uint8_t g0[64], g1[64], walk[64], chain[64], post[64];
    fillGrass(g0, 1);
    fillGrass(g1, 2);
    for (int i = 0; i < 64; i++) walk[i] = 7;
    walk[5 * 8 + 2] = 9;
    walk[5 * 8 + 3] = 9;
    walk[3 * 8 + 6] = 8;
    std::memset(chain, 0, sizeof chain);
    for (int i = 0; i < 8; i++) {
        chain[i * 8 + i] = 10;
        chain[i * 8 + (7 - i)] = 10;
    }
    std::memset(post, 0, sizeof post);
    for (int y = 0; y < 8; y++)
        for (int x = 2; x <= 5; x++) post[y * 8 + x] = x < 4 ? 12 : 11;
    a.grass[0] = tiles.alloc(1);
    a.grass[1] = tiles.alloc(1);
    a.walk = tiles.alloc(1);
    a.chain = tiles.alloc(1);
    a.fencePost = tiles.alloc(1);
    vdp.loadTile(a.grass[0], g0);
    vdp.loadTile(a.grass[1], g1);
    vdp.loadTile(a.walk, walk);
    vdp.loadTile(a.chain, chain);
    vdp.loadTile(a.fencePost, post);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    auto ink = [&](int pal, uint16_t c) {
        setPal(vdp, pal, {0, c, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    };
    ink(PAL_HUD, gs::rgb4(15, 15, 15));
    ink(PAL_DIM, gs::rgb4(8, 8, 10));
    ink(PAL_ALERT, gs::rgb4(15, 4, 3));
    ink(PAL_OK, gs::rgb4(6, 15, 7));
    ink(PAL_GOLD, gs::rgb4(15, 12, 3));

    setPal(vdp, PAL_KID,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(14, 10, 7), gs::rgb4(11, 7, 5), gs::rgb4(3, 2, 1),
            gs::rgb4(2, 7, 13), gs::rgb4(1, 4, 8), gs::rgb4(15, 12, 3), gs::rgb4(3, 4, 8), gs::rgb4(2, 2, 5),
            gs::rgb4(14, 14, 13), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(12, 5, 5), shadow});
    auto tgt = [&](int pal, uint16_t ringc, uint16_t mid) {
        setPal(vdp, pal,
               {0, gs::rgb4(14, 13, 11), gs::rgb4(9, 7, 5), ringc, gs::rgb4((ringc >> 8) * 2 / 3, ((ringc >> 4) & 15) * 2 / 3, (ringc & 15) * 2 / 3),
                mid, gs::rgb4(9, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 12, 13), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, shadow});
    };
    tgt(PAL_T1, gs::rgb4(13, 2, 2), gs::rgb4(15, 12, 2));
    tgt(PAL_T2, gs::rgb4(2, 6, 14), gs::rgb4(14, 14, 15));
    tgt(PAL_T3, gs::rgb4(14, 8, 1), gs::rgb4(15, 14, 4));
    setPal(vdp, PAL_GATE,
           {0, gs::rgb4(5, 5, 6), gs::rgb4(9, 9, 11), gs::rgb4(13, 13, 14), gs::rgb4(15, 12, 2), gs::rgb4(12, 8, 1),
            gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 6), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BALL, {0, gs::rgb4(15, 15, 14), gs::rgb4(12, 12, 11), gs::rgb4(13, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_JUNK,
           {0, gs::rgb4(8, 6, 3), gs::rgb4(11, 8, 4), gs::rgb4(6, 4, 2), gs::rgb4(13, 10, 2), gs::rgb4(10, 4, 2),
            gs::rgb4(4, 8, 3), gs::rgb4(7, 12, 4), gs::rgb4(3, 3, 4), gs::rgb4(10, 10, 11), gs::rgb4(14, 12, 6), 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 13, 10), gs::rgb4(10, 9, 7), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FIELD,
           {0, gs::rgb4(3, 8, 2), gs::rgb4(2, 5, 2), gs::rgb4(5, 10, 3), gs::rgb4(7, 12, 4), gs::rgb4(8, 6, 3),
            gs::rgb4(6, 4, 2), gs::rgb4(8, 8, 9), gs::rgb4(11, 11, 12), gs::rgb4(5, 5, 6), gs::rgb4(11, 12, 13),
            gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), 0, 0, shadow});

    loadFont(vdp, art);
    art.kidFront = gs::uploadMipped(vdp, kidFront());
    art.kidBack = gs::uploadMipped(vdp, kidBack());
    art.kidSide = gs::uploadMipped(vdp, kidSide());
    art.targetUp = gs::uploadMipped(vdp, targetUp());
    art.targetDown = gs::uploadMipped(vdp, targetDown());
    art.post = gs::uploadMipped(vdp, gatePost());
    art.bar = gs::uploadMipped(vdp, gateBar());
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.tire = gs::uploadMipped(vdp, tireArt());
    art.weed = gs::uploadMipped(vdp, weedArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.star = gs::uploadMipped(vdp, starArt());

    auto grassAt = [&](int x, int y) {
        bool gap = y == 6 && x >= 16 && x <= 23;
        if (y < 5) return;
        if (y == 5 || gap || y > 26) vdp.B.set(x, y, gs::entry(art.walk, PAL_FIELD));
        else vdp.B.set(x, y, gs::entry(art.grass[(x * 3 + y) & 1], PAL_FIELD));
    };
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 64; x++) grassAt(x, y);
    for (int y = 6; y <= 26; y++) {
        vdp.A.set(1, y, gs::entry(art.fencePost, PAL_FIELD));
        vdp.A.set(38, y, gs::entry(art.fencePost, PAL_FIELD));
    }
    for (int x = 2; x <= 37; x++)
        if (x < 16 || x > 23) vdp.A.set(x, 6, gs::entry(art.chain, PAL_FIELD));
    for (int x = 1; x <= 38; x++) vdp.A.set(x, 26, gs::entry(art.chain, PAL_FIELD));

    vdp.setFogColor(gs::rgb4(6, 5, 8));
}

}  // namespace lot
