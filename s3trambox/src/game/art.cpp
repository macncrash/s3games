#include "game/art.h"

#include <cmath>
#include <string>

namespace trambox {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
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

gs::Bitmap dashArt() {
    gs::Bitmap b(220, 64);
    b.poly({{0, 28}, {48, 10}, {172, 10}, {220, 28}, {220, 64}, {0, 64}}, 1);
    b.rect(0, 36, 220, 28, 2);
    b.rect(18, 40, 70, 10, 3);
    b.rect(132, 40, 70, 10, 3);
    b.rect(96, 16, 28, 18, 4);
    b.rect(100, 20, 20, 10, 5);
    b.rect(0, 54, 220, 10, 6);
    for (int i = 0; i < 9; i++) b.rect(8.f + i * 24.f, 56, 12, 4, i % 2 ? 7 : 8);
    b.rect(6, 32, 14, 6, 9);
    b.rect(200, 32, 14, 6, 9);
    return b;
}

gs::Bitmap leverArt(float pull) {
    gs::Bitmap b(36, 48);
    b.rect(14, 28, 8, 16, 1);
    b.ellipse(18, 40, 10, 5, 2);
    float tip = 10.f + pull * 14.f;
    b.line(18, 30, 18, tip, 1, 3.2f);
    b.ellipse(18, tip, 5, 4, 3);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(16, 40);
    b.rect(6, 8, 4, 32, 1);
    b.line(2, 10, 14, 10, 2, 2.f);
    b.ellipse(8, 6, 3, 3, 3);
    return b;
}

gs::Bitmap blockArt() {
    gs::Bitmap b(32, 52);
    b.rect(0, 6, 32, 46, 1);
    b.poly({{0, 8}, {16, 0}, {32, 8}}, 2);
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 3; col++) {
            int lit = ((row + col) % 3 == 0) ? 4 : 3;
            b.rect(3.f + col * 10.f, 12.f + row * 9.f, 6, 6, lit);
        }
    return b;
}

gs::Bitmap wireArt() {
    gs::Bitmap b(48, 10);
    b.line(0, 5, 48, 5, 1, 1.5f);
    b.line(0, 7, 48, 3, 2, 1.f);
    return b;
}

gs::Bitmap shelterArt() {
    gs::Bitmap b(40, 36);
    b.rect(2, 10, 36, 4, 1);
    b.rect(4, 14, 3, 20, 2);
    b.rect(33, 14, 3, 20, 2);
    b.rect(8, 16, 24, 14, 3);
    b.rect(10, 18, 8, 6, 4);
    return b;
}

gs::Bitmap boardArt() {
    gs::TextStyle st{2, 1, 0, 0, 1};
    gs::Bitmap word = gs::textBitmap("BOX", st);
    gs::Bitmap b(word.w + 10, word.h + 8);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.rect(2, 2, float(b.w) - 4, float(b.h) - 4, 3);
    b.blit(word, 5, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 7));
    loadFont(vdp, art);

    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(6, 5, 4), gs::rgb4(2, 5, 4), gs::rgb4(3, 3, 4), gs::rgb4(14, 13, 8),
            gs::rgb4(4, 2, 2), gs::rgb4(15, 14, 6), gs::rgb4(8, 7, 6), gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_CITY,
           {0, gs::rgb4(7, 4, 3), gs::rgb4(10, 5, 4), gs::rgb4(8, 9, 11), gs::rgb4(15, 13, 6), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(3, 3, 4), gs::rgb4(6, 6, 7), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_LEVER, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 6), gs::rgb4(14, 10, 2)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(1, 1, 1), gs::rgb4(12, 2, 2), gs::rgb4(15, 13, 3)});

    auto roadPal = [&](int pal, uint16_t ground, uint16_t verge, uint16_t asphalt, uint16_t paint) {
        for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, asphalt);
        vdp.setColor(pal * 16 + 0, 0);
        vdp.setColor(pal * 16 + 1, ground);
        vdp.setColor(pal * 16 + 2, gs::rgb4(4, 5, 3));
        vdp.setColor(pal * 16 + 3, gs::rgb4(6, 7, 4));
        vdp.setColor(pal * 16 + 4, verge);
        vdp.setColor(pal * 16 + 5, gs::rgb4(5, 5, 5));
        vdp.setColor(pal * 16 + 6, asphalt);
        vdp.setColor(pal * 16 + 7, gs::rgb4(3, 3, 4));
        vdp.setColor(pal * 16 + 8, gs::rgb4(4, 4, 5));
        vdp.setColor(pal * 16 + 9, gs::rgb4(2, 2, 3));
        vdp.setColor(pal * 16 + 10, gs::rgb4(7, 6, 5));
        vdp.setColor(pal * 16 + 14, paint);
        vdp.setColor(pal * 16 + 15, gs::rgb4(9, 9, 8));
    };
    roadPal(PAL_ROAD, gs::rgb4(4, 6, 3), gs::rgb4(5, 5, 4), gs::rgb4(5, 5, 6), gs::rgb4(12, 12, 10));
    roadPal(PAL_BAY, gs::rgb4(4, 6, 3), gs::rgb4(9, 7, 2), gs::rgb4(13, 10, 2), gs::rgb4(15, 14, 4));

    art.dash = gs::uploadMipped(vdp, dashArt());
    for (int i = 0; i < 4; i++) art.lever[i] = gs::uploadMipped(vdp, leverArt(i / 3.f));
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.block = gs::uploadMipped(vdp, blockArt());
    art.wire = gs::uploadMipped(vdp, wireArt());
    art.shelter = gs::uploadMipped(vdp, shelterArt());
    art.board = gs::uploadMipped(vdp, boardArt());
    vdp.setFogColor(gs::rgb4(7, 8, 10));
}

}  // namespace trambox
