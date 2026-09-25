#include "game/cabart.h"

#include <cmath>
#include <initializer_list>

namespace rickshaw {
namespace {

constexpr float kPi = 3.14159265f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
}

void closeHoles(Bitmap& b) {
    Bitmap src = b;
    for (int y = 1; y < b.h - 1; y++) {
        for (int x = 1; x < b.w - 1; x++) {
            if (src.get(x, y)) continue;
            int count[16] = {};
            int n = 0;
            const int nb[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            for (auto d : nb) {
                int p = src.get(x + d[0], y + d[1]);
                if (!p) continue;
                count[p]++;
                n++;
            }
            if (n < 3) continue;
            int best = 1, bc = 0;
            for (int c = 1; c < 16; c++) {
                if (count[c] > bc) {
                    bc = count[c];
                    best = c;
                }
            }
            b.set(x, y, best);
        }
    }
}

Bitmap tilt(const Bitmap& src, float deg, float pivotX, float pivotY) {
    Bitmap dst(src.w, src.h);
    float rad = deg * kPi / 180.f;
    float c = std::cos(rad), s = std::sin(rad);
    for (int y = 0; y < dst.h; y++) {
        for (int x = 0; x < dst.w; x++) {
            float dx = float(x) - pivotX;
            float dy = float(y) - pivotY;
            int sx = int(std::lround(pivotX + dx * c + dy * s));
            int sy = int(std::lround(pivotY - dx * s + dy * c));
            int p = src.get(sx, sy);
            if (p) dst.set(x, y, p);
        }
    }
    closeHoles(dst);
    dst.outline(15, false);
    return dst;
}

Bitmap paintCab() {
    Bitmap b(148, 124);
    b.ellipse(52, 100, 18, 18, 1);
    b.ellipse(96, 100, 18, 18, 1);
    b.ellipse(52, 100, 10, 10, 9);
    b.ellipse(96, 100, 10, 10, 9);
    b.line(52, 100, 34, 84, 12, 1.4f);
    b.line(52, 100, 70, 84, 12, 1.4f);
    b.line(34, 116, 70, 84, 12, 1.2f);
    b.line(70, 116, 34, 84, 12, 1.2f);
    b.line(96, 100, 78, 84, 12, 1.4f);
    b.line(96, 100, 114, 84, 12, 1.4f);
    b.line(78, 116, 114, 84, 12, 1.2f);
    b.line(114, 116, 78, 84, 12, 1.2f);
    b.ellipse(52, 100, 4, 4, 11);
    b.ellipse(96, 100, 4, 4, 11);
    b.line(40, 102, 108, 102, 2, 3.f);
    b.rect(46, 96, 8, 10, 14);
    b.rect(94, 96, 8, 10, 14);

    b.poly({{40, 72}, {28, 48}, {74, 22}, {120, 48}, {108, 72}}, 3);
    b.poly({{48, 68}, {38, 50}, {74, 30}, {110, 50}, {100, 68}}, 4);
    b.line(34, 100, 36, 58, 11, 2.4f);
    b.line(114, 100, 112, 58, 11, 2.4f);
    b.line(36, 58, 74, 26, 11, 2.f);
    b.line(112, 58, 74, 26, 11, 2.f);
    for (int i = 0; i < 7; i++) {
        float x = 40.f + i * 11.f;
        b.ellipse(x, 70, 5.5f, 4.2f, i & 1 ? 4 : 3);
    }

    b.ellipse(74, 52, 6, 6, 6);
    b.ellipse(74, 46, 5, 4, 7);
    b.rect(68, 54, 12, 10, 13);
    b.line(66, 58, 54, 64, 13, 2.f);
    b.line(82, 58, 96, 64, 13, 2.f);
    b.ellipse(98, 50, 3.2f, 3.2f, 11);
    b.ellipse(98, 50, 1.4f, 1.4f, 12);

    b.rect(50, 74, 48, 26, 5);
    b.rect(54, 78, 40, 8, 8);
    b.ellipse(74, 66, 11, 10, 6);
    b.ellipse(74, 58, 8, 8, 7);
    b.ellipse(70, 56, 3, 3, 7);
    b.set(71, 64, 12);
    b.set(77, 64, 12);
    b.rect(62, 88, 24, 8, 10);
    gs::TextStyle plate{1, 2, 0, 0, 0};
    Bitmap name = gs::textBitmap("NINA", plate);
    b.blit(name, 74 - name.w / 2, 89);
    b.rect(44, 78, 8, 18, 4);
    b.rect(96, 78, 8, 18, 4);
    b.outline(15, false);
    return b;
}

Bitmap paintSpilled() {
    Bitmap b(148, 100);
    b.ellipse(40, 70, 16, 16, 1);
    b.ellipse(40, 70, 8, 8, 9);
    b.ellipse(40, 70, 3, 3, 11);
    b.ellipse(78, 78, 14, 14, 1);
    b.ellipse(78, 78, 7, 7, 9);
    b.poly({{48, 74}, {96, 40}, {118, 48}, {108, 70}, {70, 84}}, 3);
    b.poly({{58, 70}, {96, 46}, {108, 52}, {92, 68}}, 4);
    b.ellipse(112, 58, 8, 8, 6);
    b.ellipse(116, 52, 6, 5, 7);
    b.rect(100, 60, 16, 12, 8);
    b.ellipse(30, 48, 7, 7, 6);
    b.ellipse(30, 42, 5, 4, 7);
    b.rect(24, 50, 12, 14, 13);
    b.line(36, 58, 52, 66, 2, 2.f);
    b.outline(15, false);
    return b;
}

Bitmap paintShop() {
    Bitmap b(96, 80);
    b.rect(8, 22, 80, 54, 1);
    b.rect(8, 22, 80, 10, 2);
    b.poly({{4, 24}, {48, 6}, {92, 24}}, 3);
    b.rect(14, 36, 22, 18, 8);
    b.rect(18, 40, 14, 10, 9);
    b.rect(44, 36, 18, 28, 7);
    b.rect(68, 40, 12, 12, 8);
    b.rect(10, 58, 76, 10, 5);
    b.rect(16, 60, 10, 6, 11);
    b.rect(32, 60, 10, 6, 12);
    b.rect(48, 60, 10, 6, 6);
    b.rect(64, 60, 10, 6, 14);
    b.rect(40, 18, 16, 8, 10);
    b.outline(15, false);
    return b;
}

Bitmap paintHouse() {
    Bitmap b(72, 96);
    b.rect(10, 20, 52, 72, 1);
    b.rect(10, 20, 52, 8, 2);
    b.rect(8, 16, 56, 8, 3);
    b.rect(18, 32, 14, 16, 8);
    b.rect(40, 32, 14, 16, 8);
    b.rect(20, 34, 10, 8, 9);
    b.rect(42, 34, 10, 8, 9);
    b.rect(28, 58, 16, 28, 7);
    b.rect(6, 48, 14, 6, 6);
    b.ellipse(62, 28, 4, 4, 11);
    b.rect(14, 8, 6, 12, 10);
    b.outline(15, false);
    return b;
}

Bitmap paintTemple() {
    Bitmap b(88, 100);
    b.rect(16, 46, 56, 50, 1);
    b.rect(22, 58, 16, 18, 6);
    b.rect(50, 58, 14, 14, 8);
    b.poly({{10, 48}, {44, 16}, {78, 48}}, 3);
    b.poly({{22, 40}, {44, 22}, {66, 40}}, 4);
    b.rect(40, 8, 8, 14, 5);
    b.ellipse(44, 8, 5, 5, 13);
    b.rect(6, 70, 10, 22, 7);
    b.rect(72, 70, 10, 22, 7);
    b.poly({{8, 36}, {14, 36}, {14, 48}, {8, 52}}, 14);
    b.ellipse(30, 78, 3, 3, 8);
    b.ellipse(58, 80, 3, 3, 8);
    b.outline(15, false);
    return b;
}

Bitmap paintShed() {
    Bitmap b(100, 64);
    b.rect(6, 24, 88, 36, 3);
    b.rect(6, 24, 88, 8, 4);
    b.poly({{4, 26}, {50, 8}, {96, 26}}, 2);
    b.rect(14, 36, 28, 16, 9);
    b.rect(52, 38, 30, 14, 7);
    b.line(20, 40, 36, 48, 10, 1.4f);
    b.rect(8, 52, 84, 6, 10);
    b.outline(15, false);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(28, 72);
    b.rect(12, 20, 4, 48, 10);
    b.ellipse(14, 16, 8, 8, 14);
    b.ellipse(14, 16, 4, 4, 9);
    b.rect(6, 64, 16, 4, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintStall() {
    Bitmap b(80, 56);
    b.rect(8, 28, 64, 8, 10);
    b.rect(12, 36, 4, 16, 10);
    b.rect(64, 36, 4, 16, 10);
    b.poly({{10, 28}, {40, 10}, {70, 28}}, 5);
    b.ellipse(22, 24, 5, 5, 12);
    b.ellipse(36, 22, 5, 5, 6);
    b.ellipse(50, 24, 5, 5, 14);
    b.ellipse(62, 22, 4, 4, 11);
    b.outline(15, false);
    return b;
}

Bitmap paintSign(bool left) {
    Bitmap b(72, 64);
    b.rect(32, 28, 6, 32, 4);
    b.rect(8, 8, 56, 26, 1);
    b.rect(8, 8, 56, 4, 2);
    float x0 = left ? 50.f : 18.f;
    float x1 = left ? 18.f : 50.f;
    b.poly({{x0, 14}, {x0, 28}, {x1, 21}}, 6);
    b.outline(15, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(36, 88);
    b.rect(14, 18, 8, 64, 4);
    b.rect(4, 8, 28, 18, 5);
    b.rect(6, 10, 24, 4, 7);
    gs::TextStyle st{1, 3, 0, 0, 0};
    Bitmap label = gs::textBitmap("STAND", st);
    if (label.w < 34) b.blit(label, 18 - label.w / 2, 14);
    b.rect(8, 78, 20, 6, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintCow() {
    Bitmap b(56, 36);
    b.ellipse(28, 20, 16, 10, 3);
    b.ellipse(44, 16, 7, 6, 3);
    b.ellipse(14, 14, 5, 4, 3);
    b.rect(18, 24, 3, 8, 4);
    b.rect(28, 24, 3, 8, 4);
    b.rect(36, 24, 3, 8, 4);
    b.set(46, 15, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintDog() {
    Bitmap b(40, 28);
    b.ellipse(18, 16, 10, 6, 5);
    b.ellipse(30, 14, 5, 4, 5);
    b.ellipse(10, 12, 3, 4, 5);
    b.rect(12, 18, 2, 6, 2);
    b.rect(22, 18, 2, 6, 2);
    b.line(8, 14, 2, 8, 5, 1.4f);
    b.set(32, 13, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintBird(bool up) {
    Bitmap b(28, 16);
    b.ellipse(14, 9, 3, 2, 1);
    float tip = up ? 3.f : 12.f;
    b.line(14, 8, 2, tip, 1, 1.5f);
    b.line(14, 8, 26, tip, 1, 1.5f);
    b.set(17, 8, 6);
    return b;
}

Bitmap paintFlower() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 3, 3, 9);
    b.ellipse(8, 4, 2, 2, 11);
    b.ellipse(4, 8, 2, 2, 11);
    b.ellipse(12, 8, 2, 2, 11);
    b.ellipse(8, 12, 2, 2, 10);
    b.ellipse(8, 8, 1.3f, 1.3f, 14);
    return b;
}

Bitmap paintSun() {
    Bitmap b(40, 40);
    b.ellipse(20, 20, 12, 12, 7);
    b.ellipse(20, 20, 7, 7, 8);
    b.line(20, 4, 20, 10, 7, 2.f);
    b.line(20, 30, 20, 36, 7, 2.f);
    b.line(4, 20, 10, 20, 7, 2.f);
    b.line(30, 20, 36, 20, 7, 2.f);
    return b;
}

Bitmap paintDust() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 6, 3, 3);
    b.ellipse(8, 6, 3, 1.6f, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art, gs::TileAlloc& tiles) {
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(2, 1, 1);
    auto C = gs::rgb4;
    setPal(vdp, PAL_HUD, {0, C(15, 14, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, C(15, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, C(8, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BANNER, {0, C(15, 10, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_CAB,
           {0, C(2, 2, 3), C(4, 4, 6), C(15, 9, 1), C(12, 5, 1), C(12, 2, 3), C(13, 8, 5), C(2, 1, 1), C(15, 14, 12),
            C(1, 1, 2), C(15, 13, 8), C(13, 10, 3), C(15, 15, 14), C(2, 8, 4), C(8, 5, 2), ink});
    setPal(vdp, PAL_CITY,
           {0, C(14, 12, 9), C(9, 7, 5), C(12, 5, 3), C(7, 2, 2), C(14, 8, 1), C(2, 8, 5), C(6, 3, 2), C(4, 6, 8),
            C(13, 13, 11), C(8, 5, 2), C(3, 9, 3), C(12, 2, 2), C(15, 14, 12), C(15, 12, 5), ink});
    setPal(vdp, PAL_TEMPLE,
           {0, C(14, 13, 11), C(9, 8, 7), C(14, 7, 1), C(10, 4, 1), C(13, 10, 3), C(4, 2, 2), C(8, 7, 6), C(15, 8, 2),
            C(2, 7, 3), C(12, 2, 3), 0, C(15, 14, 10), C(12, 9, 3), C(15, 3, 2), ink});
    setPal(vdp, PAL_SIGN, {0, C(15, 13, 8), C(9, 6, 2), C(3, 1, 1), C(7, 4, 2), C(14, 8, 1), C(12, 2, 2), C(15, 15, 13),
                           C(2, 8, 4), C(13, 10, 3), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LIFE, {0, C(15, 15, 14), C(2, 1, 1), C(8, 5, 2), C(4, 2, 1), C(11, 7, 3), C(14, 8, 2), C(15, 11, 3),
                           C(15, 15, 10), C(14, 6, 2), C(3, 8, 3), C(14, 7, 8), C(15, 14, 12), 0, C(13, 10, 3), ink});
    setPal(vdp, PAL_STREET,
           {0, C(12, 10, 6), C(9, 7, 4), C(7, 6, 4), C(8, 7, 5), C(5, 4, 3), C(6, 5, 4), C(3, 3, 3), C(7, 6, 5),
            C(4, 4, 4), C(8, 7, 6), C(4, 5, 6), C(5, 6, 7), C(9, 8, 7), C(13, 10, 3), C(8, 7, 6)});
    vdp.setFogColor(C(14, 9, 5));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    const float pivX = 74.f, pivY = 108.f;
    art.pivX = pivX;
    art.pivY = pivY;
    Bitmap cab = paintCab();
    for (int i = 0; i < 9; i++) {
        float deg = -(i - 4) * 8.f;
        art.cab[i] = gs::uploadMipped(vdp, tilt(cab, deg, pivX, pivY));
    }
    art.spilled = gs::uploadMipped(vdp, paintSpilled());
    art.shop = gs::uploadMipped(vdp, paintShop());
    art.house = gs::uploadMipped(vdp, paintHouse());
    art.temple = gs::uploadMipped(vdp, paintTemple());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.stall = gs::uploadMipped(vdp, paintStall());
    art.signL = gs::uploadMipped(vdp, paintSign(true));
    art.signR = gs::uploadMipped(vdp, paintSign(false));
    art.post = gs::uploadMipped(vdp, paintPost());
    art.cow = gs::uploadMipped(vdp, paintCow());
    art.dog = gs::uploadMipped(vdp, paintDog());
    art.bird[0] = gs::uploadMipped(vdp, paintBird(false));
    art.bird[1] = gs::uploadMipped(vdp, paintBird(true));
    art.flower = gs::uploadMipped(vdp, paintFlower());
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.dust = gs::uploadMipped(vdp, paintDust());
    art.title = words(vdp, "RICKSHAW", 4);
    art.sub = words(vdp, "ONE FARE", 2);
    art.paid = words(vdp, "FARE PAID", 3);
    art.tipped = words(vdp, "TIPPED", 3);
}

}  // namespace rickshaw
