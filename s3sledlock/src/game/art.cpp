#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace sledlock {
namespace {

constexpr float kPi = 3.14159265f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

uint16_t C(int r, int g, int b) { return gs::rgb4(r, g, b); }

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    // +ly is the nose. Heading 0 points up; positive heading turns clockwise.
    float wx = lx * c + ly * s;
    float wy = -lx * s + ly * c;
    return {cx + wx, cy - wy};
}

Bitmap rotateCW(const Bitmap& src, float a) {
    const float ca = std::cos(a), sa = std::sin(a);
    const int nw = int(std::ceil(src.w * std::fabs(ca) + src.h * std::fabs(sa))) + 4;
    const int nh = int(std::ceil(src.w * std::fabs(sa) + src.h * std::fabs(ca))) + 4;
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

Bitmap paintSled(float heading) {
    Bitmap b(104, 104);
    const float cx = 52.f, cy = 52.f;
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
    // Runners, nose curl, deck. Nose sits at local +32 so SLED_PX matches the body.
    stroke(-9.f, 28.f, -9.f, -32.f, 5, 3.2f);
    stroke(9.f, 28.f, 9.f, -32.f, 5, 3.2f);
    stroke(-9.f, 28.f, -9.f, -32.f, 4, 1.6f);
    stroke(9.f, 28.f, 9.f, -32.f, 4, 1.6f);
    stroke(-9.f, 26.f, 0.f, 32.f, 13, 2.4f);
    stroke(9.f, 26.f, 0.f, 32.f, 13, 2.4f);
    poly({{-7.f, 24.f}, {7.f, 24.f}, {6.f, -26.f}, {-6.f, -26.f}}, 1);
    poly({{-5.f, 18.f}, {5.f, 18.f}, {4.f, -22.f}, {-4.f, -22.f}}, 2);
    for (float ly : {-16.f, -6.f, 4.f, 14.f}) stroke(-6.f, ly, 6.f, ly, 3, 1.6f);
    poly({{-5.2f, 8.f}, {5.2f, 8.f}, {4.2f, -12.f}, {-4.2f, -12.f}}, 6);
    poly({{-3.2f, 6.f}, {3.2f, 6.f}, {2.4f, -8.f}, {-2.4f, -8.f}}, 7);
    blob(0.f, 12.f, 3.3f, 3.5f, 9);
    blob(0.f, 14.6f, 3.6f, 2.3f, 8);
    blob(0.f, 17.2f, 1.3f, 1.2f, 8);
    stroke(-1.f, 10.f, -7.f, 1.f, 10, 2.2f);
    blob(-7.f, 1.f, 1.7f, 1.5f, 10);
    blob(-9.f, 6.f, 1.9f, 1.6f, 11);
    blob(9.f, 6.f, 1.9f, 1.6f, 11);
    blob(-3.1f, -9.f, 1.9f, 2.3f, 12);
    blob(3.1f, -9.f, 1.9f, 2.3f, 12);
    blob(-1.15f, 12.1f, 0.5f, 0.5f, 15);
    blob(1.15f, 12.1f, 0.5f, 0.5f, 15);
    b.outline(14, false);
    return b;
}

Bitmap paintLeaf() {
    Bitmap b(78, 18);
    b.rect(8, 3, 62, 12, 1);
    b.rect(8, 3, 62, 3, 4);
    b.rect(8, 12, 62, 3, 3);
    b.rect(14, 4, 3, 10, 6);
    b.rect(32, 4, 3, 10, 6);
    b.rect(50, 4, 3, 10, 6);
    b.ellipse(20, 9, 1.3f, 1.3f, 7);
    b.ellipse(38, 9, 1.3f, 1.3f, 7);
    b.ellipse(56, 9, 1.3f, 1.3f, 7);
    b.rect(10, 5, 2, 3, 5);
    b.rect(28, 5, 2, 3, 8);
    b.rect(46, 5, 2, 3, 5);
    b.outline(14, false);
    return b;
}

Bitmap paintSnow() {
    Bitmap b(64, 36);
    b.rect(0, 0, 64, 36, 1);
    for (int i = 0; i < 14; i++) {
        float x = float((i * 19) % 52 + 6);
        float y = float((i * 11) % 24 + 6);
        b.ellipse(x, y, 7.f, 4.f, (i & 1) ? 2 : 3);
        if (i % 4 == 0) b.ellipse(x + 3.f, y + 1.f, 1.2f, 1.f, 4);
    }
    return b;
}

Bitmap paintStone() {
    Bitmap b(36, 28);
    b.rect(0, 0, 36, 28, 1);
    b.rect(0, 0, 8, 28, 4);
    b.rect(6, 0, 3, 28, 2);
    b.rect(30, 0, 6, 28, 3);
    for (int y = 5; y < 28; y += 8) b.rect(9, y, 20, 1, 5);
    b.rect(18, 8, 1, 6, 6);
    b.rect(24, 16, 1, 5, 6);
    return b;
}

Bitmap paintDrift() {
    Bitmap b(28, 14);
    b.ellipse(14, 9, 12, 5, 1);
    b.ellipse(10, 8, 6, 3, 2);
    return b;
}

Bitmap paintPine() {
    Bitmap b(40, 56);
    b.rect(18, 40, 5, 14, 7);
    b.poly({{20, 6}, {36, 28}, {4, 28}}, 5);
    b.poly({{20, 16}, {34, 36}, {6, 36}}, 6);
    b.poly({{20, 26}, {32, 46}, {8, 46}}, 5);
    b.poly({{14, 12}, {22, 8}, {24, 18}}, 1);
    b.poly({{12, 24}, {20, 20}, {22, 30}}, 1);
    b.poly({{14, 36}, {22, 32}, {20, 42}}, 2);
    b.outline(14, false);
    return b;
}

Bitmap paintCabin() {
    Bitmap b(52, 44);
    b.rect(8, 20, 36, 18, 1);
    b.rect(8, 20, 36, 4, 4);
    b.poly({{4, 22}, {26, 6}, {48, 22}}, 2);
    b.poly({{12, 20}, {26, 10}, {40, 20}}, 1);
    b.rect(22, 26, 8, 12, 5);
    b.rect(24, 28, 4, 10, 8);
    b.rect(12, 24, 7, 6, 3);
    b.rect(33, 24, 7, 6, 3);
    b.rect(34, 8, 4, 10, 7);
    b.rect(32, 6, 8, 3, 7);
    b.outline(14, false);
    return b;
}

Bitmap paintKeeper() {
    Bitmap b(24, 36);
    b.ellipse(12, 9, 5.2f, 5.f, 4);
    b.ellipse(12, 7, 5.4f, 3.2f, 3);
    b.rect(8, 13, 8, 12, 1);
    b.rect(7, 14, 3, 8, 2);
    b.rect(14, 15, 4, 7, 2);
    b.rect(9, 24, 3, 8, 5);
    b.rect(13, 24, 3, 8, 5);
    b.rect(8, 31, 4, 3, 6);
    b.rect(13, 31, 4, 3, 6);
    b.ellipse(18, 18, 2.2f, 2.6f, 8);
    b.ellipse(18, 18, 1.1f, 1.3f, 1);
    b.ellipse(10.5f, 9.2f, 0.5f, 0.5f, 15);
    b.ellipse(13.5f, 9.2f, 0.5f, 0.5f, 15);
    b.outline(14, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(16, 20);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(8, 8, 3.2f, 3.2f, 3);
    b.rect(5, 2, 6, 3, 4);
    b.rect(6, 12, 4, 6, 6);
    b.outline(14, false);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(14, 28);
    b.rect(6, 10, 3, 16, 3);
    b.ellipse(7, 7, 5, 5, 1);
    b.ellipse(7, 7, 2.4f, 2.4f, 2);
    b.rect(4, 12, 6, 2, 4);
    return b;
}

Bitmap paintSign() {
    Bitmap b(46, 28);
    b.rect(2, 4, 42, 16, 1);
    b.rect(2, 4, 42, 3, 2);
    b.rect(21, 20, 4, 6, 7);
    gs::TextStyle st{2, 6, 0, 0, 1};
    Bitmap word = gs::textBitmap("LOCK", st);
    b.blit(word, 7, 8);
    b.outline(14, false);
    return b;
}

Bitmap paintHare() {
    Bitmap b(26, 18);
    b.ellipse(12, 11, 8, 4.5f, 1);
    b.ellipse(18, 10, 3.2f, 2.6f, 2);
    b.ellipse(6, 12, 2.2f, 1.6f, 1);
    b.line(16, 8, 15, 2, 2, 1.4f);
    b.line(19, 8, 20, 2, 2, 1.4f);
    b.ellipse(15, 2, 1.1f, 1.3f, 3);
    b.ellipse(20, 2, 1.1f, 1.3f, 3);
    b.ellipse(19.5f, 10, 0.5f, 0.5f, 4);
    b.outline(14, false);
    return b;
}

Bitmap paintLadder() {
    Bitmap b(12, 36);
    b.rect(1, 2, 2, 32, 3);
    b.rect(9, 2, 2, 32, 3);
    for (int y = 6; y < 32; y += 6) b.rect(2, y, 8, 2, 1);
    return b;
}

Bitmap paintFlake() {
    Bitmap b(5, 5);
    b.rect(2, 0, 1, 5, 2);
    b.rect(0, 2, 5, 1, 2);
    b.set(1, 1, 1);
    b.set(3, 3, 1);
    return b;
}

Bitmap paintSpray() {
    Bitmap b(16, 10);
    b.ellipse(8, 5, 7, 4, 1);
    b.ellipse(8, 5, 3.5f, 2, 2);
    return b;
}

Bitmap paintCrack() {
    Bitmap b(18, 40);
    b.line(9, 2, 6, 12, 4, 1.4f);
    b.line(6, 12, 12, 20, 4, 1.4f);
    b.line(12, 20, 7, 30, 4, 1.3f);
    b.line(7, 30, 11, 38, 4, 1.3f);
    b.line(6, 12, 2, 16, 3, 1.1f);
    b.line(12, 20, 16, 24, 3, 1.1f);
    return b;
}

Bitmap paintBlob() {
    Bitmap b(32, 16);
    b.ellipse(16, 8, 14, 6, 1);
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
    const uint16_t shadow = C(1, 1, 2);
    const uint16_t ink = C(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, C(8, 10, 12), C(15, 15, 15), C(15, 5, 4), C(6, 14, 8), C(15, 12, 4), C(6, 8, 12),
                          0, 0, 0, 0, 0, 0, shadow, shadow});
    setPal(vdp, PAL_AMBER, {0, C(15, 12, 4), C(15, 8, 2), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow, shadow});
    setPal(vdp, PAL_RED, {0, C(15, 4, 3), ink, C(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow, shadow});
    setPal(vdp, PAL_GREEN, {0, C(6, 14, 8), ink, C(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow, shadow});
    setPal(vdp, PAL_SLED,
           {0, C(13, 2, 2), C(8, 1, 1), C(14, 12, 8), C(12, 13, 14), C(5, 6, 8), C(2, 3, 8), C(1, 2, 5), C(14, 3, 3),
            C(13, 8, 6), C(14, 11, 2), C(12, 3, 3), C(3, 2, 2), C(9, 6, 3), C(1, 1, 2), C(1, 1, 1)});
    setPal(vdp, PAL_GATE,
           {0, C(8, 5, 2), C(11, 8, 4), C(4, 3, 1), C(14, 15, 15), C(9, 12, 13), C(3, 3, 4), C(12, 9, 3), C(6, 9, 11),
            0, 0, 0, 0, 0, C(1, 1, 2), C(2, 2, 2)});
    setPal(vdp, PAL_SNOW,
           {0, C(14, 15, 15), C(8, 11, 14), C(12, 14, 15), C(15, 15, 15), C(2, 6, 3), C(4, 9, 4), C(6, 4, 2), C(3, 5, 3),
            0, 0, 0, 0, 0, C(1, 2, 3), C(1, 1, 2)});
    setPal(vdp, PAL_STONE,
           {0, C(7, 8, 8), C(10, 11, 11), C(3, 4, 5), C(13, 14, 15), C(5, 5, 6), C(4, 6, 4), 0, 0, 0, 0, 0, 0, 0,
            C(1, 1, 2), 0});
    setPal(vdp, PAL_CABIN,
           {0, C(7, 4, 2), C(14, 15, 15), C(14, 11, 3), C(5, 5, 6), C(8, 2, 2), C(1, 1, 2), C(4, 4, 4), C(4, 1, 1), 0,
            0, 0, 0, 0, C(1, 1, 2), shadow});
    setPal(vdp, PAL_KEEP,
           {0, C(12, 2, 2), C(7, 1, 1), C(12, 10, 7), C(13, 8, 6), C(2, 3, 6), C(2, 2, 2), C(10, 2, 2), C(14, 12, 3),
            0, 0, 0, 0, 0, C(1, 1, 2), C(1, 1, 1)});
    setPal(vdp, PAL_FX, {0, C(10, 13, 15), C(15, 15, 15), C(6, 9, 12), C(3, 6, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow,
                         shadow});
    setPal(vdp, PAL_LAMP,
           {0, C(14, 12, 3), C(15, 15, 10), C(5, 4, 3), C(13, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow, shadow});
    setPal(vdp, PAL_HARE, {0, C(14, 14, 14), C(8, 8, 9), C(12, 7, 7), C(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, C(1, 1, 2),
                           shadow});

    for (int i = 0; i < YAWS; i++) {
        float a = float(i) * (2.f * kPi / float(YAWS));
        art.sled[i] = gs::uploadMipped(vdp, paintSled(a));
    }
    Bitmap leaf = paintLeaf();
    for (int i = 0; i < GATE_DIRS; i++) {
        float a = float(i) * (2.f * kPi / float(GATE_DIRS));
        art.leaf[i] = gs::uploadMipped(vdp, rotateCW(leaf, a));
    }
    art.snow = gs::uploadMipped(vdp, paintSnow());
    art.stone = gs::uploadMipped(vdp, paintStone());
    art.drift = gs::uploadMipped(vdp, paintDrift());
    art.pine = gs::uploadMipped(vdp, paintPine());
    art.cabin = gs::uploadMipped(vdp, paintCabin());
    art.keeper = gs::uploadMipped(vdp, paintKeeper());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.sign = gs::uploadMipped(vdp, paintSign());
    art.hare = gs::uploadMipped(vdp, paintHare());
    art.ladder = gs::uploadMipped(vdp, paintLadder());
    art.flake = gs::uploadMipped(vdp, paintFlake());
    art.spray = gs::uploadMipped(vdp, paintSpray());
    art.crack = gs::uploadMipped(vdp, paintCrack());
    art.blob = gs::uploadMipped(vdp, paintBlob());
    loadFont(vdp, art);
}

}  // namespace sledlock
