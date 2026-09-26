#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace fairchime {
namespace {

constexpr float kTau = 6.2831853f;
constexpr int kDial = 48;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge, uint16_t shadow) {
    setPal(vdp, pal, {0, ink, edge});
    vdp.setColor(pal * 16 + 15, shadow);
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
                if (y + 1 < 8 && x + 2 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Bitmap kidArt(bool step) {
    gs::Bitmap b(20, 36);
    b.ellipse(10, 8, 4.4f, 4.6f, 1);
    b.rect(5, 2, 10, 4, 7);
    b.rect(5, 5, 3, 2, 8);
    b.rect(12, 5, 3, 2, 8);
    b.rect(7, 12, 6, 10, 2);
    b.rect(5, 13, 3, 7, 5);
    b.rect(12, 13, 3, 7, 5);
    b.rect(8, 14, 4, 3, 6);
    if (step) {
        b.rect(6, 22, 3, 10, 3);
        b.rect(11, 23, 3, 9, 3);
        b.rect(5, 31, 4, 3, 4);
        b.rect(11, 31, 4, 3, 4);
    } else {
        b.rect(7, 22, 3, 10, 3);
        b.rect(11, 22, 3, 10, 3);
        b.rect(6, 31, 4, 3, 4);
        b.rect(11, 31, 4, 3, 4);
    }
    b.set(8, 8, 4);
    b.set(12, 8, 4);
    b.outline(4, false);
    return b;
}

gs::Bitmap bottleArt() {
    gs::Bitmap b(18, 48);
    b.ellipse(9, 6, 6.4f, 2.6f, 1);
    b.rect(4, 6, 10, 3, 2);
    b.rect(6, 9, 6, 9, 1);
    b.rect(7, 9, 2, 9, 3);
    b.poly({{3, 20}, {15, 20}, {16, 26}, {2, 26}}, 1);
    b.rect(2, 25, 14, 16, 1);
    b.rect(3, 27, 4, 12, 3);
    b.ellipse(9, 41, 7.4f, 3.4f, 1);
    b.rect(2, 39, 14, 4, 2);
    b.outline(4, false);
    return b;
}

gs::Bitmap ringArt() {
    gs::Bitmap b(18, 10);
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 18; x++) {
            float dx = (x + 0.5f - 9.f) / 8.2f;
            float dy = (y + 0.5f - 5.f) / 4.1f;
            float d = dx * dx + dy * dy;
            if (d < 1.f && d > 0.38f) b.set(x, y, d > 0.72f ? 2 : 1);
        }
    }
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(18, 16);
    b.rect(8, 1, 2, 3, 3);
    b.ellipse(9, 9, 7.2f, 5.4f, 1);
    b.ellipse(7, 7, 2.2f, 1.8f, 2);
    b.rect(2, 11, 14, 3, 1);
    b.ellipse(9, 10, 1.3f, 1.6f, 3);
    return b;
}

gs::Bitmap awningArt() {
    gs::Bitmap b(72, 16);
    for (int x = 0; x < 72; x++) {
        int stripe = (x / 8) % 3;
        int c = stripe == 0 ? 1 : (stripe == 1 ? 2 : 3);
        for (int y = 0; y < 9; y++) b.set(x, y, c);
        int scallop = x % 8;
        int cut = scallop < 4 ? scallop : 7 - scallop;
        for (int y = 9; y < 9 + cut && y < 16; y++) b.set(x, y, c);
    }
    b.rect(0, 0, 72, 2, 4);
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(64, 40);
    b.rect(0, 0, 64, 40, 1);
    for (int x = 0; x < 64; x += 8)
        for (int y = 0; y < 40; y++) b.set(x, y, 2);
    b.rect(0, 0, 64, 3, 3);
    b.rect(0, 37, 64, 3, 2);
    return b;
}

gs::Bitmap counterArt() {
    gs::Bitmap b(80, 14);
    b.rect(0, 0, 80, 5, 3);
    b.rect(0, 5, 80, 9, 1);
    for (int x = 4; x < 80; x += 10) b.rect(float(x), 6, 2, 8, 2);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(28, 52);
    b.rect(1, 10, 4, 42, 1);
    b.rect(23, 10, 4, 42, 1);
    b.rect(2, 10, 2, 42, 2);
    b.rect(0, 6, 28, 6, 1);
    b.rect(2, 7, 24, 2, 3);
    b.rect(7, 16, 14, 8, 5);
    return b;
}

gs::Bitmap balloonArt() {
    gs::Bitmap b(14, 20);
    b.ellipse(7, 7, 5.6f, 6.2f, 1);
    b.ellipse(5, 5, 2.f, 2.2f, 2);
    b.poly({{7, 12}, {9, 15}, {5, 15}}, 3);
    b.line(7, 15, 8, 19, 3, 1);
    return b;
}

