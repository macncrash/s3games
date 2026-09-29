#include "game/art.h"

#include <initializer_list>

namespace qcler {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i >= 16) break;
        vdp.setColor(pal * 16 + i++, c);
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap loaderArt(int step) {
    gs::Bitmap b(48, 28);
    b.rect(8, 8, 22, 10, 1);
    b.rect(10, 4, 12, 6, 4);
    b.rect(12, 5, 6, 3, 5);
    b.rect(26, 10, 14, 6, 2);
    b.rect(4, 16, 36, 4, 3);
    if (step == 0) {
        b.ellipse(12, 22, 6, 5, 6);
        b.ellipse(34, 22, 6, 5, 6);
        b.ellipse(12, 22, 2, 2, 7);
        b.ellipse(34, 22, 2, 2, 7);
    } else {
        b.ellipse(14, 22, 6, 5, 6);
        b.ellipse(32, 22, 6, 5, 6);
        b.ellipse(14, 22, 2, 2, 7);
        b.ellipse(32, 22, 2, 2, 7);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap bucketArt(int raised) {
    gs::Bitmap b(22, 16);
    if (raised) {
        b.poly({{2, 10}, {18, 4}, {20, 8}, {6, 14}}, 1);
        b.rect(4, 6, 10, 3, 2);
    } else {
        b.poly({{2, 4}, {18, 4}, {20, 12}, {4, 14}}, 1);
        b.rect(4, 6, 12, 3, 2);
    }
    b.outline(3, false);
    return b;
}

gs::Bitmap slabArt() {
    gs::Bitmap b(36, 16);
    b.poly({{2, 10}, {10, 4}, {32, 6}, {30, 14}, {6, 14}}, 1);
    b.rect(12, 7, 10, 3, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap boulderArt() {
    gs::Bitmap b(28, 22);
    b.ellipse(14, 12, 12, 8, 1);
    b.ellipse(10, 10, 4, 3, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap spoilArt() {
    gs::Bitmap b(32, 18);
    b.poly({{4, 14}, {10, 6}, {16, 12}, {22, 4}, {30, 14}}, 1);
    b.ellipse(16, 14, 12, 3, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap oreArt() {
    gs::Bitmap b(24, 20);
    b.poly({{4, 16}, {8, 4}, {16, 8}, {20, 4}, {22, 16}}, 1);
    b.rect(9, 9, 5, 4, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap blockArt() {
    gs::Bitmap b(26, 18);
    b.rect(3, 4, 20, 12, 1);
    b.rect(3, 4, 20, 3, 2);
    b.rect(3, 4, 4, 12, 4);
    b.outline(3, false);
    return b;
}

gs::Bitmap chunkArt() {
    gs::Bitmap b(20, 16);
    b.poly({{2, 12}, {8, 3}, {16, 6}, {18, 13}, {6, 14}}, 1);
    b.outline(2, false);
    return b;
}

gs::Bitmap crusherArt() {
    gs::Bitmap b(70, 64);
    b.rect(8, 18, 48, 36, 1);
    b.poly({{8, 18}, {32, 6}, {56, 18}}, 2);
    b.rect(16, 26, 16, 12, 4);
    b.rect(18, 28, 6, 6, 5);
    b.rect(36, 30, 14, 20, 3);
    b.rect(4, 50, 58, 8, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap hopperArt() {
    gs::Bitmap b(36, 22);
    b.poly({{2, 2}, {34, 2}, {28, 18}, {8, 18}}, 1);
    b.rect(10, 14, 16, 6, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap beltArt() {
    gs::Bitmap b(40, 10);
    b.rect(1, 2, 38, 6, 1);
    for (int x = 4; x < 36; x += 8) b.rect(float(x), 3.f, 3.f, 4.f, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(80, 36);
    b.poly({{2, 32}, {18, 8}, {62, 6}, {78, 30}}, 1);
    b.line(18, 8, 22, 32, 2, 1.4f);
    b.line(40, 6, 42, 32, 2, 1.2f);
    b.line(60, 7, 58, 32, 2, 1.2f);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 2, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(24, 8);
    b.ellipse(12, 4, 10, 3, 1);
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
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(5, 14, 6)});
    setPal(vdp, PAL_LOADER,
           {0, gs::rgb4(14, 11, 2), gs::rgb4(11, 8, 1), gs::rgb4(8, 6, 2), gs::rgb4(6, 8, 9), gs::rgb4(10, 13, 14),
            gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 5), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_ROCK,
           {0, gs::rgb4(8, 7, 5), gs::rgb4(6, 5, 4), gs::rgb4(4, 3, 2), gs::rgb4(10, 9, 7)});
    setPal(vdp, PAL_ORE, {0, gs::rgb4(7, 6, 4), gs::rgb4(12, 9, 3), gs::rgb4(4, 3, 2), gs::rgb4(9, 7, 3)});
    setPal(vdp, PAL_STEEL,
           {0, gs::rgb4(7, 8, 8), gs::rgb4(5, 6, 6), gs::rgb4(3, 4, 4), gs::rgb4(9, 10, 10), gs::rgb4(12, 13, 12),
            gs::rgb4(4, 5, 6), gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WALL, {0, gs::rgb4(9, 8, 6), gs::rgb4(7, 6, 5), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(12, 10, 7), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_BELT, {0, gs::rgb4(3, 3, 3), gs::rgb4(10, 8, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_CAB, {0, gs::rgb4(14, 10, 2), gs::rgb4(8, 6, 2), gs::rgb4(1, 1, 1)});

    art.loader[0] = gs::uploadMipped(vdp, loaderArt(0));
    art.loader[1] = gs::uploadMipped(vdp, loaderArt(1));
    art.bucket[0] = gs::uploadMipped(vdp, bucketArt(0));
    art.bucket[1] = gs::uploadMipped(vdp, bucketArt(1));
    art.slab = gs::uploadMipped(vdp, slabArt());
    art.boulder = gs::uploadMipped(vdp, boulderArt());
    art.spoil = gs::uploadMipped(vdp, spoilArt());
    art.ore = gs::uploadMipped(vdp, oreArt());
    art.block = gs::uploadMipped(vdp, blockArt());
    art.chunk = gs::uploadMipped(vdp, chunkArt());
    art.crusher = gs::uploadMipped(vdp, crusherArt());
    art.hopper = gs::uploadMipped(vdp, hopperArt());
    art.belt = gs::uploadMipped(vdp, beltArt());
    art.face = gs::uploadMipped(vdp, faceArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    glyphs(vdp, art);
    vdp.setFogColor(gs::rgb4(6, 5, 4));
}

}  // namespace qcler
