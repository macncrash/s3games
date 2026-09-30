#include "game/art.h"

#include <initializer_list>

namespace wharfpace {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) vdp.setColor(pal * 16 + i++, c);
}

gs::Bitmap keeper(int step) {
    gs::Bitmap b(26, 46);
    b.rect(9, 1, 8, 7, 3);
    b.rect(10, 2, 6, 4, 8);
    b.ellipse(13, 0, 7, 3, 2);
    b.rect(7, 8, 12, 16, 1);
    b.rect(9, 11, 8, 7, 4);
    b.rect(4, 10, 3, 11, 5);
    b.rect(19, 10, 3, 11, 5);
    int lx = step ? 8 : 11;
    int rx = step ? 15 : 12;
    b.rect(lx, 24, 4, 15, 6);
    b.rect(rx, 24, 4, 15, 7);
    b.rect(lx - 1, 38, 6, 4, 9);
    b.rect(rx - 1, 38, 6, 4, 9);
    b.rect(18, 14, 7, 2, 10);
    return b;
}

gs::Bitmap downed() {
    gs::Bitmap b(46, 16);
    b.ellipse(22, 9, 16, 5, 1);
    b.rect(4, 5, 9, 7, 3);
    b.rect(28, 7, 12, 4, 4);
    b.rect(36, 10, 6, 3, 9);
    return b;
}

gs::Bitmap pile() {
    gs::Bitmap b(10, 48);
    b.rect(3, 0, 4, 48, 1);
    b.rect(2, 0, 6, 4, 2);
    for (int y = 8; y < 44; y += 10) b.rect(2, y, 6, 2, 3);
    return b;
}

gs::Bitmap lamp() {
    gs::Bitmap b(14, 22);
    b.rect(6, 0, 2, 5, 2);
    b.rect(2, 5, 10, 12, 1);
    b.rect(4, 7, 6, 8, 3);
    b.rect(5, 17, 4, 4, 2);
    return b;
}

gs::Bitmap gull(int wing) {
    gs::Bitmap b(28, 12);
    b.line(0, wing ? 8 : 3, 12, 6, 1, 2);
    b.line(12, 6, 27, wing ? 2 : 9, 1, 2);
    b.ellipse(13, 7, 2, 2, 2);
    return b;
}

gs::Bitmap crate() {
    gs::Bitmap b(22, 18);
    b.rect(1, 2, 20, 14, 1);
    b.line(1, 2, 20, 15, 2, 1);
    b.line(20, 2, 1, 15, 3, 1);
    b.rect(0, 0, 22, 3, 4);
    return b;
}

gs::Bitmap coil() {
    gs::Bitmap b(20, 14);
    b.ellipse(10, 7, 9, 5, 1);
    b.ellipse(10, 7, 4, 2, 2);
    b.ellipse(10, 7, 1, 1, 3);
    return b;
}

gs::Bitmap sight() {
    gs::Bitmap b(16, 16);
    b.rect(7, 0, 2, 5, 1);
    b.rect(7, 11, 2, 5, 1);
    b.rect(0, 7, 5, 2, 1);
    b.rect(11, 7, 5, 2, 1);
    b.rect(7, 7, 2, 2, 2);
    return b;
}

gs::Bitmap flash() {
    gs::Bitmap b(20, 16);
    b.ellipse(10, 8, 8, 4, 1);
    b.ellipse(10, 8, 3, 2, 2);
    return b;
}

gs::Bitmap splash() {
    gs::Bitmap b(22, 12);
    b.ellipse(11, 7, 9, 3, 1);
    b.ellipse(6, 5, 3, 2, 2);
    b.ellipse(16, 6, 3, 2, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    using gs::rgb4;
    setPal(vdp, PAL_TEXT, {0, rgb4(14, 13, 9), rgb4(6, 5, 4), rgb4(2, 2, 3)});
    setPal(vdp, PAL_LAMP, {0, rgb4(14, 11, 3), rgb4(8, 5, 2), rgb4(15, 15, 8)});
    setPal(vdp, PAL_ALERT, {0, rgb4(13, 3, 2), rgb4(8, 2, 2), rgb4(15, 8, 4)});
    setPal(vdp, PAL_LIVE, {0, rgb4(4, 12, 6), rgb4(2, 7, 4), rgb4(12, 15, 9)});
    setPal(vdp, PAL_TIMBER, {0, rgb4(8, 5, 2), rgb4(5, 3, 1), rgb4(11, 8, 4), rgb4(3, 2, 1)});
    setPal(vdp, PAL_COAT,
           {0, rgb4(3, 4, 6), rgb4(2, 2, 2), rgb4(12, 9, 6), rgb4(6, 7, 8), rgb4(4, 3, 3), rgb4(2, 2, 4),
            rgb4(1, 1, 2), rgb4(10, 8, 6), rgb4(1, 1, 1), rgb4(7, 6, 4)});
    setPal(vdp, PAL_ROPE, {0, rgb4(10, 8, 4), rgb4(6, 4, 2), rgb4(13, 11, 6)});
    setPal(vdp, PAL_GULL, {0, rgb4(14, 14, 13), rgb4(8, 8, 9)});
    setPal(vdp, PAL_CRATE, {0, rgb4(9, 6, 2), rgb4(5, 3, 1), rgb4(12, 9, 4), rgb4(7, 4, 2)});
    setPal(vdp, PAL_FX, {0, rgb4(15, 14, 8), rgb4(15, 15, 14), rgb4(8, 12, 14)});
    setPal(vdp, PAL_SKY, {0, rgb4(6, 7, 10)});
    setPal(vdp, PAL_PILE, {0, rgb4(4, 4, 5), rgb4(8, 8, 7), rgb4(2, 3, 4)});
    // Road bank 12: timber deck, rope verge, harbour water.
    setPal(vdp, PAL_DECK,
           {0, rgb4(3, 5, 6), rgb4(2, 4, 5), rgb4(5, 7, 6), rgb4(7, 5, 2), rgb4(5, 3, 1), rgb4(8, 6, 3),
            rgb4(6, 4, 2), rgb4(10, 8, 4), rgb4(4, 3, 1), rgb4(9, 7, 3), rgb4(2, 5, 7), rgb4(1, 4, 6),
            rgb4(4, 8, 10), rgb4(12, 10, 5), rgb4(7, 5, 2)});
    vdp.setFogColor(rgb4(2, 3, 5));

    art.keeper[0] = gs::uploadMipped(vdp, keeper(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeper(1));
    art.downed = gs::uploadMipped(vdp, downed());
    art.pile = gs::uploadMipped(vdp, pile());
    art.lamp = gs::uploadMipped(vdp, lamp());
    art.gull[0] = gs::uploadMipped(vdp, gull(0));
    art.gull[1] = gs::uploadMipped(vdp, gull(1));
    art.crate = gs::uploadMipped(vdp, crate());
    art.coil = gs::uploadMipped(vdp, coil());
    art.sight = gs::uploadMipped(vdp, sight());
    art.flash = gs::uploadMipped(vdp, flash());
    art.splash = gs::uploadMipped(vdp, splash());
    loadFont(vdp, art);
}

}  // namespace wharfpace
