#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace skifflock {
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
    float wx = lx * c + ly * s;
    float wy = -lx * s + ly * c;
    return {cx + wx, cy - wy};
}

// Nose is +ly. Heading 0 points up the bitmap; positive heading turns clockwise.
Bitmap paintSkiff(float heading) {
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
    poly({{0.f, 32.f}, {10.f, 16.f}, {10.f, -22.f}, {7.f, -30.f}, {-7.f, -30.f}, {-10.f, -22.f}, {-10.f, 16.f}}, 2);
    poly({{0.f, 26.f}, {7.f, 14.f}, {7.f, -20.f}, {4.f, -26.f}, {-4.f, -26.f}, {-7.f, -20.f}, {-7.f, 14.f}}, 1);
    poly({{0.f, 22.f}, {5.5f, 12.f}, {5.5f, -18.f}, {3.f, -24.f}, {-3.f, -24.f}, {-5.5f, -18.f}, {-5.5f, 12.f}}, 4);
    stroke(-8.f, 14.f, -7.f, -18.f, 13, 1.5f);
    stroke(8.f, 14.f, 7.f, -18.f, 13, 1.5f);
    stroke(-6.f, 12.f, 6.f, 12.f, 5, 1.6f);
    stroke(-5.f, -6.f, 5.f, -6.f, 5, 1.6f);
    stroke(-9.f, 8.f, -8.f, -16.f, 14, 1.5f);
    stroke(9.f, 8.f, 8.f, -16.f, 14, 1.5f);
    blob(0.f, 4.f, 3.1f, 3.4f, 6);
    blob(0.f, 6.2f, 3.3f, 1.7f, 8);
    poly({{-3.2f, 2.f}, {3.2f, 2.f}, {2.6f, -8.f}, {-2.6f, -8.f}}, 7);
    blob(-1.f, 4.6f, 0.55f, 0.55f, 9);
    blob(1.f, 4.6f, 0.55f, 0.55f, 9);
    poly({{-3.4f, -22.f}, {3.4f, -22.f}, {2.6f, -29.f}, {-2.6f, -29.f}}, 10);
    blob(0.f, -27.f, 2.2f, 1.6f, 11);
    blob(0.f, 29.f, 2.6f, 2.2f, 12);
    b.outline(15, false);
    return b;
}

Bitmap paintLeaf() {
    Bitmap b(64, 20);
    b.rect(4, 2, 56, 16, 1);
    b.rect(4, 2, 56, 4, 2);
    b.rect(8, 6, 5, 8, 3);
    b.rect(22, 6, 5, 8, 3);
    b.rect(36, 6, 5, 8, 3);
    b.rect(48, 3, 8, 14, 4);
    b.rect(52, 3, 4, 14, 5);
    b.ellipse(12, 10, 1.6f, 1.6f, 6);
    b.ellipse(26, 10, 1.6f, 1.6f, 6);
    b.ellipse(40, 10, 1.6f, 1.6f, 6);
    b.outline(7, false);
    return b;
}

Bitmap paintGrass() {
    Bitmap b(96, 40);
    b.rect(0, 0, 96, 40, 1);
    for (int i = 0; i < 28; i++) {
        float x = float((i * 17) % 78 + 4);
        float y = float((i * 9) % 30 + 4);
        b.ellipse(x, y, 4.2f, 2.6f, (i % 3) ? 2 : 3);
        if (i % 5 == 0) b.ellipse(x + 3.f, y + 1.f, 1.1f, 1.1f, 6);
    }
    b.rect(78, 0, 18, 40, 4);
    b.rect(78, 0, 3, 40, 5);
    b.rect(92, 0, 4, 40, 2);
    for (int y = 4; y < 40; y += 8) b.rect(84, y, 6, 1, 5);
    return b;
}

Bitmap paintField() {
    Bitmap b(96, 40);
    b.rect(0, 0, 96, 40, 3);
    for (int i = 0; i < 16; i++) {
        float x = float((i * 23) % 84 + 6);
        float y = float((i * 11) % 28 + 5);
        b.ellipse(x, y, 5.f, 3.f, (i & 1) ? 1 : 2);
    }
    return b;
}

Bitmap paintStone() {
    Bitmap b(48, 22);
    b.rect(0, 0, 48, 22, 1);
    b.rect(0, 0, 48, 5, 2);
    b.rect(0, 17, 48, 5, 3);
    for (int x = 6; x < 40; x += 12) b.rect(x, 5, 1, 12, 3);
    b.rect(14, 8, 1, 6, 4);
    b.rect(28, 6, 1, 7, 4);
    b.rect(40, 0, 8, 22, 5);
    b.rect(40, 0, 2, 22, 6);
    return b;
}

