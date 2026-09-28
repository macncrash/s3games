#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace luge {
namespace {

constexpr float kTau = 6.2831853f;

using gs::Bitmap;
using gs::Pt;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    uint16_t c[16] = {};
    int i = 0;
    for (uint16_t v : cs)
        if (i < 16) c[i++] = v;
    for (int k = 0; k < 16; k++) vdp.setColor(pal * 16 + k, c[k]);
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    float wx = ly * c + lx * s;
    float wy = ly * s - lx * c;
    return {cx + wx, cy - wy};
}

Bitmap paintPod(float heading) {
    Bitmap b(72, 72);
    const float cx = 36.f, cy = 36.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto poly = [&](std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
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
    stroke(-7.2f, 16.f, -7.2f, -18.f, 5, 2.2f);
    stroke(7.2f, 16.f, 7.2f, -18.f, 5, 2.2f);
    poly({{0.f, 22.f}, {8.f, 12.f}, {8.f, -8.f}, {5.f, -16.f}, {-5.f, -16.f}, {-8.f, -8.f}, {-8.f, 12.f}}, 1);
    poly({{0.f, 18.f}, {5.5f, 8.f}, {5.f, -6.f}, {3.f, -12.f}, {-3.f, -12.f}, {-5.f, -6.f}, {-5.5f, 8.f}}, 2);
    blob(0.f, 4.f, 4.2f, 6.4f, 3);
    blob(0.f, 10.f, 3.4f, 3.2f, 4);
    blob(0.f, 11.f, 1.6f, 1.4f, 6);
    stroke(-6.f, 6.f, 6.f, 6.f, 7, 1.4f);
    blob(0.f, -14.f, 2.2f, 1.6f, 8);
    b.outline(15, false);
    return b;
}

Bitmap paintPlank() {
    Bitmap b(28, 96);
    b.rect(2, 2, 24, 92, 1);
    for (int i = 0; i < 8; i++) b.rect(4, 4 + i * 11, 20, 8, (i & 1) ? 2 : 3);
    b.rect(2, 2, 3, 92, 4);
    b.rect(23, 2, 3, 92, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintCap() {
    Bitmap b(120, 22);
    b.rect(2, 4, 116, 14, 1);
    for (int i = 0; i < 10; i++) b.rect(4 + i * 11, 6, 8, 10, (i & 1) ? 2 : 3);
    b.rect(2, 3, 116, 3, 4);
    b.outline(5, false);
    return b;
}

Bitmap paintPost() {
    Bitmap b(16, 28);
    b.rect(6, 6, 4, 18, 1);
    b.ellipse(8, 6, 5, 3, 2);
    b.ellipse(8, 5, 2, 1, 3);
    b.rect(5, 22, 6, 3, 4);
    return b;
}

Bitmap paintCleat() {
    Bitmap b(16, 10);
    b.rect(2, 4, 12, 3, 1);
    b.rect(3, 2, 3, 6, 2);
    b.rect(10, 2, 3, 6, 2);
    return b;
}

Bitmap paintShed() {
    Bitmap b(40, 30);
    b.poly({{2, 14}, {20, 3}, {38, 14}}, 4);
    b.rect(5, 14, 30, 13, 1);
    b.rect(16, 17, 7, 10, 3);
    b.rect(8, 17, 5, 5, 2);
    b.rect(27, 17, 5, 5, 2);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(12, 28);
    b.rect(5, 8, 2, 18, 1);
    b.ellipse(6, 6, 4, 3, 2);
    b.ellipse(6, 6, 2, 1, 3);
    return b;
}

Bitmap paintStripe() {
    Bitmap b(48, 10);
    b.rect(0, 2, 48, 6, 1);
    for (int i = 0; i < 6; i++) b.rect(i * 8, 2, 4, 6, 2);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(18, 22);
    b.rect(2, 2, 2, 18, 3);
    b.poly({{4, 3}, {16, 7}, {4, 11}}, 1);
    return b;
}

Bitmap paintCrack() {
    Bitmap b(36, 18);
    b.line(2, 9, 10, 4, 1, 1.2f);
    b.line(10, 4, 18, 12, 1, 1.2f);
    b.line(18, 12, 28, 6, 2, 1.1f);
    b.line(28, 6, 34, 10, 1, 1.1f);
    return b;
}

Bitmap paintSpray() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 3, 1);
    b.ellipse(5, 6, 2, 2, 2);
    b.ellipse(11, 10, 2, 1.2f, 2);
    return b;
}

Bitmap paintBird(int wing) {
    Bitmap b(20, 12);
    b.line(2, 8, 10, 6, 1, 1.4f);
    b.line(10, 6, 18, wing ? 2 : 9, 1, 1.4f);
    b.ellipse(10, 7, 1.4f, 1.2f, 2);
    return b;
}

Bitmap paintEndMark() {
    Bitmap b(20, 20);
    b.poly({{10, 1}, {19, 10}, {10, 19}, {1, 10}}, 1);
    b.poly({{10, 6}, {14, 10}, {10, 14}, {6, 10}}, 2);
    return b;
}

gs::Mipped word(gs::VDP& vdp, const char* s, int scale, int color, int outline) {
    gs::TextStyle st{scale, color, outline, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(s, st));
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t line = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 11, 13), gs::rgb4(15, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 3, 4), line});
    setPal(vdp, PAL_POD,
           {0, gs::rgb4(12, 2, 2), gs::rgb4(15, 4, 3), gs::rgb4(2, 2, 3), gs::rgb4(14, 12, 8), gs::rgb4(10, 11, 12),
            gs::rgb4(15, 14, 6), gs::rgb4(6, 6, 7), gs::rgb4(13, 13, 14), 0, 0, 0, 0, 0, gs::rgb4(3, 3, 4), line});
    setPal(vdp, PAL_TIMBER,
           {0, gs::rgb4(10, 7, 4), gs::rgb4(8, 6, 3), gs::rgb4(12, 9, 5), gs::rgb4(5, 4, 3), gs::rgb4(3, 2, 2), line});
    setPal(vdp, PAL_POST, {0, gs::rgb4(7, 6, 5), gs::rgb4(14, 14, 12), gs::rgb4(15, 13, 6), gs::rgb4(4, 4, 4), line, line});
    setPal(vdp, PAL_END, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 14, 4), gs::rgb4(14, 14, 13), line, line});
    setPal(vdp, PAL_SPRAY, {0, gs::rgb4(13, 15, 15), gs::rgb4(8, 12, 14), line, line});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(14, 14, 15), gs::rgb4(3, 3, 4), line, line});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(14, 15, 15), gs::rgb4(9, 12, 13), gs::rgb4(6, 8, 10), line, line});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 5, 6), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 13), line, line});
    setPal(vdp, PAL_SHED,
           {0, gs::rgb4(9, 8, 7), gs::rgb4(12, 14, 15), gs::rgb4(3, 3, 4), gs::rgb4(8, 3, 2), line, line});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 8), gs::rgb4(8, 7, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 1), line});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(8, 15, 7), gs::rgb4(3, 8, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1), line});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1), line});
    setPal(vdp, PAL_ICE,
           {0, gs::rgb4(10, 13, 14), gs::rgb4(6, 9, 12), gs::rgb4(14, 15, 15), line, line});
    setPal(vdp, PAL_TIDE, {0, gs::rgb4(15, 15, 14), gs::rgb4(13, 4, 2), gs::rgb4(3, 6, 10), line, line});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 3), gs::rgb4(12, 3, 2), line, line});
    vdp.setFogColor(gs::rgb4(6, 8, 11));

    for (int i = 0; i < 8; i++) art.pod[i] = gs::uploadMipped(vdp, paintPod(i * kTau / 8.f));
    art.plank = gs::uploadMipped(vdp, paintPlank());
    art.cap = gs::uploadMipped(vdp, paintCap());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.cleat = gs::uploadMipped(vdp, paintCleat());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.stripe = gs::uploadMipped(vdp, paintStripe());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.crack = gs::uploadMipped(vdp, paintCrack());
    art.spray = gs::uploadMipped(vdp, paintSpray());
    art.bird[0] = gs::uploadMipped(vdp, paintBird(0));
    art.bird[1] = gs::uploadMipped(vdp, paintBird(1));
    art.endMark = gs::uploadMipped(vdp, paintEndMark());
    art.title = word(vdp, "LUGE SLIP", 3, 1, 14);
    art.slipWord = word(vdp, "SLIP", 2, 1, 14);
    art.berthed = word(vdp, "BERTHED", 2, 1, 14);
    art.missed = word(vdp, "MISSED", 2, 1, 14);
    art.tide = word(vdp, "TIDE", 2, 1, 14);
    art.wall = word(vdp, "WALL", 2, 1, 14);
    art.paused = word(vdp, "PAUSED", 2, 1, 14);
    loadFont(vdp, art);
}

}  // namespace luge
