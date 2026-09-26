#include "game/art.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace gliderturn {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 1, 1));
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

Ship finish(gs::VDP& vdp, Bitmap& b, float ax, float ay) {
    int x0 = b.w, y0 = b.h, x1 = -1, y1 = -1;
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++)
            if (b.get(x, y)) {
                x0 = std::min(x0, x);
                y0 = std::min(y0, y);
                x1 = std::max(x1, x);
                y1 = std::max(y1, y);
            }
    if (x1 < x0) {
        x0 = y0 = 0;
        x1 = y1 = 0;
    }
    Bitmap cropped(std::max(1, x1 - x0 + 1), std::max(1, y1 - y0 + 1));
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) cropped.set(x - x0, y - y0, b.get(x, y));
    Ship s;
    s.img = gs::uploadMipped(vdp, cropped);
    s.ax = ax - float(x0);
    s.ay = ay - float(y0);
    s.ppm = 14.f;
    return s;
}

// bank -1..1 rolls the wing in the picture plane. The fuselage stays along the flight.
Ship drawShip(gs::VDP& vdp, float bank) {
    Bitmap b(260, 220);
    const float cx = 132.f, cy = 108.f;
    const float span = 8.f + std::fabs(bank) * 58.f;
    const float lean = bank * 26.f;
    b.poly({{cx - 20.f - lean, cy - span},
            {cx + 16.f - lean, cy - span},
            {cx + 14.f + lean, cy + span},
            {cx - 22.f + lean, cy + span}},
           2);
    b.poly({{cx - 12.f - lean * 0.65f, cy - span * 0.62f},
            {cx + 8.f - lean * 0.65f, cy - span * 0.62f},
            {cx + 7.f + lean * 0.65f, cy + span * 0.62f},
            {cx - 13.f + lean * 0.65f, cy + span * 0.62f}},
           3);
    if (std::fabs(bank) > 0.22f) {
        float tx = cx + (bank >= 0.f ? lean : -lean);
        float ty = cy + span;
        b.ellipse(tx, ty, 6.5f, 5.5f, bank >= 0.f ? 7 : 8);
    }

    b.poly({{cx - 96.f, cy - 7.f},
            {cx + 46.f, cy - 9.f},
            {cx + 78.f, cy - 1.f},
            {cx + 46.f, cy + 8.f},
            {cx - 96.f, cy + 7.f}},
           1);
    b.poly({{cx + 44.f, cy - 8.f}, {cx + 84.f, cy}, {cx + 44.f, cy + 8.f}}, 5);
    b.poly({{cx - 108.f, cy - 3.f}, {cx - 78.f, cy - 5.f}, {cx - 70.f, cy - 32.f}, {cx - 92.f, cy - 30.f}}, 1);
    b.poly({{cx - 112.f, cy - 34.f}, {cx - 58.f, cy - 32.f}, {cx - 60.f, cy - 24.f}, {cx - 110.f, cy - 26.f}}, 2);
    b.ellipse(cx + 28.f, cy - 12.f, 18.f, 10.f, 4);
    b.ellipse(cx + 30.f, cy - 14.f, 8.f, 4.f, 3);
    b.ellipse(cx + 34.f, cy - 12.f, 3.2f, 3.2f, 9);
    b.line(cx + 4.f, cy + 7.f, cx + 4.f, cy + 16.f, 6, 2.2f);
    b.ellipse(cx + 4.f, cy + 18.f, 5.2f, 5.2f, 6);
    b.ellipse(cx + 4.f, cy + 18.f, 2.1f, 2.1f, 3);
    b.line(cx - 48.f, cy + 7.f, cx - 28.f, cy + 13.f, 6, 2.f);
    b.outline(6, false);
    return finish(vdp, b, cx, cy);
}

Bitmap poleArt() {
    Bitmap b(20, 100);
    b.rect(8, 0, 4, 100, 1);
    for (int y = 0; y < 96; y += 14) b.rect(8, y, 4, 7, 2);
    b.rect(3, 90, 14, 10, 3);
    b.rect(1, 96, 18, 4, 5);
    return b;
}

Bitmap flagArt() {
    Bitmap b(40, 26);
    b.rect(2, 4, 3, 20, 3);
    b.poly({{6, 6}, {36, 13}, {6, 20}}, 2);
    b.poly({{8, 9}, {28, 13}, {8, 17}}, 4);
    return b;
}

