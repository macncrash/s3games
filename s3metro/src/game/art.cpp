#include "game/art.h"

#include <cmath>
#include <cstring>
#include <string>

namespace metro {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void brick(uint8_t* px, int shift) {
    std::memset(px, 1, 64);
    for (int x = 0; x < 8; x++) px[7 * 8 + x] = 3;
    int vx = shift;
    for (int y = 0; y < 7; y++) px[y * 8 + vx] = 3;
    px[2 * 8 + ((shift + 2) & 7)] = 2;
    px[5 * 8 + ((shift + 5) & 7)] = 2;
}

void niche(uint8_t* px) {
    std::memset(px, 7, 64);
    for (int y = 1; y < 7; y++)
        for (int x = 1; x < 7; x++) px[y * 8 + x] = 3;
    px[3 * 8 + 3] = 9;
}

void loadFont(gs::VDP& vdp, Art& a, gs::TileAlloc& tiles) {
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

void loadWall(gs::VDP& vdp, gs::TileAlloc& tiles) {
    uint8_t a[64], b[64], n[64];
    brick(a, 2);
    brick(b, 6);
    niche(n);
    int t0 = tiles.alloc(1);
    int t1 = tiles.alloc(1);
    int tn = tiles.alloc(1);
    vdp.loadTile(t0, a);
    vdp.loadTile(t1, b);
    vdp.loadTile(tn, n);
    vdp.A.enabled = false;
    for (int cy = 4; cy <= 20; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            int tile = ((cx + cy) & 1) ? t1 : t0;
            if (cy >= 7 && cy <= 12 && (cx % 11 == 4)) tile = tn;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_WALL));
        }
    }
}

Bitmap leadCar() {
    Bitmap b(148, 70);
    b.rect(16, 8, 112, 6, 3);
    b.rect(28, 3, 24, 7, 9);
    b.rect(58, 4, 18, 6, 9);
    b.rect(8, 14, 120, 42, 2);
    b.rect(8, 14, 120, 3, 1);
    b.rect(8, 40, 120, 7, 4);
    b.rect(8, 46, 120, 2, 11);
    b.rect(10, 56, 116, 6, 3);
    b.poly({{128, 14}, {142, 34}, {128, 56}}, 2);
    b.poly({{129, 18}, {136, 34}, {129, 24}}, 1);
    b.poly({{130, 22}, {139, 34}, {130, 48}}, 5);
    b.poly({{131, 24}, {135, 32}, {131, 30}}, 6);
    b.ellipse(131, 36, 3.2f, 3.4f, 14);
    b.ellipse(140, 40, 2.6f, 2.2f, 8);
    b.rect(140, 29, 3, 3, 12);
    b.rect(140, 48, 3, 3, 4);
    for (int i = 0; i < 4; i++) {
        int x = 14 + i * 24;
        b.rect(x, 20, 18, 14, 5);
        b.rect(x + 1, 21, 16, 4, 6);
        b.ellipse(float(x + 6), 31, 2.4f, 2.6f, 7);
        if (i & 1) b.ellipse(float(x + 12), 31.5f, 2.1f, 2.5f, 7);
    }
    b.rect(108, 18, 16, 34, 9);
    b.rect(109, 19, 14, 12, 5);
    b.rect(116, 20, 1, 30, 3);
    b.rect(120, 32, 2, 4, 8);
    b.rect(16, 16, 52, 8, 10);
    b.rect(18, 18, 48, 4, 5);
    b.ellipse(40, 62, 12, 8, 7);
    b.ellipse(108, 62, 12, 8, 7);
    b.rect(0, 30, 8, 14, 7);
    b.outline(15, false);
    return b;
}

Bitmap midCar() {
    Bitmap b(140, 70);
    b.rect(8, 8, 124, 6, 3);
    b.rect(24, 3, 22, 7, 9);
    b.rect(70, 4, 16, 6, 9);
    b.rect(6, 14, 128, 42, 2);
    b.rect(6, 14, 128, 3, 1);
    b.rect(6, 40, 128, 7, 4);
    b.rect(6, 46, 128, 2, 11);
    b.rect(8, 56, 124, 6, 3);
    for (int i = 0; i < 3; i++) {
        int x = 14 + i * 28;
        b.rect(x, 20, 18, 14, 5);
        b.rect(x + 1, 21, 16, 4, 6);
        b.ellipse(float(x + 8), 31, 2.4f, 2.6f, 7);
    }
    b.rect(96, 18, 16, 34, 9);
    b.rect(97, 19, 14, 12, 5);
    b.rect(103, 20, 1, 30, 3);
    b.ellipse(36, 62, 12, 8, 7);
    b.ellipse(104, 62, 12, 8, 7);
    b.rect(0, 28, 6, 16, 7);
    b.rect(134, 28, 6, 16, 7);
    b.outline(15, false);
    return b;
}

