#include "art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace skiffplat {
namespace {

constexpr float kTau = 6.2831853f;

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

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    float wx = ly * c + lx * s;
    float wy = ly * s - lx * c;
    return {cx + wx, cy - wy};
}

// White skiff, transom stern, kicker shipped, red band amidships.
// Local +y is the bow, +x is starboard. Heading 0 points east.
Bitmap paintHull(float heading) {
    Bitmap b(96, 96);
    const float cx = 48.f, cy = 48.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        w.reserve(local.size());
        for (const Pt& p : local) w.push_back(spin(cx, cy, p.first, p.second, c, s));
        b.poly(w, col);
    };
    auto stroke = [&](float ax, float ay, float bx, float by, int col, float th) {
        Pt A = spin(cx, cy, ax, ay, c, s);
        Pt B = spin(cx, cy, bx, by, c, s);
        b.line(A.first, A.second, B.first, B.second, col, th);
    };
    auto blob = [&](float lx, float ly, float rx, float ry, int col) {
        Pt p = spin(cx, cy, lx, ly, c, s);
        b.ellipse(p.first, p.second, rx, ry, col);
    };
    poly({{0.f, 34.f}, {7.f, 24.f}, {14.f, 10.f}, {15.f, -2.f}, {13.f, -18.f}, {9.f, -31.f}, {-9.f, -31.f},
          {-13.f, -18.f}, {-15.f, -2.f}, {-14.f, 10.f}, {-7.f, 24.f}},
         1);
    poly({{0.f, 27.f}, {8.f, 12.f}, {9.f, -4.f}, {6.f, -22.f}, {-6.f, -22.f}, {-9.f, -4.f}, {-8.f, 12.f}}, 3);
    poly({{-14.f, 4.2f}, {14.f, 4.2f}, {14.f, -4.2f}, {-14.f, -4.2f}}, 5);
    poly({{-14.f, 2.2f}, {14.f, 2.2f}, {14.f, -2.2f}, {-14.f, -2.2f}}, 6);
    stroke(-11.f, 16.f, -8.f, -20.f, 8, 1.5f);
    stroke(11.f, 16.f, 8.f, -20.f, 8, 1.5f);
    stroke(-13.f, 8.f, -6.f, 4.f, 11, 1.6f);
    stroke(13.f, 8.f, 6.f, 4.f, 11, 1.6f);
    stroke(0.f, 30.f, 0.f, 20.f, 13, 1.3f);
    poly({{-4.f, 8.f}, {4.f, 8.f}, {3.2f, -6.f}, {-3.2f, -6.f}}, 4);
    blob(0.f, 2.f, 2.6f, 3.2f, 7);
    blob(0.f, 4.2f, 2.8f, 1.5f, 9);
    blob(-0.8f, 4.4f, 0.55f, 0.55f, 2);
    poly({{-2.4f, -24.f}, {2.4f, -24.f}, {3.2f, -33.f}, {-3.2f, -33.f}}, 10);
    blob(0.f, -31.f, 1.6f, 2.2f, 10);
    stroke(0.f, -33.f, 0.f, -36.f, 10, 1.2f);
    b.outline(15, false);
    return b;
}

Bitmap paintDeck() {
    // 110x250 matches a 22 by 50 world stage. The red band is the level mark.
    Bitmap b(110, 250);
    b.rect(0, 0, 110, 250, 2);
    for (int i = 0; i < 18; ++i) {
        int y = 4 + i * 13;
        bool slot = y > 90 && y < 156;
        int col = slot ? ((i & 1) ? 4 : 3) : ((i & 1) ? 2 : 1);
        b.rect(10, y, 90, 11, col);
        b.rect(10, y, 90, 1, 9);
    }
    b.rect(0, 109, 110, 32, 5);
    b.rect(0, 117, 110, 16, 6);
    b.rect(0, 123, 110, 6, 5);
    b.rect(0, 0, 8, 250, 7);
    b.rect(102, 0, 8, 250, 8);
    for (int y = 18; y < 236; y += 26) {
        b.ellipse(24, y, 1.6f, 1.6f, 8);
        b.ellipse(86, y, 1.6f, 1.6f, 8);
    }
    b.rect(40, 30, 22, 16, 10);
    b.rect(42, 32, 18, 12, 11);
    b.outline(15, false);
    return b;
}

Bitmap paintPile() {
    Bitmap b(22, 22);
    b.ellipse(11, 11, 9, 9, 1);
    b.ellipse(11, 11, 6.4f, 6.4f, 4);
    b.ellipse(11, 11, 4.6f, 4.6f, 2);
    b.ellipse(8, 8, 2.0f, 2.0f, 3);
    return b;
}

