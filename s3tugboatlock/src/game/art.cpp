#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace tuglock {
namespace {

constexpr float kPi = 3.14159265f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap rotateCW(const Bitmap& src, float a) {
    const float ca = std::cos(a), sa = std::sin(a);
    const float c = std::fabs(ca), s = std::fabs(sa);
    const int nw = int(std::ceil(src.w * c + src.h * s)) + 4;
    const int nh = int(std::ceil(src.w * s + src.h * c)) + 4;
    Bitmap o(nw, nh);
    const float cx = src.w * 0.5f, cy = src.h * 0.5f;
    const float ocx = nw * 0.5f, ocy = nh * 0.5f;
    auto at = [&](float x, float y) { return src.get(int(std::lround(x)), int(std::lround(y))); };
    for (int y = 0; y < nh; y++) {
        for (int x = 0; x < nw; x++) {
            float dx = x + 0.5f - ocx, dy = y + 0.5f - ocy;
            float sx = cx + dx * ca + dy * sa;
            float sy = cy - dx * sa + dy * ca;
            int p = at(sx, sy);
            if (!p) p = at(sx + 0.45f, sy);
            if (!p) p = at(sx - 0.45f, sy);
            if (!p) p = at(sx, sy + 0.45f);
            if (!p) p = at(sx, sy - 0.45f);
            if (p) o.set(x, y, p);
        }
    }
    return o;
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + lx * c + ly * s, cy - (-lx * s + ly * c)};
}

// Nose is +ly. Heading 0 points up the bitmap.
Bitmap paintTug(float heading) {
    Bitmap b(112, 112);
    const float cx = 56.f, cy = 56.f;
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

    poly({{0.f, 46.f}, {13.f, 30.f}, {15.f, 6.f}, {15.f, -30.f}, {8.f, -40.f}, {-8.f, -40.f}, {-15.f, -30.f}, {-15.f, 6.f},
          {-13.f, 30.f}},
         1);
    poly({{0.f, 38.f}, {10.f, 26.f}, {11.f, 4.f}, {11.f, -28.f}, {5.f, -36.f}, {-5.f, -36.f}, {-11.f, -28.f}, {-11.f, 4.f},
          {-10.f, 26.f}},
         2);
    stroke(-14.2f, 24.f, -13.4f, -32.f, 3, 2.4f);
    stroke(14.2f, 24.f, 13.4f, -32.f, 3, 2.4f);
    for (float y : {18.f, 4.f, -12.f, -26.f}) {
        blob(-14.0f, y, 2.5f, 2.2f, 9);
        blob(14.0f, y, 2.5f, 2.2f, 9);
    }
    poly({{-8.2f, 26.f}, {8.2f, 26.f}, {8.2f, 8.f}, {-8.2f, 8.f}}, 4);
    poly({{-7.f, 24.f}, {7.f, 24.f}, {7.f, 10.f}, {-7.f, 10.f}}, 5);
    poly({{-6.2f, 22.f}, {-1.2f, 22.f}, {-1.2f, 14.f}, {-6.2f, 14.f}}, 6);
    poly({{1.2f, 22.f}, {6.2f, 22.f}, {6.2f, 14.f}, {1.2f, 14.f}}, 6);
    blob(0.f, 27.4f, 1.7f, 1.3f, 12);
    poly({{-3.4f, -1.f}, {3.4f, -1.f}, {2.8f, -16.f}, {-2.8f, -16.f}}, 7);
    poly({{-3.5f, -6.f}, {3.5f, -6.f}, {3.2f, -11.f}, {-3.2f, -11.f}}, 8);
    stroke(0.f, 28.f, 0.f, 38.f, 11, 1.5f);
    stroke(-5.f, 36.f, 5.f, 36.f, 11, 1.3f);
    poly({{-6.f, -22.f}, {6.f, -22.f}, {5.f, -36.f}, {-5.f, -36.f}}, 10);
    blob(-3.2f, -30.f, 1.4f, 1.4f, 13);
    blob(3.2f, -30.f, 1.4f, 1.4f, 13);
    blob(0.f, 43.f, 1.5f, 2.1f, 14);
    stroke(-11.f, 28.f, -10.f, -34.f, 14, 1.15f);
    b.outline(15, false);
    return b;
}

Bitmap paintLeaf() {
    Bitmap b(72, 18);
    b.rect(4, 3, 64, 12, 1);
    b.rect(4, 3, 64, 3, 2);
    b.rect(4, 12, 64, 3, 3);
    b.rect(10, 4, 3, 10, 4);
    b.rect(24, 4, 3, 10, 4);
    b.rect(38, 4, 3, 10, 4);
    b.rect(52, 4, 3, 10, 4);
    b.rect(58, 3, 5, 12, 5);
    b.rect(63, 3, 5, 12, 6);
    b.ellipse(16, 9, 1.3f, 1.3f, 7);
    b.ellipse(30, 9, 1.3f, 1.3f, 7);
    b.ellipse(44, 9, 1.3f, 1.3f, 7);
    b.outline(8, false);
    return b;
}

