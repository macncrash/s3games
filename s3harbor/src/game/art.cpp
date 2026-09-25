#include "game/art.h"

#include <cstdint>

namespace harbor {
namespace {

void pal(gs::VDP& v, int p, int i, int r, int g, int b) { v.setColor(p * 16 + i, gs::rgb4(r, g, b)); }

gs::Bitmap boatBmp() {
    gs::Bitmap b(48, 64);
    b.poly({{24, 1}, {44, 20}, {41, 60}, {7, 60}, {4, 20}}, 1);
    b.poly({{24, 6}, {38, 22}, {36, 54}, {12, 54}, {10, 22}}, 2);
    b.poly({{24, 16}, {32, 26}, {30, 48}, {18, 48}, {16, 26}}, 3);
    b.ellipse(24, 34, 7.5f, 7.5f, 5);
    b.ellipse(24, 34, 4.2f, 4.2f, 4);
    b.rect(22, 14, 4, 18, 5);
    b.rect(21, 12, 6, 4, 4);
    b.rect(20, 44, 8, 7, 5);
    b.rect(21, 42, 6, 3, 6);
    b.line(31, 50, 31, 58, 5, 1);
    b.rect(31, 50, 8, 5, 8);
    for (int i = 0; i < 3; i++) {
        b.rect(14, 28 + i * 6.f, 2, 2, 7);
        b.rect(32, 28 + i * 6.f, 2, 2, 7);
    }
    b.rect(16, 56, 16, 2, 9);
    b.line(24, 3, 24, 12, 7, 1);
    b.outline(5, false);
    return b;
}

gs::Bitmap fortBmp() {
    gs::Bitmap f(56, 48);
    f.poly({{4, 12}, {38, 8}, {46, 18}, {42, 40}, {8, 42}, {2, 22}}, 2);
    f.poly({{10, 16}, {34, 13}, {38, 22}, {34, 34}, {12, 36}, {8, 24}}, 1);
    f.poly({{14, 18}, {30, 16}, {28, 24}, {16, 26}}, 3);
    f.rect(12, 12, 6, 4, 3);
    f.rect(22, 10, 6, 4, 3);
    f.rect(32, 12, 6, 4, 8);
    f.rect(32, 22, 8, 8, 5);
    f.rect(38, 24, 14, 4, 4);
    f.rect(50, 23, 4, 6, 5);
    f.line(16, 6, 16, 16, 5, 1);
    f.rect(16, 4, 9, 5, 6);
    f.rect(10, 36, 12, 3, 7);
    f.ellipse(22, 28, 2, 2, 5);
    f.outline(5, false);
    return f;
}

gs::Bitmap rubbleBmp() {
    gs::Bitmap r(40, 24);
    r.ellipse(20, 15, 16, 6, 2);
    r.rect(6, 12, 9, 6, 1);
    r.rect(16, 9, 10, 7, 3);
    r.rect(27, 13, 7, 5, 2);
    r.rect(20, 6, 3, 7, 5);
    r.rect(30, 8, 4, 3, 4);
    return r;
}

gs::Bitmap towerBmp() {
    gs::Bitmap t(24, 72);
    t.rect(4, 64, 16, 6, 8);
    t.rect(8, 22, 8, 44, 5);
    t.rect(8, 28, 8, 5, 6);
    t.rect(8, 40, 8, 5, 6);
    t.rect(8, 52, 8, 5, 6);
    t.rect(5, 16, 14, 8, 8);
    t.rect(8, 6, 8, 12, 7);
    t.ellipse(12, 10, 2.2f, 2.2f, 1);
    t.poly({{12, 4}, {20, 12}, {12, 10}}, 1);
    t.outline(8, false);
    return t;
}

gs::Bitmap shellBmp(int body, int hot) {
    gs::Bitmap s(8, 8);
    s.ellipse(4, 4, 3, 3, body);
    s.ellipse(3, 3, 1.2f, 1.2f, hot);
    return s;
}

gs::Bitmap burstBmp() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 2);
    b.ellipse(14, 14, 7, 7, 1);
    b.ellipse(14, 14, 3, 3, 4);
    b.line(14, 1, 14, 8, 1, 2);
    b.line(14, 20, 14, 27, 1, 2);
    b.line(1, 14, 8, 14, 1, 2);
    b.line(20, 14, 27, 14, 1, 2);
    b.line(4, 4, 9, 9, 3, 2);
    b.line(24, 4, 19, 9, 3, 2);
    return b;
}

gs::Bitmap splashBmp() {
    gs::Bitmap s(26, 16);
    s.ellipse(13, 10, 12, 5, 5);
    s.ellipse(13, 9, 6, 2.5f, 4);
    s.line(13, 1, 13, 6, 4, 1);
    s.line(5, 4, 9, 8, 4, 1);
    s.line(21, 4, 17, 8, 4, 1);
    return s;
}

