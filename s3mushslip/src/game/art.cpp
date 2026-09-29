#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace mushslip {
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

// Local +y is north, +x is east. ang is clockwise from north, in radians.
Pt spin(float cx, float cy, float lx, float ly, float c, float s) {
    return {cx + lx * c + ly * s, cy + lx * s - ly * c};
}

Bitmap paintSled(int turn) {
    Bitmap b(84, 84);
    const float cx = 42.f, cy = 42.f;
    const float ang = turn * 3.14159265f / 4.f;
    const float c = std::cos(ang), s = std::sin(ang);
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
        blob(x, y, 2.6f, 4.4f, body);
        blob(x, y + 5.2f, 1.9f, 2.1f, body);
        blob(x - 0.9f, y + 7.0f, 0.5f, 0.55f, 12);
        blob(x + 0.9f, y + 7.0f, 0.5f, 0.55f, 12);
        stroke(x - 1.5f, y - 1.4f, x - 3.2f, y - 4.8f, 8, 1.15f);
        stroke(x + 1.5f, y - 1.4f, x + 3.2f, y - 4.8f, 8, 1.15f);
    };
    stroke(-7.5f, -14.f, -7.5f, 8.f, 2, 2.1f);
    stroke(7.5f, -14.f, 7.5f, 8.f, 2, 2.1f);
    stroke(-7.5f, 6.f, 7.5f, 6.f, 3, 2.0f);
    stroke(-7.5f, -14.f, -3.f, -20.f, 2, 1.8f);
    stroke(7.5f, -14.f, 3.f, -20.f, 2, 1.8f);
    b.poly({spin(cx, cy, -6.5f, -4.f, c, s), spin(cx, cy, 6.5f, -4.f, c, s), spin(cx, cy, 5.5f, 8.f, c, s),
            spin(cx, cy, -5.5f, 8.f, c, s)},
           4);
    blob(0.f, 2.f, 3.4f, 5.0f, 5);
    blob(0.f, -1.2f, 2.4f, 1.8f, 6);
    stroke(0.f, 5.f, -4.6f, 12.f, 9, 1.15f);
    stroke(0.f, 5.f, 4.6f, 12.f, 9, 1.15f);
    dog(-5.5f, 14.f, 7);
    dog(5.5f, 15.f, 10);
    dog(-1.6f, 22.f, 11);
    dog(2.4f, 28.f, 7);
    return b.cropToContent(1);
}

Bitmap paintPost() {
    Bitmap b(10, 28);
    b.rect(3, 4, 4, 22, 1);
    b.rect(1, 2, 8, 4, 2);
    b.rect(4, 24, 2, 3, 3);
    return b;
}

Bitmap paintLamp() {
    Bitmap b(12, 30);
    b.rect(5, 8, 2, 20, 1);
    b.ellipse(6, 5, 4, 4, 2);
    b.ellipse(6, 5, 2, 2, 3);
    return b;
}

Bitmap paintFlag() {
    Bitmap b(20, 28);
    b.rect(2, 2, 2, 24, 1);
    b.poly({{4, 3}, {18, 8}, {4, 14}}, 2);
    b.poly({{4, 6}, {12, 9}, {4, 12}}, 3);
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

void glyphs(gs::VDP& vdp, Art& art) {
    for (int i = 0; i < 96; i++) {
        Bitmap b(5, 7);
        const uint8_t* g = gs::glyph(char(32 + i));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) b.set(x, y, 1);
        art.glyph[i] = gs::uploadImage(vdp, b);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 5), gs::rgb4(8, 12, 14), gs::rgb4(14, 5, 3)});
    setPal(vdp, PAL_TEAM,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(9, 6, 3), gs::rgb4(12, 8, 4), gs::rgb4(10, 4, 3), gs::rgb4(14, 11, 8),
            gs::rgb4(8, 5, 4), gs::rgb4(12, 10, 8), gs::rgb4(5, 4, 3), gs::rgb4(13, 12, 6), gs::rgb4(7, 6, 5),
            gs::rgb4(11, 9, 7), gs::rgb4(2, 2, 2), gs::rgb4(15, 14, 10)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(4, 2, 2), gs::rgb4(8, 3, 3), gs::rgb4(11, 4, 3), gs::rgb4(9, 2, 2), gs::rgb4(13, 8, 7),
            gs::rgb4(7, 3, 3), gs::rgb4(10, 6, 5), gs::rgb4(4, 2, 2), gs::rgb4(14, 10, 4), gs::rgb4(6, 3, 3),
            gs::rgb4(9, 5, 4), gs::rgb4(2, 1, 1), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(11, 7, 3), gs::rgb4(5, 3, 1), gs::rgb4(13, 10, 6)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 4, 5), gs::rgb4(15, 13, 6), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_FLAG, {0, gs::rgb4(6, 4, 2), gs::rgb4(12, 3, 3), gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(12, 15, 10), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 8, 4), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 14, 12), gs::rgb4(14, 6, 4), gs::rgb4(15, 13, 6)});

    for (int i = 0; i < 8; i++) art.sled[i] = gs::uploadMipped(vdp, paintSled(i));
    art.post = gs::uploadMipped(vdp, paintPost());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.flag = gs::uploadMipped(vdp, paintFlag());
    art.puff = gs::uploadMipped(vdp, paintPuff());
    art.title = gs::uploadMipped(vdp, banner("MUSH SLIP", 1));
    art.sub = gs::uploadMipped(vdp, banner("BERTH BEFORE THE TIDE", 2));
    art.berthed = gs::uploadMipped(vdp, banner("BERTHED", 1));
    art.beaten = gs::uploadMipped(vdp, banner("OTHER CREW", 1));
    art.tide = gs::uploadMipped(vdp, banner("TIDE TURNED", 1));
    art.paused = gs::uploadMipped(vdp, banner("PAUSED", 1));
    glyphs(vdp, art);
    vdp.setFogColor(gs::rgb4(6, 8, 10));
}

}  // namespace mushslip
