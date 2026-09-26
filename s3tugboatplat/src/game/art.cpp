#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace tugplat {
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

// Local +y is the bow, +x is starboard. Heading 0 points east.
Bitmap paintTug(float heading) {
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

    poly({{0.f, 32.f}, {7.f, 22.f}, {12.f, 10.f}, {13.f, -6.f}, {12.f, -20.f}, {8.f, -30.f}, {-8.f, -30.f},
          {-12.f, -20.f}, {-13.f, -6.f}, {-12.f, 10.f}, {-7.f, 22.f}},
         1);
    poly({{0.f, 26.f}, {8.f, 14.f}, {9.f, -4.f}, {7.f, -22.f}, {-7.f, -22.f}, {-9.f, -4.f}, {-8.f, 14.f}}, 2);
    stroke(-12.4f, 16.f, -11.2f, -24.f, 3, 2.4f);
    stroke(12.4f, 16.f, 11.2f, -24.f, 3, 2.4f);
    stroke(13.4f, 18.f, 12.6f, -22.f, 8, 2.6f);
    poly({{-7.2f, 16.f}, {7.2f, 16.f}, {6.4f, 4.f}, {-6.4f, 4.f}}, 4);
    poly({{-5.4f, 14.f}, {5.4f, 14.f}, {4.8f, 6.f}, {-4.8f, 6.f}}, 6);
    poly({{-3.6f, 13.f}, {-0.8f, 13.f}, {-0.8f, 8.f}, {-3.6f, 8.f}}, 5);
    poly({{0.8f, 13.f}, {3.6f, 13.f}, {3.6f, 8.f}, {0.8f, 8.f}}, 5);
    poly({{-3.4f, -1.f}, {3.4f, -1.f}, {2.8f, -12.f}, {-2.8f, -12.f}}, 3);
    poly({{-2.6f, -2.f}, {2.6f, -2.f}, {2.2f, -6.f}, {-2.2f, -6.f}}, 7);
    blob(0.f, -13.5f, 2.2f, 1.4f, 7);
    stroke(0.f, 16.f, 0.f, 28.f, 11, 1.6f);
    stroke(-3.2f, 26.f, 3.2f, 26.f, 11, 1.2f);
    stroke(-13.f, 0.f, 13.f, 0.f, 14, 3.4f);
    blob(-7.2f, 9.f, 1.15f, 1.15f, 9);
    blob(7.2f, 9.f, 1.15f, 1.15f, 10);
    blob(-4.2f, -24.f, 1.35f, 1.35f, 7);
    blob(4.2f, -24.f, 1.35f, 1.35f, 7);
    blob(0.f, 20.f, 1.1f, 1.1f, 12);
    b.outline(15, false);
    return b;
}

Bitmap paintPlatform() {
    Bitmap b(40, 224);
    b.rect(0, 0, 40, 224, 1);
    for (int y = 2; y < 222; y += 7) {
        int band = (y / 7) & 1;
        b.rect(5, y, 34, 5, band ? 2 : 3);
    }
    b.rect(0, 0, 5, 224, 4);
    for (int y = 8; y < 220; y += 14) b.rect(1, y, 2, 2, 5);
    b.rect(5, 98, 35, 28, 8);
    b.rect(5, 104, 35, 16, 6);
    b.rect(5, 109, 35, 6, 7);
    for (int i = 0; i < 4; i++) {
        b.poly({{8.f + i * 8.f, 90.f}, {12.f + i * 8.f, 90.f}, {10.f + i * 8.f, 96.f}}, 6);
        b.poly({{8.f + i * 8.f, 134.f}, {12.f + i * 8.f, 134.f}, {10.f + i * 8.f, 128.f}}, 6);
    }
    return b;
}

Bitmap paintPlank() {
    Bitmap b(40, 48);
    b.rect(0, 0, 40, 48, 1);
    for (int y = 2; y < 46; y += 7) b.rect(5, y, 34, 5, ((y / 7) & 1) ? 2 : 3);
    b.rect(0, 0, 5, 48, 4);
    for (int y = 6; y < 44; y += 14) b.rect(1, y, 2, 2, 5);
    return b;
}

