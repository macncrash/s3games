#include "game/art.h"

namespace maga {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap archArt() {
    gs::Bitmap b(48, 96);
    b.rect(6, 8, 36, 84, 2);
    b.rect(10, 4, 28, 10, 3);
    b.rect(14, 28, 20, 36, 4);
    b.poly({{14, 28}, {24, 16}, {34, 28}}, 4);
    b.rect(4, 86, 40, 8, 1);
    for (int i = 0; i < 5; i++) b.rect(8, 14 + i * 16, 32, 2, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap manArt(bool real) {
    gs::Bitmap b(40, 64);
    b.ellipse(20, 10, 7, 7, 3);
    b.rect(14, 18, 12, 18, real ? 1 : 2);
    b.rect(8, 20, 6, 14, real ? 2 : 4);
    b.rect(26, 20, 6, 14, real ? 2 : 4);
    b.rect(14, 36, 5, 20, 5);
    b.rect(21, 36, 5, 20, 5);
    if (real) {
        b.line(28, 22, 38, 16, 6, 2.0f);
        b.rect(34, 14, 5, 3, 6);
        b.rect(16, 22, 8, 6, 7);
    } else {
        b.rect(12, 18, 16, 4, 7);
        b.ellipse(20, 40, 6, 3, 7);
    }
    b.outline(8, false);
    return b;
}

gs::Bitmap gunArt() {
    gs::Bitmap b(96, 40);
    b.rect(8, 16, 70, 12, 2);
    b.rect(70, 18, 22, 6, 3);
    b.rect(18, 28, 28, 8, 1);
    b.rect(4, 12, 16, 18, 4);
    b.rect(40, 10, 10, 6, 5);
    b.outline(6, false);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 14, 12), gs::rgb4(8, 8, 9), gs::rgb4(15, 12, 4), gs::rgb4(15, 5, 3),
                          gs::rgb4(6, 14, 8), gs::rgb4(10, 12, 14), gs::rgb4(4, 4, 5), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 11, 9), gs::rgb4(8, 8, 7), gs::rgb4(14, 13, 11), gs::rgb4(3, 4, 6),
                            gs::rgb4(6, 6, 6), gs::rgb4(2, 2, 2), gs::rgb4(15, 14, 10), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_REAL, {0, gs::rgb4(12, 3, 2), gs::rgb4(7, 2, 2), gs::rgb4(14, 11, 8), gs::rgb4(5, 5, 6),
                           gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2), gs::rgb4(15, 13, 6), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FEINT, {0, gs::rgb4(10, 10, 11), gs::rgb4(7, 7, 8), gs::rgb4(13, 12, 11), gs::rgb4(5, 5, 6),
                            gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 2), gs::rgb4(13, 12, 6), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GUN, {0, gs::rgb4(5, 5, 4), gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 4), gs::rgb4(10, 8, 5),
                          gs::rgb4(14, 12, 6), gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 8, 2), gs::rgb4(15, 15, 13), gs::rgb4(15, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t field[16] = {
        0,
        gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 5), gs::rgb4(4, 4, 4),
        gs::rgb4(9, 8, 7), gs::rgb4(6, 6, 5),
        gs::rgb4(8, 7, 6), gs::rgb4(5, 5, 4),
        gs::rgb4(3, 3, 3), gs::rgb4(6, 5, 4), gs::rgb4(4, 4, 3),
        gs::rgb4(3, 5, 7), gs::rgb4(2, 4, 6), gs::rgb4(8, 10, 11),
        gs::rgb4(12, 11, 8), gs::rgb4(9, 8, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);
    vdp.setFogColor(gs::rgb4(6, 7, 10));

    loadFont(vdp, art);
    art.arch = gs::uploadMipped(vdp, archArt());
    art.real = gs::uploadMipped(vdp, manArt(true));
    art.feint = gs::uploadMipped(vdp, manArt(false));
    art.gun = gs::uploadMipped(vdp, gunArt());
    gs::Bitmap flash(20, 16);
    flash.ellipse(10, 8, 9, 6, 1);
    flash.ellipse(10, 8, 4, 3, 3);
    art.flash = gs::uploadMipped(vdp, flash);
    gs::Bitmap chev(24, 16);
    chev.poly({{2, 2}, {12, 14}, {22, 2}, {12, 6}}, 1);
    art.chev = gs::uploadMipped(vdp, chev);
}

}  // namespace maga
