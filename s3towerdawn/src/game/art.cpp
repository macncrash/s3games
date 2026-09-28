#include "game/art.h"

#include <string>

namespace tower {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap towerArt() {
    Bitmap b(96, 168);
    b.rect(18, 28, 60, 132, 2);
    b.rect(22, 32, 52, 122, 1);
    b.poly({{10, 32}, {48, 6}, {86, 32}}, 3);
    b.poly({{22, 28}, {48, 12}, {74, 28}}, 7);
    for (int i = 0; i < 6; i++) {
        b.rect(12 + i * 13, 18, 8, 14, 3);
        b.rect(14 + i * 13, 20, 4, 8, 2);
    }
    b.rect(40, 108, 16, 48, 4);
    b.rect(44, 116, 8, 14, 5);
    b.rect(30, 52, 10, 16, 6);
    b.rect(56, 52, 10, 16, 6);
    b.rect(32, 54, 6, 8, 5);
    b.rect(58, 54, 6, 8, 5);
    b.rect(42, 78, 12, 14, 6);
    b.rect(44, 80, 8, 8, 5);
    b.rect(16, 148, 64, 16, 3);
    b.outline(4, false);
    return b;
}

Bitmap keeperArt() {
    Bitmap b(40, 64);
    b.ellipse(20, 14, 9, 10, 2);
    b.ellipse(20, 12, 7, 6, 1);
    b.rect(12, 22, 16, 26, 2);
    b.rect(14, 24, 12, 20, 1);
    b.rect(10, 46, 8, 16, 3);
    b.rect(22, 46, 8, 16, 3);
    b.ellipse(20, 20, 3, 3, 4);
    b.rect(26, 28, 4, 18, 5);
    b.ellipse(28, 24, 5, 7, 6);
    b.ellipse(28, 22, 2, 3, 1);
    b.outline(3, false);
    return b;
}

Bitmap brazierArt() {
    Bitmap b(40, 32);
    b.poly({{8, 10}, {32, 10}, {28, 20}, {12, 20}}, 2);
    b.poly({{10, 10}, {30, 10}, {26, 18}, {14, 18}}, 1);
    b.rect(18, 20, 4, 10, 3);
    b.rect(10, 28, 20, 3, 3);
    b.rect(12, 8, 16, 3, 2);
    b.outline(4, false);
    return b;
}

Bitmap flameArt(int frame) {
    Bitmap b(32, 48);
    float lean = (frame - 1) * 3.0f;
    b.ellipse(16 + lean * 0.3f, 30, 8, 10, 4);
    b.ellipse(16 + lean * 0.5f, 22, 7, 12, 3);
    b.ellipse(16 + lean, 16, 5, 9, 2);
    b.ellipse(16 + lean * 0.6f, 12, 2, 4, 1);
    if (frame != 1) b.ellipse(20 + lean, 26, 2, 3, 2);
    return b;
}

Bitmap moonArt() {
    Bitmap b(36, 36);
    b.ellipse(18, 18, 14, 14, 1);
    b.ellipse(23, 15, 11, 11, 0);
    b.ellipse(12, 14, 2, 2, 2);
    b.ellipse(16, 22, 3, 2, 2);
    return b;
}

Bitmap starArt() {
    Bitmap b(8, 8);
    b.set(4, 1, 1);
    b.set(4, 2, 1);
    b.set(3, 3, 1);
    b.set(4, 3, 1);
    b.set(5, 3, 1);
    b.set(2, 4, 1);
    b.set(3, 4, 1);
    b.set(4, 4, 1);
    b.set(5, 4, 1);
    b.set(6, 4, 1);
    b.set(3, 5, 1);
    b.set(4, 5, 1);
    b.set(5, 5, 1);
    b.set(4, 6, 1);
    return b;
}

void cobble(uint8_t* px) {
    for (int i = 0; i < 64; i++) px[i] = 2;
    for (int x = 0; x < 8; x++) {
        px[x] = 3;
        px[7 * 8 + x] = 4;
    }
    px[2 * 8 + 2] = 1;
    px[3 * 8 + 5] = 1;
    px[5 * 8 + 3] = 3;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 2);
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(8, 8, 10), gs::rgb4(15, 14, 8), gs::rgb4(15, 5, 3),
                          gs::rgb4(4, 12, 6), gs::rgb4(15, 11, 3), gs::rgb4(6, 8, 14), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(12, 11, 9), gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 6), gs::rgb4(2, 2, 3),
                            gs::rgb4(15, 10, 3), gs::rgb4(2, 2, 5), gs::rgb4(5, 4, 6), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_KEEPER, {0, gs::rgb4(10, 8, 6), gs::rgb4(5, 4, 7), gs::rgb4(2, 2, 4), gs::rgb4(12, 8, 6),
                             gs::rgb4(6, 4, 2), gs::rgb4(15, 10, 2), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_FLAME, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 13, 3), gs::rgb4(15, 7, 1), gs::rgb4(12, 2, 1),
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(10, 9, 8), gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2),
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(14, 14, 12), gs::rgb4(9, 9, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    uint8_t cob[64];
    cobble(cob);
    vdp.loadTile(1, cob);
    for (int y = 18; y < 28; y++)
        for (int x = 0; x < 40; x++) vdp.B.set(x, y, gs::entry(1, PAL_STONE, (x + y) & 1, 0));

    loadFont(vdp, art);
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.keeper = gs::uploadMipped(vdp, keeperArt());
    art.brazier = gs::uploadMipped(vdp, brazierArt());
    for (int i = 0; i < 3; i++) art.flame[i] = gs::uploadMipped(vdp, flameArt(i));
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());
}

}  // namespace tower