Bitmap paintReed() {
    Bitmap b(18, 28);
    b.line(4, 26, 3, 6, 1, 1.4f);
    b.line(9, 26, 8, 2, 2, 1.6f);
    b.line(14, 26, 15, 8, 1, 1.3f);
    b.ellipse(8, 3, 2.2f, 1.4f, 3);
    return b;
}

Bitmap paintTree() {
    Bitmap b(36, 44);
    b.rect(16, 28, 5, 14, 4);
    b.ellipse(18, 18, 14, 13, 1);
    b.ellipse(13, 16, 8, 7, 2);
    b.ellipse(22, 20, 6, 5, 3);
    b.outline(5, false);
    return b;
}

Bitmap paintCottage() {
    Bitmap b(52, 40);
    b.rect(8, 16, 36, 20, 2);
    b.rect(8, 16, 36, 6, 3);
    b.poly({{4, 18}, {26, 4}, {48, 18}}, 4);
    b.poly({{10, 16}, {26, 7}, {42, 16}}, 5);
    b.rect(22, 22, 9, 14, 7);
    b.rect(12, 20, 7, 6, 6);
    b.rect(33, 20, 7, 6, 6);
    b.rect(24, 4, 3, 8, 3);
    b.outline(8, false);
    return b;
}

Bitmap paintSign() {
    Bitmap b(46, 28);
    b.rect(2, 4, 42, 20, 2);
    b.rect(2, 4, 42, 4, 3);
    b.rect(20, 22, 4, 5, 4);
    gs::TextStyle st{2, 1, 0, 0, 1};
    Bitmap word = gs::textBitmap("LOCK", st);
    b.blit(word, 6, 8);
    b.outline(8, false);
    return b;
}

Bitmap paintKeeper() {
    Bitmap b(20, 34);
    b.ellipse(10, 8, 4.2f, 4.2f, 1);
    b.rect(6, 4, 8, 4, 6);
    b.rect(7, 12, 6, 10, 3);
    b.rect(5, 13, 3, 7, 7);
    b.rect(12, 13, 3, 7, 7);
    b.rect(7, 21, 2, 8, 4);
    b.rect(11, 21, 2, 8, 4);
    b.rect(6, 28, 4, 3, 5);
    b.rect(11, 28, 4, 3, 5);
    b.rect(8, 9, 2, 1, 2);
    b.rect(11, 9, 2, 1, 2);
    return b;
}

