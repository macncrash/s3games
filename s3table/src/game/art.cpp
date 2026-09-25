#include "game/art.h"

#include "game/pitch.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace table {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void inkPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t shadow) {
    setPal(vdp, pal, {0, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, shadow});
}

Bitmap malletArt(bool you) {
    Bitmap b(36, 36);
    b.ellipse(18, 18, 14, 14, 1);
    b.ellipse(18, 18, 11, 11, 2);
    b.ellipse(14, 13, 5, 4, 3);
    b.ellipse(18, 18, 4, 4, 4);
    if (you) b.poly({{18, 6}, {24, 14}, {12, 14}}, 5);
    else b.poly({{18, 30}, {24, 22}, {12, 22}}, 5);
    b.outline(6, false);
    return b;
}

Bitmap puckArt() {
    Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 1);
    b.ellipse(10, 10, 6, 6, 2);
    b.ellipse(8, 7, 2, 2, 3);
    b.ellipse(10, 10, 2, 2, 4);
    b.outline(5, false);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(32, 12);
    b.ellipse(16, 6, 13, 4, 1);
    return b;
}

Bitmap barArt() {
    Bitmap b(8, 4);
    b.rect(0, 0, 8, 4, 1);
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
    b.rect(28, 30, 264, 176, 14);
    b.rect(36, 38, 248, 160, 2);
    b.rect(38, 40, 244, 3, 3);
    b.rect(kLeft, kTop, kRight - kLeft, kBot - kTop, 4);
    for (int y = int(kTop); y < int(kBot); y += 4)
        if ((y / 4) & 1) b.rect(kLeft, float(y), kRight - kLeft, 2, 5);

    b.rect(kMouthL - 10, kTop, (kMouthR - kMouthL) + 20, 14, 9);
    b.rect(kMouthL - 10, kBot - 14, (kMouthR - kMouthL) + 20, 14, 9);

    b.ellipse(kCenter, kMid, 24, 24, 6);
    b.ellipse(kCenter, kMid, 21, 21, 4);
    b.rect(kLeft, kMid - 1, kCenter - 26 - kLeft, 3, 6);
    b.rect(kCenter + 26, kMid - 1, kRight - (kCenter + 26), 3, 6);
    b.ellipse(kLeft + 28, kMid, 3, 3, 6);
    b.ellipse(kRight - 28, kMid, 3, 3, 6);

    b.rect(kMouthL, kTop - kPocket, kMouthR - kMouthL, kPocket, 11);
    b.rect(kMouthL + 5, kTop - kPocket + 2, kMouthR - kMouthL - 10, kPocket - 4, 12);
    b.rect(kMouthL, kBot, kMouthR - kMouthL, kPocket, 11);
    b.rect(kMouthL + 5, kBot + 2, kMouthR - kMouthL - 10, kPocket - 4, 13);

    b.rect(kMouthL, kTop - 1, kMouthR - kMouthL, 3, 8);
    b.rect(kMouthL, kBot - 2, kMouthR - kMouthL, 3, 8);
    b.rect(kMouthL - 3, kTop - 4, 4, 8, 10);
    b.rect(kMouthR - 1, kTop - 4, 4, 8, 10);
    b.rect(kMouthL - 3, kBot - 4, 4, 8, 10);
    b.rect(kMouthR - 1, kBot - 4, 4, 8, 10);

    for (float x : {48.f, 272.f})
        for (float y : {48.f, 186.f}) b.ellipse(x, y, 3, 3, 15);

    gs::TextStyle brand{2, 6, 0, 0, 1};
    Bitmap mark = gs::textBitmap("S3", brand);
    b.blit(mark, int(kCenter - mark.w * 0.5f), int(kMid - mark.h * 0.5f));
    gs::TextStyle rule{1, 8, 0, 0, 1};
    Bitmap word = gs::textBitmap("CROSS", rule);
    b.blit(word, int(kCenter - word.w * 0.5f), int(kTop + 4));
    b.blit(word, int(kCenter - word.w * 0.5f), int(kBot - 5 - word.h));
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    inkPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 15), shadow);
    inkPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 3), shadow);
    inkPal(vdp, PAL_RED, gs::rgb4(15, 4, 4), shadow);
    inkPal(vdp, PAL_DIM, gs::rgb4(9, 10, 12), shadow);
    inkPal(vdp, PAL_MINT, gs::rgb4(8, 15, 12), shadow);
    setPal(vdp, PAL_YOU, {0, gs::rgb4(12, 8, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 10), gs::rgb4(6, 4, 1),
                          gs::rgb4(15, 14, 8), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(8, 1, 2), gs::rgb4(14, 2, 3), gs::rgb4(15, 8, 8), gs::rgb4(4, 1, 1),
                           gs::rgb4(15, 12, 10), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PUCK, {0, gs::rgb4(2, 2, 3), gs::rgb4(14, 14, 15), gs::rgb4(15, 15, 15), gs::rgb4(8, 1, 2),
                           gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    const uint16_t ice[16] = {
        0,
        gs::rgb4(1, 2, 4),
        gs::rgb4(2, 5, 7),
        gs::rgb4(6, 10, 12),
        gs::rgb4(10, 14, 15),
        gs::rgb4(7, 12, 14),
        gs::rgb4(13, 2, 3),
        gs::rgb4(7, 1, 2),
        gs::rgb4(15, 12, 3),
        gs::rgb4(3, 7, 11),
        gs::rgb4(14, 15, 15),
        gs::rgb4(1, 1, 2),
        gs::rgb4(10, 2, 3),
        gs::rgb4(12, 9, 2),
        gs::rgb4(5, 3, 2),
        gs::rgb4(12, 10, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ICE * 16 + i, ice[i]);
    vdp.setFogColor(gs::rgb4(1, 2, 4));

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.malletYou = gs::uploadMipped(vdp, malletArt(true));
    art.malletThem = gs::uploadMipped(vdp, malletArt(false));
    art.puck = gs::uploadMipped(vdp, puckArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.bar = gs::uploadMipped(vdp, barArt());
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, paintTable(), PAL_ICE);
}

}  // namespace table
