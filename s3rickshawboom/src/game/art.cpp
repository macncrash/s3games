#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace rickboom {
namespace {

void pal(gs::VDP& v, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        v.setColor(p * 16 + i, c);
        i++;
        if (i >= 16) break;
    }
}

gs::Bitmap banner(const char* s, int scale, int ink, int edge) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = ink;
    st.outline = edge;
    st.spacing = 1;
    return gs::textBitmap(s, st);
}

void wheel(gs::Bitmap& b, int cx, int cy, int frame) {
    b.ellipse(float(cx), float(cy), 10, 10, 1);
    b.ellipse(float(cx), float(cy), 7, 7, 2);
    b.ellipse(float(cx), float(cy), 2, 2, 3);
    float a = frame ? 0.8f : 0.1f;
    for (int k = 0; k < 4; k++) {
        float ang = a + k * 1.57f;
        b.line(float(cx), float(cy), float(cx) + std::cos(ang) * 8.f, float(cy) + std::sin(ang) * 8.f, 3, 1.2f);
    }
}

gs::Bitmap cabFrame(int frame) {
    gs::Bitmap b(112, 64);
    wheel(b, 28, 50, frame);
    wheel(b, 86, 50, frame);
    b.rect(18, 42, 74, 4, 4);
    b.line(28, 42, 38, 24, 4, 2.2f);
    b.line(86, 42, 76, 24, 4, 2.2f);
    b.poly({{36, 42}, {40, 16}, {92, 14}, {96, 42}}, 5);
    b.poly({{44, 20}, {84, 18}, {82, 34}, {46, 36}}, 6);
    b.rect(58, 30, 16, 8, 7);
    b.ellipse(66, 26, 5, 5, 8);
    b.rect(60, 30, 10, 6, 9);
    b.line(40, 16, 22, 8, 5, 2.f);
    b.line(92, 14, 104, 8, 5, 2.f);
    b.line(22, 8, 104, 8, 5, 2.4f);
    b.ellipse(18, 28, 4.5f, 4.5f, 8);
    b.rect(12, 32, 10, 8, 10);
    if (frame == 0) {
        b.line(16, 38, 6, 48, 10, 2.f);
        b.line(20, 38, 24, 48, 10, 2.f);
    } else {
        b.line(16, 38, 10, 50, 10, 2.f);
        b.line(20, 38, 28, 46, 10, 2.f);
    }
    b.rect(4, 36, 10, 3, 4);
    b.ellipse(4, 37, 3, 3, 11);
    return b;
}

gs::Bitmap shadeBmp() {
    gs::Bitmap b(48, 12);
    b.ellipse(24, 6, 22, 4, 1);
    return b;
}

gs::Bitmap driveBmp() {
    gs::Bitmap b(28, 22);
    b.rect(1, 3, 26, 16, 1);
    b.line(1, 3, 27, 19, 2, 1.2f);
    b.line(27, 3, 1, 19, 2, 1.2f);
    b.rect(8, 0, 12, 5, 3);
    b.rect(10, 7, 8, 6, 4);
    return b;
}

gs::Bitmap boomBmp() {
    gs::Bitmap b(48, 16);
    b.rect(0, 6, 48, 6, 1);
    for (int x = 2; x < 46; x += 6) b.rect(float(x), 6, 2, 6, 2);
    b.rect(0, 4, 48, 2, 3);
    return b;
}

gs::Bitmap postBmp() {
    gs::Bitmap b(10, 36);
    b.rect(4, 4, 2, 30, 1);
    b.rect(1, 0, 8, 6, 2);
    b.line(1, 6, 9, 34, 3, 1.1f);
    return b;
}

gs::Bitmap hookBmp() {
    gs::Bitmap b(12, 18);
    b.rect(5, 0, 2, 8, 1);
    b.ellipse(6, 12, 4, 4, 2);
    b.rect(8, 10, 3, 2, 1);
    return b;
}

gs::Bitmap stallBmp() {
    gs::Bitmap b(32, 48);
    b.rect(2, 10, 28, 36, 1);
    b.rect(6, 16, 8, 12, 2);
    b.rect(18, 16, 8, 18, 3);
    b.rect(4, 36, 24, 8, 4);
    b.rect(0, 6, 32, 6, 5);
    return b;
}

gs::Bitmap awningBmp() {
    gs::Bitmap b(40, 16);
    for (int x = 0; x < 40; x++) b.rect(float(x), 2, 1, 12, (x / 5) & 1 ? 1 : 2);
    b.rect(0, 0, 40, 3, 3);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(10, 28);
    b.rect(4, 10, 2, 18, 1);
    b.ellipse(5, 7, 4, 5, 2);
    b.ellipse(5, 7, 2, 2, 3);
    return b;
}