gs::Bitmap puffBmp() {
    gs::Bitmap p(18, 18);
    p.ellipse(9, 11, 7, 5, 6);
    p.ellipse(7, 8, 4, 3, 7);
    p.ellipse(12, 9, 3, 2.5f, 6);
    return p;
}

gs::Bitmap flashBmp() {
    gs::Bitmap f(12, 12);
    f.ellipse(6, 6, 5, 5, 1);
    f.ellipse(6, 6, 2, 2, 4);
    return f;
}

gs::Bitmap wakeBmp() {
    gs::Bitmap w(36, 18);
    w.poly({{18, 2}, {6, 16}, {14, 12}, {18, 5}, {22, 12}, {30, 16}}, 4);
    w.line(18, 3, 8, 16, 5, 1);
    w.line(18, 3, 28, 16, 5, 1);
    return w;
}

gs::Bitmap reticleBmp() {
    gs::Bitmap r(24, 24);
    r.rect(1, 1, 8, 2, 1);
    r.rect(1, 1, 2, 8, 1);
    r.rect(15, 1, 8, 2, 1);
    r.rect(21, 1, 2, 8, 1);
    r.rect(1, 21, 8, 2, 1);
    r.rect(1, 15, 2, 8, 1);
    r.rect(15, 21, 8, 2, 1);
    r.rect(21, 15, 2, 8, 1);
    return r;
}

gs::Bitmap buoyBmp() {
    gs::Bitmap b(14, 18);
    b.ellipse(7, 9, 6, 6, 3);
    b.rect(3, 8, 8, 3, 4);
    b.rect(6, 2, 2, 6, 5);
    b.ellipse(7, 15, 3, 1.5f, 5);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap L(12, 24);
    L.rect(5, 8, 2, 14, 1);
    L.rect(2, 20, 8, 3, 1);
    L.ellipse(6, 6, 4, 3.5f, 2);
    L.ellipse(6, 6, 1.5f, 1.5f, 4);
    return L;
}

gs::Bitmap sunBmp() {
    gs::Bitmap s(32, 32);
    s.ellipse(16, 16, 14, 14, 2);
    s.ellipse(16, 16, 8, 8, 1);
    return s;
}

gs::Bitmap cloudBmp() {
    gs::Bitmap c(44, 20);
    c.ellipse(14, 12, 10, 6, 3);
    c.ellipse(24, 9, 12, 7, 3);
    c.ellipse(32, 12, 8, 5, 4);
    return c;
}

gs::Bitmap plateBmp() {
    gs::Bitmap p(8, 8);
    p.rect(0, 0, 8, 8, 1);
    return p;
}

