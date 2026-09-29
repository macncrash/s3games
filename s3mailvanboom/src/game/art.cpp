#include "game/art.h"

#include <initializer_list>

namespace boom {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

// Nose of the postal van, with the sealed drive sitting on the dash.
gs::Bitmap hoodArt() {
    gs::Bitmap b(200, 78);
    b.poly({{10, 28}, {40, 6}, {160, 6}, {190, 28}, {200, 78}, {0, 78}}, 1);
    b.rect(18, 32, 164, 22, 2);
    b.rect(26, 36, 42, 12, 3);
    b.rect(132, 36, 42, 12, 3);
    b.rect(78, 12, 44, 16, 4);
    b.rect(84, 16, 32, 8, 5);
    b.rect(88, 42, 24, 10, 6);
    b.rect(92, 44, 16, 6, 7);
    b.rect(0, 60, 200, 16, 8);
    for (int i = 0; i < 8; i++) b.rect(6.f + i * 24.f, 64, 14, 6, i % 2 ? 9 : 10);
    b.rect(4, 22, 16, 6, 11);
    b.rect(180, 22, 16, 6, 11);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(64, 48);
    b.poly({{6, 20}, {16, 6}, {48, 6}, {58, 20}, {64, 48}, {0, 48}}, 1);
    b.rect(10, 22, 44, 12, 2);
    b.rect(16, 10, 32, 8, 3);
    b.rect(22, 28, 20, 8, 4);
    b.rect(0, 38, 64, 8, 5);
    return b;
}

gs::Bitmap armArt() {
    gs::Bitmap b(180, 22);
    for (int i = 0; i < 9; i++) b.rect(float(i * 20), 2, 20, 16, i % 2 ? 1 : 2);
    b.rect(0, 0, 180, 3, 3);
    b.rect(0, 18, 180, 3, 3);
    b.rect(8, 6, 10, 8, 4);
    return b;
}

gs::Bitmap armUpArt() {
    gs::Bitmap b(28, 90);
    for (int i = 0; i < 8; i++) b.rect(4, float(i * 11), 18, 11, i % 2 ? 1 : 2);
    b.rect(2, 0, 4, 90, 3);
    b.rect(22, 0, 4, 90, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(28, 72);
    b.rect(8, 8, 12, 62, 1);
    b.rect(4, 4, 20, 12, 2);
    b.rect(10, 18, 8, 8, 3);
    b.rect(6, 64, 16, 8, 4);
    return b;
}

gs::Bitmap bollardArt() {
    gs::Bitmap b(16, 36);
    b.rect(4, 4, 8, 28, 1);
    b.rect(4, 4, 8, 8, 2);
    b.rect(4, 16, 8, 6, 2);
    b.rect(2, 30, 12, 4, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(14, 13, 10));
    textPal(vdp, PAL_CREAM, gs::rgb4(15, 14, 8));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 14, 7));

    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(14, 12, 6), gs::rgb4(12, 9, 4), gs::rgb4(3, 3, 4), gs::rgb4(8, 10, 12), gs::rgb4(4, 8, 12),
            gs::rgb4(10, 4, 3), gs::rgb4(15, 13, 8), gs::rgb4(2, 2, 2), gs::rgb4(15, 12, 2), gs::rgb4(12, 3, 2),
            gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_RIVAL,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(6, 2, 2), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 4), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_BOOM, {0, gs::rgb4(14, 12, 2), gs::rgb4(2, 2, 2), gs::rgb4(8, 7, 5), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_POST,
           {0, gs::rgb4(6, 6, 7), gs::rgb4(12, 10, 3), gs::rgb4(14, 4, 3), gs::rgb4(4, 4, 4), gs::rgb4(13, 11, 2)});

    // Road bank: grass, verge, tarmac, yellow paint.
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(3, 8, 3), gs::rgb4(2, 6, 2), gs::rgb4(5, 9, 4), gs::rgb4(7, 7, 5), gs::rgb4(5, 5, 4),
            gs::rgb4(4, 4, 5), gs::rgb4(5, 5, 6), gs::rgb4(7, 7, 6), gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 5),
            gs::rgb4(2, 5, 8), gs::rgb4(3, 6, 9), gs::rgb4(5, 8, 11), gs::rgb4(14, 12, 3), gs::rgb4(8, 8, 9)});

    vdp.setFogColor(gs::rgb4(8, 9, 10));
    loadFont(vdp, art);
    art.hood = gs::uploadMipped(vdp, hoodArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.arm = gs::uploadMipped(vdp, armArt());
    art.armUp = gs::uploadMipped(vdp, armUpArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.bollard = gs::uploadMipped(vdp, bollardArt());
}

}  // namespace boom