gs::Bitmap crateBmp() {
    gs::Bitmap b(16, 14);
    b.rect(1, 1, 14, 12, 1);
    b.line(1, 1, 15, 13, 2, 1);
    b.line(15, 1, 1, 13, 2, 1);
    return b;
}

gs::Bitmap solid(int c) {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, c);
    return b;
}

gs::Bitmap clockBmp() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 9, 9, 2);
    b.ellipse(14, 14, 2, 2, 3);
    b.line(14, 14, 14, 6, 3, 1.4f);
    b.line(14, 14, 20, 16, 4, 1.4f);
    for (int k = 0; k < 12; k++) {
        float a = k * 0.5236f;
        b.set(14 + int(std::cos(a) * 10), 14 + int(std::sin(a) * 10), 3);
    }
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    vdp.setFogColor(rgb4(2, 1, 3));
    pal(vdp, PAL_HUD, {rgb4(0, 0, 0), rgb4(15, 14, 10), rgb4(3, 2, 2), rgb4(12, 8, 3)});
    pal(vdp, PAL_CAB,
        {rgb4(0, 0, 0), rgb4(1, 1, 1), rgb4(4, 4, 5), rgb4(10, 8, 4), rgb4(9, 3, 2), rgb4(13, 7, 2), rgb4(12, 10, 6),
         rgb4(6, 3, 2), rgb4(13, 9, 6), rgb4(10, 3, 4), rgb4(2, 3, 8), rgb4(14, 13, 8)});
    pal(vdp, PAL_STREET,
        {rgb4(0, 0, 0), rgb4(6, 5, 5), rgb4(3, 5, 8), rgb4(9, 6, 4), rgb4(4, 3, 3), rgb4(12, 4, 3)});
    pal(vdp, PAL_BOOM, {rgb4(0, 0, 0), rgb4(11, 11, 12), rgb4(6, 7, 8), rgb4(14, 12, 4), rgb4(8, 3, 2)});
    pal(vdp, PAL_DRIVE, {rgb4(0, 0, 0), rgb4(12, 8, 3), rgb4(6, 4, 2), rgb4(14, 12, 6), rgb4(3, 6, 10)});
    pal(vdp, PAL_BANNER, {rgb4(0, 0, 0), rgb4(15, 13, 6), rgb4(2, 1, 3)});
    pal(vdp, PAL_ALERT, {rgb4(0, 0, 0), rgb4(15, 6, 4), rgb4(3, 1, 1)});
    pal(vdp, PAL_WIN, {rgb4(0, 0, 0), rgb4(8, 15, 8), rgb4(1, 3, 2)});
    pal(vdp, PAL_CREW, {rgb4(0, 0, 0), rgb4(12, 9, 4), rgb4(15, 14, 10), rgb4(2, 2, 3), rgb4(14, 4, 3)});
    pal(vdp, PAL_LAMP, {rgb4(0, 0, 0), rgb4(5, 4, 3), rgb4(15, 13, 5), rgb4(15, 15, 12)});

    art.cab[0] = gs::uploadMipped(vdp, cabFrame(0));
    art.cab[1] = gs::uploadMipped(vdp, cabFrame(1));
    art.shade = gs::uploadMipped(vdp, shadeBmp());
    art.drive = gs::uploadMipped(vdp, driveBmp());
    art.boom = gs::uploadMipped(vdp, boomBmp());
    art.post = gs::uploadMipped(vdp, postBmp());
    art.hook = gs::uploadMipped(vdp, hookBmp());
    art.stall = gs::uploadMipped(vdp, stallBmp());
    art.awning = gs::uploadMipped(vdp, awningBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.crate = gs::uploadMipped(vdp, crateBmp());
    art.hatch = gs::uploadMipped(vdp, solid(1));
    art.stripe = gs::uploadMipped(vdp, solid(2));
    art.clock = gs::uploadMipped(vdp, clockBmp());
    art.title = gs::uploadMipped(vdp, banner("RICKSHAW BOOM", 2, 1, 2));
    art.delivered = gs::uploadMipped(vdp, banner("DRIVE ON THE BOOM", 2, 1, 2));
    art.missed = gs::uploadMipped(vdp, banner("PAST THE BOOM", 2, 1, 2));
    art.offBoom = gs::uploadMipped(vdp, banner("OFF THE BOOM", 2, 1, 2));
    art.shortB = gs::uploadMipped(vdp, banner("SHORT OF THE BOOM", 2, 1, 2));
    art.broke = gs::uploadMipped(vdp, banner("BROKE THE BOOM", 2, 1, 2));
    art.crew = gs::uploadMipped(vdp, banner("OTHER CREW", 2, 1, 2));
    art.paused = gs::uploadMipped(vdp, banner("PAUSED", 2, 1, 2));
    gs::TextStyle g;
    g.scale = 1;
    g.color = 1;
    g.outline = 0;
    g.spacing = 0;
    for (int c = 32; c < 128; c++) art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), g));
}

}  // namespace rickboom