void glyphs(gs::VDP& vdp, Art& art) {
    for (int c = 32; c < 96; c++) {
        gs::Bitmap b(14, 16);
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) b.rect(float(x * 2 + 2), float(y * 2 + 1), 2, 2, 1);
        b.outline(2, false);
        art.glyph[c] = gs::uploadImage(vdp, b);
    }
    art.cellW = 16;
    art.cellH = 16;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    pal(vdp, PAL_TEXT, 1, 15, 15, 15);
    pal(vdp, PAL_TEXT, 2, 1, 1, 3);
    pal(vdp, PAL_AMBER, 1, 15, 12, 4);
    pal(vdp, PAL_AMBER, 2, 3, 1, 0);
    pal(vdp, PAL_RED, 1, 15, 5, 3);
    pal(vdp, PAL_RED, 2, 3, 0, 0);
    pal(vdp, PAL_DIM, 1, 8, 12, 12);
    pal(vdp, PAL_DIM, 2, 1, 2, 3);

    pal(vdp, PAL_BOAT, 1, 3, 4, 4);
    pal(vdp, PAL_BOAT, 2, 5, 6, 6);
    pal(vdp, PAL_BOAT, 3, 9, 7, 3);
    pal(vdp, PAL_BOAT, 4, 13, 10, 4);
    pal(vdp, PAL_BOAT, 5, 2, 2, 2);
    pal(vdp, PAL_BOAT, 6, 10, 3, 2);
    pal(vdp, PAL_BOAT, 7, 14, 14, 13);
    pal(vdp, PAL_BOAT, 8, 13, 2, 2);
    pal(vdp, PAL_BOAT, 9, 2, 6, 7);

    pal(vdp, PAL_FORT, 1, 7, 6, 5);
    pal(vdp, PAL_FORT, 2, 4, 4, 4);
    pal(vdp, PAL_FORT, 3, 10, 9, 7);
    pal(vdp, PAL_FORT, 4, 3, 3, 3);
    pal(vdp, PAL_FORT, 5, 1, 1, 1);
    pal(vdp, PAL_FORT, 6, 12, 2, 2);
    pal(vdp, PAL_FORT, 7, 3, 5, 3);
    pal(vdp, PAL_FORT, 8, 8, 7, 5);

    pal(vdp, PAL_LIT, 1, 10, 9, 7);
    pal(vdp, PAL_LIT, 2, 6, 5, 4);
    pal(vdp, PAL_LIT, 3, 14, 12, 8);
    pal(vdp, PAL_LIT, 4, 14, 11, 4);
    pal(vdp, PAL_LIT, 5, 2, 1, 1);
    pal(vdp, PAL_LIT, 6, 15, 3, 2);
    pal(vdp, PAL_LIT, 7, 4, 7, 4);
    pal(vdp, PAL_LIT, 8, 12, 10, 6);

    pal(vdp, PAL_SHOT, 1, 5, 5, 6);
    pal(vdp, PAL_SHOT, 2, 12, 12, 13);
    pal(vdp, PAL_SHOT, 3, 14, 7, 2);
    pal(vdp, PAL_SHOT, 4, 15, 14, 6);

    pal(vdp, PAL_FX, 1, 15, 14, 6);
    pal(vdp, PAL_FX, 2, 14, 7, 2);
    pal(vdp, PAL_FX, 3, 12, 4, 1);
    pal(vdp, PAL_FX, 4, 14, 15, 15);
    pal(vdp, PAL_FX, 5, 6, 12, 13);
    pal(vdp, PAL_FX, 6, 8, 8, 8);
    pal(vdp, PAL_FX, 7, 4, 4, 5);

    pal(vdp, PAL_QUAY, 1, 5, 4, 3);
    pal(vdp, PAL_QUAY, 2, 14, 12, 5);
    pal(vdp, PAL_QUAY, 3, 12, 2, 2);
    pal(vdp, PAL_QUAY, 4, 14, 14, 14);
    pal(vdp, PAL_QUAY, 5, 3, 3, 4);

    pal(vdp, PAL_SKY, 1, 15, 15, 10);
    pal(vdp, PAL_SKY, 2, 15, 12, 5);
    pal(vdp, PAL_SKY, 3, 12, 12, 13);
    pal(vdp, PAL_SKY, 4, 8, 8, 11);
    pal(vdp, PAL_SKY, 5, 13, 13, 12);
    pal(vdp, PAL_SKY, 6, 12, 2, 2);
    pal(vdp, PAL_SKY, 7, 15, 14, 8);
    pal(vdp, PAL_SKY, 8, 6, 6, 6);

    pal(vdp, PAL_DEAD, 1, 1, 2, 3);
    pal(vdp, PAL_DEAD, 2, 4, 4, 5);

    pal(vdp, PAL_ROAD, 1, 7, 7, 6);
    pal(vdp, PAL_ROAD, 2, 4, 4, 3);
    pal(vdp, PAL_ROAD, 3, 8, 7, 5);
    pal(vdp, PAL_ROAD, 4, 5, 5, 4);
    pal(vdp, PAL_ROAD, 5, 3, 3, 3);
    pal(vdp, PAL_ROAD, 6, 2, 4, 6);
    pal(vdp, PAL_ROAD, 7, 1, 3, 5);
    pal(vdp, PAL_ROAD, 8, 7, 6, 4);
    pal(vdp, PAL_ROAD, 9, 3, 5, 7);
    pal(vdp, PAL_ROAD, 10, 4, 7, 8);
    pal(vdp, PAL_ROAD, 11, 1, 4, 7);
    pal(vdp, PAL_ROAD, 12, 2, 6, 9);
    pal(vdp, PAL_ROAD, 13, 4, 9, 12);
    pal(vdp, PAL_ROAD, 14, 9, 8, 6);
    pal(vdp, PAL_ROAD, 15, 6, 11, 13);

    vdp.setFogColor(gs::rgb4(6, 7, 8));

    art.boat = gs::uploadMipped(vdp, boatBmp());
    art.fort = gs::uploadMipped(vdp, fortBmp());
    art.rubble = gs::uploadMipped(vdp, rubbleBmp());
    art.tower = gs::uploadMipped(vdp, towerBmp());
    art.shell = gs::uploadImage(vdp, shellBmp(1, 2));
    art.hot = gs::uploadImage(vdp, shellBmp(3, 4));
    art.burst = gs::uploadImage(vdp, burstBmp());
    art.splash = gs::uploadImage(vdp, splashBmp());
    art.puff = gs::uploadImage(vdp, puffBmp());
    art.flash = gs::uploadImage(vdp, flashBmp());
    art.reticle = gs::uploadImage(vdp, reticleBmp());
    art.wake = gs::uploadImage(vdp, wakeBmp());
    art.buoy = gs::uploadImage(vdp, buoyBmp());
    art.lamp = gs::uploadImage(vdp, lampBmp());
    art.sun = gs::uploadImage(vdp, sunBmp());
    art.cloud = gs::uploadImage(vdp, cloudBmp());
    art.plate = gs::uploadImage(vdp, plateBmp());
    glyphs(vdp, art);
}

}  // namespace harbor
