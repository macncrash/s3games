#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace rickbox {
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
    b.ellipse(float(cx), float(cy), 9, 9, 1);
    b.ellipse(float(cx), float(cy), 6, 6, 2);
    b.ellipse(float(cx), float(cy), 2, 2, 3);
    float a = frame ? 0.7f : 0.15f;
    for (int k = 0; k < 4; k++) {
        float ang = a + k * 1.57f;
        b.line(float(cx), float(cy), float(cx) + std::cos(ang) * 7.f, float(cy) + std::sin(ang) * 7.f, 3, 1.2f);
    }
}

gs::Bitmap cabFrame(int frame) {
    gs::Bitmap b(96, 56);
    wheel(b, 22, 42, frame);
    wheel(b, 74, 42, frame);
    b.rect(16, 34, 62, 4, 4);
    b.line(22, 34, 30, 22, 4, 2.2f);
    b.line(74, 34, 66, 22, 4, 2.2f);
    b.poly({{28, 34}, {32, 16}, {78, 14}, {80, 34}}, 5);
    b.poly({{34, 18}, {70, 17}, {68, 28}, {36, 30}}, 6);
    b.rect(48, 26, 16, 8, 7);
    b.ellipse(56, 22, 5, 5, 8);
    b.rect(50, 26, 10, 6, 9);
    b.ellipse(30, 24, 4.2f, 4.2f, 8);
    b.rect(26, 27, 8, 7, 10);
    if (frame == 0) {
        b.line(28, 32, 18, 40, 10, 2.f);
        b.line(32, 32, 36, 40, 10, 2.f);
    } else {
        b.line(28, 32, 24, 41, 10, 2.f);
        b.line(32, 32, 40, 38, 10, 2.f);
    }
    b.rect(12, 30, 10, 3, 4);
    b.ellipse(10, 31, 3, 3, 11);
    return b;
}

gs::Bitmap shadeBmp() {
    gs::Bitmap b(40, 12);
    b.ellipse(20, 6, 18, 4, 1);
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

gs::Bitmap postBmp() {
    gs::Bitmap b(8, 20);
    b.rect(3, 2, 2, 16, 1);
    b.rect(1, 0, 6, 4, 2);
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
        {rgb4(0, 0, 0), rgb4(1, 1, 1), rgb4(4, 4, 5), rgb4(10, 8, 4), rgb4(8, 2, 2), rgb4(14, 8, 2), rgb4(12, 10, 6),
         rgb4(6, 3, 2), rgb4(13, 9, 6), rgb4(10, 3, 4), rgb4(2, 3, 8), rgb4(14, 13, 8)});
    pal(vdp, PAL_STREET,
        {rgb4(0, 0, 0), rgb4(6, 5, 5), rgb4(3, 5, 8), rgb4(9, 6, 4), rgb4(4, 3, 3), rgb4(12, 4, 3)});
    pal(vdp, PAL_BOX, {rgb4(0, 0, 0), rgb4(14, 12, 3), rgb4(10, 8, 2), rgb4(15, 15, 8)});
    pal(vdp, PAL_BANNER, {rgb4(0, 0, 0), rgb4(15, 13, 6), rgb4(2, 1, 3)});
    pal(vdp, PAL_ALERT, {rgb4(0, 0, 0), rgb4(15, 6, 4), rgb4(3, 1, 1)});
    pal(vdp, PAL_WIN, {rgb4(0, 0, 0), rgb4(8, 15, 8), rgb4(1, 3, 2)});
    pal(vdp, PAL_CREW, {rgb4(0, 0, 0), rgb4(12, 9, 4), rgb4(15, 14, 10), rgb4(2, 2, 3), rgb4(14, 4, 3)});
    pal(vdp, PAL_LAMP, {rgb4(0, 0, 0), rgb4(5, 4, 3), rgb4(15, 13, 5), rgb4(15, 15, 12)});

    art.cab[0] = gs::uploadMipped(vdp, cabFrame(0));
    art.cab[1] = gs::uploadMipped(vdp, cabFrame(1));
    art.shade = gs::uploadMipped(vdp, shadeBmp());
    art.stall = gs::uploadMipped(vdp, stallBmp());
    art.awning = gs::uploadMipped(vdp, awningBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.crate = gs::uploadMipped(vdp, crateBmp());
    art.hatch = gs::uploadMipped(vdp, solid(1));
    art.stripe = gs::uploadMipped(vdp, solid(2));
    art.post = gs::uploadMipped(vdp, postBmp());
    art.clock = gs::uploadMipped(vdp, clockBmp());
    art.title = gs::uploadMipped(vdp, banner("RICKSHAW BOX", 2, 1, 2));
    art.stopped = gs::uploadMipped(vdp, banner("STOPPED IN THE BOX", 2, 1, 2));
    art.missed = gs::uploadMipped(vdp, banner("PAST THE BOX", 2, 1, 2));
    art.outside = gs::uploadMipped(vdp, banner("OUTSIDE THE BOX", 2, 1, 2));
    art.shortB = gs::uploadMipped(vdp, banner("SHORT OF THE BOX", 2, 1, 2));
    art.crew = gs::uploadMipped(vdp, banner("OTHER CREW", 2, 1, 2));
    art.paused = gs::uploadMipped(vdp, banner("PAUSED", 2, 1, 2));
    gs::TextStyle g;
    g.scale = 1;
    g.color = 1;
    g.outline = 0;
    g.spacing = 0;
    for (int c = 32; c < 128; c++) art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), g));
}

}  // namespace rickbox
