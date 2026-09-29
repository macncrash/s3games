#include "game/art.h"

namespace memorybell {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t cs[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, cs[i]);
}

void inkPal(gs::VDP& vdp, int pal, uint16_t c, uint16_t shade) {
    uint16_t cs[16] = {};
    cs[1] = c;
    cs[15] = shade;
    setPal(vdp, pal, cs);
}

gs::Bitmap bellBmp() {
    gs::Bitmap b(28, 44);
    b.rect(13, 0, 2, 5, 1);
    b.ellipse(14, 8, 3, 2, 6);
    b.ellipse(14, 16, 9, 8, 2);
    b.ellipse(12, 14, 3, 4, 3);
    b.poly({{5, 16}, {23, 16}, {26, 32}, {2, 32}}, 2);
    b.poly({{6, 18}, {12, 18}, {10, 30}, {4, 30}}, 3);
    b.rect(2, 31, 24, 4, 4);
    b.rect(2, 34, 24, 1, 1);
    b.ellipse(14, 38, 2.2f, 2.4f, 5);
    b.rect(13, 22, 2, 10, 5);
    return b;
}

gs::Bitmap markBmp() {
    gs::Bitmap b(14, 10);
    b.poly({{7, 0}, {14, 10}, {0, 10}}, 1);
    b.poly({{7, 3}, {11, 9}, {3, 9}}, 2);
    return b;
}

gs::Bitmap beamBmp() {
    gs::Bitmap b(64, 10);
    b.rect(0, 0, 64, 10, 1);
    b.rect(0, 2, 64, 5, 2);
    b.rect(0, 7, 64, 2, 3);
    for (int x = 6; x < 64; x += 14) b.ellipse(float(x), 4.5f, 1.6f, 1.6f, 4);
    return b;
}

gs::Bitmap pipBmp() {
    gs::Bitmap b(8, 8);
    b.ellipse(4, 4, 3.2f, 3.2f, 1);
    b.ellipse(3, 3, 1.2f, 1.2f, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

void bellPal(gs::VDP& vdp, int pal, uint16_t body, uint16_t hi, uint16_t lip, uint16_t rib) {
    uint16_t cs[16] = {};
    cs[1] = gs::rgb4(2, 2, 3);
    cs[2] = body;
    cs[3] = hi;
    cs[4] = lip;
    cs[5] = gs::rgb4(4, 4, 5);
    cs[6] = rib;
    setPal(vdp, pal, cs);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    bellPal(vdp, PAL_B0, gs::rgb4(12, 9, 3), gs::rgb4(15, 13, 6), gs::rgb4(8, 6, 2), gs::rgb4(13, 2, 2));
    bellPal(vdp, PAL_B1, gs::rgb4(11, 6, 3), gs::rgb4(15, 10, 6), gs::rgb4(7, 3, 2), gs::rgb4(2, 5, 13));
    bellPal(vdp, PAL_B2, gs::rgb4(10, 11, 12), gs::rgb4(15, 15, 15), gs::rgb4(6, 7, 8), gs::rgb4(2, 11, 4));
    bellPal(vdp, PAL_B3, gs::rgb4(6, 6, 8), gs::rgb4(11, 11, 13), gs::rgb4(3, 3, 5), gs::rgb4(13, 10, 2));
    inkPal(vdp, PAL_INK, gs::rgb4(14, 14, 15), gs::rgb4(1, 1, 3));
    inkPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(4, 2, 0));
    inkPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 3), gs::rgb4(3, 0, 0));
    inkPal(vdp, PAL_OK, gs::rgb4(6, 15, 7), gs::rgb4(0, 3, 1));
    inkPal(vdp, PAL_DIM, gs::rgb4(7, 8, 10), gs::rgb4(1, 1, 2));
    uint16_t beam[16] = {};
    beam[1] = gs::rgb4(4, 3, 2);
    beam[2] = gs::rgb4(8, 6, 3);
    beam[3] = gs::rgb4(3, 2, 1);
    beam[4] = gs::rgb4(12, 10, 4);
    setPal(vdp, PAL_BEAM, beam);

    art.bell = gs::uploadMipped(vdp, bellBmp());
    art.mark = gs::uploadMipped(vdp, markBmp());
    art.beam = gs::uploadMipped(vdp, beamBmp());
    art.pip = gs::uploadMipped(vdp, pipBmp());
    loadFont(vdp, art);
}

}  // namespace memorybell
