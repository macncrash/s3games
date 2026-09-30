#include "game/art.h"

#include <initializer_list>

namespace pressmark {
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

gs::Bitmap frameArt() {
    gs::Bitmap b(88, 150);
    for (int y = 0; y < 150; y++) {
        for (int x = 0; x < 88; x++) {
            bool post = x < 10 || x >= 78;
            bool cap = y < 12 || (y > 128 && y < 140);
            bool cheek = (x >= 10 && x < 16) || (x >= 72 && x < 78);
            int c = 0;
            if (post || cap) c = ((x + y) & 3) == 0 ? 3 : 2;
            else if (cheek && y < 128) c = 4;
            if (y >= 140) c = ((x * 3 + y) & 5) == 0 ? 5 : 4;
            b.set(x, y, c);
        }
    }
    b.rect(18, 16, 52, 8, 6);
    b.rect(40, 24, 8, 70, 7);
    return b;
}

gs::Bitmap platenArt() {
    gs::Bitmap b(64, 18);
    b.rect(0, 2, 64, 14, 2);
    b.rect(2, 4, 60, 10, 3);
    b.rect(28, 0, 8, 6, 4);
    b.rect(4, 8, 56, 2, 5);
    return b;
}

gs::Bitmap bedArt() {
    gs::Bitmap b(78, 36);
    b.rect(0, 4, 78, 28, 2);
    b.rect(4, 8, 70, 20, 3);
    b.rect(34, 6, 2, 24, 6);
    b.rect(28, 16, 14, 2, 6);
    b.rect(6, 0, 66, 4, 4);
    return b;
}

gs::Bitmap sheetArt() {
    gs::Bitmap b(56, 40);
    b.rect(1, 1, 54, 38, 1);
    b.rect(0, 0, 56, 40, 2);
    b.rect(2, 2, 52, 36, 3);
    for (int i = 0; i < 5; i++) b.rect(8, 8 + i * 5, 28, 1, 4);
    b.rect(40, 10, 8, 10, 5);
    return b;
}

gs::Bitmap typeArt() {
    gs::Bitmap b(40, 26);
    b.rect(0, 0, 40, 26, 1);
    b.rect(3, 4, 10, 14, 2);
    b.rect(15, 4, 6, 14, 3);
    b.rect(23, 8, 12, 8, 2);
    b.rect(6, 20, 28, 3, 4);
    return b;
}

gs::Bitmap rollerArt() {
    gs::Bitmap b(34, 16);
    b.ellipse(17, 8, 16, 6, 1);
    b.ellipse(17, 8, 14, 4, 2);
    b.rect(0, 6, 4, 4, 3);
    b.rect(30, 6, 4, 4, 3);
    return b;
}

gs::Bitmap figureArt() {
    gs::Bitmap b(26, 48);
    b.ellipse(13, 8, 6, 6, 1);
    b.rect(8, 15, 10, 14, 2);
    b.rect(4, 16, 4, 12, 3);
    b.rect(18, 16, 4, 12, 3);
    b.rect(8, 29, 4, 14, 4);
    b.rect(14, 29, 4, 14, 4);
    b.rect(6, 42, 6, 4, 5);
    b.rect(14, 42, 6, 4, 5);
    b.set(11, 7, 6);
    b.set(15, 7, 6);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 8, 8, 2);
    b.rect(12, 4, 4, 20, 3);
    b.rect(4, 12, 20, 4, 3);
    b.ellipse(14, 14, 3, 3, 4);
    return b;
}

gs::Bitmap gaugeArt() {
    gs::Bitmap b(14, 86);
    b.rect(4, 2, 6, 80, 1);
    b.rect(5, 4, 4, 76, 2);
    b.rect(2, 58, 10, 8, 3);
    b.rect(1, 8, 3, 2, 4);
    return b;
}

gs::Bitmap needleArt() {
    gs::Bitmap b(12, 6);
    b.rect(0, 1, 12, 4, 1);
    b.rect(8, 0, 4, 6, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(14, 12, 4), gs::rgb4(12, 4, 3), gs::rgb4(8, 14, 8),
                          shadow, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(6, 3, 1), gs::rgb4(10, 6, 2), gs::rgb4(13, 8, 3), gs::rgb4(4, 2, 1),
                           gs::rgb4(7, 7, 7), gs::rgb4(14, 12, 5), gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_PAPER, {0, gs::rgb4(8, 5, 2), gs::rgb4(14, 13, 10), gs::rgb4(15, 15, 13), gs::rgb4(11, 10, 8),
                            gs::rgb4(3, 3, 4), gs::rgb4(14, 11, 3)});
    setPal(vdp, PAL_BRASS, {0, gs::rgb4(6, 5, 2), gs::rgb4(12, 9, 3), gs::rgb4(15, 13, 5), gs::rgb4(8, 8, 9),
                            gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(3, 3, 4), gs::rgb4(7, 7, 8), gs::rgb4(11, 11, 12), gs::rgb4(14, 12, 4),
                           gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2), gs::rgb4(4, 4, 5), gs::rgb4(8, 2, 2),
                          gs::rgb4(14, 12, 4)});
    setPal(vdp, PAL_MAN, {0, gs::rgb4(13, 9, 6), gs::rgb4(4, 6, 9), gs::rgb4(6, 8, 11), gs::rgb4(2, 2, 3),
                          gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WIN, {0, gs::rgb4(12, 3, 2), gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 6), gs::rgb4(8, 2, 2),
                          gs::rgb4(15, 15, 12)});
    vdp.setFogColor(gs::rgb4(4, 3, 2));
    loadFont(vdp, art);
    art.frame = gs::uploadMipped(vdp, frameArt());
    art.platen = gs::uploadMipped(vdp, platenArt());
    art.bed = gs::uploadMipped(vdp, bedArt());
    art.sheet = gs::uploadMipped(vdp, sheetArt());
    art.type = gs::uploadMipped(vdp, typeArt());
    art.roller = gs::uploadMipped(vdp, rollerArt());
    art.figure = gs::uploadMipped(vdp, figureArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    art.gauge = gs::uploadMipped(vdp, gaugeArt());
    art.needle = gs::uploadMipped(vdp, needleArt());
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.clear();
}

}  // namespace pressmark
