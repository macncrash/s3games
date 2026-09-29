#include "game/art.h"

#include <string>

namespace palisadepouc {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < n && i < 16; i++) vdp.setColor(pal * 16 + i, cs[i]);
}

gs::Bitmap glyphBmp(char ch) {
    gs::Bitmap b(5, 7);
    const uint8_t* g = gs::glyph(ch);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (g[y * 5 + x]) b.set(x, y, 1);
    return b;
}

gs::Bitmap runnerBmp(int pose) {
    gs::Bitmap b(28, 46);
    const bool leap = pose == 3;
    b.ellipse(14, 8, 5, 5, 3);
    b.rect(11, 6, 6, 3, 4);
    b.set(12, 8, 8);
    b.set(16, 8, 8);
    b.rect(9, 13, 11, 14, 2);
    b.rect(11, 15, 4, 8, 1);
    b.rect(6, 15, 4, 10, 2);
    b.rect(19, 15, 4, 10, 2);
    if (leap) {
        b.rect(8, 26, 5, 8, 5);
        b.rect(16, 24, 5, 8, 5);
        b.rect(6, 32, 8, 3, 6);
        b.rect(16, 30, 8, 3, 6);
    } else if (pose == 1) {
        b.rect(9, 26, 4, 14, 5);
        b.rect(16, 28, 4, 10, 5);
        b.rect(7, 38, 7, 3, 6);
        b.rect(15, 36, 7, 3, 6);
    } else if (pose == 2) {
        b.rect(16, 26, 4, 14, 5);
        b.rect(9, 28, 4, 10, 5);
        b.rect(14, 38, 7, 3, 6);
        b.rect(7, 36, 7, 3, 6);
    } else {
        b.rect(9, 26, 4, 14, 5);
        b.rect(16, 26, 4, 14, 5);
        b.rect(7, 38, 7, 3, 6);
        b.rect(15, 38, 7, 3, 6);
    }
    b.outline(7, false);
    return b;
}

gs::Bitmap pouchBmp() {
    gs::Bitmap b(22, 18);
    b.ellipse(11, 11, 8, 6, 1);
    b.rect(6, 7, 10, 8, 2);
    b.rect(8, 3, 6, 5, 3);
    b.rect(10, 1, 2, 4, 4);
    b.rect(9, 9, 4, 3, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap stakeBmp() {
    gs::Bitmap b(14, 72);
    b.poly({{7, 0}, {2, 16}, {12, 16}}, 2);
    b.rect(3, 12, 8, 54, 1);
    b.rect(5, 14, 3, 48, 3);
    for (int y = 20; y < 60; y += 10) b.rect(3, float(y), 8, 2, 4);
    b.rect(1, 64, 12, 6, 5);
    b.outline(6, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(15, 14, 10), gs::rgb4(8, 7, 4)};
    const uint16_t wood[] = {0, gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(12, 9, 5), gs::rgb4(4, 3, 2),
                             gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 1)};
    const uint16_t earth[] = {0, gs::rgb4(3, 6, 2), gs::rgb4(5, 8, 3), gs::rgb4(8, 6, 3), gs::rgb4(2, 3, 2),
                              gs::rgb4(1, 2, 4), gs::rgb4(4, 5, 6)};
    const uint16_t pouch[] = {0, gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 3), gs::rgb4(5, 3, 1), gs::rgb4(14, 12, 6),
                              gs::rgb4(9, 2, 2), gs::rgb4(2, 1, 1)};
    const uint16_t runner[] = {0, gs::rgb4(3, 5, 3), gs::rgb4(4, 8, 4), gs::rgb4(12, 9, 6), gs::rgb4(6, 4, 3),
                               gs::rgb4(2, 3, 5), gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 1), gs::rgb4(2, 2, 2)};
    const uint16_t sky[] = {0, gs::rgb4(6, 8, 10), gs::rgb4(10, 8, 4)};
    const uint16_t bar[] = {0, gs::rgb4(9, 8, 7), gs::rgb4(13, 12, 9), gs::rgb4(5, 5, 5), gs::rgb4(14, 4, 2)};
    const uint16_t go[] = {0, gs::rgb4(6, 14, 7), gs::rgb4(12, 15, 10)};
    const uint16_t alert[] = {0, gs::rgb4(14, 4, 3), gs::rgb4(14, 10, 3)};
    setPal(vdp, PAL_HUD, hud, 3);
    setPal(vdp, PAL_WOOD, wood, 7);
    setPal(vdp, PAL_EARTH, earth, 7);
    setPal(vdp, PAL_POUCH, pouch, 7);
    setPal(vdp, PAL_RUNNER, runner, 9);
    setPal(vdp, PAL_SKY, sky, 3);
    setPal(vdp, PAL_BAR, bar, 5);
    setPal(vdp, PAL_GO, go, 3);
    setPal(vdp, PAL_ALERT, alert, 3);
    vdp.setFogColor(gs::rgb4(3, 4, 5));

    for (int c = 32; c < 128; c++) art.glyph[c - 32] = gs::uploadMipped(vdp, glyphBmp(char(c)));

    art.stand = gs::uploadMipped(vdp, runnerBmp(0));
    art.runA = gs::uploadMipped(vdp, runnerBmp(1));
    art.runB = gs::uploadMipped(vdp, runnerBmp(2));
    art.leap = gs::uploadMipped(vdp, runnerBmp(3));
    art.pouch = gs::uploadMipped(vdp, pouchBmp());
    art.stake = gs::uploadMipped(vdp, stakeBmp());
    {
        gs::Bitmap b(16, 88);
        b.rect(3, 0, 10, 80, 1);
        b.rect(5, 4, 4, 70, 3);
        b.rect(0, 76, 16, 10, 5);
        b.rect(5, 18, 6, 4, 2);
        b.outline(6, false);
        art.post = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(32, 18);
        b.rect(0, 4, 32, 14, 1);
        b.rect(0, 2, 32, 4, 2);
        for (int x = 2; x < 30; x += 6) b.rect(float(x), 6, 2, 8, 3);
        art.turf = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(32, 28);
        b.rect(0, 0, 32, 28, 4);
        b.rect(0, 0, 32, 4, 5);
        b.ellipse(10, 14, 6, 3, 5);
        b.ellipse(22, 18, 5, 2, 5);
        art.ditch = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(64, 10);
        b.rect(0, 2, 64, 6, 1);
        b.rect(0, 3, 64, 2, 2);
        b.rect(4, 0, 6, 10, 3);
        b.rect(54, 0, 6, 10, 3);
        b.outline(4, false);
        art.bar = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(18, 28);
        b.rect(7, 0, 4, 22, 1);
        b.ellipse(9, 6, 6, 6, 2);
        b.rect(4, 22, 10, 5, 3);
        b.outline(6, false);
        art.mark = gs::uploadMipped(vdp, b);
    }
}

}  // namespace palisadepouc
