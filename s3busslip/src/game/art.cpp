#include "game/art.h"

#include <initializer_list>

namespace slip {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void glyphs(gs::VDP& vdp, Art& a) {
    for (int c = 32; c < 128; c++) {
        gs::Bitmap b(8, 8);
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) b.set(x + 1, y, 1);
        a.glyph[c - 32] = gs::uploadMipped(vdp, b);
    }
}

gs::Bitmap busArt() {
    gs::Bitmap b(72, 36);
    b.rect(6, 10, 58, 16, 2);
    b.rect(8, 4, 40, 8, 2);
    b.rect(10, 6, 14, 5, 4);
    b.rect(26, 6, 12, 5, 4);
    b.rect(40, 6, 6, 5, 5);
    b.rect(6, 18, 58, 3, 3);
    b.rect(4, 12, 4, 8, 6);
    b.rect(62, 14, 6, 6, 1);
    b.rect(64, 16, 3, 2, 7);
    b.rect(18, 22, 8, 4, 1);
    b.rect(44, 22, 8, 4, 1);
    b.outline(8, false);
    return b;
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 3, 3, 2);
    b.rect(6, 2, 2, 10, 3);
    b.rect(2, 6, 10, 2, 3);
    return b;
}

gs::Bitmap quayArt() {
    gs::Bitmap b(48, 28);
    b.rect(0, 0, 48, 8, 2);
    b.rect(0, 0, 48, 3, 3);
    b.rect(0, 8, 48, 20, 1);
    for (int x = 4; x < 48; x += 12) b.line(float(x), 8, float(x), 26, 4, 1.f);
    b.rect(0, 24, 48, 4, 5);
    return b;
}

gs::Bitmap pilingArt() {
    gs::Bitmap b(10, 40);
    b.rect(2, 0, 6, 38, 1);
    b.rect(2, 0, 6, 4, 2);
    b.rect(1, 8, 8, 3, 3);
    b.rect(1, 20, 8, 3, 3);
    return b;
}

gs::Bitmap fenderArt() {
    gs::Bitmap b(16, 22);
    b.ellipse(8, 11, 6, 9, 1);
    b.ellipse(8, 11, 3, 5, 2);
    b.rect(6, 1, 4, 4, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 28);
    b.rect(3, 8, 2, 18, 1);
    b.rect(1, 2, 6, 6, 2);
    b.rect(2, 3, 4, 3, 3);
    return b;
}

gs::Bitmap waveArt() {
    gs::Bitmap b(32, 10);
    b.ellipse(8, 6, 7, 3, 1);
    b.ellipse(22, 5, 8, 3, 2);
    b.rect(0, 7, 32, 3, 3);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(16, 8);
    b.line(1, 5, 7, 2, 1, 1.2f);
    b.line(7, 2, 15, 5, 1, 1.2f);
    b.set(7, 3, 2);
    return b;
}

gs::Bitmap stopArt() {
    gs::Bitmap b(8, 24);
    b.rect(2, 0, 4, 22, 1);
    b.rect(0, 4, 8, 6, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 14), gs::rgb4(2, 2, 4), gs::rgb4(8, 8, 10)});
    setPal(vdp, PAL_BUS, {0, gs::rgb4(2, 2, 3), gs::rgb4(14, 12, 3), gs::rgb4(12, 3, 2), gs::rgb4(10, 13, 15),
                          gs::rgb4(6, 8, 10), gs::rgb4(15, 14, 8), gs::rgb4(15, 6, 2), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_QUAY, {0, gs::rgb4(6, 6, 6), gs::rgb4(10, 10, 9), gs::rgb4(13, 13, 11), gs::rgb4(4, 4, 4),
                           gs::rgb4(3, 5, 4)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(4, 10, 13), gs::rgb4(8, 13, 15), gs::rgb4(2, 6, 10), gs::rgb4(1, 3, 6)});
    setPal(vdp, PAL_SLIP, {0, gs::rgb4(8, 5, 3), gs::rgb4(12, 8, 4), gs::rgb4(5, 3, 2), gs::rgb4(14, 11, 6),
                           gs::rgb4(3, 6, 4)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 6, 3), gs::rgb4(8, 2, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 15, 8), gs::rgb4(2, 8, 3)});
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 2)});

    glyphs(vdp, art);
    art.bus = gs::uploadMipped(vdp, busArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.quay = gs::uploadMipped(vdp, quayArt());
    art.piling = gs::uploadMipped(vdp, pilingArt());
    art.fender = gs::uploadMipped(vdp, fenderArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.wave = gs::uploadMipped(vdp, waveArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.stop = gs::uploadMipped(vdp, stopArt());

    vdp.setFogColor(gs::rgb4(6, 8, 12));
}

}  // namespace slip