Bitmap paintPost() {
    Bitmap b(14, 36);
    for (int i = 0; i < 6; i++) b.rect(4, 3 + i * 5, 6, 4, (i & 1) ? 4 : 5);
    b.ellipse(7, 32, 5, 2.2f, 3);
    b.outline(7, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(16, 22);
    b.rect(7, 1, 2, 4, 2);
    b.ellipse(8, 11, 6, 6, 1);
    b.rect(3, 10, 10, 2, 2);
    b.ellipse(8, 18, 3.2f, 2.2f, 3);
    b.outline(4, false);
    return b;
}

Bitmap paintHeron() {
    Bitmap b(28, 32);
    b.ellipse(12, 16, 6, 3.2f, 1);
    b.ellipse(18, 14, 3.2f, 2.4f, 2);
    b.line(20, 14, 26, 12, 3, 1.4f);
    b.line(10, 18, 8, 28, 4, 1.2f);
    b.line(14, 18, 15, 28, 4, 1.2f);
    b.set(19, 13, 5);
    return b;
}

Bitmap paintGull(bool up) {
    Bitmap b(30, 16);
    b.ellipse(15, 9, 2.4f, 1.6f, 1);
    float tip = up ? 2.f : 12.f;
    b.line(15, 8, 2, tip, 1, 1.5f);
    b.line(15, 8, 28, tip, 1, 1.5f);
    b.line(15, 9, 6, (tip + 9.f) * 0.5f, 2, 1.1f);
    b.line(15, 9, 24, (tip + 9.f) * 0.5f, 2, 1.1f);
    return b;
}

Bitmap paintRipple() {
    Bitmap b(24, 14);
    b.ellipse(12, 7, 10, 5, 1);
    b.ellipse(12, 7, 6, 3, 2);
    b.ellipse(12, 7, 3, 1.4f, 0);
    return b;
}

Bitmap paintFoam() {
    Bitmap b(18, 12);
    b.ellipse(9, 6, 7, 4, 2);
    b.ellipse(9, 6, 3.5f, 2, 3);
    return b;
}

Bitmap paintBlob() {
    Bitmap b(32, 18);
    b.ellipse(16, 9, 14, 7, 1);
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t ink = gs::rgb4(15, 15, 14);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 9, 10), gs::rgb4(15, 15, 15), gs::rgb4(15, 5, 3), gs::rgb4(5, 14, 6),
                          gs::rgb4(15, 12, 3), gs::rgb4(6, 9, 13), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 8, 2), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), ink, gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(5, 14, 6), ink, gs::rgb4(1, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BOAT, {0, gs::rgb4(14, 14, 12), gs::rgb4(11, 12, 11), gs::rgb4(7, 8, 7), gs::rgb4(10, 7, 4),
                           gs::rgb4(6, 4, 2), gs::rgb4(13, 9, 6), gs::rgb4(2, 4, 9), gs::rgb4(12, 2, 2),
                           gs::rgb4(2, 1, 1), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 9), gs::rgb4(14, 13, 10),
                           gs::rgb4(2, 5, 9), gs::rgb4(9, 6, 3), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_GATE, {0, gs::rgb4(7, 4, 2), gs::rgb4(11, 8, 4), gs::rgb4(5, 5, 6), gs::rgb4(15, 15, 13),
                           gs::rgb4(1, 1, 1), gs::rgb4(12, 12, 11), gs::rgb4(2, 1, 0), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(7, 7, 6), gs::rgb4(11, 11, 10), gs::rgb4(4, 4, 3), gs::rgb4(3, 5, 2),
                            gs::rgb4(13, 12, 10), gs::rgb4(5, 6, 6), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(2, 6, 2), gs::rgb4(4, 9, 3), gs::rgb4(1, 4, 1), gs::rgb4(8, 6, 3),
                           gs::rgb4(5, 4, 2), gs::rgb4(12, 11, 4), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(4, 10, 12), gs::rgb4(8, 13, 14), gs::rgb4(13, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                         0, 0, shadow});
    setPal(vdp, PAL_KEEP, {0, gs::rgb4(13, 9, 6), gs::rgb4(3, 2, 1), gs::rgb4(2, 3, 6), gs::rgb4(2, 2, 3),
                           gs::rgb4(3, 2, 1), gs::rgb4(8, 2, 2), gs::rgb4(12, 12, 10), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(2, 1, 1), gs::rgb4(12, 10, 7), gs::rgb4(9, 7, 5), gs::rgb4(6, 2, 2),
                            gs::rgb4(8, 3, 2), gs::rgb4(6, 8, 10), gs::rgb4(4, 3, 2), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0,
                            0, shadow});
    setPal(vdp, PAL_PORT, {0, gs::rgb4(3, 12, 5), gs::rgb4(15, 15, 14), gs::rgb4(1, 5, 2), gs::rgb4(1, 2, 1), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STBD, {0, gs::rgb4(14, 3, 2), gs::rgb4(15, 15, 14), gs::rgb4(6, 1, 1), gs::rgb4(2, 1, 1), 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(14, 14, 15), gs::rgb4(8, 8, 9), gs::rgb4(13, 10, 2), gs::rgb4(7, 6, 4),
                           gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    for (int i = 0; i < YAWS; i++) {
        float a = float(i) * (2.f * kPi / float(YAWS));
        art.skiff[i] = gs::uploadMipped(vdp, paintSkiff(a));
    }
    Bitmap leaf = paintLeaf();
    for (int i = 0; i < GATE_DIRS; i++) {
        float a = float(i) * (2.f * kPi / float(GATE_DIRS));
        art.leaf[i] = gs::uploadMipped(vdp, rotateCW(leaf, a));
    }
    art.grass = gs::uploadMipped(vdp, paintGrass());
    art.field = gs::uploadMipped(vdp, paintField());
    art.stone = gs::uploadMipped(vdp, paintStone());
    art.reed = gs::uploadMipped(vdp, paintReed());
    art.tree = gs::uploadMipped(vdp, paintTree());
    art.cottage = gs::uploadMipped(vdp, paintCottage());
    art.sign = gs::uploadMipped(vdp, paintSign());
    art.keeper = gs::uploadMipped(vdp, paintKeeper());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.heron = gs::uploadMipped(vdp, paintHeron());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(false));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(true));
    art.ripple = gs::uploadMipped(vdp, paintRipple());
    art.foam = gs::uploadMipped(vdp, paintFoam());
    art.blob = gs::uploadMipped(vdp, paintBlob());
    loadFont(vdp, art);
}

}  // namespace skifflock