Bitmap wheelArt(int phase) {
    Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 7);
    b.ellipse(11, 11, 7.2f, 7.2f, 13);
    b.ellipse(11, 11, 2.4f, 2.4f, 3);
    const float pi = 3.1415926f;
    float a0 = phase * pi / 3.f;
    for (int i = 0; i < 3; i++) {
        float a = a0 + i * pi / 3.f;
        b.line(11, 11, 11 + std::cos(a) * 6.5f, 11 + std::sin(a) * 6.5f, 1, 1.6f);
    }
    b.ellipse(11, 11, 2.2f, 2.2f, 1);
    return b;
}

Bitmap slabArt() {
    Bitmap b(80, 56);
    b.rect(0, 0, 80, 7, 4);
    b.rect(0, 5, 80, 3, 5);
    b.rect(0, 8, 80, 48, 2);
    for (int x = 0; x < 80; x += 20) b.rect(x, 8, 1, 48, 3);
    for (int x = 3; x < 78; x += 6) b.rect(x, 11, 2, 2, 10);
    for (int i = 0; i < 36; i++) b.set((i * 17) % 80, 18 + (i * 11) % 34, (i & 1) ? 1 : 3);
    b.rect(0, 52, 80, 4, 9);
    return b;
}

Bitmap railArt() {
    Bitmap b(80, 16);
    for (int x = 2; x < 80; x += 10) b.rect(x, 1, 6, 14, 3);
    b.rect(0, 3, 80, 2, 8);
    b.rect(0, 11, 80, 2, 8);
    b.rect(0, 3, 80, 1, 1);
    return b;
}

Bitmap postArt() {
    Bitmap b(6, 100);
    b.rect(1, 0, 4, 96, 1);
    for (int y = 4; y < 90; y += 12) b.rect(1, y, 4, 4, 2);
    b.rect(0, 92, 6, 8, 3);
    return b;
}

Bitmap edgeArt() {
    Bitmap b(2, 108);
    b.rect(0, 0, 2, 108, 1);
    return b;
}

Bitmap boxSegArt() {
    Bitmap b(8, 16);
    b.rect(0, 0, 8, 16, 4);
    b.rect(0, 0, 8, 2, 7);
    b.line(0, 16, 8, 2, 11, 1.4f);
    return b;
}

Bitmap markArt() {
    Bitmap b(2, 18);
    b.rect(0, 0, 2, 18, 1);
    return b;
}

Bitmap chevronArt() {
    Bitmap b(12, 16);
    b.poly({{1, 1}, {11, 8}, {1, 15}}, 1);
    b.poly({{3, 5}, {8, 8}, {3, 11}}, 2);
    return b;
}

Bitmap personArt(int coat, bool bag) {
    Bitmap b(18, 40);
    b.ellipse(9, 7, 4.5f, 4.6f, 1);
    b.rect(5, 3, 8, 4, 2);
    b.rect(5, 12, 8, 13, coat);
    b.rect(6, 13, 6, 4, 8);
    b.rect(3, 13, 3, 11, coat);
    b.rect(12, 13, 3, 11, coat);
    b.rect(6, 25, 3, 10, 6);
    b.rect(10, 25, 3, 10, 6);
    b.rect(5, 34, 4, 3, 7);
    b.rect(10, 34, 4, 3, 7);
    b.set(7, 7, 2);
    b.set(11, 7, 2);
    if (bag) b.rect(13, 16, 4, 7, 9);
    b.outline(15, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 52);
    b.rect(7, 12, 2, 40, 1);
    b.rect(2, 10, 12, 3, 1);
    b.ellipse(5, 10, 5, 4, 4);
    b.ellipse(5, 12, 3, 2.2f, 2);
    b.ellipse(5, 12, 1.3f, 1.1f, 3);
    return b;
}

