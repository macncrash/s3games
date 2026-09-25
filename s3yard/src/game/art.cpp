#include "game/art.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>

namespace yard {
namespace {

constexpr float PI = 3.14159265f;

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(p * 16 + i, c);
        i++;
    }
    while (i < 16) vdp.setColor(p * 16 + i++, 0);
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

// Machine pixels share one layout across palettes:
// 1 outline, 2 body, 3 body dark, 4 highlight, 5 glass, 6 shine,
// 7 tire, 8 hub, 9 lamp, 10 tail, 11 chrome, 12 grill, 13 rust, 14 pipe.
gs::Bitmap drawCar() {
    gs::Bitmap b(64, 64);
    b.rect(18, 18, 8, 13, 7);
    b.rect(38, 18, 8, 13, 7);
    b.rect(18, 36, 8, 13, 7);
    b.rect(38, 36, 8, 13, 7);
    b.rect(20, 21, 4, 7, 8);
    b.rect(40, 21, 4, 7, 8);
    b.rect(20, 39, 4, 7, 8);
    b.rect(40, 39, 4, 7, 8);
    b.poly({{26, 16}, {38, 16}, {43, 24}, {43, 44}, {38, 50}, {26, 50}, {21, 44}, {21, 24}}, 2);
    b.poly({{24, 32}, {40, 32}, {42, 46}, {22, 46}}, 3);
    b.poly({{28, 18}, {36, 18}, {39, 26}, {25, 26}}, 4);
    b.poly({{27, 27}, {37, 27}, {35, 36}, {29, 36}}, 5);
    b.rect(29, 29, 4, 3, 6);
    b.rect(27, 15, 4, 3, 9);
    b.rect(33, 15, 4, 3, 9);
    b.rect(24, 47, 5, 3, 10);
    b.rect(35, 47, 5, 3, 10);
    b.rect(26, 13, 12, 3, 11);
    b.rect(24, 49, 16, 2, 11);
    b.rect(30, 18, 4, 2, 12);
    b.rect(22, 40, 4, 3, 13);
    b.rect(38, 38, 3, 3, 13);
    b.rect(30, 51, 4, 3, 14);
    b.outline(1, false);
    return b;
}

gs::Bitmap spin(const gs::Bitmap& src, float ang) {
    gs::Bitmap d(src.w, src.h);
    const float cx = (src.w - 1) * 0.5f;
    const float cy = (src.h - 1) * 0.5f;
    const float c = std::cos(ang);
    const float s = std::sin(ang);
    for (int y = 0; y < d.h; y++) {
        for (int x = 0; x < d.w; x++) {
            float dx = x - cx;
            float dy = y - cy;
            int sx = int(std::lround(cx + c * dx + s * dy));
            int sy = int(std::lround(cy - s * dx + c * dy));
            d.set(x, y, src.get(sx, sy));
        }
    }
    return d;
}

gs::Bitmap drawWreck() {
    gs::Bitmap b(56, 40);
    b.ellipse(28, 24, 22, 12, 3);
    b.poly({{10, 18}, {30, 12}, {44, 16}, {40, 28}, {16, 30}}, 2);
    b.poly({{18, 20}, {36, 18}, {34, 26}, {20, 28}}, 4);
    b.rect(8, 22, 10, 6, 7);
    b.rect(38, 14, 10, 6, 7);
    b.rect(34, 26, 12, 7, 7);
    b.ellipse(14, 26, 5, 5, 7);
    b.ellipse(14, 26, 2, 2, 8);
    b.rect(22, 14, 8, 3, 5);
    b.rect(30, 22, 6, 4, 13);
    b.rect(40, 20, 3, 2, 10);
    b.outline(1, false);
    return b;
}

gs::Bitmap drawPile(int kind) {
    gs::Bitmap b(64, 52);
    b.ellipse(32, 34, 24, 14, 3);
    b.ellipse(30, 28, 18, 12, 2);
    b.rect(14, 24, 26, 6, 4);
    b.rect(18, 20, 18, 4, 9);
    b.rect(36, 16, 7, 18, 5);
    b.ellipse(22, 30, 7, 7, 7);
    b.ellipse(22, 30, 3, 3, 8);
    if (kind == 0) {
        b.rect(34, 22, 14, 10, 6);
        b.rect(44, 24, 2, 6, 11);
        b.rect(16, 18, 12, 4, 10);
    } else {
        b.poly({{16, 16}, {34, 12}, {36, 22}, {18, 26}}, 10);
        b.rect(40, 28, 12, 8, 4);
        b.rect(44, 30, 4, 4, 12);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap drawDrum() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 15, 11, 11, 5);
    b.ellipse(14, 14, 9, 9, 2);
    b.ellipse(14, 13, 6, 5, 9);
    b.rect(6, 12, 16, 2, 4);
    b.rect(6, 17, 16, 2, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap drawCrane() {
    gs::Bitmap b(72, 56);
    b.rect(8, 28, 22, 16, 10);
    b.rect(10, 30, 10, 8, 5);
    b.rect(12, 32, 4, 3, 6);
    b.rect(6, 40, 26, 6, 3);
    b.line(28, 34, 58, 16, 11, 3);
    b.line(30, 38, 56, 22, 5, 2);
    b.ellipse(58, 16, 6, 6, 12);
    b.ellipse(58, 16, 3, 3, 1);
    b.rect(4, 44, 8, 4, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap drawPurse() {
    gs::Bitmap b(40, 36);
    b.rect(6, 28, 28, 5, 10);
    b.rect(8, 30, 24, 2, 9);
    b.ellipse(20, 18, 12, 11, 3);
    b.ellipse(20, 17, 10, 9, 2);
    b.rect(10, 10, 20, 5, 4);
    b.rect(17, 6, 6, 7, 5);
    b.ellipse(20, 19, 4, 3, 6);
    b.ellipse(16, 20, 2, 2, 7);
    b.ellipse(23, 21, 2, 2, 7);
    b.line(12, 12, 8, 6, 8, 2);
    b.line(28, 12, 32, 6, 8, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap drawSign() {
    gs::TextStyle st;
    st.scale = 3;
    st.color = 1;
    st.shadow = 15;
    st.spacing = 1;
    gs::Bitmap word = gs::textBitmap("YARD", st);
    gs::Bitmap sign(word.w + 16, word.h + 14);
    sign.rect(0, 0, float(sign.w), float(sign.h), 4);
    sign.rect(3, 3, float(sign.w - 6), float(sign.h - 6), 5);
    sign.blit(word, 8, 6);
    sign.rect(4, 4, 2, 2, 1);
    sign.rect(float(sign.w - 6), 4, 2, 2, 1);
    sign.rect(4, float(sign.h - 6), 2, 2, 1);
    sign.rect(float(sign.w - 6), float(sign.h - 6), 2, 2, 1);
    return sign;
}

void fillDirt(uint8_t* px, int seed) {
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            uint32_t h = uint32_t(x * 131 + y * 719 + seed * 997);
            h ^= h >> 13;
            h *= 0x5bd1e995u;
            int c = 2;
            if ((h & 15) == 0) c = 3;
            else if ((h & 15) == 1) c = 1;
            if (h % 23 == 0) c = 4;
            if (h % 31 == 0) c = 5;
            px[y * 8 + x] = uint8_t(c);
        }
    }
}

void fillStain(uint8_t* px) {
    std::memset(px, 0, 64);
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int d = (x - 3) * (x - 3) + (y - 4) * (y - 4);
            if (d < 7) px[y * 8 + x] = 6;
            if (d < 2) px[y * 8 + x] = 1;
        }
}

void fillPad(uint8_t* px) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int c = 8;
            if (x == 0 || y == 0 || x == 7 || y == 7) c = 1;
            else if ((x + y * 2) % 5 == 0) c = 7;
            px[y * 8 + x] = uint8_t(c);
        }
}

void fillFence(uint8_t* px) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int col = x & 3;
            int c = col == 0 ? 1 : (col == 3 ? 3 : 2);
            if (y == 0 || y == 7) c = 3;
            if (x == 2 && (y == 2 || y == 5)) c = 5;
            if ((x * 3 + y) % 11 == 0) c = 4;
            px[y * 8 + x] = uint8_t(c);
        }
}

