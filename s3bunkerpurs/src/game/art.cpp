#include "game/art.h"

#include <cstdint>

namespace bunkerpurs {
namespace {

void pal(gs::VDP& v, int p, const uint16_t* c, int n) {
    for (int i = 0; i < n; i++) v.setColor(p * 16 + 1 + i, c[i]);
}

gs::Bitmap glyphBmp(char ch) {
    gs::Bitmap b(5, 7);
    const uint8_t* g = gs::glyph(ch);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (g[y * 5 + x]) b.set(x, y, 1);
    return b;
}

void wallTile(gs::VDP& vdp) {
    uint8_t px[64];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int n = (x * 13 + y * 7 + (x ^ y) * 3) & 15;
            int c = 1;
            if (n > 11) c = 2;
            if (n > 14) c = 3;
            if (y == 0 || x == 0) c = 4;
            px[y * 8 + x] = uint8_t(c);
        }
    }
    vdp.loadTile(1, px);
    uint8_t seam[64];
    for (int i = 0; i < 64; i++) seam[i] = (i % 8 == 0) ? 5 : 1;
    vdp.loadTile(2, seam);
}

gs::Bitmap plantBmp() {
    gs::Bitmap b(56, 36);
    b.rect(4, 16, 48, 16, 2);
    b.rect(8, 8, 18, 12, 3);
    b.rect(28, 6, 8, 14, 4);
    b.rect(38, 10, 10, 8, 3);
    b.rect(6, 28, 44, 6, 5);
    b.rect(10, 30, 8, 4, 1);
    b.rect(22, 30, 8, 4, 1);
    b.rect(34, 30, 8, 4, 1);
    b.rect(12, 12, 6, 4, 6);
    b.rect(44, 18, 8, 4, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap lightBmp() {
    gs::Bitmap b(40, 24);
    b.rect(4, 8, 28, 12, 2);
    b.rect(28, 10, 8, 6, 3);
    b.rect(8, 4, 10, 6, 4);
    b.rect(6, 18, 8, 4, 5);
    b.rect(18, 18, 8, 4, 5);
    b.rect(30, 12, 6, 3, 6);
    b.rect(10, 11, 5, 4, 7);
    b.outline(8, false);
    return b;
}

gs::Bitmap heavyBmp() {
    gs::Bitmap b(52, 28);
    b.rect(10, 8, 36, 14, 2);
    b.poly({{2, 16}, {12, 8}, {12, 20}, {2, 22}}, 3);
    b.rect(16, 4, 16, 6, 4);
    b.rect(12, 20, 10, 6, 5);
    b.rect(26, 20, 10, 6, 5);
    b.rect(38, 20, 8, 5, 5);
    b.rect(34, 11, 8, 5, 6);
    b.rect(18, 11, 6, 4, 7);
    b.outline(8, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    static const uint16_t hud[] = {gs::rgb4(15, 15, 14), gs::rgb4(14, 11, 4), gs::rgb4(8, 8, 7), gs::rgb4(15, 6, 3)};
    static const uint16_t conc[] = {gs::rgb4(3, 3, 4), gs::rgb4(5, 5, 6), gs::rgb4(7, 7, 7), gs::rgb4(2, 2, 3),
                                     gs::rgb4(1, 1, 2)};
    static const uint16_t plant[] = {gs::rgb4(1, 2, 1), gs::rgb4(3, 5, 2), gs::rgb4(5, 7, 3), gs::rgb4(8, 9, 5),
                                      gs::rgb4(2, 2, 2), gs::rgb4(14, 12, 3), gs::rgb4(15, 8, 2), gs::rgb4(0, 0, 0)};
    static const uint16_t light[] = {gs::rgb4(3, 2, 1), gs::rgb4(8, 4, 2), gs::rgb4(12, 6, 2), gs::rgb4(6, 5, 4),
                                      gs::rgb4(2, 2, 2), gs::rgb4(15, 13, 6), gs::rgb4(4, 8, 10), gs::rgb4(1, 0, 0)};
    static const uint16_t heavy[] = {gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 6), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5),
                                      gs::rgb4(1, 1, 1), gs::rgb4(14, 4, 2), gs::rgb4(10, 8, 3), gs::rgb4(0, 0, 0)};
    static const uint16_t bolt[] = {gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 2), gs::rgb4(15, 4, 2), gs::rgb4(8, 12, 14)};
    static const uint16_t fx[] = {gs::rgb4(15, 15, 12), gs::rgb4(12, 12, 12), gs::rgb4(6, 6, 6), gs::rgb4(14, 7, 2)};
    static const uint16_t door[] = {gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5), gs::rgb4(14, 3, 2), gs::rgb4(8, 2, 1),
                                     gs::rgb4(10, 10, 8)};
    static const uint16_t wreck[] = {gs::rgb4(2, 2, 2), gs::rgb4(4, 4, 4), gs::rgb4(6, 5, 4), gs::rgb4(3, 3, 3),
                                      gs::rgb4(1, 1, 1), gs::rgb4(5, 3, 2), gs::rgb4(3, 3, 3), gs::rgb4(0, 0, 0)};
    pal(vdp, PAL_HUD, hud, 4);
    pal(vdp, PAL_CONC, conc, 5);
    pal(vdp, PAL_PLANT, plant, 8);
    pal(vdp, PAL_LIGHT, light, 8);
    pal(vdp, PAL_HEAVY, heavy, 8);
    pal(vdp, PAL_BOLT, bolt, 4);
    pal(vdp, PAL_FX, fx, 4);
    pal(vdp, PAL_DOOR, door, 5);
    pal(vdp, PAL_WRECK, wreck, 8);
    vdp.setFogColor(gs::rgb4(1, 1, 2));

    wallTile(vdp);
    for (int cy = 0; cy < 28; cy++) {
        for (int cx = 0; cx < 40; cx++) {
            int tile = ((cx + cy) % 7 == 0) ? 2 : 1;
            vdp.B.set(cx, cy, gs::entry(tile, PAL_CONC));
        }
    }

    art.plant = gs::uploadMipped(vdp, plantBmp());
    art.lightM = gs::uploadMipped(vdp, lightBmp());
    art.heavyM = gs::uploadMipped(vdp, heavyBmp());

    gs::Bitmap boltB(10, 4);
    boltB.rect(0, 1, 10, 2, 1);
    boltB.rect(6, 0, 4, 4, 2);
    art.bolt = gs::uploadMipped(vdp, boltB);
    gs::Bitmap eb(8, 4);
    eb.rect(0, 1, 8, 2, 3);
    eb.rect(0, 0, 3, 4, 4);
    art.ebolt = gs::uploadMipped(vdp, eb);

    gs::Bitmap sp(6, 6);
    sp.rect(2, 0, 2, 6, 1);
    sp.rect(0, 2, 6, 2, 2);
    art.spark = gs::uploadMipped(vdp, sp);

    gs::Bitmap slab(18, 64);
    slab.rect(2, 0, 14, 64, 1);
    slab.rect(6, 4, 6, 56, 2);
    for (int i = 0; i < 4; i++) slab.rect(4, 8 + i * 14, 10, 3, 5);
    slab.rect(7, 28, 4, 4, 3);
    art.door = gs::uploadMipped(vdp, slab);

    gs::Bitmap bay(200, 18);
    bay.rect(0, 0, 200, 18, 1);
    bay.rect(0, 16, 200, 2, 2);
    art.bay = gs::uploadMipped(vdp, bay);

    gs::Bitmap lamp(8, 8);
    lamp.ellipse(4, 4, 3, 3, 1);
    lamp.rect(3, 3, 2, 2, 2);
    art.lamp = gs::uploadMipped(vdp, lamp);

    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    big.outline = 3;
    big.shadow = 4;
    art.titleA = gs::uploadMipped(vdp, gs::textBitmap("S3 BUNKER", big));
    art.titleB = gs::uploadMipped(vdp, gs::textBitmap("PURSUIT", big));
    gs::TextStyle sub;
    sub.scale = 1;
    sub.color = 2;
    sub.shadow = 4;
    art.sub = gs::uploadMipped(vdp, gs::textBitmap("LAST MACHINE STILL RUNNING", sub));

    for (int i = 0; i < 96; i++) art.glyph[i] = gs::uploadMipped(vdp, glyphBmp(char(32 + i)));
}

}  // namespace bunkerpurs
