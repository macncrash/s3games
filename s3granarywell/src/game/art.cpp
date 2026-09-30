#include "game/art.h"

#include <string>

namespace granary {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap granaryArt() {
    Bitmap b(120, 96);
    b.rect(8, 36, 104, 56, 3);
    b.rect(10, 38, 100, 52, 2);
    b.poly({{4, 38}, {60, 8}, {116, 38}}, 4);
    b.poly({{14, 36}, {60, 14}, {106, 36}}, 5);
    b.rect(50, 58, 20, 34, 6);
    b.rect(54, 64, 12, 22, 1);
    for (int i = 0; i < 4; i++) {
        b.rect(16 + i * 24, 46, 12, 10, 7);
        b.rect(18 + i * 24, 48, 8, 6, 8);
    }
    b.rect(18, 70, 16, 8, 9);
    b.rect(86, 70, 16, 8, 9);
    b.outline(1, false);
    return b;
}

Bitmap wellArt() {
    Bitmap b(80, 104);
    b.poly({{8, 28}, {40, 6}, {72, 28}}, 4);
    b.rect(14, 26, 52, 6, 5);
    b.rect(18, 30, 6, 48, 3);
    b.rect(56, 30, 6, 48, 3);
    b.ellipse(40, 78, 30, 12, 2);
    b.ellipse(40, 76, 24, 8, 6);
    b.ellipse(40, 75, 14, 5, 7);
    b.rect(36, 34, 8, 36, 3);
    b.line(40, 36, 40, 70, 1, 1.5f);
    b.outline(1, false);
    return b;
}

Bitmap wellFallArt() {
    Bitmap b(88, 72);
    b.poly({{6, 20}, {30, 8}, {48, 24}, {20, 36}}, 4);
    b.ellipse(50, 48, 32, 14, 2);
    b.ellipse(48, 46, 22, 8, 6);
    b.rect(18, 30, 28, 8, 3);
    b.rect(40, 22, 8, 20, 5);
    b.outline(1, false);
    return b;
}

Bitmap guardArt() {
    Bitmap b(36, 52);
    b.ellipse(18, 10, 7, 7, 2);
    b.rect(12, 18, 12, 16, 3);
    b.rect(14, 20, 8, 10, 4);
    b.rect(8, 20, 4, 14, 3);
    b.rect(24, 20, 4, 14, 3);
    b.rect(13, 34, 4, 14, 5);
    b.rect(19, 34, 4, 14, 5);
    b.rect(11, 46, 7, 3, 1);
    b.rect(18, 46, 7, 3, 1);
    b.rect(26, 16, 3, 28, 6);
    b.outline(1, false);
    return b;
}

Bitmap stoneArt() {
    Bitmap b(20, 18);
    b.ellipse(10, 9, 8, 7, 2);
    b.ellipse(8, 7, 4, 3, 3);
    b.ellipse(13, 11, 3, 2, 1);
    b.outline(1, false);
    return b;
}

Bitmap staveArt() {
    Bitmap b(40, 16);
    b.ellipse(20, 8, 18, 5, 2);
    b.ellipse(20, 8, 10, 3, 3);
    return b;
}

Bitmap puffArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 10, 2);
    b.ellipse(12, 12, 6, 5, 3);
    return b;
}

Bitmap bucketArt() {
    Bitmap b(16, 18);
    b.poly({{3, 4}, {13, 4}, {11, 16}, {5, 16}}, 2);
    b.rect(3, 3, 10, 3, 3);
    b.line(8, 1, 8, 4, 1, 1);
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
    const uint16_t ink = gs::rgb4(15, 14, 10);
    const uint16_t shadow = gs::rgb4(1, 1, 1);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(8, 10, 6), gs::rgb4(15, 12, 4), gs::rgb4(15, 5, 3), gs::rgb4(4, 12, 6),
                          gs::rgb4(12, 10, 6), gs::rgb4(6, 8, 12), gs::rgb4(5, 4, 3), 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(3, 3, 3), gs::rgb4(8, 8, 7), gs::rgb4(13, 12, 10), shadow});
    setPal(vdp, PAL_WELL, {0, gs::rgb4(2, 2, 2), gs::rgb4(9, 8, 6), gs::rgb4(6, 5, 4), gs::rgb4(10, 4, 2),
                           gs::rgb4(12, 8, 4), gs::rgb4(4, 6, 8), gs::rgb4(2, 3, 5), shadow});
    setPal(vdp, PAL_GUARD, {0, gs::rgb4(2, 2, 1), gs::rgb4(12, 9, 6), gs::rgb4(6, 8, 4), gs::rgb4(9, 11, 6),
                            gs::rgb4(4, 3, 2), gs::rgb4(11, 8, 3), shadow});
    setPal(vdp, PAL_GRANARY, {0, gs::rgb4(2, 1, 1), gs::rgb4(12, 8, 4), gs::rgb4(9, 5, 2), gs::rgb4(13, 4, 2),
                              gs::rgb4(10, 3, 2), gs::rgb4(5, 3, 2), gs::rgb4(14, 12, 6), gs::rgb4(6, 8, 10),
                              gs::rgb4(7, 5, 2), shadow});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), gs::rgb4(14, 12, 8), gs::rgb4(15, 15, 12), shadow});

    const uint16_t yard[16] = {
        0,
        gs::rgb4(6, 8, 3), gs::rgb4(5, 6, 2), gs::rgb4(7, 6, 3),
        gs::rgb4(8, 7, 4), gs::rgb4(4, 5, 2),
        gs::rgb4(9, 8, 5), gs::rgb4(6, 5, 3),
        gs::rgb4(5, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(3, 3, 2),
        gs::rgb4(3, 5, 8), gs::rgb4(2, 4, 7), gs::rgb4(10, 11, 12),
        gs::rgb4(12, 10, 6), gs::rgb4(8, 7, 4),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_YARD * 16 + i, yard[i]);

    loadFont(vdp, art);
    art.granary = gs::uploadMipped(vdp, granaryArt());
    art.well = gs::uploadMipped(vdp, wellArt());
    art.wellFall = gs::uploadMipped(vdp, wellFallArt());
    art.guard = gs::uploadMipped(vdp, guardArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.stave = gs::uploadMipped(vdp, staveArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.bucket = gs::uploadMipped(vdp, bucketArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
}

}  // namespace granary