void layYard(gs::VDP& vdp, int dirt0, int dirt1, int dirt2, int stain, int pad, int fence) {
    const int dirt[3] = {dirt0, dirt1, dirt2};
    for (int cy = 0; cy < 32; cy++) {
        for (int cx = 0; cx < 64; cx++) {
            vdp.B.set(cx, cy, gs::entry(dirt[(cx * 3 + cy * 5) & 3 ? (cx + cy) % 3 : 1], PAL_GROUND));
        }
    }
    for (int cy = 0; cy < 28; cy++) {
        for (int cx = 0; cx < 40; cx++) {
            bool border = cx < 2 || cx >= 38 || cy < 2 || cy >= 26;
            if (border) vdp.A.set(cx, cy, gs::entry(fence, PAL_FENCE));
            else if (((cx * 13 + cy * 7) % 17) == 0) vdp.A.set(cx, cy, gs::entry(stain, PAL_GROUND));
        }
    }
    for (int cy = 12; cy <= 16; cy++)
        for (int cx = 18; cx <= 22; cx++) vdp.A.set(cx, cy, gs::entry(pad, PAL_GROUND));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t black = gs::rgb4(1, 1, 1);
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    pal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 12), gs::rgb4(8, 7, 6), gs::rgb4(15, 10, 2), gs::rgb4(15, 4, 3),
                       gs::rgb4(6, 14, 6), gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    pal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    pal(vdp, PAL_OK, {0, gs::rgb4(8, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    pal(vdp, PAL_PRIZE, {0, gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    auto machine = [&](int p, uint16_t body, uint16_t dark, uint16_t lite, uint16_t glass) {
        pal(vdp, p, {0, black, body, dark, lite, glass, gs::rgb4(13, 15, 15), gs::rgb4(2, 2, 2), gs::rgb4(10, 10, 9),
                     gs::rgb4(15, 15, 6), gs::rgb4(15, 2, 1), gs::rgb4(13, 13, 12), gs::rgb4(4, 4, 4), gs::rgb4(12, 5, 2),
                     gs::rgb4(6, 6, 6), black});
    };
    machine(PAL_YOU, gs::rgb4(15, 8, 1), gs::rgb4(10, 4, 1), gs::rgb4(15, 13, 6), gs::rgb4(6, 10, 12));
    machine(PAL_BLUE, gs::rgb4(4, 8, 14), gs::rgb4(2, 4, 8), gs::rgb4(10, 14, 15), gs::rgb4(8, 12, 14));
    machine(PAL_OLIVE, gs::rgb4(6, 10, 3), gs::rgb4(3, 6, 2), gs::rgb4(12, 14, 6), gs::rgb4(7, 11, 10));
    machine(PAL_CREAM, gs::rgb4(14, 12, 9), gs::rgb4(10, 8, 6), gs::rgb4(15, 14, 12), gs::rgb4(8, 10, 11));
    machine(PAL_WRECK, gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 2), gs::rgb4(7, 6, 5), gs::rgb4(3, 3, 3));

    pal(vdp, PAL_SCRAP,
        {0, black, gs::rgb4(11, 6, 2), gs::rgb4(6, 4, 3), gs::rgb4(12, 9, 5), gs::rgb4(8, 8, 9), gs::rgb4(10, 12, 9),
         gs::rgb4(2, 2, 2), gs::rgb4(9, 7, 5), gs::rgb4(14, 10, 6), gs::rgb4(14, 12, 2), gs::rgb4(5, 5, 6),
         gs::rgb4(14, 3, 2), gs::rgb4(8, 4, 2), gs::rgb4(3, 2, 2), black});
    pal(vdp, PAL_PURSE,
        {0, black, gs::rgb4(12, 7, 3), gs::rgb4(8, 4, 2), gs::rgb4(14, 9, 4), gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 8),
         gs::rgb4(14, 11, 4), gs::rgb4(6, 4, 2), gs::rgb4(11, 8, 4), gs::rgb4(7, 5, 3), 0, 0, 0, 0, black});
    pal(vdp, PAL_FX, {0, gs::rgb4(12, 11, 10), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(15, 13, 3),
                      gs::rgb4(15, 15, 13), gs::rgb4(15, 8, 2), gs::rgb4(14, 4, 1), 0, 0, 0, 0, 0, 0, 0, shadow});
    pal(vdp, PAL_GROUND,
        {0, gs::rgb4(3, 2, 2), gs::rgb4(6, 5, 4), gs::rgb4(8, 7, 5), gs::rgb4(9, 8, 7), gs::rgb4(6, 7, 3),
         gs::rgb4(2, 2, 2), gs::rgb4(3, 2, 2), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, black});
    pal(vdp, PAL_FENCE, {0, gs::rgb4(3, 3, 3), gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 12), gs::rgb4(10, 5, 2),
                         gs::rgb4(14, 13, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, black});
    pal(vdp, PAL_SIGN, {0, gs::rgb4(15, 14, 10), 0, 0, gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                        shadow});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);
    int dirt[3];
    for (int i = 0; i < 3; i++) {
        uint8_t px[64];
        fillDirt(px, 3 + i * 9);
        dirt[i] = tiles.alloc(1);
        vdp.loadTile(dirt[i], px);
    }
    uint8_t stain[64], padpx[64], fence[64];
    fillStain(stain);
    fillPad(padpx);
    fillFence(fence);
    int stainT = tiles.alloc(1);
    int padT = tiles.alloc(1);
    int fenceT = tiles.alloc(1);
    vdp.loadTile(stainT, stain);
    vdp.loadTile(padT, padpx);
    vdp.loadTile(fenceT, fence);
    layYard(vdp, dirt[0], dirt[1], dirt[2], stainT, padT, fenceT);

    gs::Bitmap car = drawCar();
    for (int i = 0; i < 8; i++) {
        float rot = i * (PI / 4.f) + PI / 2.f;
        art.car[i] = gs::uploadMipped(vdp, spin(car, rot).cropToContent(1));
    }
    art.wreck = gs::uploadMipped(vdp, drawWreck());
    art.pile[0] = gs::uploadMipped(vdp, drawPile(0));
    art.pile[1] = gs::uploadMipped(vdp, drawPile(1));
    art.drum = gs::uploadMipped(vdp, drawDrum());
    art.crane = gs::uploadMipped(vdp, drawCrane());
    art.purse = gs::uploadMipped(vdp, drawPurse());
    art.sign = gs::uploadMipped(vdp, drawSign());

    gs::Bitmap smoke(32, 32);
    smoke.ellipse(18, 18, 12, 10, 2);
    smoke.ellipse(14, 14, 8, 7, 1);
    smoke.ellipse(15, 15, 3, 3, 3);
    art.smoke = gs::uploadMipped(vdp, smoke);

    gs::Bitmap spark(16, 16);
    spark.line(8, 1, 8, 14, 4, 2);
    spark.line(1, 8, 14, 8, 4, 2);
    spark.line(3, 3, 12, 12, 5, 1);
    spark.line(12, 3, 3, 12, 5, 1);
    art.spark = gs::uploadMipped(vdp, spark);

    gs::Bitmap flame(16, 20);
    flame.ellipse(8, 12, 6, 7, 7);
    flame.ellipse(8, 10, 4, 6, 6);
    flame.ellipse(8, 8, 2, 3, 5);
    art.flame = gs::uploadMipped(vdp, flame);

    gs::Bitmap shade(48, 16);
    shade.ellipse(24, 8, 20, 6, 1);
    art.shadow = gs::uploadMipped(vdp, shade);

    uint16_t dust = gs::rgb4(5, 4, 3);
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.lineBackdrop[y] = dust;
    vdp.setFogColor(gs::rgb4(6, 5, 4));
    vdp.A.enabled = true;
    vdp.B.enabled = true;
}

}  // namespace yard
