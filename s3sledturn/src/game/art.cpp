#include "art.h"

#include <algorithm>
#include <initializer_list>
#include <vector>

namespace sledturn {
namespace {

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t shade) {
    const uint16_t edge = gs::rgb4(1, 1, 2);
    setPal(vdp, pal, {0, ink, shade, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, edge});
}

void stampDigit(Bitmap& b, int d, int x, int y, int c) {
    static const uint8_t rows[10][7] = {
        {0b11111, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11111},
        {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110},
        {0b11110, 0b00001, 0b00001, 0b11110, 0b10000, 0b10000, 0b11111},
        {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110},
        {0b10001, 0b10001, 0b10001, 0b11111, 0b00001, 0b00001, 0b00001},
        {0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110},
        {0b01111, 0b10000, 0b10000, 0b11111, 0b10001, 0b10001, 0b11111},
        {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000},
        {0b11111, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b11111},
        {0b11111, 0b10001, 0b10001, 0b11111, 0b00001, 0b00001, 0b11110},
    };
    d = std::clamp(d, 0, 9);
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (rows[d][row] & (1 << (4 - col))) b.rect(float(x + col * 2), float(y + row * 2), 2, 2, c);
        }
    }
}

// Rear view of the freight sled. pose 0 leans left, 3 sits flat, 6 leans right.
Bitmap paintSled(int pose) {
    Bitmap b(108, 132);
    const float roll = (pose - 3) * 9.4f;
    auto X = [&](float x, float y) { return x + roll * (1.f - y / 132.f); };
    auto line = [&](float x0, float y0, float x1, float y1, int c, float th) {
        b.line(X(x0, y0), y0, X(x1, y1), y1, c, th);
    };
    auto ell = [&](float x, float y, float rx, float ry, int c) { b.ellipse(X(x, y), y, rx, ry, c); };
    auto poly = [&](std::initializer_list<Pt> pts, int c) {
        std::vector<Pt> w;
        w.reserve(pts.size());
        for (const Pt& p : pts) w.push_back({X(p.first, p.second), p.second});
        b.poly(w, c);
    };

    const float liftL = std::max(0, pose - 3) * 7.f;
    const float liftR = std::max(0, 3 - pose) * 7.f;
    const float hang = (pose - 3) * 3.6f;
    const float slide = (pose - 3) * 2.6f;

    line(34, 88 - liftL, 22, 124 - liftL, 5, 3.4f);
    line(74, 88 - liftR, 86, 124 - liftR, 5, 3.4f);
    line(20, 122 - liftL, 32, 128 - liftL, 13, 2.4f);
    line(76, 122 - liftR, 90, 128 - liftR, 13, 2.4f);
    line(36, 98, 72, 98, 5, 2.6f);

    poly({{30, 58}, {78, 58}, {86, 102}, {22, 102}}, 1);
    poly({{36, 64}, {72, 64}, {78, 96}, {30, 96}}, 2);
    poly({{42 + slide, 68}, {68 + slide, 68}, {70 + slide, 90}, {40 + slide, 90}}, 7);
    line(42 + slide, 68, 70 + slide, 90, 12, 1.6f);
    line(68 + slide, 68, 40 + slide, 90, 12, 1.6f);

    line(28, 54, 80, 54, 14, 3.4f);
    ell(28, 54, 3.4f, 3.4f, 9);
    ell(80, 54, 3.4f, 3.4f, 9);
    line(36, 54, 34, 80, 5, 2.3f);
    line(72, 54, 74, 80, 5, 2.3f);

    const float bx = 54.f + hang;
    poly({{bx - 13, 28}, {bx + 13, 28}, {bx + 16, 62}, {bx - 16, 62}}, 3);
    poly({{bx - 8, 34}, {bx + 8, 34}, {bx + 9, 56}, {bx - 9, 56}}, 10);
    ell(bx, 20, 11.f, 9.f, 11);
    ell(bx, 22, 6.2f, 5.2f, 4);
    ell(bx, 15, 4.2f, 2.2f, 8);
    line(bx - 8, 40, 32, 54, 3, 4.4f);
    line(bx + 8, 40, 76, 54, 3, 4.4f);
    ell(bx - 2.4f, 22, 1.1f, 1.2f, 14);
    ell(bx + 2.6f, 22, 1.1f, 1.2f, 14);
    b.outline(15, false);
    return b;
}

