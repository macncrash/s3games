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

gs::Bitmap sentryArt(int step) {
    gs::Bitmap b(36, 56);
    b.ellipse(18, 8, 10, 5, 3);
    b.rect(9, 8, 18, 4, 2);
    b.rect(8, 11, 20, 2, 4);
    b.ellipse(18, 16, 7, 7, 5);
    b.set(15, 16, 8);
    b.set(21, 16, 8);
    b.rect(16, 20, 4, 2, 6);
    b.poly({{10, 24}, {26, 24}, {30, 40}, {6, 40}}, 1);
    b.poly({{10, 24}, {18, 24}, {16, 40}, {6, 40}}, 2);
    b.rect(15, 26, 6, 8, 7);
    b.line(26, 28, 33, 38, 4, 2.2f);
    b.rect(30, 36, 4, 3, 9);
    if (step == 0) {
        b.rect(11, 40, 6, 12, 2);
        b.rect(20, 40, 6, 10, 2);
        b.rect(9, 50, 9, 3, 3);
        b.rect(19, 48, 9, 3, 3);
    } else {
        b.rect(11, 40, 6, 10, 2);
        b.rect(20, 40, 6, 12, 2);
        b.rect(10, 48, 9, 3, 3);
        b.rect(18, 50, 9, 3, 3);
    }
    b.outline(10, false);
    return b;
}

