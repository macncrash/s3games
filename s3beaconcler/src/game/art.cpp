#include "game/art.h"

#include <initializer_list>

namespace bcler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap keeperArt(int step) {
    gs::Bitmap b(32, 54);
    b.ellipse(16, 6, 8, 3, 3);
    b.rect(10, 7, 12, 4, 2);
    b.ellipse(16, 15, 6, 6, 5);
    b.set(14, 15, 8);
    b.set(18, 15, 8);
    b.poly({{9, 22}, {23, 22}, {26, 38}, {6, 38}}, 1);
    b.rect(13, 24, 6, 10, 4);
    b.rect(22, 26, 8, 3, 6);
    if (step == 0) {
        b.rect(9, 38, 6, 12, 2);
        b.rect(17, 38, 6, 10, 2);
        b.rect(7, 48, 9, 3, 3);
        b.rect(16, 46, 9, 3, 3);
    } else {
        b.rect(9, 38, 6, 10, 2);
        b.rect(17, 38, 6, 12, 2);
        b.rect(8, 46, 9, 3, 3);
        b.rect(15, 48, 9, 3, 3);
    }
    b.outline(10, false);
    return b;
}

gs::Bitmap gaffArt() {
    gs::Bitmap b(16, 30);
    b.rect(7, 8, 2, 18, 2);
    b.poly({{4, 4}, {12, 8}, {8, 2}}, 1);
    b.line(4, 6, 12, 2, 3, 1.4f);
    b.rect(6, 24, 4, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap kelpArt() {
    gs::Bitmap b(36, 26);
    b.line(6, 22, 10, 6, 1, 2.4f);
    b.line(14, 22, 12, 4, 2, 2.2f);
    b.line(22, 20, 20, 6, 1, 2.2f);
    b.line(28, 22, 30, 8, 3, 2.0f);
    b.ellipse(18, 20, 14, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(32, 28);
    b.poly({{4, 10}, {16, 4}, {28, 10}, {16, 16}}, 2);
    b.poly({{4, 10}, {16, 16}, {16, 26}, {4, 20}}, 1);
    b.poly({{16, 16}, {28, 10}, {28, 20}, {16, 26}}, 3);
    b.line(4, 14, 16, 20, 4, 1.2f);
    b.line(16, 8, 16, 16, 4, 1.2f);
    b.outline(5, false);
    return b;
}

gs::Bitmap buoyArt() {
    gs::Bitmap b(22, 30);
    b.ellipse(11, 12, 8, 8, 1);
    b.rect(9, 18, 4, 8, 2);
    b.rect(4, 22, 14, 3, 3);
    b.rect(10, 2, 2, 4, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap ropeArt() {
    gs::Bitmap b(34, 24);
    b.ellipse(17, 13, 14, 8, 1);
    b.ellipse(17, 13, 7, 4, 2);
    b.line(4, 8, 14, 18, 3, 1.6f);
    b.line(22, 6, 30, 16, 3, 1.6f);
    b.outline(4, false);
    return b;
}

gs::Bitmap driftArt() {
    gs::Bitmap b(48, 16);
    b.poly({{2, 10}, {12, 4}, {40, 6}, {46, 11}, {8, 14}}, 1);
    b.rect(18, 5, 8, 3, 2);
    b.rect(30, 7, 6, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap lanternArt() {
    gs::Bitmap b(18, 26);
    b.rect(7, 2, 4, 3, 2);
    b.poly({{3, 6}, {15, 6}, {13, 20}, {5, 20}}, 1);
    b.rect(6, 9, 6, 6, 3);
    b.rect(4, 20, 10, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap towerArt() {
    gs::Bitmap b(70, 140);
    b.poly({{18, 138}, {26, 28}, {44, 28}, {52, 138}}, 1);
    b.rect(22, 28, 26, 8, 2);
    b.ellipse(35, 22, 16, 10, 3);
    b.rect(28, 16, 14, 8, 4);
    b.rect(32, 8, 6, 10, 5);
    for (int y = 48; y < 128; y += 16) b.rect(30.f, float(y), 10.f, 4.f, 6);
    b.rect(14, 130, 42, 6, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap lampHouse() {
    gs::Bitmap b(20, 16);
    b.rect(2, 4, 16, 10, 1);
    b.rect(5, 6, 4, 6, 2);
    b.rect(11, 6, 4, 6, 3);
    b.poly({{2, 4}, {10, 0}, {18, 4}}, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(90, 18);
    b.poly({{0, 8}, {88, 1}, {88, 17}, {0, 10}}, 1);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(120, 14);
    b.rect(2, 6, 116, 3, 1);
    for (int x = 8; x < 116; x += 16) b.rect(float(x), 2.f, 3.f, 10.f, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap sprayArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 2, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9, 3, 1);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(8, 4, 6, 2, 1);
    b.ellipse(8, 4, 2, 1, 2);
    return b;
}

void glyphs(gs::VDP& vdp, Art& art) {
    for (int i = 0; i < 96; i++) {
        gs::Bitmap g(8, 8);
        const uint8_t* rows = gs::glyph(char(32 + i));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (rows && rows[y * 5 + x]) g.set(x + 1, y, 1);
        art.glyph[i] = gs::uploadMipped(vdp, g);
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 14, 13)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(5, 14, 10)});
    setPal(vdp, PAL_TOWER,
           {0, gs::rgb4(9, 9, 10), gs::rgb4(6, 6, 8), gs::rgb4(12, 12, 13), gs::rgb4(14, 13, 8),
            gs::rgb4(4, 4, 6), gs::rgb4(7, 7, 8), gs::rgb4(5, 5, 4), gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_KEEPER,
           {0, gs::rgb4(2, 4, 8), gs::rgb4(1, 3, 6), gs::rgb4(8, 8, 9), gs::rgb4(12, 10, 4),
            gs::rgb4(13, 9, 6), gs::rgb4(6, 5, 3), gs::rgb4(10, 8, 3), gs::rgb4(1, 1, 1), gs::rgb4(3, 3, 4),
            gs::rgb4(0, 0, 1)});
    setPal(vdp, PAL_KELP, {0, gs::rgb4(2, 7, 3), gs::rgb4(3, 9, 4), gs::rgb4(1, 5, 2), gs::rgb4(5, 8, 3), gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_STEEL,
           {0, gs::rgb4(7, 8, 9), gs::rgb4(10, 11, 12), gs::rgb4(5, 5, 6), gs::rgb4(12, 10, 4), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(9, 7, 3), gs::rgb4(6, 5, 2), gs::rgb4(11, 9, 5), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_BUOY, {0, gs::rgb4(14, 3, 3), gs::rgb4(12, 12, 12), gs::rgb4(2, 3, 8), gs::rgb4(15, 12, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(6, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(12, 13, 14), gs::rgb4(8, 10, 12)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(8, 9, 12)});

    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.gaff = gs::uploadMipped(vdp, gaffArt());
    art.kelp = gs::uploadMipped(vdp, kelpArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.buoy = gs::uploadMipped(vdp, buoyArt());
    art.rope = gs::uploadMipped(vdp, ropeArt());
    art.drift = gs::uploadMipped(vdp, driftArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.lamp = gs::uploadMipped(vdp, lampHouse());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    glyphs(vdp, art);
    vdp.setFogColor(gs::rgb4(1, 2, 4));
}

}  // namespace bcler