Bitmap pipeArt() {
    Bitmap b(48, 16);
    b.rect(0, 6, 48, 5, 8);
    b.rect(0, 6, 48, 1, 9);
    b.rect(10, 2, 5, 12, 8);
    b.rect(30, 2, 5, 12, 8);
    b.ellipse(12, 8, 4, 4, 7);
    return b;
}

Bitmap posterArt(int kind) {
    Bitmap b(28, 36);
    b.rect(0, 0, 28, 36, 3);
    b.rect(2, 2, 24, 32, kind == 0 ? 4 : kind == 1 ? 5 : 6);
    if (kind == 0) {
        b.rect(5, 6, 18, 6, 6);
        b.ellipse(14, 22, 6, 6, 5);
    } else if (kind == 1) {
        b.poly({{6, 28}, {14, 10}, {22, 28}}, 6);
        b.rect(5, 6, 18, 4, 4);
    } else {
        b.ellipse(14, 16, 8, 8, 4);
        b.rect(6, 26, 16, 4, 5);
    }
    return b;
}

Bitmap girderArt() {
    Bitmap b(22, 88);
    b.rect(1, 0, 4, 88, 8);
    b.rect(17, 0, 4, 88, 8);
    for (int y = 4; y < 80; y += 14) b.line(4, float(y), 18, float(y + 8), 9, 1.6f);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(72, 26);
    b.ellipse(22, 15, 16, 8, 2);
    b.ellipse(40, 13, 20, 10, 1);
    b.ellipse(56, 16, 14, 7, 2);
    return b;
}

Bitmap signalArt() {
    Bitmap b(14, 22);
    b.rect(1, 1, 12, 16, 1);
    b.ellipse(7, 8, 4.2f, 4.2f, 2);
    b.ellipse(6, 7, 1.4f, 1.2f, 3);
    b.rect(6, 17, 2, 5, 1);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 2, 3);
    b.ellipse(3, 3, 1.4f, 1.2f, 1);
    return b;
}

Bitmap rainArt() {
    Bitmap b(2, 12);
    b.line(0, 0, 1, 11, 2, 1.2f);
    return b;
}

Bitmap plateArt() {
    Bitmap b(240, 86);
    b.rect(0, 0, 240, 86, 7);
    b.rect(3, 3, 234, 80, 3);
    return b;
}

Bitmap signArt() {
    Bitmap b(96, 24);
    b.rect(0, 0, 96, 24, 8);
    b.rect(3, 3, 90, 18, 3);
    b.rect(42, 0, 10, 4, 8);
    return b;
}

}  // namespace

