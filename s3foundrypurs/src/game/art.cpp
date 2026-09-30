#include "game/art.h"

#include <initializer_list>
#include <string>

namespace foundryp {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
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

Bitmap furnaceArt() {
    Bitmap b(36, 80);
    b.rect(4, 18, 28, 56, 1);
    b.rect(8, 24, 20, 22, 2);
    b.rect(10, 28, 16, 14, 6);
    b.rect(6, 50, 24, 16, 3);
    b.rect(12, 54, 12, 8, 5);
    b.rect(14, 6, 8, 16, 4);
    b.ellipse(18, 5, 6, 3, 2);
    b.rect(2, 72, 32, 6, 7);
    return b;
}

Bitmap ladleArt() {
    Bitmap b(28, 20);
    b.rect(2, 6, 16, 8, 1);
    b.ellipse(10, 10, 7, 6, 2);
    b.ellipse(10, 10, 3, 2, 6);
    b.rect(16, 8, 10, 3, 3);
    b.rect(22, 6, 4, 7, 4);
    return b;
}

Bitmap slagArt() {
    Bitmap b(40, 22);
    b.rect(4, 4, 26, 10, 1);
    b.rect(8, 6, 8, 5, 6);
    b.rect(18, 6, 8, 5, 3);
    b.rect(26, 6, 8, 6, 4);
    b.ellipse(10, 17, 4, 4, 7);
    b.ellipse(24, 17, 4, 4, 5);
    return b;
}

Bitmap crucibleArt() {
    Bitmap b(36, 28);
    b.poly({{6, 22}, {10, 6}, {26, 6}, {30, 22}}, 1);
    b.rect(12, 8, 12, 8, 6);
    b.rect(4, 18, 28, 4, 2);
    b.ellipse(10, 24, 4, 3, 7);
    b.ellipse(26, 24, 4, 3, 5);
    b.rect(14, 2, 8, 5, 4);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

Bitmap emberArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 2, 1);
    b.set(4, 3, 2);
    return b;
}

Bitmap stackArt() {
    Bitmap b(10, 16);
    b.rect(3, 2, 4, 12, 1);
    b.ellipse(5, 2, 3, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 12, 9), gs::rgb4(6, 4, 3), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_HEAT, {0, gs::rgb4(15, 8, 2), gs::rgb4(10, 3, 1), gs::rgb4(4, 1, 0)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(8, 2, 2), gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(10, 15, 6), gs::rgb4(3, 8, 2), gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_SLAG, {0, gs::rgb4(9, 7, 5), gs::rgb4(5, 4, 3), gs::rgb4(12, 8, 4), gs::rgb4(15, 10, 3),
                           gs::rgb4(3, 3, 3), gs::rgb4(14, 12, 8), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_CRUC, {0, gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(12, 11, 10), gs::rgb4(15, 7, 2),
                           gs::rgb4(6, 3, 1), gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_FURN, {0, gs::rgb4(7, 5, 4), gs::rgb4(15, 6, 1), gs::rgb4(4, 3, 3), gs::rgb4(10, 8, 6),
                           gs::rgb4(14, 12, 4), gs::rgb4(15, 11, 3), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_LADLE, {0, gs::rgb4(11, 10, 8), gs::rgb4(15, 8, 2), gs::rgb4(6, 5, 4), gs::rgb4(14, 13, 10),
                            gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_SPARK, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 7, 1)});
    setPal(vdp, PAL_EMBER, {0, gs::rgb4(14, 6, 2), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_SOOT, {0, gs::rgb4(6, 6, 6), gs::rgb4(10, 9, 8)});
    vdp.setFogColor(gs::rgb4(6, 3, 1));
    loadFont(vdp, art);
    art.furnace = gs::uploadMipped(vdp, furnaceArt());
    art.ladle = gs::uploadMipped(vdp, ladleArt());
    art.slag = gs::uploadMipped(vdp, slagArt());
    art.crucible = gs::uploadMipped(vdp, crucibleArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.ember = gs::uploadMipped(vdp, emberArt());
    art.stack = gs::uploadMipped(vdp, stackArt());
}

}  // namespace foundryp
