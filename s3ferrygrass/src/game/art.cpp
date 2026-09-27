#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <vector>

namespace ferrygrass {
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
    p.poly({{-11.f, -34.f}, {-13.f, -20.f}, {-13.f, 18.f}, {-8.f, 30.f}, {0.f, 34.f},
            {8.f, 30.f}, {13.f, 18.f}, {13.f, -20.f}, {11.f, -34.f}},
           2);
    p.poly({{-9.f, -30.f}, {-10.f, 16.f}, {-6.f, 26.f}, {0.f, 30.f}, {6.f, 26.f}, {10.f, 16.f}, {9.f, -30.f}}, 1);
    p.poly({{-6.f, 8.f}, {6.f, 8.f}, {6.f, 22.f}, {-6.f, 22.f}}, 4);
    p.poly({{-4.5f, 10.f}, {4.5f, 10.f}, {4.5f, 18.f}, {-4.5f, 18.f}}, 5);
    p.stroke(-12.f, -16.f, -12.f, 14.f, 3, 1.6f);
    p.stroke(12.f, -16.f, 12.f, 14.f, 3, 1.6f);
    p.poly({{-2.f, -22.f}, {2.f, -22.f}, {1.4f, -14.f}, {-1.4f, -14.f}}, 6);
    p.stroke(0.f, -10.f, 0.f, -24.f, 12, 1.2f);
    const int cars[] = {8, 9, 10, 7};
    int ci = 0;
    for (float ly = -8.f; ly <= 4.f; ly += 6.f) {
        int col = cars[ci++ % 4];
        p.poly({{-7.f, ly}, {-2.2f, ly}, {-2.2f, ly + 4.2f}, {-7.f, ly + 4.2f}}, col);
        p.poly({{2.2f, ly}, {7.f, ly}, {7.f, ly + 4.2f}, {2.2f, ly + 4.2f}}, col);
    }
    p.b.outline(12, false);
    return p.b;
}

Bitmap tuftArt() {
    Bitmap b(18, 22);
    b.poly({{9.f, 2.f}, {16.f, 20.f}, {2.f, 20.f}}, 1);
    b.poly({{5.f, 6.f}, {10.f, 20.f}, {1.f, 18.f}}, 2);
    b.poly({{13.f, 5.f}, {17.f, 19.f}, {9.f, 20.f}}, 3);
    return b;
}

Bitmap postArt() {
    Bitmap b(14, 32);
    b.rect(5, 12, 4, 16, 1);
    b.ellipse(7, 28, 5.f, 2.4f, 2);
    b.poly({{7.f, 2.f}, {12.f, 12.f}, {2.f, 12.f}}, 3);
    b.ellipse(7, 10, 1.8f, 1.8f, 4);
    return b;
}

Bitmap barArt(bool vertical) {
    Bitmap b = vertical ? Bitmap(6, 16) : Bitmap(16, 6);
    if (vertical) b.rect(1, 0, 4, 16, 3);
    else b.rect(0, 1, 16, 4, 3);
    return b;
}

Bitmap shedArt() {
    Bitmap b(64, 40);
    b.rect(6, 16, 52, 20, 1);
    b.poly({{4.f, 16.f}, {32.f, 4.f}, {60.f, 16.f}}, 2);
    b.rect(26, 22, 12, 14, 3);
    b.rect(12, 22, 10, 8, 4);
    b.rect(42, 22, 10, 8, 4);
    return b;
}

Bitmap foamArt() {
    Bitmap b(16, 10);
    b.ellipse(8, 5, 6.f, 3.f, 1);
    b.ellipse(5, 5, 2.2f, 1.4f, 2);
    return b;
}

Bitmap smokeArt() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 5.f, 4.5f, 1);
    b.ellipse(5, 6, 2.f, 1.6f, 2);
    return b;
}

Bitmap gullArt(bool up) {
    Bitmap b(24, 12);
    b.ellipse(12, 7, 2.6f, 1.5f, 1);
    float tip = up ? 2.f : 10.f;
    b.line(12, 6, 2, tip, 1, 1.4f);
    b.line(12, 6, 22, tip, 2, 1.4f);
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
           {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 8, 6), gs::rgb4(3, 6, 12), gs::rgb4(13, 12, 9), gs::rgb4(2, 4, 7),
            gs::rgb4(12, 3, 2), gs::rgb4(14, 11, 2), gs::rgb4(11, 3, 3), gs::rgb4(3, 6, 11), gs::rgb4(14, 14, 15),
            gs::rgb4(13, 8, 2), gs::rgb4(2, 2, 3), 0, 0, shadow});
    setPal(vdp, PAL_POST, {0, gs::rgb4(14, 12, 3), gs::rgb4(3, 3, 3), gs::rgb4(15, 15, 12), gs::rgb4(15, 13, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_SHORE,
           {0, gs::rgb4(6, 11, 4), gs::rgb4(3, 8, 2), gs::rgb4(10, 13, 5), gs::rgb4(12, 11, 7), gs::rgb4(4, 3, 2),
            gs::rgb4(8, 6, 3), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(15, 15, 15), gs::rgb4(11, 14, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(8, 12, 4), gs::rgb4(4, 8, 2), gs::rgb4(14, 14, 5), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(6, 13, 6), gs::rgb4(2, 7, 3), gs::rgb4(13, 15, 10), gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), gs::rgb4(6, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 9), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(10, 12, 13), gs::rgb4(3, 4, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(14, 14, 13), gs::rgb4(8, 3, 2), gs::rgb4(4, 5, 8), gs::rgb4(12, 11, 8), gs::rgb4(3, 3, 4),
            gs::rgb4(10, 8, 2), gs::rgb4(13, 10, 3), gs::rgb4(6, 2, 2), gs::rgb4(4, 5, 9), gs::rgb4(13, 13, 14),
            gs::rgb4(11, 7, 2), gs::rgb4(2, 2, 3), 0, 0, shadow});

    loadFont(vdp, art);
    for (int i = 0; i < 8; i++) art.hull[i] = gs::uploadMipped(vdp, paintFerry(i * kTau / 8.f));
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.hbar = gs::uploadMipped(vdp, barArt(false));
    art.vbar = gs::uploadMipped(vdp, barArt(true));
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(true));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(false));
    art.title = words(vdp, "FERRY", 3, 1, 2);
    art.grassWord = words(vdp, "GRASS", 3, 1, 2);
    art.stopped = words(vdp, "FULL STOP", 2, 1, 2);
    art.missed = words(vdp, "MISSED", 3, 1, 2);
    art.late = words(vdp, "THEIR GRASS", 2, 1, 2);
    art.paused = words(vdp, "PAUSED", 3, 1, 2);
}

}  // namespace ferrygrass