gs::Bitmap bulbArt() {
    gs::Bitmap b(8, 10);
    b.ellipse(4, 4, 3.1f, 3.2f, 1);
    b.ellipse(3, 3, 1.2f, 1.1f, 2);
    b.rect(3, 7, 2, 2, 3);
    return b;
}

gs::Bitmap pennantArt() {
    gs::Bitmap b(10, 14);
    b.poly({{5, 1}, {9, 12}, {1, 12}}, 1);
    b.poly({{5, 4}, {7, 10}, {3, 10}}, 2);
    b.rect(4, 0, 2, 2, 3);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6.2f, 6.2f, 1);
    b.ellipse(11, 7, 5.f, 5.f, 0);
    b.set(5, 6, 2);
    b.set(7, 11, 2);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(7, 7);
    b.line(3, 0, 3, 6, 1, 1);
    b.line(0, 3, 6, 3, 1, 1);
    b.set(3, 3, 2);
    return b;
}

gs::Bitmap burstArt() {
    gs::Bitmap b(12, 12);
    b.line(6, 0, 6, 11, 1, 1);
    b.line(0, 6, 11, 6, 1, 1);
    b.line(2, 2, 9, 9, 2, 1);
    b.line(9, 2, 2, 9, 2, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9.f, 3.f, 1);
    return b;
}

gs::Bitmap hubArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7.f, 7.f, 1);
    b.ellipse(8, 8, 3.f, 3.f, 2);
    for (int i = 0; i < 6; i++) {
        float a = i * kTau / 6.f;
        b.line(8, 8, 8 + std::cos(a) * 6.f, 8 + std::sin(a) * 6.f, 3, 1.f);
    }
    return b;
}

gs::Bitmap carArt() {
    gs::Bitmap b(10, 8);
    b.rect(1, 2, 8, 5, 1);
    b.rect(2, 1, 6, 2, 2);
    return b;
}

gs::Bitmap standArt() {
    gs::Bitmap b(12, 36);
    b.poly({{6, 1}, {11, 34}, {1, 34}}, 1);
    b.poly({{6, 8}, {9, 32}, {3, 32}}, 2);
    b.rect(2, 33, 8, 3, 3);
    return b;
}

void shaft(gs::Bitmap& b, float theta, float len, float back, float w, int col) {
    float dx = std::sin(theta), dy = -std::cos(theta);
    float px = -dy, py = dx;
    float cx = b.w * 0.5f, cy = b.h * 0.5f;
    float x0 = cx - dx * back, y0 = cy - dy * back;
    float x1 = cx + dx * len, y1 = cy + dy * len;
    b.poly({{x0 + px * w * 0.45f, y0 + py * w * 0.45f},
            {x1 + px * w, y1 + py * w},
            {x1 - px * w, y1 - py * w},
            {x0 - px * w * 0.45f, y0 - py * w * 0.45f}},
           col);
}

gs::Bitmap handArt(int kind, int step) {
    gs::Bitmap b(kDial, kDial);
    float theta = step * (kTau / 60.f);
    if (kind == 0) shaft(b, theta, 11.f, 3.f, 2.3f, 1);
    else if (kind == 1) shaft(b, theta, 16.f, 3.2f, 1.15f, 3);
    else {
        shaft(b, theta, 18.f, 4.f, 0.55f, 4);
        float dx = std::sin(theta), dy = -std::cos(theta);
        b.ellipse(24.f - dx * 3.5f, 24.f - dy * 3.5f, 1.6f, 1.6f, 5);
    }
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(kDial, kDial);
    b.ellipse(24, 24, 22.5f, 22.5f, 1);
    b.ellipse(24, 24, 19.2f, 19.2f, 2);
    b.ellipse(24, 24, 17.6f, 17.6f, 3);
    for (int i = 0; i < 12; i++) {
        float a = i * (kTau / 12.f);
        float s = std::sin(a), c = std::cos(a);
        b.line(24 + s * 13.5f, 24 - c * 13.5f, 24 + s * 16.4f, 24 - c * 16.4f, i == 0 ? 5 : 4, i == 0 ? 1.7f : 1.1f);
    }
    auto stamp = [&](const char* t, int x, int y) {
        gs::Bitmap num = gs::textBitmap(t, {1, 6, 0, 0, 0});
        b.blit(num, x - num.w / 2, y - num.h / 2);
    };
    stamp("12", 24, 8);
    stamp("3", 39, 24);
    stamp("6", 24, 40);
    stamp("9", 9, 24);
    return b;
}