Bitmap paintYard() {
    Bitmap b(64, 48);
    b.rect(0, 0, 64, 48, 1);
    for (int y = 0; y < 48; y += 8) b.rect(0, y, 64, 3, (y / 8) & 1 ? 2 : 1);
    for (int i = 0; i < 8; i++) b.rect(4 + (i * 9) % 52, 6 + (i * 5) % 36, 6, 3, i & 1 ? 3 : 5);
    b.rect(0, 20, 18, 10, 4);
    return b;
}

Bitmap paintPile() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 5.5f, 5.5f, 1);
    b.ellipse(6, 6, 2.4f, 2.2f, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintCrane() {
    Bitmap b(88, 56);
    b.rect(62, 6, 7, 44, 1);
    b.rect(60, 46, 12, 6, 2);
    b.line(66, 10, 8, 22, 1, 2.4f);
    b.line(66, 14, 10, 28, 4, 1.2f);
    b.line(66, 8, 48, 4, 1, 1.6f);
    b.line(48, 4, 66, 18, 2, 1.3f);
    b.rect(54, 16, 10, 8, 5);
    b.rect(56, 18, 4, 3, 3);
    b.line(8, 22, 8, 40, 4, 1.1f);
    b.ellipse(8, 40, 2.2f, 2.2f, 3);
    b.outline(15, false);
    return b;
}

Bitmap paintShed() {
    Bitmap b(44, 36);
    b.rect(4, 14, 36, 18, 1);
    b.poly({{2.f, 16.f}, {22.f, 4.f}, {42.f, 16.f}}, 2);
    b.rect(18, 20, 8, 12, 3);
    b.rect(8, 18, 6, 5, 4);
    b.rect(30, 18, 6, 5, 4);
    b.rect(20, 6, 3, 6, 5);
    b.outline(15, false);
    return b;
}

Bitmap paintBuoy() {
    Bitmap b(16, 22);
    b.ellipse(8, 13, 6.f, 6.f, 1);
    b.ellipse(8, 12, 3.2f, 2.4f, 2);
    b.rect(7, 3, 2, 7, 3);
    b.ellipse(8, 3, 1.7f, 1.7f, 2);
    b.outline(15, false);
    return b;
}

Bitmap paintGull(bool flap) {
    Bitmap b(26, 14);
    if (flap) {
        b.line(2, 11, 13, 4, 1, 1.6f);
        b.line(13, 4, 24, 11, 1, 1.6f);
    } else {
        b.line(2, 6, 13, 8, 1, 1.6f);
        b.line(13, 8, 24, 6, 1, 1.6f);
    }
    b.ellipse(13, 8, 2.1f, 1.4f, 2);
    b.ellipse(15.2f, 8.2f, 1.1f, 0.7f, 3);
    return b;
}

Bitmap paintSmoke() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 6.f, 5.f, 3);
    b.ellipse(7, 8, 3.f, 2.4f, 1);
    return b;
}

Bitmap paintWake() {
    Bitmap b(28, 12);
    b.ellipse(14, 6, 12.f, 4.2f, 2);
    b.ellipse(14, 6, 6.f, 2.f, 1);
    return b;
}

Bitmap paintShadow() {
    Bitmap b(36, 16);
    b.ellipse(18, 8, 15.f, 5.5f, 1);
    return b;
}

Bitmap paintDiamond() {
    Bitmap b(18, 18);
    b.poly({{9.f, 1.f}, {17.f, 9.f}, {9.f, 17.f}, {1.f, 9.f}}, 1);
    b.poly({{9.f, 4.f}, {14.f, 9.f}, {9.f, 14.f}, {4.f, 9.f}}, 2);
    b.outline(3, false);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(12, 20);
    b.rect(5, 8, 2, 10, 2);
    b.ellipse(6, 6, 4.2f, 4.2f, 1);
    b.ellipse(6, 5.5f, 1.8f, 1.8f, 3);
    b.rect(3, 17, 6, 2, 2);
    return b;
}

