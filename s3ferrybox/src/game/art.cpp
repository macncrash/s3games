#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace ferrybox {
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
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

struct Painter {
    Bitmap b;
    float cx, cy, c, s;
    Painter(int w, int h, float heading)
        : b(w, h), cx(w * 0.5f), cy(h * 0.5f), c(std::cos(heading)), s(std::sin(heading)) {}
    Pt at(float lx, float ly) const {
        float wx = ly * c + lx * s;
        float wy = ly * s - lx * c;
        return {cx + wx, cy - wy};
    }
    void poly(std::initializer_list<Pt> local, int col) {
        std::vector<Pt> w;
        w.reserve(local.size());
        for (const Pt& p : local) w.push_back(at(p.first, p.second));
        b.poly(w, col);
    }
    void stroke(float ax, float ay, float bx, float by, int col, float th) {
        Pt A = at(ax, ay);
        Pt B = at(bx, by);
        b.line(A.first, A.second, B.first, B.second, col, th);
    }
};

Bitmap paintFerry(float heading) {
    Painter p(kHullPx, kHullPx, heading);
    p.poly({{-13.f, -37.f}, {-15.f, -26.f}, {-15.f, 16.f}, {-11.f, 30.f}, {-5.f, 37.f},
            {5.f, 37.f}, {11.f, 30.f}, {15.f, 16.f}, {15.f, -26.f}, {13.f, -37.f}},
           2);
    p.poly({{-11.f, -33.f}, {-12.f, 14.f}, {-8.f, 28.f}, {0.f, 33.f}, {8.f, 28.f}, {12.f, 14.f}, {11.f, -33.f}}, 1);
    p.stroke(-14.2f, -22.f, -14.2f, 14.f, 3, 2.0f);
    p.stroke(14.2f, -22.f, 14.2f, 14.f, 3, 2.0f);
    p.poly({{-6.f, 29.f}, {6.f, 29.f}, {4.f, 36.f}, {-4.f, 36.f}}, 11);
    p.stroke(-4.4f, 30.f, -4.4f, 35.f, 12, 1.15f);
    p.stroke(0.f, 30.f, 0.f, 35.f, 12, 1.15f);
    p.stroke(4.4f, 30.f, 4.4f, 35.f, 12, 1.15f);
    p.stroke(-3.2f, 8.f, -3.2f, 26.f, 12, 1.1f);
    p.stroke(3.2f, 8.f, 3.2f, 26.f, 12, 1.1f);
    const float laneX[2] = {-7.6f, 2.4f};
    const int cols[6] = {8, 9, 10, 7, 8, 9};
    int ci = 0;
    for (float ly = 10.f; ly <= 23.f; ly += 6.5f) {
        for (float lx : laneX) {
            int col = cols[ci++ % 6];
            p.poly({{lx, ly}, {lx + 5.0f, ly}, {lx + 5.0f, ly + 4.4f}, {lx, ly + 4.4f}}, col);
        }
    }
    p.poly({{-8.f, -6.f}, {8.f, -6.f}, {8.f, 8.f}, {-8.f, 8.f}}, 4);
    p.poly({{-6.f, -4.f}, {6.f, -4.f}, {6.f, 6.f}, {-6.f, 6.f}}, 5);
    p.poly({{2.2f, -1.5f}, {7.2f, -1.5f}, {7.2f, 5.f}, {2.2f, 5.f}}, 4);
    p.poly({{-2.3f, -19.f}, {2.3f, -19.f}, {1.6f, -11.f}, {-1.6f, -11.f}}, 6);
    p.poly({{-2.3f, -17.2f}, {2.3f, -17.2f}, {2.0f, -15.2f}, {-2.0f, -15.2f}}, 7);
    p.stroke(0.f, -8.f, 0.f, -23.f, 12, 1.3f);
    p.poly({{-12.2f, -2.f}, {-10.f, -2.f}, {-10.f, 6.f}, {-12.2f, 6.f}}, 13);
    p.poly({{10.f, -2.f}, {12.2f, -2.f}, {12.2f, 6.f}, {10.f, 6.f}}, 13);
    p.stroke(-10.f, -34.f, 10.f, -34.f, 7, 1.5f);
    p.b.outline(12, false);
    return p.b;
}

Bitmap padArt() {
    Bitmap b(48, 88);
    b.rect(0, 0, 48, 88, 1);
    b.rect(2, 2, 44, 84, 2);
    b.rect(5, 5, 38, 78, 1);
    b.poly({{24.f, 18.f}, {33.f, 36.f}, {15.f, 36.f}}, 4);
    b.rect(22, 32, 4, 16, 4);
    return b;
}

Bitmap postArt() {
    Bitmap b(16, 36);
    b.ellipse(8, 30, 6.2f, 3.2f, 2);
    b.rect(6, 10, 4, 20, 1);
    b.poly({{8.f, 2.f}, {13.f, 11.f}, {3.f, 11.f}}, 3);
    b.ellipse(8, 9, 2.1f, 2.1f, 4);
    b.outline(2, false);
    return b;
}

Bitmap barArt(bool vertical) {
    Bitmap b = vertical ? Bitmap(6, 18) : Bitmap(18, 6);
    if (vertical) b.rect(1, 0, 4, 18, 3);
    else b.rect(0, 1, 18, 4, 3);
    return b;
}