Bitmap paintPost() {
    Bitmap b(12, 36);
    b.rect(4, 6, 4, 26, 1);
    b.rect(3, 4, 6, 5, 2);
    b.ellipse(6, 32, 4, 2, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintShed() {
    Bitmap b(64, 48);
    b.rect(6, 16, 52, 26, 1);
    b.poly({{4, 18}, {32, 4}, {60, 18}}, 2);
    b.rect(28, 24, 12, 18, 3);
    b.rect(12, 22, 10, 8, 4);
    b.rect(44, 22, 10, 8, 4);
    b.rect(50, 6, 2, 14, 5);
    b.poly({{52, 6}, {62, 10}, {52, 14}}, 6);
    b.outline(7, false);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(18, 40);
    b.rect(7, 12, 4, 24, 1);
    b.ellipse(9, 10, 6, 5, 2);
    b.ellipse(9, 10, 3, 2.4f, 3);
    b.ellipse(9, 36, 5, 2, 4);
    return b;
}

Bitmap paintStaff() {
    Bitmap b(14, 48);
    b.rect(6, 2, 3, 42, 1);
    for (int y = 6; y < 42; y += 6) b.rect(3, y, 8, 1, 2);
    b.rect(2, 18, 10, 2, 3);
    b.ellipse(7, 44, 4, 2, 4);
    return b;
}

Bitmap paintPip() {
    Bitmap b(10, 10);
    b.poly({{5, 1}, {9, 5}, {5, 9}, {1, 5}}, 1);
    b.poly({{5, 3}, {7, 5}, {5, 7}, {3, 5}}, 2);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(16, 36);
    b.rect(7, 8, 2, 22, 1);
    b.ellipse(8, 10, 5, 4, 2);
    b.ellipse(8, 10, 2.5f, 2, 3);
    b.ellipse(8, 30, 4, 2, 4);
    return b;
}

Bitmap paintReed() {
    Bitmap b(20, 28);
    b.line(4, 26, 3, 6, 1, 1.4f);
    b.line(10, 26, 11, 3, 2, 1.6f);
    b.line(16, 26, 15, 8, 1, 1.3f);
    b.ellipse(11, 4, 1.6f, 1.6f, 3);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(16, 22);
    b.rect(3, 2, 2, 18, 1);
    b.poly({{5, 3}, {14, 7}, {5, 11}}, 2);
    return b;
}

Bitmap paintRope() {
    Bitmap b(48, 8);
    b.line(2, 4, 46, 4, 1, 2.2f);
    for (int x = 4; x < 46; x += 6) b.rect(x, 2, 2, 4, 2);
    return b;
}

Bitmap paintCoil() {
    Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 1);
    b.ellipse(9, 9, 3.5f, 3.5f, 2);
    b.ellipse(9, 9, 1.4f, 1.4f, 1);
    return b;
}

Bitmap paintHand(bool reach) {
    Bitmap b(28, 36);
    b.ellipse(14, 8, 4.2f, 4.2f, 1);
    b.rect(10, 12, 8, 12, 2);
    b.rect(11, 13, 6, 4, 3);
    if (reach) {
        b.line(10, 16, 2, 20, 1, 2.f);
        b.line(18, 16, 26, 18, 1, 2.f);
    } else {
        b.line(10, 16, 6, 24, 1, 2.f);
        b.line(18, 16, 22, 24, 1, 2.f);
    }
    b.rect(11, 24, 3, 8, 4);
    b.rect(15, 24, 3, 8, 4);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(28, 14);
    b.ellipse(14, 8, 2.2f, 1.5f, 1);
    float tip = up ? 2.f : 11.f;
    b.line(14, 7, 2, tip, 1, 1.5f);
    b.line(14, 7, 26, tip, 1, 1.5f);
    b.set(17, 7, 2);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 10);
    b.ellipse(8, 5, 6, 3, 1);
    b.ellipse(8, 5, 3, 1.5f, 2);
    return b;
}

Bitmap paintPin() {
    Bitmap b(14, 14);
    b.poly({{7, 1}, {13, 7}, {7, 13}, {1, 7}}, 1);
    b.poly({{7, 4}, {10, 7}, {7, 10}, {4, 7}}, 2);
    return b;
}