const Theme& themeFor(int i) {
    static const Theme t[5] = {
        {gs::rgb4(1, 3, 6), gs::rgb4(2, 6, 8), gs::rgb4(1, 1, 2), gs::rgb4(4, 8, 9), gs::rgb4(2, 5, 6),
         gs::rgb4(2, 8, 10), gs::rgb4(12, 8, 3), gs::rgb4(12, 11, 6), false, false},
        {gs::rgb4(5, 2, 2), gs::rgb4(8, 4, 3), gs::rgb4(2, 1, 1), gs::rgb4(9, 6, 5), gs::rgb4(6, 4, 3),
         gs::rgb4(13, 3, 2), gs::rgb4(14, 11, 4), gs::rgb4(15, 12, 5), false, false},
        {gs::rgb4(1, 3, 2), gs::rgb4(3, 6, 5), gs::rgb4(1, 1, 1), gs::rgb4(4, 7, 6), gs::rgb4(2, 5, 4),
         gs::rgb4(8, 10, 3), gs::rgb4(3, 7, 9), gs::rgb4(11, 13, 7), false, true},
        {gs::rgb4(5, 9, 14), gs::rgb4(9, 12, 15), gs::rgb4(4, 6, 8), gs::rgb4(8, 10, 12), gs::rgb4(6, 7, 9),
         gs::rgb4(13, 5, 3), gs::rgb4(4, 8, 13), gs::rgb4(15, 15, 13), true, false},
        {gs::rgb4(4, 1, 2), gs::rgb4(7, 2, 3), gs::rgb4(1, 0, 1), gs::rgb4(8, 4, 5), gs::rgb4(5, 2, 3),
         gs::rgb4(13, 10, 3), gs::rgb4(10, 2, 3), gs::rgb4(15, 12, 6), false, false},
    };
    if (i < 0) i = 0;
    if (i > 4) i = 4;
    return t[i];
}

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_WHITE, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 14), gs::rgb4(8, 9, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 3, 2), gs::rgb4(2, 1, 1), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 15, 6), gs::rgb4(2, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(8, 10, 12), gs::rgb4(11, 13, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    setPal(vdp, PAL_TRAIN,
           {0, gs::rgb4(14, 15, 15), gs::rgb4(10, 12, 13), gs::rgb4(5, 6, 8), gs::rgb4(13, 2, 3), gs::rgb4(1, 2, 4),
            gs::rgb4(7, 12, 14), gs::rgb4(1, 1, 2), gs::rgb4(15, 14, 6), gs::rgb4(7, 8, 10), gs::rgb4(13, 11, 8),
            gs::rgb4(7, 1, 2), gs::rgb4(15, 12, 2), gs::rgb4(3, 3, 4), gs::rgb4(12, 8, 6), gs::rgb4(0, 0, 1)});
    setPal(vdp, PAL_PLAT,
           {0, gs::rgb4(13, 13, 12), gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 5), gs::rgb4(15, 13, 2), gs::rgb4(11, 8, 1),
            gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 15), gs::rgb4(6, 7, 8), gs::rgb4(1, 1, 2), gs::rgb4(7, 7, 6),
            gs::rgb4(2, 2, 2), 0, 0, 0, shadow});
    setPal(vdp, PAL_WALL,
           {0, gs::rgb4(4, 8, 9), gs::rgb4(2, 5, 6), gs::rgb4(2, 2, 3), gs::rgb4(2, 8, 10), gs::rgb4(12, 8, 3),
            gs::rgb4(14, 12, 9), gs::rgb4(1, 1, 2), gs::rgb4(6, 7, 8), gs::rgb4(12, 12, 11), gs::rgb4(3, 5, 4),
            gs::rgb4(8, 5, 4), gs::rgb4(5, 3, 3), 0, 0, shadow});
    setPal(vdp, PAL_PEEP,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(2, 1, 1), gs::rgb4(13, 2, 2), gs::rgb4(2, 4, 10), gs::rgb4(3, 8, 4),
            gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 1), gs::rgb4(14, 14, 13), gs::rgb4(6, 4, 2), gs::rgb4(10, 6, 4),
            gs::rgb4(5, 3, 2), gs::rgb4(13, 11, 3), 0, 0, shadow});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(3, 3, 4), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 13), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_POST, {0, gs::rgb4(15, 13, 2), gs::rgb4(2, 2, 2), gs::rgb4(10, 8, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SIGNAL, {0, gs::rgb4(3, 3, 4), gs::rgb4(15, 11, 2), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 4, 2), gs::rgb4(15, 10, 2), gs::rgb4(8, 8, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    gs::TileAlloc tiles(vdp);
    loadWall(vdp, tiles);
    loadFont(vdp, art, tiles);

    art.lead = gs::uploadMipped(vdp, leadCar());
    art.mid = gs::uploadMipped(vdp, midCar());
    for (int i = 0; i < 3; i++) art.wheel[i] = gs::uploadMipped(vdp, wheelArt(i));
    art.slab = gs::uploadMipped(vdp, slabArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.edge = gs::uploadMipped(vdp, edgeArt());
    art.boxSeg = gs::uploadMipped(vdp, boxSegArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    art.chevron = gs::uploadMipped(vdp, chevronArt());
    const int coats[4] = {3, 4, 5, 12};
    for (int i = 0; i < 4; i++) art.person[i] = gs::uploadMipped(vdp, personArt(coats[i], i == 1 || i == 2));
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.pipe = gs::uploadMipped(vdp, pipeArt());
    for (int i = 0; i < 3; i++) art.poster[i] = gs::uploadMipped(vdp, posterArt(i));
    art.girder = gs::uploadMipped(vdp, girderArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.signal = gs::uploadMipped(vdp, signalArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.rain = gs::uploadMipped(vdp, rainArt());
    art.plate = gs::uploadMipped(vdp, plateArt());
    art.sign = gs::uploadMipped(vdp, signArt());
}

}  // namespace metro
