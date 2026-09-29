#include "game/art.h"

#include <string>

namespace well {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap wellArt() {
    Bitmap b(96, 128);
    b.rect(18, 36, 60, 86, 2);
    b.rect(22, 40, 52, 78, 1);
    b.ellipse(48, 40, 30, 12, 3);
    b.ellipse(48, 40, 22, 8, 6);
    b.ellipse(48, 42, 14, 5, 5);
    b.rect(26, 52, 8, 18, 3);
    b.rect(62, 70, 8, 22, 3);
    b.rect(40, 88, 10, 16, 4);
    b.rect(30, 108, 36, 8, 6);
    for (int i = 0; i < 5; i++) b.rect(20, 48 + i * 14, 56, 2, 7);
    b.outline(8, false);
    return b;
}

Bitmap mouthArt() {
    Bitmap b(40, 16);
    b.ellipse(20, 8, 16, 6, 5);
    b.ellipse(20, 8, 8, 3, 1);
    return b;
}

Bitmap keeperArt() {
    Bitmap b(40, 56);
    b.ellipse(20, 10, 8, 8, 1);
    b.rect(16, 8, 8, 4, 4);
    b.rect(12, 18, 16, 20, 2);
    b.rect(14, 20, 12, 8, 3);
    b.rect(10, 20, 4, 14, 2);
    b.rect(26, 20, 4, 14, 2);
    b.rect(14, 38, 5, 14, 5);
    b.rect(21, 38, 5, 14, 5);
    b.rect(28, 16, 4, 22, 6);
    b.ellipse(30, 14, 4, 4, 7);
    b.outline(8, false);
    return b;
}

Bitmap raiderArt() {
    Bitmap b(40, 56);
    b.ellipse(20, 10, 8, 8, 1);
    b.rect(14, 6, 12, 4, 4);
    b.rect(12, 18, 16, 18, 2);
    b.rect(14, 20, 12, 6, 3);
    b.rect(8, 22, 6, 4, 5);
    b.rect(26, 22, 8, 3, 5);
    b.rect(14, 36, 5, 16, 6);
    b.rect(21, 36, 5, 16, 6);
    b.outline(8, false);
    return b;
}

Bitmap bruteArt() {
    Bitmap b(56, 64);
    b.ellipse(28, 12, 10, 9, 1);
    b.rect(16, 20, 24, 22, 2);
    b.rect(18, 22, 20, 8, 3);
    b.rect(8, 24, 10, 6, 4);
    b.rect(38, 22, 12, 6, 4);
    b.rect(18, 42, 7, 18, 5);
    b.rect(28, 42, 7, 18, 5);
    b.rect(6, 28, 44, 4, 6);
    b.outline(8, false);
    return b;
}

Bitmap dropArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 5, 6, 1);
    b.ellipse(7, 7, 3, 3, 2);
    b.set(6, 5, 3);
    return b;
}

Bitmap splashArt() {
    Bitmap b(32, 24);
    b.ellipse(16, 14, 12, 6, 2);
    b.ellipse(10, 10, 4, 5, 3);
    b.ellipse(22, 9, 3, 4, 3);
    b.ellipse(16, 8, 3, 5, 1);
    return b;
}

Bitmap reedArt() {
    Bitmap b(24, 40);
    b.line(8, 38, 10, 8, 2, 2);
    b.line(14, 38, 12, 4, 1, 2);
    b.ellipse(10, 8, 5, 3, 3);
    b.ellipse(13, 6, 4, 2, 2);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 12, 4), gs::rgb4(8, 10, 12), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(5, 5, 5), gs::rgb4(9, 9, 8), gs::rgb4(13, 12, 10), gs::rgb4(3, 8, 3), gs::rgb4(1, 3, 6),
            gs::rgb4(14, 13, 11), gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_KEEPER,
           {0, gs::rgb4(13, 8, 5), gs::rgb4(2, 6, 11), gs::rgb4(5, 12, 15), gs::rgb4(4, 2, 1), gs::rgb4(2, 2, 2),
            gs::rgb4(6, 4, 2), gs::rgb4(8, 14, 15), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_RAIDER,
           {0, gs::rgb4(12, 8, 5), gs::rgb4(11, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(3, 2, 1), gs::rgb4(8, 6, 3),
            gs::rgb4(3, 2, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 8, 13), gs::rgb4(6, 13, 15), gs::rgb4(14, 15, 15)});
    setPal(vdp, PAL_EARTH, {0, gs::rgb4(4, 8, 3), gs::rgb4(6, 10, 4), gs::rgb4(10, 12, 5), gs::rgb4(3, 5, 2)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 3), gs::rgb4(8, 8, 8)});
    setPal(vdp, PAL_BRUTE,
           {0, gs::rgb4(11, 7, 4), gs::rgb4(6, 4, 3), gs::rgb4(9, 5, 2), gs::rgb4(5, 3, 2), gs::rgb4(3, 2, 2),
            gs::rgb4(7, 7, 8), gs::rgb4(1, 1, 1)});
    art.well = gs::uploadMipped(vdp, wellArt());
    art.mouth = gs::uploadMipped(vdp, mouthArt());
    art.keeper = gs::uploadMipped(vdp, keeperArt());
    art.raider = gs::uploadMipped(vdp, raiderArt());
    art.brute = gs::uploadMipped(vdp, bruteArt());
    art.drop = gs::uploadMipped(vdp, dropArt());
    art.splash = gs::uploadMipped(vdp, splashArt());
    art.reed = gs::uploadMipped(vdp, reedArt());
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(6, 7, 9));
}

}  // namespace well
