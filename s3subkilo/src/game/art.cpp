#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace subkilo {
namespace {

void pal(gs::VDP& vdp, int bank, std::initializer_list<uint16_t> cols) {
    int i = 1;
    for (uint16_t c : cols) {
        if (i > 15) break;
        vdp.setColor(bank * 16 + i, c);
        i++;
    }
}

gs::Bitmap wheelFrame(int turn) {
    gs::Bitmap b(64, 64);
    b.ellipse(32, 32, 28, 28, 1);
    b.ellipse(32, 32, 22, 22, 2);
    b.ellipse(32, 32, 8, 8, 3);
    for (int s = 0; s < 6; s++) {
        float a = (s / 6.0f + turn / 4.0f) * 6.2831853f;
        b.line(32, 32, 32 + std::cos(a) * 26, 32 + std::sin(a) * 26, 4, 2.2f);
    }
    b.ellipse(32, 32, 5, 5, 4);
    b.outline(5, true);
    return b;
}

gs::Bitmap subArt() {
    gs::Bitmap b(80, 36);
    b.ellipse(40, 18, 34, 12, 1);
    b.ellipse(48, 16, 16, 7, 2);
    b.rect(18, 16, 28, 8, 1);
    b.ellipse(58, 14, 6, 5, 3);
    b.ellipse(58, 14, 3, 2, 4);
    b.rect(8, 14, 10, 8, 2);
    b.line(10, 10, 10, 26, 5, 2);
    b.line(6, 18, 14, 14, 5, 1.5f);
    b.line(6, 18, 14, 22, 5, 1.5f);
    b.rect(70, 15, 8, 3, 5);
    b.rect(70, 20, 8, 3, 5);
    b.outline(5, true);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(16, 48);
    b.rect(4, 0, 8, 48, 1);
    b.rect(2, 0, 12, 6, 2);
    b.rect(6, 8, 3, 36, 3);
    return b;
}

gs::Bitmap bubbleArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 4, 4, 1);
    b.set(4, 4, 2);
    return b;
}

gs::Bitmap kelpArt() {
    gs::Bitmap b(16, 40);
    b.line(8, 39, 5, 24, 1, 2);
    b.line(5, 24, 11, 12, 2, 2);
    b.line(11, 12, 6, 0, 1, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    pal(vdp, PAL_HUD, {rgb4(15, 15, 14), rgb4(15, 12, 4), rgb4(8, 14, 12), rgb4(15, 6, 4), rgb4(4, 6, 8)});
    pal(vdp, PAL_SUB, {rgb4(12, 11, 3), rgb4(15, 14, 6), rgb4(6, 12, 14), rgb4(14, 15, 15), rgb4(2, 3, 4)});
    pal(vdp, PAL_WHEEL, {rgb4(8, 5, 3), rgb4(4, 3, 3), rgb4(10, 8, 6), rgb4(14, 12, 8), rgb4(1, 1, 2)});
    pal(vdp, PAL_GATE, {rgb4(14, 12, 4), rgb4(15, 15, 8), rgb4(6, 8, 4), rgb4(2, 3, 2)});
    pal(vdp, PAL_FX, {rgb4(10, 14, 15), rgb4(15, 15, 15)});
    pal(vdp, PAL_KELP, {rgb4(2, 8, 4), rgb4(4, 12, 6)});

    art.sub = gs::uploadMipped(vdp, subArt());
    for (int i = 0; i < 4; i++) art.wheel[i] = gs::uploadMipped(vdp, wheelFrame(i));
    art.post = gs::uploadMipped(vdp, postArt());
    art.bubble = gs::uploadMipped(vdp, bubbleArt());
    art.kelp = gs::uploadMipped(vdp, kelpArt());

    gs::TextStyle big;
    big.scale = 1;
    big.color = 1;
    for (int c = 32; c < 128; c++) art.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
}

}  // namespace subkilo
