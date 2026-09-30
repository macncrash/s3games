#include "game/art.h"

#include <initializer_list>
#include <string>

namespace ccler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap ringBmp() {
    gs::Bitmap b(48, 96);
    for (int y = 0; y < 96; y++) {
        for (int x = 0; x < 48; x++) {
            float ny = (float(y) - 47.5f) / 44.f;
            float edge = 8.f + ny * ny * 28.f;
            if (float(x) < edge) continue;
            int rib = (y / 6) & 1;
            int c = rib ? 2 : 1;
            if (float(x) < edge + 3.f) c = 4;
            if (((x * 5 + y * 3) % 37) == 0) c = 3;
            b.set(x, y, c);
        }
    }
    return b;
}

gs::Bitmap mouthBmp() {
    gs::Bitmap b(72, 40);
    for (int y = 0; y < 40; y++) {
        for (int x = 0; x < 72; x++) {
            float nx = (float(x) - 35.5f) / 34.f;
            float ny = (float(y) - 19.5f) / 16.f;
            float r2 = nx * nx + ny * ny;
            if (r2 > 1.f) continue;
            int c = r2 > 0.55f ? 2 : 1;
            if (r2 > 0.88f) c = 3;
            if (y > 30 && r2 < 0.7f) c = 4;
            b.set(x, y, c);
        }
    }
    return b;
}

gs::Bitmap waderBmp(int step) {
    gs::Bitmap b(28, 48);
    b.ellipse(14, 7, 5.f, 5.2f, 3);
    b.rect(11, 11, 6, 3, 3);
    b.set(12, 7, 6);
    b.set(16, 7, 6);
    b.rect(8, 14, 12, 16, 1);
    b.rect(6, 16, 3, 9, 2);
    b.rect(19, 16, 3, 9, 2);
    b.rect(9, 29, 4, 12, 4);
    b.rect(15, 29, 4, 12, 4);
    int lift = step ? 3 : 0;
    b.rect(8, 40 - lift, 6, 4, 5);
    b.rect(14, 40 - (step ? 0 : 3), 6, 4, 5);
    b.rect(20, 22, 6, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap shovelBmp() {
    gs::Bitmap b(18, 32);
    b.line(4, 2, 10, 20, 3, 2.f);
    b.poly({{4, 18}, {16, 20}, {14, 30}, {3, 26}}, 1);
    b.rect(6, 21, 6, 4, 2);
    b.outline(4, false);
    return b;
}

gs::Bitmap siltBmp() {
    gs::Bitmap b(30, 16);
    b.ellipse(15, 10, 13.f, 5.f, 1);
    b.ellipse(10, 8, 6.f, 4.f, 2);
    b.ellipse(20, 9, 5.f, 3.f, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap brickBmp() {
    gs::Bitmap b(26, 14);
    b.rect(2, 3, 22, 8, 1);
    b.rect(2, 3, 22, 2, 2);
    b.line(12, 3, 12, 11, 3, 1.f);
    b.outline(4, false);
    return b;
}

gs::Bitmap branchBmp() {
    gs::Bitmap b(36, 12);
    b.line(2, 8, 32, 3, 2, 2.2f);
    b.line(10, 7, 8, 2, 1, 1.4f);
    b.line(20, 5, 24, 1, 1, 1.4f);
    b.outline(3, false);
    return b;
}

gs::Bitmap tinBmp() {
    gs::Bitmap b(28, 18);
    b.poly({{3, 4}, {24, 2}, {26, 14}, {5, 16}}, 1);
    b.line(6, 8, 22, 6, 2, 1.f);
    b.rect(12, 9, 4, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap clodBmp() {
    gs::Bitmap b(20, 16);
    b.ellipse(10, 9, 8.f, 5.5f, 1);
    b.ellipse(7, 7, 3.f, 2.5f, 2);
    b.set(13, 6, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap wheelBmp() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 9.f, 9.f, 1);
    b.ellipse(11, 11, 4.f, 4.f, 2);
    b.line(11, 3, 11, 19, 3, 1.2f);
    b.line(3, 11, 19, 11, 3, 1.2f);
    b.outline(4, false);
    return b;
}

gs::Bitmap dripBmp() {
    gs::Bitmap b(5, 10);
    b.ellipse(2, 2, 1.6f, 1.8f, 1);
    for (int y = 4; y < 9; y++) b.set(2, y, 2);
    return b;
}

gs::Bitmap puffBmp() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4.f, 3.5f, 1);
    b.ellipse(3, 4, 2.f, 1.6f, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& a) {
    vdp.setFogColor(gs::rgb4(1, 2, 2));
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 9), gs::rgb4(6, 7, 6)});
    setPal(vdp, PAL_PIPE, {0, gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 4), gs::rgb4(3, 5, 3), gs::rgb4(9, 9, 8)});
    setPal(vdp, PAL_MOSS, {0, gs::rgb4(2, 5, 2), gs::rgb4(4, 7, 3), gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(4, 8, 10), gs::rgb4(2, 5, 7), gs::rgb4(8, 11, 12)});
    setPal(vdp, PAL_WADER,
           {0, gs::rgb4(3, 4, 3), gs::rgb4(2, 2, 2), gs::rgb4(10, 8, 6), gs::rgb4(2, 3, 4), gs::rgb4(1, 1, 1),
            gs::rgb4(14, 12, 6), gs::rgb4(8, 7, 4), gs::rgb4(0, 0, 0)});
    setPal(vdp, PAL_SILT, {0, gs::rgb4(7, 6, 3), gs::rgb4(5, 4, 2), gs::rgb4(9, 8, 4), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(10, 4, 3), gs::rgb4(13, 6, 4), gs::rgb4(6, 3, 2), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_TIN, {0, gs::rgb4(9, 9, 8), gs::rgb4(12, 12, 10), gs::rgb4(5, 6, 6), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 14, 7), gs::rgb4(3, 6, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 4), gs::rgb4(8, 2, 2)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(12, 11, 6), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 6), gs::rgb4(4, 6, 5)});

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    a.ring = gs::uploadMipped(vdp, ringBmp());
    a.mouth = gs::uploadMipped(vdp, mouthBmp());
    a.wader[0] = gs::uploadMipped(vdp, waderBmp(0));
    a.wader[1] = gs::uploadMipped(vdp, waderBmp(1));
    a.shovel = gs::uploadMipped(vdp, shovelBmp());
    a.silt = gs::uploadMipped(vdp, siltBmp());
    a.brick = gs::uploadMipped(vdp, brickBmp());
    a.branch = gs::uploadMipped(vdp, branchBmp());
    a.tin = gs::uploadMipped(vdp, tinBmp());
    a.clod = gs::uploadMipped(vdp, clodBmp());
    a.wheel = gs::uploadMipped(vdp, wheelBmp());
    a.drip = gs::uploadMipped(vdp, dripBmp());
    a.puff = gs::uploadMipped(vdp, puffBmp());
    for (int c = 32; c < 127; c++) a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
}

}  // namespace ccler