Bitmap paintLine() {
    Bitmap b(32, 8);
    b.rect(0, 2, 32, 4, 1);
    b.rect(0, 3, 32, 2, 2);
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
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 10, 12), gs::rgb4(15, 15, 15), gs::rgb4(15, 5, 3), gs::rgb4(4, 14, 6),
                          gs::rgb4(15, 12, 3), gs::rgb4(6, 12, 13), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 2), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 2, 2), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(4, 14, 6), gs::rgb4(2, 6, 3), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_TUG,
           {0, gs::rgb4(1, 2, 4), gs::rgb4(3, 6, 8), gs::rgb4(13, 2, 2), gs::rgb4(14, 14, 13), gs::rgb4(3, 8, 12),
            gs::rgb4(9, 8, 6), gs::rgb4(2, 2, 2), gs::rgb4(12, 10, 7), gs::rgb4(15, 2, 2), gs::rgb4(2, 13, 4),
            gs::rgb4(12, 12, 11), gs::rgb4(15, 15, 14), gs::rgb4(8, 6, 3), gs::rgb4(15, 15, 15), gs::rgb4(0, 0, 1)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(1, 5, 3), gs::rgb4(3, 8, 5), gs::rgb4(14, 7, 1), gs::rgb4(13, 14, 12), gs::rgb4(3, 7, 10),
            gs::rgb4(8, 8, 6), gs::rgb4(2, 2, 2), gs::rgb4(11, 9, 6), gs::rgb4(15, 3, 2), gs::rgb4(6, 14, 5),
            gs::rgb4(11, 12, 10), gs::rgb4(15, 14, 12), gs::rgb4(7, 6, 3), gs::rgb4(15, 15, 14), gs::rgb4(0, 1, 1)});
    setPal(vdp, PAL_TIMBER,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(8, 6, 3), gs::rgb4(2, 2, 2), gs::rgb4(12, 12, 11),
            gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 13), gs::rgb4(1, 1, 1), gs::rgb4(4, 5, 6), 0, 0, 0, 0, 0,
            gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(10, 5, 3), gs::rgb4(5, 5, 6), gs::rgb4(3, 2, 2), gs::rgb4(8, 11, 9),
                           gs::rgb4(12, 11, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 12, 14), gs::rgb4(10, 11, 12), gs::rgb4(13, 14, 14), 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(15, 15, 15), gs::rgb4(7, 8, 9), gs::rgb4(14, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, shadow});
    setPal(vdp, PAL_CRANE, {0, gs::rgb4(8, 9, 10), gs::rgb4(3, 4, 5), gs::rgb4(10, 5, 2), gs::rgb4(14, 14, 12),
                            gs::rgb4(13, 10, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 10, 2), gs::rgb4(3, 3, 3), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, shadow});
    setPal(vdp, PAL_YARD, {0, gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 4), gs::rgb4(3, 6, 2), gs::rgb4(6, 5, 3),
                           gs::rgb4(9, 9, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 15, 14), gs::rgb4(3, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, shadow});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, shadow});

    for (int i = 0; i < kYaws; i++) art.tug[i] = gs::uploadMipped(vdp, paintTug(float(i) * (kTau / float(kYaws))));
    art.platform = gs::uploadMipped(vdp, paintPlatform());
    art.plank = gs::uploadMipped(vdp, paintPlank());
    art.yard = gs::uploadMipped(vdp, paintYard());
    art.pile = gs::uploadMipped(vdp, paintPile());
    art.crane = gs::uploadMipped(vdp, paintCrane());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.gull[0] = gs::uploadMipped(vdp, paintGull(false));
    art.gull[1] = gs::uploadMipped(vdp, paintGull(true));
    art.smoke = gs::uploadMipped(vdp, paintSmoke());
    art.wake = gs::uploadMipped(vdp, paintWake());
    art.shadow = gs::uploadMipped(vdp, paintShadow());
    art.diamond = gs::uploadMipped(vdp, paintDiamond());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.line = gs::uploadMipped(vdp, paintLine());
    loadFont(vdp, art);
}

}  // namespace tugplat
