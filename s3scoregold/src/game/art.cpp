#include "game/art.h"

namespace scoregold {
namespace {

void ramp(gs::VDP& v, int p, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) v.setColor(p * 16 + i, c[i]);
}

void palettes(gs::VDP& v) {
    uint16_t z[16] = {};
    z[1] = gs::rgb4(15, 14, 8);
    z[14] = gs::rgb4(15, 15, 12);
    z[15] = gs::rgb4(2, 3, 2);
    ramp(v, PAL_HUD, z);

    uint16_t wood[16] = {};
    wood[1] = gs::rgb4(1, 2, 1);
    wood[2] = gs::rgb4(6, 4, 2);
    wood[3] = gs::rgb4(10, 7, 3);
    wood[4] = gs::rgb4(4, 8, 3);
    wood[5] = gs::rgb4(2, 5, 2);
    wood[6] = gs::rgb4(13, 10, 5);
    ramp(v, PAL_WOOD, wood);

    uint16_t chalk[16] = {};
    chalk[1] = gs::rgb4(14, 15, 13);
    chalk[2] = gs::rgb4(8, 9, 8);
    ramp(v, PAL_CHALK, chalk);

    uint16_t cream[16] = {};
    cream[1] = gs::rgb4(15, 13, 9);
    cream[2] = gs::rgb4(12, 9, 6);
    ramp(v, PAL_CREAM, cream);

    uint16_t gold[16] = {};
    gold[1] = gs::rgb4(15, 12, 2);
    gold[2] = gs::rgb4(15, 15, 8);
    gold[3] = gs::rgb4(10, 7, 1);
    ramp(v, PAL_GOLD, gold);

    uint16_t bad[16] = {};
    bad[1] = gs::rgb4(14, 3, 2);
    ramp(v, PAL_BAD, bad);

    uint16_t ok[16] = {};
    ok[1] = gs::rgb4(6, 14, 5);
    ok[2] = gs::rgb4(15, 14, 6);
    ramp(v, PAL_OK, ok);

    uint16_t ink[16] = {};
    ink[1] = gs::rgb4(15, 14, 10);
    ink[2] = gs::rgb4(3, 4, 3);
    ramp(v, PAL_INK, ink);
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

gs::Bitmap boardArt() {
    gs::Bitmap b(220, 150);
    b.rect(0, 0, 220, 150, 2);
    b.rect(8, 8, 204, 134, 3);
    b.rect(14, 14, 192, 122, 5);
    for (int i = 0; i < 6; i++) b.rect(22, float(28 + i * 18), 176, 1, 4);
    b.rect(18, 16, 8, 8, 6);
    b.rect(194, 16, 8, 8, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap ledgeArt() {
    gs::Bitmap b(240, 28);
    b.rect(4, 4, 232, 16, 3);
    b.rect(4, 18, 232, 6, 2);
    b.rect(20, 8, 40, 6, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap penArt() {
    gs::Bitmap b(48, 12);
    b.rect(2, 4, 36, 4, 2);
    b.rect(36, 3, 8, 6, 1);
    b.rect(42, 4, 4, 4, 3);
    return b;
}

gs::Bitmap slashArt() {
    gs::Bitmap b(16, 22);
    b.line(3, 18, 12, 3, 1, 2.2f);
    return b;
}

gs::Bitmap creamArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 6, 1);
    b.ellipse(9, 8, 4, 3, 2);
    return b;
}

gs::Bitmap goldArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 3);
    b.ellipse(10, 10, 6, 6, 1);
    b.ellipse(8, 8, 2, 2, 2);
    return b;
}

gs::Image words(gs::VDP& v, const char* s, int color) {
    gs::TextStyle st;
    st.scale = 2;
    st.color = color;
    st.outline = 0;
    st.spacing = 1;
    return gs::uploadImage(v, gs::textBitmap(s, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    palettes(vdp);
    vdp.setFogColor(gs::rgb4(1, 2, 1));
    gs::TileAlloc tiles(vdp);
    font(vdp, tiles, art);
    art.board = gs::uploadMipped(vdp, boardArt());
    art.ledge = gs::uploadMipped(vdp, ledgeArt());
    art.pen = gs::uploadMipped(vdp, penArt());
    art.bare = gs::uploadMipped(vdp, slashArt());
    art.cream = gs::uploadMipped(vdp, creamArt());
    art.gold = gs::uploadMipped(vdp, goldArt());
    gs::Bitmap sq(8, 8);
    sq.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadImage(vdp, sq);
    art.title = words(vdp, "SCORE GOLD", 1);
    art.rule = words(vdp, "ONLY GOLD DOUBLES", 1);
    art.filed = words(vdp, "SHEET FILED", 1);
    art.shorted = words(vdp, "SHEET SHORT", 1);
}

}  // namespace scoregold