Bitmap paintBrick() {
    Bitmap b(64, 26);
    b.rect(0, 0, 64, 26, 1);
    b.rect(0, 0, 8, 26, 2);
    b.rect(8, 0, 3, 26, 3);
    for (int row = 0; row < 4; row++) {
        int y = 2 + row * 6;
        int off = (row & 1) ? 10 : 0;
        for (int x = 12 + off; x < 64; x += 18) b.rect(x, y, 16, 5, ((x + row) & 2) ? 4 : 5);
    }
    for (int y = 7; y < 26; y += 6) b.rect(12, y, 52, 1, 6);
    return b;
}

Bitmap paintPath() {
    Bitmap b(88, 40);
    b.rect(0, 0, 88, 40, 1);
    b.rect(0, 0, 26, 40, 4);
    b.rect(24, 0, 4, 40, 5);
    for (int i = 0; i < 18; i++) {
        float x = float((i * 19) % 56 + 30);
        float y = float((i * 11) % 30 + 4);
        b.ellipse(x, y, 4.2f, 2.4f, (i % 3) ? 2 : 3);
        if (i % 4 == 0) b.ellipse(x + 2.f, y + 1.f, 1.1f, 1.1f, 6);
    }
    return b;
}

Bitmap paintField() {
    Bitmap b(80, 40);
    b.rect(0, 0, 80, 40, 3);
    for (int i = 0; i < 14; i++) {
        float x = float((i * 23) % 68 + 6);
        float y = float((i * 9) % 28 + 5);
        b.ellipse(x, y, 5.f, 3.f, (i & 1) ? 1 : 2);
    }
    return b;
}

Bitmap paintReed() {
    Bitmap b(16, 26);
    b.line(3, 24, 2, 6, 1, 1.3f);
    b.line(8, 24, 7, 2, 2, 1.5f);
    b.line(13, 24, 14, 8, 1, 1.2f);
    b.ellipse(7, 3, 2.1f, 1.3f, 3);
    return b;
}

Bitmap paintPost() {
    Bitmap b(14, 18);
    b.ellipse(7, 8, 5.5f, 5.5f, 1);
    b.ellipse(7, 8, 3.2f, 3.2f, 2);
    b.rect(6, 2, 2, 12, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(16, 22);
    b.ellipse(8, 12, 6.f, 6.f, 1);
    b.ellipse(8, 10, 3.2f, 2.4f, 2);
    b.rect(7, 3, 2, 6, 3);
    b.ellipse(8, 3, 1.6f, 1.6f, 2);
    b.outline(4, false);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(12, 22);
    b.rect(5, 8, 2, 12, 3);
    b.ellipse(6, 7, 4.2f, 3.4f, 2);
    b.ellipse(6, 7, 2.2f, 1.8f, 1);
    return b;
}

Bitmap paintKeeper() {
    Bitmap b(22, 28);
    b.ellipse(11, 8, 4.2f, 3.6f, 2);
    b.ellipse(11, 6, 4.6f, 2.2f, 3);
    b.rect(6, 12, 10, 10, 1);
    b.rect(7, 14, 8, 4, 4);
    b.rect(9, 22, 4, 4, 5);
    b.outline(6, false);
    return b;
}

Bitmap paintShed() {
    Bitmap b(70, 46);
    b.rect(6, 10, 58, 30, 1);
    b.rect(6, 10, 58, 6, 2);
    b.poly({{4, 16}, {35, 4}, {66, 16}}, 3);
    b.rect(30, 22, 12, 18, 4);
    b.rect(12, 20, 10, 8, 5);
    b.rect(48, 20, 10, 8, 5);
    b.rect(44, 6, 4, 8, 6);
    b.outline(7, false);
    return b;
}

Bitmap paintCrane() {
    Bitmap b(64, 48);
    b.rect(8, 28, 16, 14, 1);
    b.rect(14, 10, 4, 22, 2);
    b.line(16, 12, 52, 20, 3, 2.4f);
    b.line(48, 20, 48, 36, 4, 1.3f);
    b.ellipse(48, 38, 3.2f, 2.2f, 5);
    b.outline(6, false);
    return b;
}

Bitmap paintTank() {
    Bitmap b(36, 36);
    b.ellipse(18, 18, 15, 15, 1);
    b.ellipse(18, 18, 11, 11, 2);
    b.ellipse(14, 14, 4, 3, 3);
    b.ellipse(18, 18, 2.2f, 2.2f, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintGull(bool flap) {
    Bitmap b(28, 14);
    if (flap) {
        b.line(2, 10, 14, 6, 1, 1.6f);
        b.line(14, 6, 26, 10, 1, 1.6f);
    } else {
        b.line(2, 6, 14, 8, 1, 1.6f);
        b.line(14, 8, 26, 6, 1, 1.6f);
    }
    b.ellipse(14, 8, 2.1f, 1.4f, 2);
    return b;
}

Bitmap paintSmoke() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 6.f, 5.f, 1);
    b.ellipse(8, 8, 3.2f, 2.6f, 2);
    return b;
}

Bitmap paintWake() {
    Bitmap b(28, 14);
    b.ellipse(14, 7, 12, 5, 1);
    b.ellipse(14, 7, 7, 3, 2);
    b.ellipse(14, 7, 3, 1.3f, 0);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 6.5f, 4.f, 3);
    b.ellipse(8, 6, 3.f, 2.f, 2);
    return b;
}