gs::Bitmap shovelArt() {
    gs::Bitmap b(14, 28);
    b.rect(6, 2, 2, 16, 2);
    b.poly({{3, 16}, {11, 16}, {12, 26}, {2, 26}}, 1);
    b.rect(4, 18, 6, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap rubbleArt() {
    gs::Bitmap b(40, 24);
    b.poly({{4, 18}, {14, 6}, {22, 16}, {8, 20}}, 1);
    b.poly({{16, 14}, {28, 4}, {36, 16}, {20, 20}}, 2);
    b.ellipse(20, 18, 16, 5, 3);
    b.rect(12, 10, 4, 3, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap coilArt() {
    gs::Bitmap b(34, 28);
    b.ellipse(17, 16, 14, 10, 1);
    b.ellipse(17, 16, 8, 5, 2);
    b.ellipse(17, 16, 3, 2, 3);
    b.line(6, 10, 28, 8, 4, 1.4f);
    b.line(8, 22, 26, 20, 4, 1.4f);
    b.outline(5, false);
    return b;
}

gs::Bitmap shellArt() {
    gs::Bitmap b(28, 16);
    b.ellipse(6, 8, 5, 5, 2);
    b.rect(6, 4, 16, 8, 1);
    b.rect(20, 5, 5, 6, 3);
    b.rect(8, 6, 10, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap bagsArt() {
    gs::Bitmap b(36, 26);
    b.ellipse(12, 16, 10, 7, 1);
    b.ellipse(22, 12, 10, 7, 2);
    b.rect(8, 8, 6, 3, 3);
    b.rect(18, 5, 6, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap timberArt() {
    gs::Bitmap b(46, 18);
    b.rect(2, 4, 40, 4, 1);
    b.rect(4, 9, 40, 4, 2);
    b.rect(8, 3, 3, 12, 3);
    b.rect(30, 3, 3, 12, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(26, 32);
    b.ellipse(13, 7, 10, 4, 3);
    b.rect(3, 7, 20, 18, 1);
    b.ellipse(13, 25, 10, 4, 2);
    b.rect(3, 12, 20, 3, 4);
    b.rect(3, 7, 3, 18, 2);
    b.outline(5, false);
    return b;
}

gs::Bitmap bunkerArt() {
    gs::Bitmap b(220, 78);
    b.poly({{8, 70}, {20, 18}, {200, 18}, {212, 70}}, 1);
    b.poly({{20, 18}, {110, 10}, {200, 18}, {110, 28}}, 2);
    b.rect(28, 30, 164, 36, 3);
    for (int x = 36; x < 180; x += 22) b.rect(float(x), 30.f, 2.f, 36.f, 4);
    b.rect(92, 40, 36, 26, 6);
    b.rect(96, 44, 28, 8, 7);
    b.rect(40, 36, 28, 8, 8);
    b.rect(152, 36, 28, 8, 8);
    b.rect(18, 64, 184, 6, 5);
    b.outline(9, false);
    return b;
}

gs::Bitmap slitArt() {
    gs::Bitmap b(30, 10);
    b.rect(1, 2, 28, 6, 1);
    b.rect(4, 4, 8, 2, 2);
    b.rect(16, 4, 8, 2, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 22);
    b.rect(7, 2, 2, 10, 2);
    b.poly({{2, 12}, {14, 12}, {12, 18}, {4, 18}}, 1);
    b.rect(4, 13, 8, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(48, 20);
    b.poly({{2, 2}, {46, 8}, {46, 12}, {2, 18}}, 1);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(10, 28);
    b.rect(4, 2, 2, 22, 1);
    b.poly({{2, 4}, {8, 4}, {5, 0}}, 2);
    b.line(1, 8, 9, 14, 3, 1.2f);
    b.line(1, 16, 9, 10, 3, 1.2f);
    b.outline(4, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(20, 8);
    b.ellipse(10, 4, 9, 3, 1);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 5, 6, 3, 1);
    b.ellipse(8, 5, 2, 1, 2);
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
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 14, 7)});
    setPal(vdp, PAL_CONCRETE,
           {0, gs::rgb4(7, 7, 8), gs::rgb4(5, 5, 6), gs::rgb4(9, 9, 10), gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 4),
            gs::rgb4(2, 2, 3), gs::rgb4(11, 10, 6), gs::rgb4(1, 2, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_SENTRY,
           {0, gs::rgb4(4, 6, 3), gs::rgb4(3, 4, 2), gs::rgb4(6, 6, 5), gs::rgb4(8, 7, 4), gs::rgb4(12, 8, 6),
            gs::rgb4(8, 4, 3), gs::rgb4(10, 9, 4), gs::rgb4(2, 2, 2), gs::rgb4(9, 9, 8), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_EARTH,
           {0, gs::rgb4(6, 4, 2), gs::rgb4(8, 6, 3), gs::rgb4(4, 3, 2), gs::rgb4(9, 8, 6), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_STEEL,
           {0, gs::rgb4(8, 9, 7), gs::rgb4(5, 6, 4), gs::rgb4(11, 11, 8), gs::rgb4(13, 12, 6), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_WIRE, {0, gs::rgb4(5, 6, 4), gs::rgb4(3, 4, 3), gs::rgb4(8, 8, 6), gs::rgb4(10, 9, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_SAND, {0, gs::rgb4(10, 8, 4), gs::rgb4(8, 6, 3), gs::rgb4(12, 10, 5), gs::rgb4(5, 4, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(6, 3, 1), gs::rgb4(4, 3, 2), gs::rgb4(10, 8, 4)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(12, 10, 6), gs::rgb4(14, 12, 5)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(12, 12, 8)});

    art.sentry[0] = gs::uploadMipped(vdp, sentryArt(0));
    art.sentry[1] = gs::uploadMipped(vdp, sentryArt(1));
    art.shovel = gs::uploadMipped(vdp, shovelArt());
    art.rubble = gs::uploadMipped(vdp, rubbleArt());
    art.coil = gs::uploadMipped(vdp, coilArt());
    art.shell = gs::uploadMipped(vdp, shellArt());
    art.bags = gs::uploadMipped(vdp, bagsArt());
    art.timber = gs::uploadMipped(vdp, timberArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.bunker = gs::uploadMipped(vdp, bunkerArt());
    art.slit = gs::uploadMipped(vdp, slitArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    glyphs(vdp, art);
    vdp.setFogColor(gs::rgb4(1, 1, 2));
}

}  // namespace bcler