Bitmap paintTrack() {
    Bitmap b(92, 14);
    b.rect(2, 4, 88, 6, 1);
    b.rect(44, 2, 4, 10, 2);
    b.rect(8, 5, 2, 4, 3);
    b.rect(82, 5, 2, 4, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintBubble() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(4.5f, 4.5f, 1.6f, 1.6f, 2);
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
    uint8_t a[64] = {};
    uint8_t w[64] = {};
    a[2 * 8 + 2] = 1;
    a[5 * 8 + 5] = 2;
    w[1 * 8 + 6] = 2;
    w[4 * 8 + 2] = 1;
    int t0 = tiles.alloc(1);
    int t1 = tiles.alloc(1);
    vdp.loadTile(t0, a);
    vdp.loadTile(t1, w);
    for (int cy = 0; cy < vdp.B.h; cy++) {
        for (int cx = 0; cx < vdp.B.w; cx++) {
            int tile = ((cx * 3 + cy * 5) & 3) == 0 ? t1 : t0;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_WAVE));
        }
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 12, 13), gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, ink});
    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(4, 8, 3), gs::rgb4(12, 8, 4), gs::rgb4(11, 7, 4), gs::rgb4(15, 15, 15),
            gs::rgb4(13, 2, 2), gs::rgb4(3, 6, 12), gs::rgb4(2, 8, 4), gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 3),
            gs::rgb4(9, 6, 3), gs::rgb4(14, 12, 8), gs::rgb4(13, 11, 6), gs::rgb4(5, 5, 6), ink});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(13, 9, 4), gs::rgb4(9, 6, 3), gs::rgb4(14, 12, 7), gs::rgb4(11, 9, 5), gs::rgb4(15, 15, 14),
            gs::rgb4(13, 2, 2), gs::rgb4(3, 2, 2), gs::rgb4(5, 4, 3), gs::rgb4(6, 4, 2), gs::rgb4(8, 5, 3),
            gs::rgb4(4, 3, 2), 0, 0, 0, ink});
    setPal(vdp, PAL_PILE, {0, gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 5), gs::rgb4(9, 9, 8), gs::rgb4(12, 8, 3), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOLK, {0, gs::rgb4(13, 9, 6), gs::rgb4(2, 5, 10), gs::rgb4(4, 8, 14), gs::rgb4(3, 3, 4), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 4, 4), gs::rgb4(14, 12, 3), gs::rgb4(15, 15, 10), gs::rgb4(3, 3, 3), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 7, 3), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 6), gs::rgb4(1, 4, 2), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                          0, ink});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 13, 7), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(7, 7, 7), gs::rgb4(13, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(2, 6, 8), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_TAG, {0, gs::rgb4(15, 14, 8), gs::rgb4(6, 4, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    // Road palettes. Indices follow the road generator: 6/7 body, 4/5 verge, 8 speck.
    setPal(vdp, PAL_GRASS,
           {0, gs::rgb4(5, 11, 3), gs::rgb4(3, 8, 2), gs::rgb4(14, 13, 6), gs::rgb4(4, 9, 3), gs::rgb4(3, 7, 2),
            gs::rgb4(4, 10, 3), gs::rgb4(3, 8, 2), gs::rgb4(8, 8, 5), gs::rgb4(6, 8, 3), gs::rgb4(5, 7, 3),
            gs::rgb4(2, 6, 7), gs::rgb4(3, 8, 8), gs::rgb4(10, 14, 14), gs::rgb4(7, 6, 3), gs::rgb4(2, 4, 1)});
    setPal(vdp, PAL_SHOAL,
           {0, gs::rgb4(10, 9, 5), gs::rgb4(8, 7, 4), gs::rgb4(12, 11, 7), gs::rgb4(9, 8, 5), gs::rgb4(7, 6, 3),
            gs::rgb4(11, 9, 5), gs::rgb4(8, 7, 4), gs::rgb4(6, 6, 5), gs::rgb4(9, 8, 4), gs::rgb4(7, 6, 3),
            gs::rgb4(4, 7, 8), gs::rgb4(5, 8, 8), gs::rgb4(12, 13, 10), gs::rgb4(10, 8, 4), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_WAVE, {0, gs::rgb4(8, 14, 14), gs::rgb4(13, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.hull[i] = gs::uploadMipped(vdp, paintHull(i * kTau / 16.f));
    art.deck = gs::uploadMipped(vdp, paintDeck());
    art.pile = gs::uploadMipped(vdp, paintPile());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.staff = gs::uploadMipped(vdp, paintStaff());
    art.pip = gs::uploadMipped(vdp, paintPip());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.rope = gs::uploadMipped(vdp, paintRope());
    art.coil = gs::uploadMipped(vdp, paintCoil());
    art.hand[0] = gs::uploadMipped(vdp, paintHand(false));
    art.hand[1] = gs::uploadMipped(vdp, paintHand(true));
    art.gull[0] = gs::uploadMipped(vdp, paintGull(true));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(false));
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.pin = gs::uploadMipped(vdp, paintPin());
    art.track = gs::uploadMipped(vdp, paintTrack());
    art.bubble = gs::uploadMipped(vdp, paintBubble());
    art.title = words(vdp, "S3 SKIFF PLAT", 3);
    art.level = words(vdp, "LEVEL", 3);
    art.withPlat = words(vdp, "WITH THE PLATFORM", 2);
    art.missed = words(vdp, "MISSED", 3);
    art.tideOut = words(vdp, "TIDE'S OUT", 2);
    art.paused = words(vdp, "PAUSED", 3);

    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(1, 4, 7));
}

}  // namespace skiffplat