Bitmap quayArt() {
    Bitmap b(48, 72);
    b.rect(0, 0, 48, 72, 7);
    b.rect(0, 0, 5, 72, 1);
    b.rect(43, 0, 5, 72, 1);
    b.rect(5, 0, 8, 72, 6);
    b.rect(35, 0, 8, 72, 6);
    for (int y = 6; y < 66; y += 12) {
        b.rect(8, y, 5, 5, 8);
        b.rect(35, y, 5, 5, 8);
        b.rect(18, y + 3, 12, 2, 5);
    }
    b.outline(5, false);
    return b;
}

Bitmap shedArt() {
    Bitmap b(72, 46);
    b.rect(8, 18, 56, 24, 1);
    b.poly({{6.f, 18.f}, {36.f, 4.f}, {66.f, 18.f}}, 2);
    b.rect(30, 24, 12, 18, 3);
    b.rect(14, 24, 12, 9, 4);
    b.rect(46, 24, 12, 9, 4);
    b.outline(5, false);
    return b;
}

Bitmap lightArt() {
    Bitmap b(28, 64);
    b.rect(4, 52, 20, 10, 5);
    b.rect(9, 26, 10, 28, 1);
    b.poly({{5.f, 26.f}, {14.f, 6.f}, {23.f, 26.f}}, 2);
    b.rect(10, 14, 8, 10, 3);
    b.ellipse(14, 18, 2.4f, 2.4f, 4);
    b.outline(6, false);
    return b;
}

Bitmap foamArt() {
    Bitmap b(16, 12);
    b.ellipse(8, 6, 6.2f, 3.3f, 1);
    b.ellipse(8, 6, 2.8f, 1.5f, 2);
    return b;
}

Bitmap smokeArt() {
    Bitmap b(14, 14);
    b.ellipse(7, 8, 5.2f, 4.2f, 1);
    b.ellipse(6, 6, 2.6f, 2.2f, 2);
    return b;
}

Bitmap chevArt() {
    Bitmap b(14, 8);
    b.poly({{1.f, 1.f}, {8.f, 4.f}, {1.f, 7.f}}, 3);
    b.poly({{5.f, 1.f}, {12.f, 4.f}, {5.f, 7.f}}, 4);
    return b;
}

Bitmap gullArt(bool up) {
    Bitmap b(26, 14);
    b.ellipse(13, 8, 3.1f, 1.8f, 1);
    float tip = up ? 2.f : 11.f;
    b.line(13, 7, 2, tip, 1, 1.5f);
    b.line(13, 7, 24, tip, 2, 1.5f);
    b.set(17, 7, 3);
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

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 11, 13), gs::rgb4(15, 12, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FERRY,
           {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 9, 6), gs::rgb4(3, 7, 13), gs::rgb4(14, 13, 10), gs::rgb4(2, 4, 8),
            gs::rgb4(13, 3, 2), gs::rgb4(15, 12, 2), gs::rgb4(12, 3, 3), gs::rgb4(3, 6, 12), gs::rgb4(14, 14, 15),
            gs::rgb4(14, 8, 2), gs::rgb4(2, 2, 3), gs::rgb4(12, 8, 4), 0, shadow});
    setPal(vdp, PAL_POST, {0, gs::rgb4(15, 13, 2), gs::rgb4(2, 2, 3), gs::rgb4(15, 15, 13), gs::rgb4(15, 14, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SHORE,
           {0, gs::rgb4(10, 10, 11), gs::rgb4(12, 4, 3), gs::rgb4(4, 3, 3), gs::rgb4(7, 12, 14), gs::rgb4(2, 2, 3),
            gs::rgb4(4, 10, 4), gs::rgb4(8, 6, 3), gs::rgb4(13, 12, 8), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(10, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(7, 8, 9), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(12, 11, 5), gs::rgb4(8, 7, 3), gs::rgb4(15, 14, 4), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(5, 12, 6), gs::rgb4(2, 7, 4), gs::rgb4(12, 15, 10), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), gs::rgb4(5, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 9), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(10, 12, 13), gs::rgb4(3, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LIGHT,
           {0, gs::rgb4(9, 9, 10), gs::rgb4(12, 3, 3), gs::rgb4(8, 12, 14), gs::rgb4(15, 14, 6), gs::rgb4(6, 6, 7),
            gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 16; i++) art.hull[i] = gs::uploadMipped(vdp, paintFerry(i * kTau / 16.f));
    art.pad = gs::uploadMipped(vdp, padArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.hbar = gs::uploadMipped(vdp, barArt(false));
    art.vbar = gs::uploadMipped(vdp, barArt(true));
    art.quay = gs::uploadMipped(vdp, quayArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.light = gs::uploadMipped(vdp, lightArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(true));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(false));
    art.title = words(vdp, "FERRY", 3, 1, 2);
    art.boxWord = words(vdp, "BOX", 4, 1, 2);
    art.stopped = words(vdp, "STOPPED", 3, 1, 2);
    art.outside = words(vdp, "OUTSIDE", 3, 1, 2);
    art.late = words(vdp, "TOO LATE", 3, 1, 2);
    art.paused = words(vdp, "PAUSED", 3, 1, 2);
    art.north = words(vdp, "N", 2, 1, 2);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
}

}  // namespace ferrybox
