#include "game/art.h"

#include <string>

namespace cabbox {
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
    gs::Bitmap b(200, 56);
    b.rect(0, 18, 200, 38, 1);
    b.poly({{0, 22}, {70, 8}, {130, 8}, {200, 22}, {200, 56}, {0, 56}}, 1);
    b.rect(8, 28, 184, 22, 2);
    b.rect(14, 32, 70, 12, 3);
    b.rect(116, 32, 68, 12, 3);
    b.ellipse(100, 30, 22, 18, 4);
    b.ellipse(100, 30, 16, 12, 5);
    b.rect(0, 48, 200, 8, 6);
    for (int i = 0; i < 8; i++) b.rect(12.f + i * 22.f, 50, 10, 3, i % 2 ? 7 : 8);
    b.rect(86, 34, 28, 6, 9);
    return b;
}

gs::Bitmap wheelArt(float ang) {
    gs::Bitmap b(48, 48);
    b.ellipse(24, 24, 20, 20, 1);
    b.ellipse(24, 24, 14, 14, 2);
    b.ellipse(24, 24, 4, 4, 1);
    const float c = std::cos(ang), s = std::sin(ang);
    for (int k = 0; k < 3; k++) {
        float a = ang + k * 2.094f;
        float ca = std::cos(a), sa = std::sin(a);
        b.line(24 + ca * 4, 24 + sa * 4, 24 + ca * 14, 24 + sa * 14, 1, 2.2f);
        (void)c;
        (void)s;
    }
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 36);
    b.rect(3, 6, 4, 30, 1);
    b.rect(1, 0, 8, 8, 2);
    b.rect(2, 2, 6, 4, 3);
    return b;
}

gs::Bitmap blockArt() {
    gs::Bitmap b(28, 48);
    b.rect(0, 4, 28, 44, 1);
    b.rect(0, 0, 28, 6, 2);
    for (int row = 0; row < 4; row++)
        for (int col = 0; col < 3; col++) {
            int lit = ((row * 3 + col) % 5 == 0) ? 4 : 3;
            b.rect(3.f + col * 8.f, 10.f + row * 9.f, 5, 6, lit);
        }
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 28);
    b.rect(3, 8, 2, 20, 1);
    b.ellipse(4, 5, 3, 3, 2);
    return b;
}

gs::Bitmap fareArt() {
    gs::TextStyle st{2, 1, 0, 0, 1};
    gs::Bitmap word = gs::textBitmap("FARE", st);
    gs::Bitmap b(word.w + 8, word.h + 6);
    b.rect(0, 0, float(b.w), float(b.h), 2);
    b.blit(word, 4, 3);
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
           {0, gs::rgb4(14, 11, 2), gs::rgb4(6, 5, 4), gs::rgb4(3, 6, 4), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 7),
            gs::rgb4(4, 3, 2), gs::rgb4(15, 13, 4), gs::rgb4(12, 4, 2), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_CITY,
           {0, gs::rgb4(5, 6, 8), gs::rgb4(8, 4, 3), gs::rgb4(10, 12, 14), gs::rgb4(15, 13, 6), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(3, 3, 4), gs::rgb4(15, 12, 2), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(2, 2, 2), gs::rgb4(7, 7, 8)});

    // Street (pal 12) and the painted bay (pal 13). Indices match the road chip.
    auto roadPal = [&](int pal, uint16_t ground, uint16_t verge, uint16_t asphalt, uint16_t paint) {
        for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, asphalt);
        vdp.setColor(pal * 16 + 0, 0);
        vdp.setColor(pal * 16 + 1, ground);
        vdp.setColor(pal * 16 + 2, gs::rgb4(4, 6, 3));
        vdp.setColor(pal * 16 + 3, gs::rgb4(7, 8, 4));
        vdp.setColor(pal * 16 + 4, verge);
        vdp.setColor(pal * 16 + 5, gs::rgb4(6, 6, 5));
        vdp.setColor(pal * 16 + 6, asphalt);
        vdp.setColor(pal * 16 + 7, gs::rgb4((asphalt >> 8) - 1, ((asphalt >> 4) & 15) - 1, (asphalt & 15)));
        vdp.setColor(pal * 16 + 8, gs::rgb4(5, 5, 4));
        vdp.setColor(pal * 16 + 9, gs::rgb4(3, 3, 3));
        vdp.setColor(pal * 16 + 10, gs::rgb4(8, 7, 5));
        vdp.setColor(pal * 16 + 14, paint);
        vdp.setColor(pal * 16 + 15, gs::rgb4(9, 9, 8));
    };
    roadPal(PAL_ROAD, gs::rgb4(3, 7, 3), gs::rgb4(5, 5, 4), gs::rgb4(4, 4, 5), gs::rgb4(14, 13, 5));
    roadPal(PAL_BAY, gs::rgb4(3, 6, 3), gs::rgb4(8, 7, 3), gs::rgb4(12, 10, 2), gs::rgb4(15, 14, 3));

    art.dash = gs::uploadMipped(vdp, dashArt());
    for (int i = 0; i < 4; i++) art.wheel[i] = gs::uploadMipped(vdp, wheelArt(i * 0.52f));
    art.post = gs::uploadMipped(vdp, postArt());
    art.block = gs::uploadMipped(vdp, blockArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    setPal(vdp, 8, {0, gs::rgb4(2, 2, 2), gs::rgb4(12, 3, 2)});
    art.fare = gs::uploadMipped(vdp, fareArt());
    vdp.setFogColor(gs::rgb4(8, 9, 11));
}

}  // namespace cabbox
