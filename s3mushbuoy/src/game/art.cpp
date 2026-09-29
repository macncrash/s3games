#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

namespace mush {
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
    vdp.setColor(pal * 16 + 0, 0);
}

Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + lx * c - ly * s, cy + lx * s + ly * c};
}

Bitmap paintSled(float heading) {
    Bitmap b(72, 72);
    const float cx = 36.f, cy = 36.f;
    const float c = std::cos(heading), s = std::sin(heading);
    auto blob = [&](float lx, float ly, float rx, float ry, int col) {
        Pt p = spin(cx, cy, lx, ly, c, s);
        b.ellipse(p.first, p.second, rx, ry, col);
    };
    auto stroke = [&](float ax, float ay, float bx, float by, int col, float th) {
        Pt A = spin(cx, cy, ax, ay, c, s);
        Pt B = spin(cx, cy, bx, by, c, s);
        b.line(A.first, A.second, B.first, B.second, col, th);
    };
    auto dog = [&](float x, float y, int body) {
        blob(x, y, 3.2f, 5.4f, body);
        blob(x, y - 6.2f, 2.4f, 2.6f, body);
        blob(x - 1.2f, y - 8.2f, 0.7f, 0.8f, 12);
        blob(x + 1.2f, y - 8.2f, 0.7f, 0.8f, 12);
        stroke(x - 2.f, y + 2.f, x - 4.f, y + 6.f, 8, 1.3f);
        stroke(x + 2.f, y + 2.f, x + 4.f, y + 6.f, 8, 1.3f);
    };
    // Nose is negative local Y (up the bitmap before the heading spin).
    stroke(-9.f, 10.f, -9.f, -14.f, 2, 2.4f);
    stroke(9.f, 10.f, 9.f, -14.f, 2, 2.4f);
    stroke(-9.f, -12.f, 9.f, -12.f, 3, 2.2f);
    stroke(-9.f, 10.f, -4.f, 16.f, 2, 2.2f);
    stroke(9.f, 10.f, 4.f, 16.f, 2, 2.2f);
    b.poly({spin(cx, cy, -8.f, 4.f, c, s), spin(cx, cy, 8.f, 4.f, c, s), spin(cx, cy, 7.f, -8.f, c, s),
            spin(cx, cy, -7.f, -8.f, c, s)},
           4);
    blob(0.f, -2.f, 4.2f, 6.f, 5);
    blob(0.f, 2.f, 3.2f, 2.4f, 6);
    blob(-1.f, 3.2f, 0.5f, 0.5f, 13);
    blob(1.f, 3.2f, 0.5f, 0.5f, 13);
    stroke(0.f, -6.f, -6.f, -14.f, 9, 1.3f);
    stroke(0.f, -6.f, 6.f, -14.f, 9, 1.3f);
    dog(-5.5f, -16.f, 7);
    dog(5.5f, -18.f, 10);
    dog(0.f, -26.f, 11);
    return b.cropToContent(1);
}

Bitmap paintBuoy() {
    Bitmap b(28, 40);
    b.ellipse(14, 34, 8, 3, 2);
    b.poly({{14, 4}, {24, 30}, {4, 30}}, 1);
    b.rect(8, 14, 12, 6, 3);
    b.rect(9, 22, 10, 4, 4);
    b.ellipse(14, 8, 3, 3, 5);
    return b;
}

Bitmap paintDock() {
    Bitmap b(64, 48);
    b.rect(4, 8, 56, 28, 1);
    for (int i = 0; i < 7; i++) b.rect(6, 10 + i * 4, 52, 1, (i & 1) ? 2 : 3);
    b.rect(0, 6, 8, 32, 4);
    b.rect(56, 6, 8, 32, 4);
    b.rect(28, 0, 8, 10, 5);
    b.rect(30, 0, 4, 6, 6);
    return b;
}

Bitmap paintShed() {
    Bitmap b(48, 36);
    b.poly({{4, 20}, {24, 4}, {44, 20}}, 1);
    b.rect(8, 18, 32, 14, 2);
    b.rect(20, 22, 8, 10, 3);
    b.rect(12, 22, 6, 5, 4);
    return b;
}

Bitmap paintTree() {
    Bitmap b(28, 36);
    b.rect(12, 22, 4, 12, 1);
    b.poly({{14, 2}, {26, 24}, {2, 24}}, 2);
    b.poly({{14, 8}, {22, 20}, {6, 20}}, 3);
    return b;
}

Bitmap paintPuff() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 4, 1);
    b.ellipse(5, 7, 3, 2, 2);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(12, 28);
    b.rect(5, 8, 2, 18, 1);
    b.ellipse(6, 6, 4, 4, 2);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(20, 28);
    b.rect(3, 2, 2, 24, 1);
    b.poly({{5, 3}, {18, 8}, {5, 14}}, 2);
    return b;
}

