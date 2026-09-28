#include "game/art.h"

#include <initializer_list>

namespace ovenmark {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
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

gs::Bitmap archArt() {
    gs::Bitmap b(220, 120);
    b.rect(0, 28, 220, 92, 2);
    b.rect(18, 40, 184, 80, 0);
    b.ellipse(110, 48, 78, 36, 0);
    b.rect(8, 18, 204, 16, 3);
    b.rect(0, 100, 220, 20, 4);
    for (int x = 6; x < 214; x += 22) b.rect(float(x), 22, 16, 8, 1);
    for (int y = 48; y < 100; y += 16) {
        b.rect(4, float(y), 12, 10, 1);
        b.rect(204, float(y), 12, 10, 1);
    }
    b.rect(96, 108, 28, 8, 5);
    return b;
}

gs::Bitmap loafArt() {
    gs::Bitmap b(72, 40);
    b.ellipse(36, 24, 32, 14, 2);
    b.ellipse(36, 22, 26, 10, 3);
    b.ellipse(28, 18, 8, 4, 4);
    b.line(22, 16, 50, 16, 1, 2);
    b.line(36, 10, 36, 22, 1, 2);
    return b;
}

gs::Bitmap coinArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 2);
    b.ellipse(11, 11, 7, 7, 3);
    b.rect(10, 5, 2, 12, 1);
    b.rect(6, 10, 10, 2, 1);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(18, 28);
    b.ellipse(9, 16, 7, 10, 2);
    b.ellipse(9, 18, 4, 7, 3);
    b.ellipse(9, 12, 3, 6, 1);
    return b;
}

gs::Bitmap peelArt() {
    gs::Bitmap b(150, 16);
    b.ellipse(28, 8, 26, 6, 2);
    b.rect(50, 6, 96, 4, 3);
    b.rect(140, 4, 8, 8, 1);
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(28, 24);
    b.ellipse(12, 12, 8, 8, 2);
    b.rect(16, 6, 8, 4, 1);
    b.rect(17, 11, 8, 3, 1);
    b.rect(16, 15, 7, 3, 1);
    b.rect(8, 18, 10, 4, 3);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 13, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 0)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(8, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_OVEN,
           {0, gs::rgb4(8, 3, 2), gs::rgb4(6, 3, 2), gs::rgb4(9, 4, 3), gs::rgb4(4, 2, 1), gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_LOAF, {0, gs::rgb4(8, 5, 2), gs::rgb4(13, 10, 6), gs::rgb4(15, 13, 8), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_PALE, {0, gs::rgb4(9, 6, 3), gs::rgb4(14, 11, 6), gs::rgb4(15, 13, 7), gs::rgb4(15, 14, 9)});
    setPal(vdp, PAL_CRUST, {0, gs::rgb4(8, 4, 1), gs::rgb4(12, 7, 2), gs::rgb4(15, 10, 3), gs::rgb4(15, 13, 5)});
    setPal(vdp, PAL_CHAR, {0, gs::rgb4(2, 1, 1), gs::rgb4(4, 2, 2), gs::rgb4(6, 3, 2), gs::rgb4(8, 4, 3)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(15, 14, 4), gs::rgb4(13, 4, 1), gs::rgb4(15, 8, 2)});
    setPal(vdp, PAL_PEEL, {0, gs::rgb4(5, 3, 1), gs::rgb4(12, 8, 3), gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(14, 9, 6), gs::rgb4(12, 7, 5), gs::rgb4(6, 3, 2)});
    setPal(vdp, PAL_COIN, {0, gs::rgb4(10, 6, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 7)});
    loadFont(vdp, art);
    art.arch = gs::uploadImage(vdp, archArt());
    art.loaf = gs::uploadImage(vdp, loafArt());
    art.coin = gs::uploadImage(vdp, coinArt());
    art.flame = gs::uploadImage(vdp, flameArt());
    art.peel = gs::uploadImage(vdp, peelArt());
    art.hand = gs::uploadImage(vdp, handArt());
    art.bar = gs::uploadImage(vdp, barArt());
}

}  // namespace ovenmark