Bitmap armArt() {
    Bitmap b(36, 10);
    b.rect(0, 3, 36, 4, 2);
    b.rect(0, 1, 8, 8, 1);
    b.rect(30, 2, 6, 6, 4);
    return b;
}

Bitmap bannerArt() {
    gs::TextStyle st{3, 2, 0, 0, 1};
    Bitmap word = gs::textBitmap("END", st);
    Bitmap b(word.w + 20, word.h + 16);
    b.rect(0, 0, float(b.w), float(b.h), 1);
    b.rect(3, 3, float(b.w - 6), float(b.h - 6), 3);
    b.blit(word, 10, 8);
    for (int x = 4; x < b.w - 4; x += 6) b.rect(float(x), float(b.h - 5), 3, 3, 5);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 88);
    b.rect(5, 0, 6, 88, 4);
    b.rect(7, 0, 2, 88, 2);
    b.rect(3, 80, 10, 8, 5);
    return b;
}

Bitmap checkArt() {
    Bitmap b(32, 16);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            int on = ((x / 8) ^ (y / 8)) & 1;
            b.set(x, y, on ? 1 : 2);
        }
    return b;
}

Bitmap stripeArt() {
    Bitmap b(32, 10);
    b.rect(0, 2, 32, 6, 1);
    for (int x = 0; x < 32; x += 8) b.rect(float(x), 2, 3, 6, 0);
    return b;
}

Bitmap grassArt() {
    Bitmap b(40, 28);
    for (int y = 0; y < b.h; y++)
        for (int x = 0; x < b.w; x++) {
            uint32_t h = hash2(x + 3, y + 11);
            int c = 1 + int(h % 3);
            if ((h % 23) == 0) c = 4;
            if ((h % 41) == 0) c = 5;
            b.set(x, y, c);
        }
    return b;
}

Bitmap pineArt() {
    Bitmap b(40, 58);
    b.poly({{20, 2}, {36, 24}, {4, 24}}, 1);
    b.poly({{20, 14}, {34, 36}, {6, 36}}, 2);
    b.poly({{20, 26}, {31, 46}, {9, 46}}, 1);
    b.rect(17, 44, 6, 14, 3);
    b.rect(18, 46, 2, 10, 4);
    return b;
}

