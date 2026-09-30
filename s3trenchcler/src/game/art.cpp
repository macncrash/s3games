#include "game/art.h"

#include <initializer_list>

namespace tcler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap diggerArt(int step) {
    gs::Bitmap b(28, 48);
    b.ellipse(14, 7, 7, 4, 3);
    b.rect(8, 6, 12, 3, 2);
    b.ellipse(14, 13, 5, 5, 4);
    b.set(12, 13, 8);
    b.set(16, 13, 8);
    b.poly({{8, 18}, {20, 18}, {23, 34}, {5, 34}}, 1);
    b.rect(12, 20, 4, 8, 5);
    b.line(6, 22, 2, 32, 6, 2.f);
    b.line(22, 22, 26, 30, 6, 2.f);
    if (step == 0) {
        b.rect(8, 34, 5, 10, 2);
        b.rect(15, 34, 5, 8, 2);
        b.rect(7, 43, 7, 3, 7);
        b.rect(14, 41, 7, 3, 7);
    } else {
        b.rect(8, 34, 5, 8, 2);
        b.rect(15, 34, 5, 10, 2);
        b.rect(7, 41, 7, 3, 7);
        b.rect(14, 43, 7, 3, 7);
    }
    b.outline(9, false);
    return b;
}

gs::Bitmap shovelArt() {
    gs::Bitmap b(12, 26);
    b.rect(5, 1, 2, 14, 2);
    b.poly({{2, 14}, {10, 14}, {11, 24}, {1, 24}}, 1);
    b.rect(3, 15, 6, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap spoilArt() {
    gs::Bitmap b(40, 22);
    b.ellipse(20, 14, 16, 6, 1);
    b.poly({{8, 14}, {16, 4}, {26, 12}, {12, 16}}, 2);
    b.poly({{18, 12}, {30, 3}, {36, 14}, {22, 16}}, 3);
    b.set(14, 8, 4);
    b.set(28, 7, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(30, 24);
    b.poly({{4, 10}, {15, 4}, {26, 10}, {15, 16}}, 1);
    b.poly({{4, 10}, {4, 18}, {15, 22}, {15, 16}}, 2);
    b.poly({{26, 10}, {26, 18}, {15, 22}, {15, 16}}, 3);
    b.line(15, 8, 15, 20, 4, 1.2f);
    b.outline(5, false);
    return b;
}

gs::Bitmap coilArt() {
    gs::Bitmap b(32, 22);
    b.ellipse(16, 12, 13, 8, 1);
    b.ellipse(16, 12, 7, 4, 2);
    b.ellipse(16, 12, 2, 2, 3);
    b.line(4, 8, 26, 6, 4, 1.3f);
    b.line(6, 16, 28, 15, 4, 1.3f);
    b.outline(5, false);
    return b;
}

gs::Bitmap bagsArt() {
    gs::Bitmap b(36, 22);
    b.ellipse(12, 13, 10, 6, 1);
    b.ellipse(22, 10, 10, 6, 2);
    b.rect(8, 8, 5, 2, 3);
    b.rect(18, 5, 5, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(44, 12);
    b.rect(2, 2, 40, 3, 1);
    b.rect(2, 6, 40, 3, 2);
    b.rect(8, 1, 2, 9, 3);
    b.rect(28, 1, 2, 9, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap shellArt() {
    gs::Bitmap b(34, 14);
    b.ellipse(6, 7, 5, 4, 2);
    b.rect(6, 3, 20, 8, 1);
    b.rect(24, 4, 6, 6, 3);
    b.rect(10, 5, 12, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 40);
    b.rect(3, 2, 4, 34, 1);
    b.rect(2, 8, 6, 2, 2);
    b.rect(2, 20, 6, 2, 2);
    b.rect(1, 34, 8, 4, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap lipArt() {
    gs::Bitmap b(48, 16);
    b.ellipse(12, 9, 10, 5, 1);
    b.ellipse(26, 7, 10, 5, 2);
    b.ellipse(38, 10, 8, 4, 1);
    b.rect(6, 4, 5, 2, 3);
    b.rect(22, 3, 5, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.set(3, 3, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(18, 8);
    b.ellipse(9, 4, 8, 3, 1);
    return b;
}

gs::Bitmap scrapeArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(8, 4, 6, 2, 1);
    b.line(3, 4, 13, 3, 2, 1.f);
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
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 5, 3)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(7, 14, 6)});
    setPal(vdp, PAL_MAN,
           {0, gs::rgb4(5, 6, 3), gs::rgb4(3, 4, 2), gs::rgb4(7, 6, 4), gs::rgb4(12, 9, 6), gs::rgb4(8, 7, 3),
            gs::rgb4(6, 5, 3), gs::rgb4(2, 2, 1), gs::rgb4(1, 1, 1), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_MUD,
           {0, gs::rgb4(5, 4, 2), gs::rgb4(7, 5, 2), gs::rgb4(4, 3, 1), gs::rgb4(8, 7, 4), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(6, 3, 1), gs::rgb4(10, 8, 4), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_STEEL,
           {0, gs::rgb4(8, 8, 6), gs::rgb4(5, 5, 4), gs::rgb4(11, 10, 7), gs::rgb4(13, 11, 5), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_BAG, {0, gs::rgb4(9, 8, 4), gs::rgb4(7, 6, 3), gs::rgb4(11, 9, 5), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_WIRE, {0, gs::rgb4(5, 6, 4), gs::rgb4(3, 4, 3), gs::rgb4(8, 8, 5), gs::rgb4(10, 8, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(11, 9, 5), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_LIP, {0, gs::rgb4(6, 5, 3), gs::rgb4(8, 6, 3), gs::rgb4(10, 8, 4), gs::rgb4(4, 3, 2)});
    // Road bank: indices match the road generator (ground, verge, mud, puddle).
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(4, 3, 2), gs::rgb4(3, 2, 1), gs::rgb4(6, 5, 3), gs::rgb4(7, 5, 2), gs::rgb4(5, 3, 1),
            gs::rgb4(6, 5, 2), gs::rgb4(4, 3, 1), gs::rgb4(5, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(7, 6, 3),
            gs::rgb4(3, 4, 5), gs::rgb4(2, 3, 4), gs::rgb4(5, 6, 7), gs::rgb4(9, 8, 5), gs::rgb4(8, 7, 4)});

    art.digger[0] = gs::uploadMipped(vdp, diggerArt(0));
    art.digger[1] = gs::uploadMipped(vdp, diggerArt(1));
    art.shovel = gs::uploadMipped(vdp, shovelArt());
    art.spoil = gs::uploadMipped(vdp, spoilArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.coil = gs::uploadMipped(vdp, coilArt());
    art.bags = gs::uploadMipped(vdp, bagsArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.shell = gs::uploadMipped(vdp, shellArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.lip = gs::uploadMipped(vdp, lipArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.scrape = gs::uploadMipped(vdp, scrapeArt());
    glyphs(vdp, art);
    vdp.setFogColor(gs::rgb4(3, 3, 2));
}

}  // namespace tcler
