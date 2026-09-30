#include "game/art.h"

namespace scoretape {
namespace {

void ramp(gs::VDP& v, int p, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) v.setColor(p * 16 + i, c[i]);
}

void palettes(gs::VDP& v) {
    uint16_t z[16] = {};
    z[1] = gs::rgb4(15, 14, 9);
    z[14] = gs::rgb4(15, 15, 12);
    z[15] = gs::rgb4(2, 2, 1);
    ramp(v, PAL_HUD, z);

    uint16_t gold[16] = {};
    gold[1] = gs::rgb4(15, 12, 2);
    gold[2] = gs::rgb4(15, 15, 8);
    ramp(v, PAL_GOLD, gold);

    uint16_t dim[16] = {};
    dim[1] = gs::rgb4(7, 7, 6);
    ramp(v, PAL_DIM, dim);

    uint16_t bad[16] = {};
    bad[1] = gs::rgb4(14, 3, 2);
    ramp(v, PAL_BAD, bad);

    uint16_t paper[16] = {};
    paper[1] = gs::rgb4(15, 14, 11);
    paper[2] = gs::rgb4(10, 8, 6);
    paper[3] = gs::rgb4(4, 3, 2);
    paper[4] = gs::rgb4(12, 4, 3);
    ramp(v, PAL_PAPER, paper);

    uint16_t ink[16] = {};
    ink[1] = gs::rgb4(2, 2, 3);
    ink[2] = gs::rgb4(6, 5, 4);
    ramp(v, PAL_INK, ink);

    uint16_t wood[16] = {};
    wood[1] = gs::rgb4(6, 3, 1);
    wood[2] = gs::rgb4(10, 6, 2);
    wood[3] = gs::rgb4(3, 2, 1);
    ramp(v, PAL_WOOD, wood);

    uint16_t q[16] = {};
    q[1] = gs::rgb4(2, 2, 8);
    q[2] = gs::rgb4(8, 8, 14);
    ramp(v, PAL_QUAVER, q);

    uint16_t m[16] = {};
    m[1] = gs::rgb4(1, 6, 3);
    m[2] = gs::rgb4(10, 14, 8);
    ramp(v, PAL_MINIM, m);

    uint16_t b[16] = {};
    b[1] = gs::rgb4(8, 4, 1);
    b[2] = gs::rgb4(15, 12, 6);
    ramp(v, PAL_BREVE, b);

    uint16_t r[16] = {};
    r[1] = gs::rgb4(10, 2, 4);
    r[2] = gs::rgb4(15, 8, 8);
    ramp(v, PAL_REST, r);
}

void font(gs::VDP& v, gs::TileAlloc& tiles, Art& a) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        a.font[c - 32] = tiles.shared(px);
    }
}

gs::Bitmap sheetArt() {
    gs::Bitmap b(200, 72);
    b.rect(0, 0, 200, 72, 1);
    b.rect(4, 4, 192, 64, 2);
    for (int i = 0; i < 5; i++) b.rect(14, float(18 + i * 8), 172, 1, 3);
    b.rect(18, 16, 2, 34, 3);
    b.rect(22, 16, 6, 10, 4);
    b.outline(3, false);
    return b;
}

gs::Bitmap quaverArt() {
    gs::Bitmap b(18, 28);
    b.ellipse(6, 20, 5, 4, 1);
    b.rect(10, 4, 2, 16, 1);
    b.line(12, 4, 16, 10, 1, 1.6f);
    b.ellipse(5, 18, 2, 1, 2);
    return b;
}

gs::Bitmap minimArt() {
    gs::Bitmap b(16, 28);
    b.ellipse(6, 20, 5, 4, 2);
    b.ellipse(6, 20, 3, 2, 0);
    b.rect(10, 4, 2, 16, 1);
    return b;
}

gs::Bitmap breveArt() {
    gs::Bitmap b(20, 16);
    b.rect(2, 3, 16, 10, 2);
    b.rect(4, 5, 12, 6, 0);
    b.rect(1, 2, 2, 12, 1);
    b.rect(17, 2, 2, 12, 1);
    return b;
}

gs::Bitmap restArt() {
    gs::Bitmap b(14, 22);
    b.rect(3, 4, 8, 3, 1);
    b.rect(3, 8, 8, 3, 2);
    b.line(4, 12, 10, 18, 1, 1.5f);
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(40, 22);
    b.rect(0, 0, 40, 22, 3);
    b.rect(2, 2, 36, 16, 1);
    b.rect(16, 16, 8, 3, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 12);
    b.ellipse(5, 5, 4, 4, 1);
    b.rect(4, 9, 2, 3, 2);
    return b;
}

gs::Bitmap penArt() {
    gs::Bitmap b(36, 10);
    b.rect(2, 3, 24, 4, 2);
    b.rect(24, 2, 8, 6, 1);
    b.rect(30, 3, 4, 4, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    vdp.setFogColor(gs::rgb4(3, 2, 1));
    gs::TileAlloc tiles(vdp);
    font(vdp, tiles, art);
    art.sheet = gs::uploadImage(vdp, sheetArt());
    art.quaver = gs::uploadImage(vdp, quaverArt());
    art.minim = gs::uploadImage(vdp, minimArt());
    art.breve = gs::uploadImage(vdp, breveArt());
    art.rest = gs::uploadImage(vdp, restArt());
    art.drawer = gs::uploadImage(vdp, drawerArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
    art.pen = gs::uploadImage(vdp, penArt());
}

}  // namespace scoretape