Bitmap rockArt() {
    Bitmap b(44, 36);
    b.poly({{4, 32}, {10, 14}, {22, 6}, {34, 16}, {40, 32}}, 1);
    b.poly({{14, 28}, {20, 12}, {30, 18}, {26, 30}}, 3);
    for (int i = 0; i < 10; i++) b.set(6 + int(hash2(i, 2) % 32), 18 + int(hash2(i, 8) % 12), 2);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(80, 30);
    b.ellipse(18, 18, 16, 9, 1);
    b.ellipse(40, 13, 20, 11, 1);
    b.ellipse(60, 18, 16, 8, 1);
    b.ellipse(38, 18, 14, 6, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(40, 40);
    for (int i = 0; i < 10; i++) {
        float a = float(i) * 0.628f;
        b.line(20 + std::cos(a) * 12, 20 + std::sin(a) * 12, 20 + std::cos(a) * 18, 20 + std::sin(a) * 18, 1, 2.f);
    }
    b.ellipse(20, 20, 10, 10, 1);
    b.ellipse(20, 20, 6, 6, 2);
    return b;
}

Bitmap ridgeArt() {
    Bitmap b(140, 52);
    b.poly({{0, 50}, {18, 30}, {36, 36}, {58, 10}, {78, 24}, {100, 8}, {122, 28}, {140, 50}}, 1);
    b.poly({{52, 16}, {64, 10}, {76, 22}, {58, 24}}, 3);
    b.rect(0, 44, 140, 8, 2);
    return b;
}

Bitmap gullArt(int frame) {
    Bitmap b(34, 16);
    float dip = frame ? 5.f : 0.f;
    b.poly({{1, 9}, {15, 6 + dip}, {17, 8}, {15, 10}}, 1);
    b.poly({{33, 9}, {19, 6 + dip}, {17, 8}, {19, 10}}, 1);
    b.ellipse(17, 8, 2.2f, 2.2f, 2);
    return b;
}

Bitmap dustArt() {
    Bitmap b(26, 14);
    b.ellipse(13, 8, 11, 4, 1);
    b.ellipse(8, 7, 4, 3, 2);
    return b;
}

Bitmap shadeArt() {
    Bitmap b(48, 12);
    b.ellipse(24, 6, 20, 4, 1);
    return b;
}

Bitmap bothyArt() {
    Bitmap b(52, 40);
    b.rect(8, 18, 36, 18, 1);
    b.poly({{4, 20}, {26, 4}, {48, 20}}, 2);
    b.rect(22, 24, 8, 12, 3);
    b.rect(12, 22, 7, 6, 4);
    b.rect(33, 22, 7, 6, 4);
    b.rect(6, 34, 40, 4, 5);
    return b;
}

Bitmap sockArt(int frame) {
    Bitmap b(46, 40);
    b.rect(6, 6, 3, 32, 3);
    b.ellipse(7, 5, 3.5f, 3.5f, 4);
    float droop = 3.f + float(frame) * 4.5f;
    b.poly({{9, 10}, {42, 8 + droop}, {40, 16 + droop}, {9, 18}}, 2);
    b.poly({{12, 12}, {34, 11 + droop}, {32, 15 + droop}, {12, 16}}, 1);
    return b;
}

Bitmap chevArt() {
    Bitmap b(22, 16);
    b.poly({{2, 2}, {18, 8}, {2, 14}, {7, 8}}, 1);
    return b;
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
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14));
    vdp.setColor(PAL_HUD * 16 + 2, gs::rgb4(8, 8, 9));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 11, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 8));

    setPal(vdp, PAL_SHIP, {0, gs::rgb4(13, 10, 3), gs::rgb4(3, 3, 4), gs::rgb4(15, 15, 14), gs::rgb4(5, 11, 13),
                           gs::rgb4(12, 4, 2), gs::rgb4(1, 1, 1), gs::rgb4(15, 3, 2), gs::rgb4(3, 6, 13),
                           gs::rgb4(12, 8, 5)});
    setPal(vdp, PAL_PYLON, {0, gs::rgb4(15, 15, 14), gs::rgb4(14, 7, 2), gs::rgb4(2, 2, 2), gs::rgb4(15, 12, 4),
                            gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(3, 8, 2), gs::rgb4(2, 6, 2), gs::rgb4(5, 10, 3), gs::rgb4(12, 11, 5),
                            gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(1, 5, 2), gs::rgb4(2, 8, 3), gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(8, 7, 6), gs::rgb4(5, 4, 4), gs::rgb4(12, 11, 9), gs::rgb4(6, 7, 5)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(15, 12, 5), gs::rgb4(15, 14, 8),
                          gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_END, {0, gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(8, 1, 1), gs::rgb4(2, 2, 2),
                          gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_FAR, {0, gs::rgb4(7, 6, 9), gs::rgb4(5, 4, 7), gs::rgb4(12, 11, 12)});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(12, 10, 7), gs::rgb4(9, 3, 2), gs::rgb4(4, 3, 2), gs::rgb4(8, 13, 14),
                            gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(10, 8, 6), gs::rgb4(13, 11, 8)});
    setPal(vdp, PAL_FIELD, {0, gs::rgb4(6, 11, 4), gs::rgb4(4, 9, 3), gs::rgb4(9, 13, 5), gs::rgb4(14, 13, 7),
                            gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_CHEV, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 14), gs::rgb4(3, 3, 2)});

    loadFont(vdp, art);
    const float banks[7] = {-0.92f, -0.62f, -0.32f, 0.f, 0.32f, 0.62f, 0.92f};
    for (int i = 0; i < 7; i++) art.ship[i] = drawShip(vdp, banks[i]);
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.arm = gs::uploadMipped(vdp, armArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.check = gs::uploadMipped(vdp, checkArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.grass = gs::uploadMipped(vdp, grassArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.ridge = gs::uploadMipped(vdp, ridgeArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(0));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(1));
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shade = gs::uploadMipped(vdp, shadeArt());
    art.bothy = gs::uploadMipped(vdp, bothyArt());
    for (int i = 0; i < 3; i++) art.sock[i] = gs::uploadMipped(vdp, sockArt(i));
    art.chev = gs::uploadMipped(vdp, chevArt());
}

}  // namespace gliderturn
