#include "game/art.h"

#include <string>

namespace sally {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap guardArt() {
    Bitmap b(48, 72);
    b.rect(18, 6, 12, 10, 7);
    b.rect(16, 4, 16, 4, 8);
    b.rect(20, 14, 8, 6, 1);
    b.rect(22, 16, 2, 2, 9);
    b.rect(26, 16, 2, 2, 9);
    b.rect(16, 20, 16, 22, 2);
    b.rect(18, 22, 12, 16, 3);
    b.rect(14, 22, 4, 14, 2);
    b.rect(30, 22, 4, 14, 2);
    b.rect(18, 42, 5, 16, 6);
    b.rect(25, 42, 5, 16, 6);
    b.rect(16, 56, 8, 4, 6);
    b.rect(24, 56, 8, 4, 6);
    b.rect(34, 18, 3, 36, 4);
    b.rect(32, 10, 7, 8, 5);
    b.rect(38, 8, 3, 4, 5);
    b.rect(20, 24, 8, 6, 10);
    return b;
}

Bitmap raiderArt() {
    Bitmap b(44, 68);
    b.ellipse(22, 12, 7, 8, 1);
    b.rect(14, 4, 16, 6, 2);
    b.rect(12, 8, 4, 8, 2);
    b.rect(16, 18, 12, 20, 2);
    b.rect(18, 20, 8, 14, 3);
    b.rect(10, 20, 6, 12, 3);
    b.rect(28, 20, 6, 12, 3);
    b.rect(16, 38, 5, 16, 6);
    b.rect(23, 38, 5, 16, 6);
    b.rect(14, 52, 8, 4, 6);
    b.rect(22, 52, 8, 4, 6);
    b.rect(30, 16, 3, 22, 5);
    b.rect(28, 12, 10, 6, 4);
    b.rect(36, 14, 4, 3, 4);
    b.rect(18, 22, 3, 3, 9);
    b.rect(24, 22, 3, 3, 9);
    return b;
}

Bitmap wellArt() {
    Bitmap b(80, 96);
    b.rect(18, 8, 6, 28, 6);
    b.rect(56, 8, 6, 28, 6);
    b.rect(16, 6, 48, 6, 6);
    b.rect(22, 4, 36, 4, 7);
    b.ellipse(40, 28, 6, 4, 8);
    b.line(40, 28, 40, 48, 8, 1.5f);
    b.rect(34, 46, 12, 8, 6);
    b.ellipse(40, 58, 28, 12, 2);
    b.ellipse(40, 56, 24, 9, 1);
    b.ellipse(40, 54, 16, 6, 3);
    b.ellipse(40, 54, 10, 4, 4);
    b.ellipse(36, 52, 4, 2, 5);
    b.rect(14, 62, 52, 22, 2);
    b.rect(16, 66, 48, 6, 1);
    b.rect(16, 76, 48, 4, 3);
    b.rect(12, 82, 56, 8, 1);
    b.rect(10, 88, 60, 5, 2);
    for (int i = 0; i < 5; i++) b.rect(18 + i * 10, 64, 2, 16, 3);
    return b;
}

Bitmap archArt() {
    Bitmap b(160, 88);
    b.rect(0, 10, 28, 78, 1);
    b.rect(132, 10, 28, 78, 1);
    b.rect(4, 16, 20, 66, 2);
    b.rect(136, 16, 20, 66, 2);
    b.rect(0, 0, 160, 18, 1);
    b.rect(8, 4, 144, 8, 3);
    b.rect(24, 18, 112, 10, 2);
    for (int i = 0; i < 8; i++) b.rect(10 + i * 18, 0, 8, 6, 6);
    b.rect(70, 28, 20, 16, 7);
    b.rect(74, 22, 12, 8, 7);
    return b;
}

Bitmap torchArt() {
    Bitmap b(16, 32);
    b.rect(6, 16, 4, 14, 4);
    b.ellipse(8, 10, 5, 7, 1);
    b.ellipse(8, 12, 3, 5, 2);
    b.ellipse(7, 14, 2, 2, 3);
    return b;
}

Bitmap stoneArt() {
    Bitmap b(8, 8);
    b.ellipse(4, 4, 3, 3, 1);
    b.set(3, 3, 2);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(16, 16);
    b.line(8, 1, 8, 15, 1, 1.5f);
    b.line(1, 8, 15, 8, 1, 1.5f);
    b.line(3, 3, 13, 13, 2, 1.2f);
    b.line(13, 3, 3, 13, 2, 1.2f);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(32, 10);
    b.ellipse(16, 5, 14, 4, 1);
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
    auto C = gs::rgb4;
    setPal(vdp, PAL_HUD, {0, C(15, 14, 12), C(14, 11, 4), C(14, 3, 3), C(4, 13, 6), C(8, 10, 14), 0, 0, 0, 0, 0, 0,
                          0, 0, 0, C(1, 1, 2)});
    setPal(vdp, PAL_STONE, {0, C(12, 11, 9), C(6, 6, 5), C(14, 13, 10), C(2, 4, 9), C(5, 8, 13), C(8, 5, 2),
                            C(10, 8, 4), C(9, 7, 3), 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_GUARD, {0, C(14, 10, 7), C(3, 4, 9), C(6, 7, 13), C(8, 5, 2), C(13, 14, 15), C(2, 2, 3),
                            C(9, 9, 11), C(12, 12, 14), C(2, 2, 4), C(14, 12, 4), 0, 0, 0, 0, 0});
    setPal(vdp, PAL_RAID, {0, C(13, 9, 6), C(11, 2, 2), C(6, 1, 1), C(10, 10, 11), C(7, 4, 2), C(2, 2, 2), 0, 0,
                           C(14, 12, 8), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FX, {0, C(15, 14, 8), C(15, 8, 2), C(15, 4, 1), C(6, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, C(0, 0, 0)});
    setPal(vdp, PAL_ROAD,
           {0, C(10, 10, 8), C(6, 6, 5), C(8, 7, 6), C(5, 5, 4), C(4, 4, 3), C(12, 11, 9), C(7, 7, 6), C(13, 12, 10),
            C(5, 5, 4), C(14, 13, 11), C(3, 5, 8), C(4, 6, 10), C(6, 8, 12), C(13, 11, 6), C(9, 8, 7)});

    art.guard = gs::uploadMipped(vdp, guardArt());
    art.raider = gs::uploadMipped(vdp, raiderArt());
    art.well = gs::uploadMipped(vdp, wellArt());
    art.arch = gs::uploadMipped(vdp, archArt());
    art.torch = gs::uploadMipped(vdp, torchArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    loadFont(vdp, art);

    vdp.A.enabled = false;
    vdp.B.enabled = false;
}

}  // namespace sally
