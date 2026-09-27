#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace tabletape {
namespace {

constexpr float kLeft = 46.f;
constexpr float kRight = 274.f;
constexpr float kTop = 28.f;
constexpr float kBot = 148.f;
constexpr float kMouthL = 118.f;
constexpr float kMouthR = 202.f;
constexpr float kPocket = 16.f;
constexpr float kCenter = 160.f;
constexpr float kMid = (kTop + kBot) * 0.5f;

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void inkPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t shadow) {
    setPal(vdp, pal, {0, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, shadow});
}

Bitmap disc(bool you) {
    Bitmap b(34, 34);
    b.ellipse(17, 17, 13, 13, 1);
    b.ellipse(17, 17, 10, 10, 2);
    b.ellipse(13, 12, 4, 3, 3);
    b.ellipse(17, 17, 3, 3, 4);
    if (you) b.rect(14, 8, 6, 5, 5);
    else b.rect(14, 21, 6, 5, 5);
    b.outline(6, false);
    return b;
}

Bitmap puckArt() {
    Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 1);
    b.ellipse(9, 9, 5, 5, 2);
    b.ellipse(7, 6, 2, 2, 3);
    b.outline(4, false);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(28, 10);
    b.ellipse(14, 5, 12, 3, 1);
    return b;
}

Bitmap barArt() {
    Bitmap b(8, 4);
    b.rect(0, 0, 8, 4, 1);
    return b;
}

Bitmap slipArt() {
    Bitmap b(36, 16);
    b.rect(0, 0, 36, 16, 1);
    b.rect(2, 2, 32, 12, 2);
    b.rect(4, 5, 18, 2, 3);
    b.rect(4, 9, 12, 2, 3);
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

Bitmap paintTable() {
    Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    b.rect(0, 0, gs::SCREEN_W, gs::SCREEN_H, 1);
    b.rect(18, 14, 284, 154, 2);
    b.rect(28, 20, 264, 142, 3);
    b.rect(kLeft, kTop, kRight - kLeft, kBot - kTop, 4);
    for (int y = int(kTop); y < int(kBot); y += 6) b.rect(kLeft, float(y), kRight - kLeft, 1, 5);
    b.rect(kLeft, kMid - 1, kRight - kLeft, 2, 6);
    b.ellipse(kCenter, kMid, 16, 16, 6);
    b.ellipse(kCenter, kMid, 13, 13, 4);

    const float third = (kMouthR - kMouthL) / 3.f;
    b.rect(kMouthL, kTop - kPocket, third, kPocket, 7);
    b.rect(kMouthL + third, kTop - kPocket, third, kPocket, 8);
    b.rect(kMouthL + third * 2.f, kTop - kPocket, third, kPocket, 9);
    b.rect(kMouthL, kBot, kMouthR - kMouthL, kPocket, 10);
    b.rect(kMouthL, kTop - 1, kMouthR - kMouthL, 2, 11);
    b.rect(kMouthL, kBot - 1, kMouthR - kMouthL, 2, 11);

    b.rect(24, 176, 272, 40, 12);
    b.rect(30, 182, 260, 28, 13);
    b.rect(36, 186, 72, 20, 14);
    for (int i = 0; i < 3; i++) b.rect(122.f + float(i) * 52.f, 186, 44, 20, 15);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 2, 1);
    inkPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 15), shadow);
    inkPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 3), shadow);
    inkPal(vdp, PAL_RED, gs::rgb4(15, 4, 4), shadow);
    inkPal(vdp, PAL_DIM, gs::rgb4(8, 10, 8), shadow);
    inkPal(vdp, PAL_MINT, gs::rgb4(8, 15, 11), shadow);
    setPal(vdp, PAL_YOU, {0, gs::rgb4(12, 8, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 10), gs::rgb4(5, 3, 1),
                          gs::rgb4(15, 14, 6), gs::rgb4(2, 1, 0), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(7, 1, 2), gs::rgb4(13, 2, 3), gs::rgb4(15, 8, 8), gs::rgb4(3, 1, 1),
                           gs::rgb4(15, 12, 10), gs::rgb4(2, 0, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PUCK, {0, gs::rgb4(2, 2, 3), gs::rgb4(13, 14, 15), gs::rgb4(15, 15, 15), gs::rgb4(4, 4, 6), 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    const uint16_t felt[16] = {
        0,
        gs::rgb4(1, 3, 2),
        gs::rgb4(4, 3, 2),
        gs::rgb4(6, 5, 3),
        gs::rgb4(2, 8, 4),
        gs::rgb4(1, 6, 3),
        gs::rgb4(12, 14, 10),
        gs::rgb4(6, 10, 4),
        gs::rgb4(12, 12, 4),
        gs::rgb4(4, 8, 12),
        gs::rgb4(8, 3, 3),
        gs::rgb4(15, 14, 8),
        gs::rgb4(6, 4, 2),
        gs::rgb4(8, 5, 2),
        gs::rgb4(14, 12, 7),
        gs::rgb4(3, 2, 1),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FELT * 16 + i, felt[i]);
    setPal(vdp, PAL_TAPE, {0, gs::rgb4(14, 13, 8), gs::rgb4(2, 2, 1), gs::rgb4(8, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(10, 7, 3), gs::rgb4(14, 12, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    vdp.setFogColor(gs::rgb4(1, 3, 2));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.malletYou = gs::uploadMipped(vdp, disc(true));
    art.malletThem = gs::uploadMipped(vdp, disc(false));
    art.puck = gs::uploadMipped(vdp, puckArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    art.slip = gs::uploadMipped(vdp, slipArt());
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, paintTable(), PAL_FELT);
}

}  // namespace tabletape
