#include "art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace rickmark {
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
    b.ellipse(float(cx), float(cy), 8, 8, 1);
    b.ellipse(float(cx), float(cy), 5, 5, 2);
    b.ellipse(float(cx), float(cy), 2, 2, 3);
    float a = frame ? 0.8f : 0.2f;
    for (int k = 0; k < 3; k++) {
        float ang = a + k * 2.094f;
        b.line(float(cx), float(cy), float(cx) + std::cos(ang) * 6.f, float(cy) + std::sin(ang) * 6.f, 3, 1.1f);
    }
}

gs::Bitmap cab(int frame, bool down) {
    gs::Bitmap b(88, 52);
    wheel(b, 58, 40, frame);
    wheel(b, 28, 40, frame);
    b.rect(22, 32, 42, 4, 4);
    b.poly({{24, 32}, {30, 16}, {70, 14}, {68, 32}}, 5);
    b.poly({{32, 18}, {62, 17}, {60, 28}, {34, 28}}, 6);
    b.rect(40, 22, 12, 7, 7);
    b.ellipse(46, 20, 4, 4, 8);
    b.ellipse(34, 22, 3.5f, 3.5f, 9);
    if (frame == 0) {
        b.line(36, 30, 30, 38, 10, 1.6f);
        b.line(40, 30, 44, 38, 10, 1.6f);
    } else {
        b.line(36, 30, 33, 39, 10, 1.6f);
        b.line(40, 30, 48, 36, 10, 1.6f);
    }
    if (down) {
        b.line(22, 32, 6, 46, 4, 2.4f);
        b.line(22, 34, 8, 48, 11, 1.6f);
        b.ellipse(6, 47, 3, 2, 11);
    } else {
        b.line(22, 32, 4, 28, 4, 2.4f);
        b.ellipse(4, 27, 3, 3, 11);
    }
    return b;
}

gs::Bitmap shadeBmp() {
    gs::Bitmap b(36, 10);
    b.ellipse(18, 5, 16, 3, 1);
    return b;
}

gs::Bitmap stallBmp() {
    gs::Bitmap b(28, 44);
    b.rect(2, 12, 24, 30, 1);
    b.rect(5, 16, 7, 10, 2);
    b.rect(15, 18, 8, 14, 3);
    b.rect(3, 34, 22, 6, 4);
    b.rect(0, 8, 28, 5, 5);
    return b;
}

gs::Bitmap awningBmp() {
    gs::Bitmap b(36, 12);
    for (int x = 0; x < 36; x++) b.rect(float(x), 2, 1, 9, (x / 4) & 1 ? 1 : 2);
    b.rect(0, 0, 36, 3, 3);
    return b;
}

gs::Bitmap lampBmp() {
    gs::Bitmap b(10, 26);
    b.rect(4, 10, 2, 16, 1);
    b.ellipse(5, 7, 4, 5, 2);
    b.ellipse(5, 6, 2, 2, 3);
    return b;
}

gs::Bitmap chalkBmp() {
    gs::Bitmap b(28, 18);
    b.line(2, 9, 14, 2, 1, 1.6f);
    b.line(14, 2, 26, 9, 1, 1.6f);
    b.line(26, 9, 14, 16, 1, 1.6f);
    b.line(14, 16, 2, 9, 1, 1.6f);
    b.line(8, 9, 20, 9, 2, 1.4f);
    b.ellipse(14, 9, 2, 2, 3);
    return b;
}

gs::Bitmap flagBmp() {
    gs::Bitmap b(14, 28);
    b.rect(2, 8, 2, 20, 1);
    b.poly({{4, 4}, {13, 8}, {4, 13}}, 2);
    return b;
}

gs::Bitmap solid(int c) {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, c);
    return b;
}

gs::Bitmap clockBmp() {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 11, 11, 1);
    b.ellipse(13, 13, 8, 8, 2);
    b.line(13, 13, 13, 6, 3, 1.3f);
    b.line(13, 13, 19, 15, 4, 1.3f);
    return b;
}

