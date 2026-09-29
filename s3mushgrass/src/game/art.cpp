#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace mushgrass {
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
    Bitmap b(80, 80);
    const float cx = 40.f, cy = 40.f;
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
        blob(x, y, 2.8f, 4.6f, body);
        blob(x, y - 5.4f, 2.0f, 2.2f, body);
        blob(x - 1.0f, y - 7.2f, 0.55f, 0.6f, 12);
        blob(x + 1.0f, y - 7.2f, 0.55f, 0.6f, 12);
        stroke(x - 1.6f, y + 1.6f, x - 3.4f, y + 5.2f, 8, 1.2f);
        stroke(x + 1.6f, y + 1.6f, x + 3.4f, y + 5.2f, 8, 1.2f);
    };
    stroke(-8.f, 12.f, -8.f, -10.f, 2, 2.2f);
    stroke(8.f, 12.f, 8.f, -10.f, 2, 2.2f);
    stroke(-8.f, -8.f, 8.f, -8.f, 3, 2.0f);
    stroke(-8.f, 12.f, -3.f, 18.f, 2, 2.0f);
    stroke(8.f, 12.f, 3.f, 18.f, 2, 2.0f);
    b.poly({spin(cx, cy, -7.f, 6.f, c, s), spin(cx, cy, 7.f, 6.f, c, s), spin(cx, cy, 6.f, -6.f, c, s),
            spin(cx, cy, -6.f, -6.f, c, s)},
           4);
    blob(0.f, 0.f, 3.6f, 5.2f, 5);
    blob(0.f, 3.f, 2.6f, 2.0f, 6);
    stroke(0.f, -4.f, -5.f, -12.f, 9, 1.2f);
    stroke(0.f, -4.f, 5.f, -12.f, 9, 1.2f);
    dog(-6.f, -14.f, 7);
    dog(6.f, -15.f, 10);
    dog(-2.f, -22.f, 11);
    dog(3.f, -28.f, 7);
    return b.cropToContent(1);
}

Bitmap paintTuft() {
    Bitmap b(18, 16);
    b.poly({{9, 1}, {14, 15}, {4, 15}}, 1);
    b.poly({{6, 3}, {10, 15}, {2, 15}}, 2);
    b.poly({{12, 4}, {16, 15}, {8, 15}}, 3);
    return b;
}

Bitmap paintTree() {
    Bitmap b(26, 34);
    b.rect(11, 22, 4, 11, 1);
    b.poly({{13, 2}, {24, 24}, {2, 24}}, 2);
    b.poly({{13, 8}, {20, 20}, {6, 20}}, 3);
    return b;
}

Bitmap paintCabin() {
    Bitmap b(44, 32);
    b.poly({{2, 16}, {22, 3}, {42, 16}}, 1);
    b.rect(6, 15, 32, 14, 2);
    b.rect(18, 20, 8, 9, 3);
    b.rect(10, 19, 5, 4, 4);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(18, 26);
    b.rect(2, 2, 2, 22, 1);
    b.poly({{4, 3}, {16, 8}, {4, 13}}, 2);
    return b;
}

Bitmap paintPost() {
    Bitmap b(8, 22);
    b.rect(3, 2, 2, 18, 1);
    b.ellipse(4, 3, 3, 3, 2);
    return b;
}

Bitmap paintPuff() {
    Bitmap b(14, 12);
    b.ellipse(7, 6, 5, 3, 1);
    b.ellipse(4, 5, 2, 2, 2);
    return b;
}

Bitmap banner(const std::string& s, int color) {
    return gs::textBitmap(s, gs::TextStyle{2, color, 0, 0, 1});
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 13, 6), gs::rgb4(6, 10, 8), gs::rgb4(14, 4, 3)});
    setPal(vdp, PAL_TEAM, {0, gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(11, 8, 4), gs::rgb4(13, 10, 6),
                           gs::rgb4(3, 5, 10), gs::rgb4(14, 12, 9), gs::rgb4(10, 7, 4), gs::rgb4(5, 3, 2),
                           gs::rgb4(8, 6, 3), gs::rgb4(12, 9, 5), gs::rgb4(7, 5, 3), gs::rgb4(2, 2, 2),
                           gs::rgb4(15, 15, 13), gs::rgb4(13, 5, 2), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(3, 4, 5), gs::rgb4(4, 5, 6), gs::rgb4(5, 6, 7), gs::rgb4(6, 7, 8),
                           gs::rgb4(2, 4, 7), gs::rgb4(8, 9, 10), gs::rgb4(5, 5, 6), gs::rgb4(2, 2, 3),
                           gs::rgb4(4, 4, 5), gs::rgb4(7, 7, 8), gs::rgb4(3, 4, 5), gs::rgb4(1, 1, 2),
                           gs::rgb4(12, 13, 14), gs::rgb4(6, 3, 2), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_GRASS, {0, gs::rgb4(2, 9, 3), gs::rgb4(3, 11, 4), gs::rgb4(1, 6, 2)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(7, 5, 3), gs::rgb4(14, 12, 4)});
    setPal(vdp, PAL_CABIN, {0, gs::rgb4(8, 3, 2), gs::rgb4(10, 7, 4), gs::rgb4(4, 3, 2), gs::rgb4(13, 12, 8)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(6, 4, 2), gs::rgb4(1, 6, 2), gs::rgb4(3, 9, 3)});
    setPal(vdp, PAL_SNOW, {0, gs::rgb4(14, 15, 15), gs::rgb4(11, 13, 14)});
    setPal(vdp, PAL_BANNER, {0, gs::rgb4(15, 14, 8), gs::rgb4(10, 14, 8), gs::rgb4(15, 8, 3)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(12, 15, 8), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_TUFT, {0, gs::rgb4(2, 8, 2), gs::rgb4(4, 12, 3), gs::rgb4(1, 5, 1)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(8, 6, 3), gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_PUFF, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 14, 14)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(6, 10, 14)});
    setPal(vdp, PAL_DOG, {0, gs::rgb4(10, 7, 3)});

    for (int i = 0; i < 8; i++) {
        float face = 1.5707963f - float(i) * 0.78539816f;
        art.sled[i] = gs::uploadMipped(vdp, paintSled(face));
    }
    art.tuft = gs::uploadMipped(vdp, paintTuft());
    art.tree = gs::uploadMipped(vdp, paintTree());
    art.cabin = gs::uploadMipped(vdp, paintCabin());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.post = gs::uploadMipped(vdp, paintPost());
    art.puff = gs::uploadMipped(vdp, paintPuff());
    art.title = gs::uploadMipped(vdp, banner("MUSH GRASS", 1));
    art.sub = gs::uploadMipped(vdp, banner("LAND AND FULL STOP", 2));
    art.landed = gs::uploadMipped(vdp, banner("FULL STOP", 1));
    art.beaten = gs::uploadMipped(vdp, banner("CREW STILL OUT", 2));
    art.lost = gs::uploadMipped(vdp, banner("CREW TOOK THE GRASS", 1));
    art.paused = gs::uploadMipped(vdp, banner("HELD", 3));
    for (int i = 0; i < 96; i++) {
        std::string s(1, char(32 + i));
        art.glyph[i] = gs::uploadMipped(vdp, gs::textBitmap(s, gs::TextStyle{1, 1, 0, 0, 0}));
    }
}

}  // namespace mushgrass