Bitmap paintBlob() {
    Bitmap b(36, 16);
    b.ellipse(18, 8, 16, 6, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 10, 11), gs::rgb4(15, 15, 15), gs::rgb4(15, 5, 3), gs::rgb4(5, 14, 7),
                          gs::rgb4(15, 12, 4), gs::rgb4(6, 10, 13), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 8, 2), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), ink, gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 14, 6), ink, gs::rgb4(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TUG,
           {0, gs::rgb4(1, 2, 2), gs::rgb4(3, 5, 5), gs::rgb4(13, 2, 2), gs::rgb4(14, 14, 13), gs::rgb4(8, 9, 9),
            gs::rgb4(3, 6, 10), gs::rgb4(14, 11, 2), gs::rgb4(1, 1, 1), gs::rgb4(6, 4, 2), gs::rgb4(5, 5, 4),
            gs::rgb4(12, 12, 11), gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 2), gs::rgb4(13, 15, 15), gs::rgb4(0, 0, 1)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(1, 4, 3), gs::rgb4(2, 6, 4), gs::rgb4(13, 13, 11), gs::rgb4(14, 14, 13), gs::rgb4(8, 9, 8),
            gs::rgb4(3, 5, 8), gs::rgb4(7, 8, 8), gs::rgb4(2, 2, 2), gs::rgb4(5, 4, 2), gs::rgb4(4, 5, 4),
            gs::rgb4(11, 12, 11), gs::rgb4(14, 12, 6), gs::rgb4(3, 3, 2), gs::rgb4(12, 14, 13), gs::rgb4(0, 1, 1)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4), gs::rgb4(5, 3, 1), gs::rgb4(4, 4, 5),
                           gs::rgb4(14, 12, 2), gs::rgb4(1, 1, 1), gs::rgb4(14, 14, 12), gs::rgb4(2, 1, 0), 0, 0, 0, 0,
                           0, 0, shadow});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(8, 4, 3), gs::rgb4(11, 10, 8), gs::rgb4(5, 4, 3), gs::rgb4(6, 3, 2),
                            gs::rgb4(11, 6, 4), gs::rgb4(3, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(2, 5, 2), gs::rgb4(4, 8, 3), gs::rgb4(1, 3, 1), gs::rgb4(7, 6, 5),
                           gs::rgb4(5, 4, 3), gs::rgb4(11, 10, 4), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(3, 8, 9), gs::rgb4(8, 12, 13), gs::rgb4(14, 15, 15), gs::rgb4(6, 6, 6),
                         gs::rgb4(10, 10, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_KEEP, {0, gs::rgb4(14, 7, 2), gs::rgb4(12, 9, 6), gs::rgb4(2, 2, 3), gs::rgb4(14, 12, 4),
                           gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_YARD, {0, gs::rgb4(6, 6, 6), gs::rgb4(9, 9, 8), gs::rgb4(8, 3, 2), gs::rgb4(3, 3, 4),
                           gs::rgb4(5, 8, 10), gs::rgb4(4, 4, 4), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PORT, {0, gs::rgb4(3, 13, 5), gs::rgb4(14, 14, 12), gs::rgb4(2, 2, 2), gs::rgb4(1, 3, 1), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STBD, {0, gs::rgb4(14, 3, 2), gs::rgb4(14, 14, 12), gs::rgb4(2, 2, 2), gs::rgb4(4, 1, 1), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(14, 14, 15), gs::rgb4(8, 8, 9), gs::rgb4(12, 9, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, shadow});

    for (int i = 0; i < YAWS; i++) {
        float a = float(i) * (2.f * kPi / float(YAWS));
        art.tug[i] = gs::uploadMipped(vdp, paintTug(a));
    }
    Bitmap leaf = paintLeaf();
    for (int i = 0; i < GATE_DIRS; i++) {
        float a = float(i) * (2.f * kPi / float(GATE_DIRS));
        art.leaf[i] = gs::uploadMipped(vdp, rotateCW(leaf, a));
    }
    art.brick = gs::uploadMipped(vdp, paintBrick());
    art.path = gs::uploadMipped(vdp, paintPath());
    art.field = gs::uploadMipped(vdp, paintField());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.keeper = gs::uploadMipped(vdp, paintKeeper());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.crane = gs::uploadMipped(vdp, paintCrane());
    art.tank = gs::uploadMipped(vdp, paintTank());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(false));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(true));
    art.smoke = gs::uploadMipped(vdp, paintSmoke());
    art.wake = gs::uploadMipped(vdp, paintWake());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.blob = gs::uploadMipped(vdp, paintBlob());
    loadFont(vdp, art);
}

}  // namespace tuglock
