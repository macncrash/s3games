#include "game/art.h"

#include <initializer_list>

namespace wcler {
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

gs::Bitmap crewArt(int step) {
    gs::Bitmap b(32, 48);
    b.ellipse(16, 8, 6, 6, 5);
    b.rect(11, 3, 10, 3, 6);
    b.rect(12, 14, 8, 12, 2);
    b.rect(8, 16, 6, 4, 2);
    b.rect(18, 16, 8, 4, 3);
    b.rect(13, 26, 6, 12, 1);
    if (step == 0) {
        b.rect(11, 36, 5, 9, 4);
        b.rect(17, 36, 5, 8, 4);
    } else {
        b.rect(11, 36, 5, 8, 4);
        b.rect(17, 36, 5, 9, 4);
    }
    b.rect(10, 44, 7, 3, 7);
    b.rect(16, 43, 7, 3, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(18, 22);
    b.line(2, 4, 14, 10, 1, 1.6f);
    b.ellipse(13, 14, 4, 4, 2);
    b.ellipse(13, 14, 2, 2, 0);
    b.rect(1, 2, 4, 3, 3);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(36, 28);
    b.rect(2, 4, 32, 20, 2);
    b.rect(2, 4, 32, 4, 3);
    b.rect(2, 20, 32, 4, 1);
    b.line(4, 6, 16, 22, 1, 1.4f);
    b.line(32, 6, 20, 22, 1, 1.4f);
    b.rect(12, 10, 12, 6, 4);
    return b;
}

gs::Bitmap netArt() {
    gs::Bitmap b(40, 22);
    b.ellipse(20, 12, 16, 8, 1);
    for (int x = 8; x < 34; x += 6) b.line(float(x), 6, float(x), 18, 2, 1.f);
    for (int y = 6; y < 18; y += 4) b.line(6, float(y), 34, float(y), 2, 1.f);
    b.ellipse(8, 8, 2, 2, 3);
    return b;
}

gs::Bitmap barrelArt() {
    gs::Bitmap b(22, 28);
    b.ellipse(11, 6, 8, 4, 2);
    b.rect(3, 6, 16, 16, 1);
    b.ellipse(11, 22, 8, 4, 1);
    b.rect(3, 10, 16, 2, 3);
    b.rect(3, 16, 16, 2, 3);
    return b;
}

gs::Bitmap coilArt() {
    gs::Bitmap b(30, 22);
    b.ellipse(15, 12, 12, 8, 1);
    b.ellipse(15, 12, 7, 4, 2);
    b.ellipse(15, 12, 2, 2, 3);
    return b;
}

gs::Bitmap timberArt() {
    gs::Bitmap b(44, 16);
    b.rect(2, 4, 40, 8, 1);
    b.rect(2, 4, 40, 2, 2);
    b.rect(2, 10, 40, 2, 3);
    for (int x = 8; x < 40; x += 10) b.rect(x, 4, 2, 8, 4);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(18, 30);
    b.ellipse(9, 10, 7, 8, 1);
    b.rect(7, 16, 4, 8, 2);
    b.ellipse(9, 26, 3, 2, 3);
    b.rect(4, 8, 10, 2, 4);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(120, 48);
    b.poly({{0, 16}, {60, 2}, {120, 16}, {120, 46}, {0, 46}}, 1);
    b.rect(8, 22, 104, 22, 2);
    for (int x = 14; x < 100; x += 18) b.rect(x, 26, 8, 10, 3);
    b.rect(48, 28, 22, 16, 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 36);
    b.rect(4, 0, 2, 20, 1);
    b.rect(2, 18, 6, 8, 2);
    b.ellipse(5, 22, 2, 2, 3);
    b.rect(4, 26, 2, 10, 1);
    return b;
}

gs::Bitmap pilingArt() {
    gs::Bitmap b(10, 48);
    b.rect(2, 0, 6, 48, 1);
    b.rect(2, 0, 2, 48, 2);
    for (int y = 6; y < 46; y += 10) b.rect(1, y, 8, 2, 3);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(24, 10);
    b.line(1, 6, 10, 3, 1, 1.4f);
    b.line(10, 3, 22, 6, 1, 1.4f);
    return b;
}

gs::Bitmap splashArt() {
    gs::Bitmap b(12, 10);
    b.ellipse(6, 6, 5, 3, 1);
    b.line(6, 1, 6, 5, 2, 1.2f);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(8, 4, 6, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 14, 13), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4), gs::rgb4(8, 6, 2)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(6, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 14, 8), gs::rgb4(2, 6, 3)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 3), gs::rgb4(5, 3, 1), gs::rgb4(10, 9, 6),
                           gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_CREW, {0, gs::rgb4(2, 3, 6), gs::rgb4(4, 6, 10), gs::rgb4(12, 8, 4), gs::rgb4(3, 2, 2),
                           gs::rgb4(13, 9, 6), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 1), gs::rgb4(0, 0, 1)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 10), gs::rgb4(6, 12, 14), gs::rgb4(1, 3, 6)});
    setPal(vdp, PAL_CRATE, {0, gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(14, 10, 4), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(9, 8, 5), gs::rgb4(5, 5, 3), gs::rgb4(13, 12, 8)});
    setPal(vdp, PAL_BARREL, {0, gs::rgb4(7, 3, 2), gs::rgb4(11, 5, 3), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_TIMBER, {0, gs::rgb4(9, 6, 3), gs::rgb4(13, 9, 4), gs::rgb4(5, 3, 1), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(12, 14, 15), gs::rgb4(8, 12, 14)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(1, 1, 2)});
    vdp.setFogColor(gs::rgb4(1, 2, 4));

    art.crew[0] = gs::uploadMipped(vdp, crewArt(0));
    art.crew[1] = gs::uploadMipped(vdp, crewArt(1));
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.net = gs::uploadMipped(vdp, netArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.coil = gs::uploadMipped(vdp, coilArt());
    art.timber = gs::uploadMipped(vdp, timberArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.piling = gs::uploadMipped(vdp, pilingArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.splash = gs::uploadMipped(vdp, splashArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    glyphs(vdp, art);
}

}  // namespace wcler