Bitmap banner(const std::string& s, int color) {
    return gs::textBitmap(s, gs::TextStyle{2, color, 0, 0, 1});
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 13, 6), gs::rgb4(8, 10, 12), gs::rgb4(14, 4, 3),
                          gs::rgb4(4, 13, 8), gs::rgb4(6, 14, 15), gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_TEAM, {0,
                           gs::rgb4(6, 4, 3),
                           gs::rgb4(8, 6, 4),
                           gs::rgb4(10, 8, 5),
                           gs::rgb4(12, 9, 5),
                           gs::rgb4(4, 6, 10),
                           gs::rgb4(13, 11, 8),
                           gs::rgb4(10, 8, 6),
                           gs::rgb4(5, 4, 3),
                           gs::rgb4(9, 7, 4),
                           gs::rgb4(12, 10, 7),
                           gs::rgb4(7, 6, 5),
                           gs::rgb4(2, 2, 2),
                           gs::rgb4(15, 15, 14),
                           gs::rgb4(14, 6, 3),
                           gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_CREW, {0,
                           gs::rgb4(3, 4, 6),
                           gs::rgb4(4, 5, 7),
                           gs::rgb4(5, 6, 8),
                           gs::rgb4(6, 7, 9),
                           gs::rgb4(3, 5, 8),
                           gs::rgb4(8, 9, 10),
                           gs::rgb4(5, 6, 7),
                           gs::rgb4(2, 3, 4),
                           gs::rgb4(4, 5, 6),
                           gs::rgb4(7, 8, 9),
                           gs::rgb4(4, 5, 6),
                           gs::rgb4(1, 1, 2),
                           gs::rgb4(12, 13, 14),
                           gs::rgb4(8, 4, 3),
                           gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(13, 2, 2), gs::rgb4(4, 4, 5), gs::rgb4(15, 15, 14), gs::rgb4(8, 1, 1),
                          gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(2, 10, 4), gs::rgb4(4, 4, 5), gs::rgb4(15, 15, 14), gs::rgb4(1, 6, 2),
                            gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(13, 8, 1), gs::rgb4(4, 4, 5), gs::rgb4(15, 15, 14), gs::rgb4(8, 5, 1),
                            gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(8, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(10, 8, 4), gs::rgb4(5, 4, 3),
                           gs::rgb4(12, 10, 6), gs::rgb4(14, 3, 2)});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(14, 15, 15), gs::rgb4(11, 13, 14)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(6, 4, 2), gs::rgb4(2, 7, 3), gs::rgb4(4, 10, 4)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 10), gs::rgb4(8, 12, 15), gs::rgb4(15, 8, 3)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(12, 15, 10), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(8, 3, 2), gs::rgb4(10, 8, 5), gs::rgb4(4, 3, 2), gs::rgb4(13, 12, 8)});
    setPal(vdp, PAL_ICE, {0, gs::rgb4(9, 12, 13), gs::rgb4(7, 10, 12)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(7, 5, 3), gs::rgb4(13, 3, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 5, 6), gs::rgb4(15, 13, 5)});

    for (int i = 0; i < 8; i++) {
        // Frame 0 faces east. Spin puts the nose on that heading.
        float face = 1.5707963f - float(i) * 0.78539816f;
        art.sled[i] = gs::uploadMipped(vdp, paintSled(face));
    }
    art.buoy = gs::uploadMipped(vdp, paintBuoy());
    art.dock = gs::uploadMipped(vdp, paintDock());
    art.shed = gs::uploadMipped(vdp, paintShed());
    art.tree = gs::uploadMipped(vdp, paintTree());
    art.puff = gs::uploadMipped(vdp, paintPuff());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.title = gs::uploadMipped(vdp, banner("MUSH BUOY", 1));
    art.sub = gs::uploadMipped(vdp, banner("ROUND THEM AND BEAT THE CREW", 2));
    art.home = gs::uploadMipped(vdp, banner("SAME DOCK", 1));
    art.beat = gs::uploadMipped(vdp, banner("CREW BEATEN", 2));
    art.missed = gs::uploadMipped(vdp, banner("CREW TOOK IT", 1));
    art.clock = gs::uploadMipped(vdp, banner("OTHER CREW", 2));
    art.paused = gs::uploadMipped(vdp, banner("HELD", 3));
    for (int i = 0; i < 96; i++) {
        std::string s(1, char(32 + i));
        art.glyph[i] = gs::uploadMipped(vdp, gs::textBitmap(s, gs::TextStyle{1, 1, 0, 0, 0}));
    }
}

}  // namespace mush
