#include "art.h"

#include <string>

namespace bwell {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap towerArt() {
    Bitmap b(48, 96);
    b.rect(16, 28, 16, 64, 3);
    b.rect(18, 30, 12, 60, 2);
    b.rect(12, 20, 24, 12, 4);
    b.rect(14, 22, 20, 8, 1);
    b.rect(20, 8, 8, 14, 5);
    b.ellipse(24, 10, 6, 5, 6);
    b.rect(22, 46, 4, 6, 7);
    b.rect(22, 62, 4, 6, 7);
    b.rect(22, 78, 4, 8, 8);
    b.outline(9, false);
    return b;
}

Bitmap wellArt() {
    Bitmap b(56, 40);
    b.ellipse(28, 22, 24, 14, 3);
    b.ellipse(28, 20, 20, 11, 2);
    b.ellipse(28, 18, 12, 6, 5);
    b.ellipse(28, 17, 6, 3, 6);
    b.rect(10, 8, 6, 10, 4);
    b.rect(40, 8, 6, 10, 4);
    b.line(13, 8, 43, 8, 1, 2);
    b.outline(8, false);
    return b;
}

Bitmap keeperArt(int step) {
    Bitmap b(32, 40);
    b.rect(14, 22, 6, 14, 3);
    b.ellipse(16, 12, 6, 6, 2);
    b.rect(10, 24, 6, 3, 4);
    b.rect(18, 24, 8, 3, 4);
    int ox = step ? 2 : -2;
    b.rect(8 + ox, 16, 5, 8, 5);
    b.ellipse(10 + ox, 14, 4, 4, 6);
    b.rect(12, 36, 4, 4, 7);
    b.rect(18, 36, 4, 4, 7);
    b.outline(8, false);
    return b;
}

Bitmap tideArt(int step) {
    Bitmap b(28, 36);
    int lean = step ? 2 : 0;
    b.ellipse(14 + lean, 22, 10, 12, 2);
    b.ellipse(14 + lean, 10, 7, 6, 3);
    b.rect(6, 28, 5, 6, 4);
    b.rect(16 + lean, 28, 5, 6, 4);
    b.rect(18, 16, 8, 3, 5);
    b.ellipse(24, 15, 3, 3, 6);
    b.outline(1, false);
    return b;
}

Bitmap wickArt(int step) {
    Bitmap b(32, 40);
    int lean = step ? 3 : -1;
    b.rect(12 + lean, 16, 8, 16, 3);
    b.ellipse(16 + lean, 10, 8, 7, 2);
    b.rect(8, 30, 6, 8, 4);
    b.rect(18 + lean, 30, 6, 8, 4);
    b.rect(20, 18, 10, 4, 5);
    b.ellipse(28, 16, 4, 4, 6);
    b.rect(6, 12, 4, 10, 7);
    b.outline(1, false);
    return b;
}

Bitmap beamArt() {
    Bitmap b(10, 80);
    b.rect(3, 0, 4, 80, 2);
    b.rect(4, 0, 2, 80, 1);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 2);
    b.ellipse(6, 6, 2, 2, 1);
    return b;
}

Bitmap rockArt() {
    Bitmap b(24, 16);
    b.ellipse(12, 10, 10, 5, 2);
    b.ellipse(8, 9, 4, 3, 3);
    b.outline(1, false);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10), gs::rgb4(6, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 13, 6), gs::rgb4(12, 8, 2), gs::rgb4(8, 5, 2), gs::rgb4(15, 15, 12), gs::rgb4(4, 3, 2), gs::rgb4(15, 14, 8), gs::rgb4(9, 8, 6), gs::rgb4(3, 2, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_SEA, {0, gs::rgb4(8, 12, 14), gs::rgb4(3, 7, 10), gs::rgb4(5, 9, 12), gs::rgb4(2, 4, 6), gs::rgb4(10, 14, 15), gs::rgb4(1, 3, 5), gs::rgb4(6, 8, 9), gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(13, 12, 10), gs::rgb4(8, 8, 7), gs::rgb4(6, 6, 5), gs::rgb4(10, 9, 8), gs::rgb4(3, 8, 11), gs::rgb4(6, 12, 14), gs::rgb4(4, 4, 4), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_KEEPER, {0, gs::rgb4(15, 14, 8), gs::rgb4(12, 9, 6), gs::rgb4(7, 5, 4), gs::rgb4(9, 7, 5), gs::rgb4(5, 4, 3), gs::rgb4(15, 12, 4), gs::rgb4(3, 3, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_TIDE, {0, gs::rgb4(1, 2, 3), gs::rgb4(4, 8, 9), gs::rgb4(6, 10, 9), gs::rgb4(3, 5, 5), gs::rgb4(8, 6, 4), gs::rgb4(12, 10, 4), gs::rgb4(2, 3, 3), gs::rgb4(9, 9, 8)});
    setPal(vdp, PAL_WICK, {0, gs::rgb4(2, 1, 1), gs::rgb4(8, 4, 3), gs::rgb4(5, 2, 2), gs::rgb4(3, 3, 3), gs::rgb4(10, 6, 2), gs::rgb4(14, 8, 2), gs::rgb4(6, 5, 4), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_SPARK, {0, gs::rgb4(15, 15, 12), gs::rgb4(15, 10, 3), gs::rgb4(12, 6, 1), gs::rgb4(8, 4, 1)});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(2, 2, 4), gs::rgb4(5, 5, 7), gs::rgb4(8, 8, 9), gs::rgb4(3, 3, 4)});

    art.tower = gs::uploadMipped(vdp, towerArt());
    art.well = gs::uploadMipped(vdp, wellArt());
    art.keeper[0] = gs::uploadMipped(vdp, keeperArt(0));
    art.keeper[1] = gs::uploadMipped(vdp, keeperArt(1));
    art.tide[0] = gs::uploadMipped(vdp, tideArt(0));
    art.tide[1] = gs::uploadMipped(vdp, tideArt(1));
    art.wick[0] = gs::uploadMipped(vdp, wickArt(0));
    art.wick[1] = gs::uploadMipped(vdp, wickArt(1));
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.rock = gs::uploadMipped(vdp, rockArt());
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(1, 2, 4));
}

}  // namespace bwell
