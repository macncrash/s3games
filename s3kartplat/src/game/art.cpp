#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace kart {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
    vdp.setColor(pal * 16 + 0, 0);
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++) {
            for (int x = 0; x < 5; x++) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale, int fill, int edge) {
    gs::TextStyle st{scale, fill, edge, 0, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const auto C = gs::rgb4;
    setPal(vdp, PAL_HUD, {C(0, 0, 0), C(15, 15, 15), C(15, 12, 6), C(10, 15, 10), C(15, 5, 4), C(8, 13, 15), C(6, 7, 8), C(0, 0, 0),
                          C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(2, 3, 4)});
    setPal(vdp, PAL_TAR, {C(0, 0, 0), C(5, 5, 6), C(3, 3, 4), C(12, 12, 13), C(8, 8, 9), C(2, 2, 3)});
    setPal(vdp, PAL_PIT, {C(0, 0, 0), C(9, 9, 10), C(4, 4, 5), C(14, 13, 6), C(6, 6, 7), C(15, 15, 12)});
    setPal(vdp, PAL_KART, {C(0, 0, 0), C(14, 3, 2), C(15, 12, 3), C(2, 2, 3), C(15, 14, 12), C(6, 6, 7), C(1, 1, 2), C(10, 6, 3)});
    setPal(vdp, PAL_TREE, {C(0, 0, 0), C(2, 7, 3), C(1, 4, 2), C(8, 10, 5), C(5, 3, 1)});
    setPal(vdp, PAL_END, {C(0, 0, 0), C(14, 3, 2), C(15, 15, 15), C(6, 1, 1), C(15, 12, 3)});
    setPal(vdp, PAL_GOLD, {C(0, 0, 0), C(15, 13, 4), C(8, 5, 1), C(15, 15, 12)});
    vdp.setFogColor(C(10, 13, 15));

    loadFont(vdp, art);

    {
        gs::Bitmap b(78, 28);
        b.rect(10, 10, 52, 10, 1);
        b.poly({{10, 12}, {22, 4}, {46, 4}, {58, 12}}, 2);
        b.rect(24, 5, 10, 6, 4);
        b.rect(36, 5, 10, 6, 4);
        b.rect(8, 16, 6, 6, 5);
        b.rect(62, 16, 8, 6, 5);
        b.rect(4, 18, 70, 3, 3);
        b.ellipse(18, 22, 7, 6, 6);
        b.ellipse(58, 22, 7, 6, 6);
        art.body = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(18, 22);
        b.ellipse(9, 6, 4.5f, 4.5f, 4);
        b.rect(6, 3, 6, 3, 2);
        b.rect(5, 10, 8, 8, 1);
        b.line(5, 12, 2, 16, 7, 1.4f);
        b.line(13, 12, 16, 16, 7, 1.4f);
        art.driver = gs::uploadMipped(vdp, b);
    }
    for (int f = 0; f < 4; f++) {
        gs::Bitmap b(16, 16);
        b.ellipse(8, 8, 6.5f, 6.5f, 3);
        b.ellipse(8, 8, 2.2f, 2.2f, 5);
        float a = f * 0.8f;
        b.line(8, 8, 8 + 5.2f * std::cos(a), 8 + 5.2f * std::sin(a), 4, 1.3f);
        b.line(8, 8, 8 - 5.2f * std::sin(a), 8 + 5.2f * std::cos(a), 4, 1.3f);
        art.wheel[f] = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(48, 14);
        b.rect(0, 2, 48, 8, 1);
        b.rect(0, 8, 48, 3, 2);
        for (int x = 2; x < 48; x += 8) b.rect(x, 3, 3, 6, 4);
        art.deck = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(48, 14);
        b.rect(0, 2, 48, 8, 3);
        b.rect(0, 8, 48, 3, 2);
        b.rect(0, 4, 48, 3, 5);
        art.level = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(14, 28);
        b.poly({{7, 2}, {13, 26}, {1, 26}}, 1);
        b.rect(3, 10, 8, 3, 2);
        b.rect(3, 18, 8, 3, 2);
        art.pylon = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(40, 36);
        b.rect(2, 14, 36, 20, 2);
        b.rect(4, 8, 32, 8, 1);
        b.rect(8, 18, 6, 6, 3);
        b.rect(18, 18, 6, 6, 3);
        b.rect(28, 18, 6, 6, 3);
        art.stand = gs::uploadMipped(vdp, b);
    }
    art.endWord = words(vdp, "END", 2, 1, 3);
    art.levelWord = words(vdp, "LEVEL", 2, 1, 2);
}

}  // namespace kart
