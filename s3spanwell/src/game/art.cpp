#include "game/art.h"

#include <initializer_list>
#include <string>

namespace spanwell {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap plankArt() {
    Bitmap b(48, 16);
    b.rect(0, 2, 48, 12, 1);
    b.rect(0, 2, 48, 3, 2);
    for (int i = 0; i < 4; i++) b.rect(4 + i * 12, 6, 2, 6, 3);
    b.rect(0, 12, 48, 2, 4);
    return b;
}

Bitmap postArt() {
    Bitmap b(14, 56);
    b.rect(4, 0, 6, 50, 1);
    b.rect(2, 0, 10, 4, 2);
    b.rect(1, 48, 12, 6, 3);
    b.rect(5, 16, 4, 3, 4);
    b.outline(5);
    return b;
}

Bitmap wellArt() {
    Bitmap b(56, 64);
    b.ellipse(28, 18, 22, 8, 2);
    b.rect(6, 18, 44, 36, 1);
    b.ellipse(28, 54, 22, 9, 1);
    b.ellipse(28, 54, 16, 5, 3);
    for (int row = 0; row < 4; row++) {
        int y = 22 + row * 8;
        for (int col = 0; col < 4; col++) b.rect(10 + col * 10, y, 8, 6, (row + col) & 1 ? 4 : 5);
    }
    b.ellipse(28, 18, 14, 5, 6);
    b.outline(7);
    return b;
}

Bitmap roofArt() {
    Bitmap b(64, 28);
    b.poly({{32, 2}, {62, 22}, {2, 22}}, 1);
    b.poly({{32, 8}, {52, 20}, {12, 20}}, 2);
    b.rect(28, 18, 8, 8, 3);
    b.outline(4);
    return b;
}

Bitmap bucketArt() {
    Bitmap b(16, 18);
    b.rect(3, 4, 10, 12, 1);
    b.poly({{3, 4}, {8, 1}, {13, 4}}, 2);
    b.ellipse(8, 15, 5, 2, 3);
    return b;
}

Bitmap keeperArt() {
    Bitmap b(36, 52);
    b.ellipse(18, 8, 7, 7, 4);
    b.rect(12, 16, 12, 16, 1);
    b.rect(13, 18, 10, 4, 2);
    b.rect(8, 18, 5, 14, 1);
    b.rect(23, 18, 5, 14, 1);
    b.rect(12, 32, 5, 14, 3);
    b.rect(19, 32, 5, 14, 3);
    b.ellipse(14, 46, 4, 3, 5);
    b.ellipse(22, 46, 4, 3, 5);
    b.outline(6);
    return b;
}

Bitmap braceArt() {
    Bitmap b(40, 10);
    b.rect(0, 3, 40, 4, 1);
    b.rect(2, 2, 6, 6, 2);
    b.rect(32, 2, 6, 6, 2);
    return b;
}

Bitmap waveArt() {
    Bitmap b(72, 28);
    b.poly({{0, 18}, {18, 16}, {28, 6}, {40, 16}, {72, 18}, {72, 26}, {0, 26}}, 1);
    b.poly({{24, 14}, {32, 4}, {42, 14}}, 2);
    b.ellipse(32, 8, 6, 4, 3);
    return b;
}

Bitmap foamArt() {
    Bitmap b(20, 10);
    b.ellipse(6, 5, 5, 3, 1);
    b.ellipse(14, 6, 4, 3, 1);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 14, 12), gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 6)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(6, 8, 12)});
    setPal(vdp, PAL_SEA, {0, gs::rgb4(2, 5, 10), gs::rgb4(3, 8, 12), gs::rgb4(1, 3, 7)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 3), gs::rgb4(5, 3, 1), gs::rgb4(3, 2, 1), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(8, 8, 7), gs::rgb4(11, 11, 10), gs::rgb4(5, 5, 5), gs::rgb4(9, 9, 8), gs::rgb4(6, 6, 6),
                           gs::rgb4(3, 6, 8), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_WELL, {0, gs::rgb4(9, 8, 6), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(12, 9, 4)});
    setPal(vdp, PAL_KEEPER, {0, gs::rgb4(4, 7, 10), gs::rgb4(12, 10, 4), gs::rgb4(3, 3, 5), gs::rgb4(13, 10, 8), gs::rgb4(2, 2, 2),
                            gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_WAVE, {0, gs::rgb4(3, 7, 13), gs::rgb4(8, 12, 15), gs::rgb4(14, 15, 15)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(8, 6, 1)});
    setPal(vdp, PAL_WARN, {0, gs::rgb4(15, 6, 3), gs::rgb4(8, 2, 1)});
    setPal(vdp, PAL_ROPE, {0, gs::rgb4(10, 8, 3), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(13, 14, 15)});
    vdp.setFogColor(gs::rgb4(4, 7, 11));
    loadFont(vdp, art);
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.well = gs::uploadMipped(vdp, wellArt());
    art.roof = gs::uploadMipped(vdp, roofArt());
    art.bucket = gs::uploadMipped(vdp, bucketArt());
    art.keeper = gs::uploadMipped(vdp, keeperArt());
    art.brace = gs::uploadMipped(vdp, braceArt());
    art.wave = gs::uploadMipped(vdp, waveArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
}

}  // namespace spanwell