Bitmap paintWreck() {
    Bitmap b(120, 72);
    b.poly({{10, 30}, {98, 16}, {104, 40}, {16, 52}}, 1);
    b.poly({{18, 32}, {90, 22}, {94, 36}, {22, 46}}, 2);
    b.poly({{34, 28}, {64, 22}, {68, 36}, {36, 42}}, 7);
    b.line(36, 26, 66, 38, 12, 1.5f);
    b.line(8, 48, 108, 34, 5, 3.2f);
    b.ellipse(86, 16, 11, 8, 11);
    b.ellipse(88, 16, 5.5f, 4.2f, 4);
    b.ellipse(104, 30, 7, 5, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintDog(int step) {
    Bitmap b(36, 48);
    const float kick = step ? 4.f : -3.f;
    b.ellipse(18, 10, 3.5f, 4.6f, 2);
    b.ellipse(18, 8, 1.7f, 2.1f, 1);
    b.poly({{12, 14}, {15, 3}, {18, 14}}, 2);
    b.poly({{18, 14}, {21, 3}, {24, 14}}, 2);
    b.ellipse(18, 20, 8.2f, 6.f, 1);
    b.ellipse(18, 30, 9.2f, 7.f, 2);
    b.ellipse(18, 28, 5.2f, 3.f, 5);
    b.rect(14, 16, 8, 3, 3);
    b.rect(10, 34, 3, 11, 2);
    b.rect(15, 34 + kick * 0.2f, 3, 11, 1);
    b.rect(20, 34 - kick * 0.25f, 3, 10, 2);
    b.rect(25, 34, 3, 11, 1);
    b.line(16, 24, 30, step ? 12.f : 18.f, 2, 2.3f);
    b.outline(15, false);
    return b;
}

Bitmap paintStake() {
    Bitmap b(16, 52);
    b.rect(7, 12, 2, 36, 1);
    b.poly({{8, 8}, {16, 14}, {8, 20}}, 2);
    b.ellipse(8, 49, 4.2f, 1.6f, 3);
    return b;
}

Bitmap paintSign(int n) {
    Bitmap b(44, 58);
    b.rect(20, 30, 4, 26, 2);
    b.rect(4, 4, 36, 30, 1);
    b.rect(7, 7, 30, 24, 4);
    stampDigit(b, n, 15, 12, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintSpruce() {
    Bitmap b(40, 76);
    b.rect(18, 58, 4, 16, 4);
    b.poly({{20, 2}, {36, 26}, {4, 26}}, 1);
    b.poly({{20, 16}, {38, 42}, {2, 42}}, 2);
    b.poly({{20, 30}, {40, 60}, {0, 60}}, 1);
    b.poly({{20, 6}, {30, 20}, {16, 18}}, 3);
    b.poly({{20, 22}, {32, 36}, {14, 34}}, 3);
    b.poly({{20, 38}, {34, 52}, {12, 50}}, 3);
    return b;
}

Bitmap paintCabin() {
    Bitmap b(64, 52);
    b.poly({{2, 22}, {32, 4}, {62, 22}}, 3);
    b.poly({{8, 16}, {32, 6}, {56, 16}}, 3);
    b.rect(8, 20, 48, 28, 1);
    b.rect(8, 20, 48, 4, 2);
    b.rect(26, 30, 12, 18, 4);
    b.rect(12, 28, 8, 7, 5);
    b.rect(44, 28, 8, 7, 5);
    b.rect(46, 8, 4, 14, 6);
    b.ellipse(48, 8, 3.4f, 2.2f, 7);
    b.outline(15, false);
    return b;
}

Bitmap paintCache() {
    Bitmap b(34, 28);
    b.poly({{2, 10}, {17, 2}, {32, 10}}, 3);
    b.rect(3, 10, 28, 15, 1);
    b.rect(3, 10, 28, 3, 2);
    b.rect(8, 16, 7, 6, 4);
    b.outline(15, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(14, 72);
    b.rect(5, 8, 4, 58, 2);
    b.rect(2, 2, 10, 8, 1);
    b.poly({{12, 3}, {14, 6}, {12, 9}}, 3);
    b.ellipse(7, 68, 5, 2.f, 4);
    return b;
}

Bitmap paintSpray() {
    Bitmap b(18, 12);
    b.ellipse(9, 6, 7.f, 3.4f, 1);
    b.ellipse(4, 5, 2.4f, 1.6f, 2);
    b.ellipse(14, 7, 2.2f, 1.5f, 2);
    return b;
}

Bitmap paintFlake() {
    Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(2, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(30, 12);
    b.ellipse(15, 6, 13, 4, 1);
    return b;
}

Bitmap paintRaven(bool up) {
    Bitmap b(26, 14);
    b.ellipse(13, 8, 2.5f, 1.8f, 1);
    const float tip = up ? 2.f : 11.f;
    b.line(13, 7, 1, tip, 1, 1.8f);
    b.line(13, 7, 25, tip, 1, 1.8f);
    b.set(16, 7, 3);
    return b;
}

Bitmap paintSun() {
    Bitmap b(22, 22);
    b.ellipse(11, 11, 8, 8, 3);
    b.ellipse(11, 11, 4.2f, 4.2f, 4);
    return b;
}

Bitmap paintCloud() {
    Bitmap b(48, 20);
    b.ellipse(16, 12, 12, 6, 1);
    b.ellipse(30, 11, 11, 6, 1);
    b.ellipse(22, 8, 8, 5, 2);
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

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14), gs::rgb4(5, 7, 9));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 3), gs::rgb4(5, 1, 1));
    textPal(vdp, PAL_WIN, gs::rgb4(8, 15, 9), gs::rgb4(1, 5, 2));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 13, 7), gs::rgb4(6, 3, 1));
    textPal(vdp, PAL_TAG, gs::rgb4(12, 15, 15), gs::rgb4(2, 5, 7));

    setPal(vdp, PAL_SLED,
           {0, gs::rgb4(13, 9, 4), gs::rgb4(7, 5, 2), gs::rgb4(12, 2, 2), gs::rgb4(14, 11, 8), gs::rgb4(10, 12, 14),
            gs::rgb4(5, 3, 2), gs::rgb4(11, 8, 4), gs::rgb4(15, 15, 15), gs::rgb4(9, 2, 2), gs::rgb4(8, 1, 1),
            gs::rgb4(4, 3, 3), gs::rgb4(14, 12, 6), gs::rgb4(13, 15, 15), gs::rgb4(2, 2, 3), ink});
    setPal(vdp, PAL_DOG,
           {0, gs::rgb4(14, 12, 9), gs::rgb4(6, 6, 8), gs::rgb4(12, 2, 2), gs::rgb4(9, 2, 2), gs::rgb4(4, 4, 6), 0, 0,
            0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_RED,
           {0, gs::rgb4(8, 6, 3), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BLUE,
           {0, gs::rgb4(8, 6, 3), gs::rgb4(3, 6, 13), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(1, 5, 2), gs::rgb4(2, 8, 3), gs::rgb4(15, 15, 15), gs::rgb4(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, ink});
    setPal(vdp, PAL_CABIN,
           {0, gs::rgb4(10, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(15, 15, 15), gs::rgb4(3, 2, 2), gs::rgb4(14, 11, 4),
            gs::rgb4(4, 4, 5), gs::rgb4(12, 12, 13), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 8), gs::rgb4(12, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           ink});
    setPal(vdp, PAL_SIGN,
           {0, gs::rgb4(12, 9, 5), gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 3), gs::rgb4(14, 12, 8), gs::rgb4(13, 2, 2), 0, 0,
            0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY,
           {0, gs::rgb4(15, 15, 15), gs::rgb4(13, 14, 15), gs::rgb4(15, 12, 6), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, ink});

    setPal(vdp, PAL_TRAIL,
           {gs::rgb4(3, 4, 6), gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 15), gs::rgb4(9, 11, 13), gs::rgb4(14, 15, 15),
            gs::rgb4(11, 13, 14), gs::rgb4(13, 15, 15), gs::rgb4(10, 13, 14), gs::rgb4(8, 10, 12), gs::rgb4(7, 10, 13),
            gs::rgb4(14, 15, 15), gs::rgb4(6, 9, 12), gs::rgb4(5, 8, 11), gs::rgb4(9, 12, 14), gs::rgb4(15, 15, 15),
            gs::rgb4(14, 15, 14)});

    loadFont(vdp, art);
    for (int i = 0; i < kPoses; i++) art.sled[i] = gs::uploadMipped(vdp, paintSled(i));
    art.wreck = gs::uploadMipped(vdp, paintWreck());
    art.dog[0] = gs::uploadMipped(vdp, paintDog(0));
    art.dog[1] = gs::uploadMipped(vdp, paintDog(1));
    art.stake = gs::uploadMipped(vdp, paintStake());
    for (int i = 0; i < 3; i++) art.sign[i] = gs::uploadMipped(vdp, paintSign(i + 1));
    art.spruce = gs::uploadMipped(vdp, paintSpruce());
    art.cabin = gs::uploadMipped(vdp, paintCabin());
    art.cache = gs::uploadMipped(vdp, paintCache());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.spray = gs::uploadMipped(vdp, paintSpray());
    art.flake = gs::uploadMipped(vdp, paintFlake());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.raven[0] = gs::uploadMipped(vdp, paintRaven(true));
    art.raven[1] = gs::uploadMipped(vdp, paintRaven(false));
    art.sun = gs::uploadMipped(vdp, paintSun());
    art.cloud = gs::uploadMipped(vdp, paintCloud());
    art.title = words(vdp, "SLED TURN", 3);
    art.upright = words(vdp, "UPRIGHT", 3);
    art.tipped = words(vdp, "TIPPED", 3);
    art.missed = words(vdp, "MISSED", 3);
    art.paused = words(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(12, 13, 15));
}

}  // namespace sledturn