gs::Bitmap walkerBmp(int frame) {
    gs::Bitmap b(18, 28);
    b.ellipse(9, 5, 3.2f, 3.2f, 2);
    b.rect(7, 8, 4, 9, 1);
    if (frame == 0) {
        b.line(8, 16, 5, 26, 1, 1.5f);
        b.line(11, 16, 14, 25, 1, 1.5f);
        b.line(7, 11, 2, 16, 1, 1.4f);
    } else {
        b.line(8, 16, 7, 26, 1, 1.5f);
        b.line(11, 16, 13, 24, 1, 1.5f);
        b.line(11, 11, 16, 14, 1, 1.4f);
    }
    b.rect(1, 14, 4, 3, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    vdp.setFogColor(rgb4(3, 2, 2));
    pal(vdp, PAL_HUD, {rgb4(0, 0, 0), rgb4(15, 14, 11), rgb4(4, 2, 2), rgb4(11, 7, 3)});
    pal(vdp, PAL_CAB,
        {rgb4(0, 0, 0), rgb4(2, 2, 2), rgb4(5, 5, 6), rgb4(9, 7, 3), rgb4(9, 3, 2), rgb4(13, 7, 2), rgb4(11, 9, 5),
         rgb4(5, 3, 2), rgb4(14, 11, 7), rgb4(12, 8, 5), rgb4(3, 4, 8), rgb4(15, 13, 8)});
    pal(vdp, PAL_STREET, {rgb4(0, 0, 0), rgb4(7, 6, 5), rgb4(4, 6, 8), rgb4(8, 5, 3), rgb4(3, 3, 3), rgb4(12, 5, 3)});
    pal(vdp, PAL_MARK, {rgb4(0, 0, 0), rgb4(15, 15, 12), rgb4(14, 12, 3), rgb4(15, 8, 2)});
    pal(vdp, PAL_BANNER, {rgb4(0, 0, 0), rgb4(15, 13, 7), rgb4(3, 1, 2)});
    pal(vdp, PAL_ALERT, {rgb4(0, 0, 0), rgb4(15, 6, 3), rgb4(4, 1, 1)});
    pal(vdp, PAL_WIN, {rgb4(0, 0, 0), rgb4(8, 15, 8), rgb4(1, 4, 2)});
    pal(vdp, PAL_CREW, {rgb4(0, 0, 0), rgb4(6, 5, 8), rgb4(14, 12, 8), rgb4(12, 3, 3), rgb4(3, 3, 4)});
    pal(vdp, PAL_LAMP, {rgb4(0, 0, 0), rgb4(5, 4, 3), rgb4(15, 13, 4), rgb4(15, 15, 11)});

    art.cabUp[0] = gs::uploadMipped(vdp, cab(0, false));
    art.cabUp[1] = gs::uploadMipped(vdp, cab(1, false));
    art.cabDown = gs::uploadMipped(vdp, cab(0, true));
    art.shade = gs::uploadMipped(vdp, shadeBmp());
    art.stall = gs::uploadMipped(vdp, stallBmp());
    art.awning = gs::uploadMipped(vdp, awningBmp());
    art.lamp = gs::uploadMipped(vdp, lampBmp());
    art.chalk = gs::uploadMipped(vdp, chalkBmp());
    art.flag = gs::uploadMipped(vdp, flagBmp());
    art.hatch = gs::uploadMipped(vdp, solid(1));
    art.stripe = gs::uploadMipped(vdp, solid(2));
    art.clock = gs::uploadMipped(vdp, clockBmp());
    art.walker[0] = gs::uploadMipped(vdp, walkerBmp(0));
    art.walker[1] = gs::uploadMipped(vdp, walkerBmp(1));
    art.title = gs::uploadMipped(vdp, banner("RICKSHAW MARK", 2, 1, 2));
    art.setdown = gs::uploadMipped(vdp, banner("SET DOWN", 2, 1, 2));
    art.missed = gs::uploadMipped(vdp, banner("PAST THE MARK", 2, 1, 2));
    art.offmark = gs::uploadMipped(vdp, banner("OFF THE MARK", 2, 1, 2));
    art.rolling = gs::uploadMipped(vdp, banner("STILL ROLLING", 2, 1, 2));
    art.crew = gs::uploadMipped(vdp, banner("OTHER CREW", 2, 1, 2));
    art.paused = gs::uploadMipped(vdp, banner("PAUSED", 2, 1, 2));
    gs::TextStyle g;
    g.scale = 1;
    g.color = 1;
    g.outline = 0;
    g.spacing = 0;
    for (int c = 32; c < 128; c++) art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), g));
}

}  // namespace rickmark
