#include "game/art.h"

#include <initializer_list>

namespace ovenchime {
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

gs::Bitmap ovenArt() {
    gs::Bitmap b(200, 118);
    b.rect(8, 28, 184, 78, 2);
    b.rect(22, 40, 156, 58, 1);
    b.ellipse(100, 48, 70, 22, 1);
    b.rect(0, 18, 200, 14, 3);
    b.rect(0, 100, 200, 16, 4);
    for (int x = 12; x < 188; x += 22) b.rect(float(x), 20, 12, 8, 5);
    b.rect(92, 4, 6, 16, 5);
    b.rect(86, 2, 18, 6, 6);
    return b;
}

gs::Bitmap loafArt() {
    gs::Bitmap b(72, 36);
    b.ellipse(36, 22, 32, 12, 2);
    b.ellipse(36, 20, 24, 8, 3);
    b.ellipse(26, 16, 8, 3, 4);
    b.ellipse(46, 15, 7, 3, 4);
    return b;
}

gs::Bitmap faceArt() {
    gs::Bitmap b(44, 44);
    b.ellipse(22, 22, 20, 20, 2);
    b.ellipse(22, 22, 16, 16, 1);
    b.rect(21, 6, 2, 4, 3);
    b.rect(21, 34, 2, 4, 3);
    b.rect(6, 21, 4, 2, 3);
    b.rect(34, 21, 4, 2, 3);
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(6, 20);
    b.rect(2, 0, 2, 16, 1);
    b.rect(1, 14, 4, 4, 2);
    return b;
}

gs::Bitmap doorArt() {
    gs::Bitmap b(64, 70);
    b.rect(0, 0, 64, 70, 2);
    b.rect(6, 6, 52, 58, 1);
    b.rect(48, 32, 6, 8, 3);
    b.rect(10, 12, 28, 4, 4);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(10, 18);
    b.ellipse(5, 11, 4, 6, 2);
    b.ellipse(5, 8, 2, 5, 1);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2, 2, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 12, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(6, 4, 0)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(5, 1, 1)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(6, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 4, 1)});
    setPal(vdp, PAL_OVEN,
           {0, gs::rgb4(3, 2, 2), gs::rgb4(8, 4, 3), gs::rgb4(12, 7, 4), gs::rgb4(5, 3, 2), gs::rgb4(14, 11, 6),
            gs::rgb4(10, 9, 8)});
    setPal(vdp, PAL_LOAF, {0, gs::rgb4(12, 8, 3), gs::rgb4(10, 6, 2), gs::rgb4(14, 10, 4), gs::rgb4(15, 13, 7)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 5, 4), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_DOOR, {0, gs::rgb4(4, 6, 8), gs::rgb4(7, 9, 11), gs::rgb4(15, 13, 4), gs::rgb4(10, 12, 14)});
    setPal(vdp, PAL_FIRE, {0, gs::rgb4(15, 14, 6), gs::rgb4(14, 6, 1)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(2, 2, 2), gs::rgb4(12, 3, 2)});
    loadFont(vdp, art);
    art.oven = gs::uploadImage(vdp, ovenArt());
    art.loaf = gs::uploadImage(vdp, loafArt());
    art.face = gs::uploadImage(vdp, faceArt());
    art.hand = gs::uploadImage(vdp, handArt());
    art.door = gs::uploadImage(vdp, doorArt());
    art.flame = gs::uploadImage(vdp, flameArt());
    art.pip = gs::uploadImage(vdp, pipArt());
}

}  // namespace ovenchime