gs::Bitmap haloArt() {
    gs::Bitmap b(kDial, kDial);
    for (int y = 0; y < kDial; y++) {
        for (int x = 0; x < kDial; x++) {
            float d = std::hypot(x + 0.5f - 24.f, y + 0.5f - 24.f);
            if (d > 20.f && d < 23.2f) b.set(x, y, 1);
        }
    }
    return b;
}

gs::Bitmap capArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3.2f, 3.2f, 1);
    b.ellipse(4, 4, 1.3f, 1.3f, 5);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3.2f, 3.2f, 1);
    return b;
}

gs::Bitmap chevronArt() {
    gs::Bitmap b(9, 7);
    b.poly({{4, 6}, {0, 1}, {2, 1}, {4, 4}, {6, 1}, {8, 1}}, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(3, 2, 4), gs::rgb4(2, 1, 3));
    textPal(vdp, PAL_WORD, gs::rgb4(15, 13, 4), gs::rgb4(4, 1, 1), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4), gs::rgb4(4, 0, 0), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 7), gs::rgb4(0, 3, 1), gs::rgb4(0, 2, 1));
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 11), gs::rgb4(10, 7, 2), gs::rgb4(5, 3, 1),
                            gs::rgb4(8, 4, 1)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 12), gs::rgb4(12, 10, 8), gs::rgb4(15, 15, 14), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_KID, {0, gs::rgb4(15, 12, 9), gs::rgb4(13, 3, 4), gs::rgb4(2, 3, 8), gs::rgb4(3, 2, 1),
                          gs::rgb4(12, 8, 6), gs::rgb4(15, 15, 14), gs::rgb4(14, 10, 3), gs::rgb4(8, 2, 3)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(12, 8, 4), gs::rgb4(7, 4, 2), gs::rgb4(15, 12, 5), gs::rgb4(4, 2, 1),
                           gs::rgb4(13, 2, 3)});
    setPal(vdp, PAL_AWN, {0, gs::rgb4(13, 2, 3), gs::rgb4(15, 12, 4), gs::rgb4(15, 14, 11), gs::rgb4(5, 1, 1)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 3, 4), gs::rgb4(15, 12, 12), gs::rgb4(6, 1, 2)});
    setPal(vdp, PAL_BLUE, {0, gs::rgb4(4, 7, 14), gs::rgb4(13, 14, 15), gs::rgb4(1, 2, 6)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(12, 9, 3), gs::rgb4(6, 4, 1), gs::rgb4(14, 13, 10), gs::rgb4(2, 2, 2),
                            gs::rgb4(15, 12, 4), gs::rgb4(2, 1, 2), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 4, 2), gs::rgb4(1, 1, 2), gs::rgb4(14, 3, 3),
                           gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_PINK, {0, gs::rgb4(14, 6, 9), gs::rgb4(15, 13, 14), gs::rgb4(7, 2, 4)});
    setPal(vdp, PAL_BULB, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 13, 5), gs::rgb4(6, 5, 2)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(3, 3, 6), gs::rgb4(8, 8, 11), gs::rgb4(12, 10, 4)});
    vdp.setFogColor(gs::rgb4(2, 1, 3));

    loadFont(vdp, art);
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 FAIRCHIME", {2, 1, 2, 0, 1}));
    art.twelve = gs::uploadImage(vdp, gs::textBitmap("12", {1, 1, 2, 0, 0}));
    art.face = gs::uploadImage(vdp, faceArt());
    art.halo = gs::uploadImage(vdp, haloArt());
    art.cap = gs::uploadImage(vdp, capArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.chevron = gs::uploadImage(vdp, chevronArt());
    for (int k = 0; k < 3; k++)
        for (int s = 0; s < 60; s++) art.hand[k][s] = gs::uploadImage(vdp, handArt(k, s));

    art.kid[0] = gs::uploadMipped(vdp, kidArt(false));
    art.kid[1] = gs::uploadMipped(vdp, kidArt(true));
    art.bottle = gs::uploadMipped(vdp, bottleArt());
    art.ring = gs::uploadMipped(vdp, ringArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.awning = gs::uploadMipped(vdp, awningArt());
    art.board = gs::uploadMipped(vdp, boardArt());
    art.counter = gs::uploadMipped(vdp, counterArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.balloon = gs::uploadMipped(vdp, balloonArt());
    art.bulb = gs::uploadMipped(vdp, bulbArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.burst = gs::uploadMipped(vdp, burstArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.hub = gs::uploadMipped(vdp, hubArt());
    art.car = gs::uploadMipped(vdp, carArt());
    art.stand = gs::uploadMipped(vdp, standArt());
}

}  // namespace fairchime
