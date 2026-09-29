#include "game/art.h"

#include <initializer_list>

namespace mush {
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
    setPal(vdp, PAL_HUD, {C(0, 0, 0), C(15, 15, 15), C(14, 12, 8), C(12, 14, 10), C(14, 5, 4), C(8, 12, 14), C(6, 7, 8), C(0, 0, 0),
                          C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(2, 3, 4)});
    setPal(vdp, PAL_SNOW, {C(0, 0, 0), C(14, 15, 15), C(10, 12, 13), C(7, 9, 11), C(4, 6, 8)});
    setPal(vdp, PAL_WOOD, {C(0, 0, 0), C(10, 6, 2), C(6, 3, 1), C(13, 10, 5), C(3, 2, 1), C(15, 14, 8)});
    setPal(vdp, PAL_TEAM, {C(0, 0, 0), C(12, 8, 4), C(14, 3, 2), C(6, 4, 3), C(15, 13, 9), C(3, 2, 2), C(14, 14, 15), C(8, 6, 4)});
    setPal(vdp, PAL_PINE, {C(0, 0, 0), C(2, 6, 3), C(1, 3, 2), C(8, 10, 6), C(5, 3, 1)});
    setPal(vdp, PAL_MARK, {C(0, 0, 0), C(14, 3, 2), C(15, 14, 12), C(6, 1, 1), C(15, 12, 4)});
    setPal(vdp, PAL_GOLD, {C(0, 0, 0), C(15, 13, 4), C(8, 5, 1), C(15, 15, 12)});
    vdp.setFogColor(C(11, 13, 15));

    loadFont(vdp, art);

    {
        gs::Bitmap b(72, 28);
        b.line(6, 18, 66, 18, 3, 2.2f);
        b.line(8, 20, 64, 22, 5, 1.4f);
        b.ellipse(28, 12, 16, 6, 1);
        b.rect(16, 8, 22, 6, 2);
        b.rect(18, 6, 8, 4, 4);
        b.line(10, 16, 8, 22, 3, 1.6f);
        b.line(48, 16, 58, 22, 3, 1.6f);
        b.line(4, 22, 68, 22, 5, 1.5f);
        art.sled = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(20, 28);
        b.ellipse(10, 8, 5, 5, 4);
        b.rect(7, 4, 6, 4, 2);
        b.rect(6, 12, 8, 10, 1);
        b.line(8, 22, 5, 27, 3, 1.6f);
        b.line(12, 22, 15, 27, 3, 1.6f);
        b.line(6, 14, 2, 18, 1, 1.4f);
        art.musher = gs::uploadMipped(vdp, b);
    }
    for (int f = 0; f < 4; f++) {
        gs::Bitmap b(36, 18);
        int lift = (f & 1) ? 2 : 0;
        int lift2 = (f & 2) ? 2 : 0;
        b.ellipse(16, 9, 10, 4.2f, 1);
        b.ellipse(28, 7, 4.2f, 3.2f, 6);
        b.rect(26, 3, 2, 3, 5);
        b.rect(30, 3, 2, 3, 5);
        b.line(10, 12, 7, 16 - lift, 3, 1.5f);
        b.line(18, 12, 20, 16 - lift2, 3, 1.5f);
        b.line(8, 10, 14, 12, 7, 1.2f);
        b.line(4, 8, 1, 6, 3, 1.2f);
        art.dog[f] = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(48, 16);
        b.rect(0, 2, 48, 8, 1);
        b.rect(0, 8, 48, 4, 2);
        b.line(0, 2, 48, 2, 3, 1.2f);
        for (int x = 4; x < 48; x += 8) b.rect(x, 4, 2, 6, 4);
        art.deck = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(48, 16);
        b.rect(0, 2, 48, 8, 3);
        b.rect(0, 8, 48, 4, 2);
        b.rect(0, 4, 48, 3, 5);
        art.level = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(10, 40);
        b.rect(3, 8, 4, 30, 2);
        b.rect(1, 4, 8, 6, 1);
        b.rect(2, 2, 6, 3, 4);
        art.post = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(32, 52);
        b.poly({{16, 2}, {30, 28}, {2, 28}}, 1);
        b.poly({{16, 14}, {28, 42}, {4, 42}}, 2);
        b.rect(13, 42, 6, 8, 4);
        b.rect(8, 20, 5, 3, 3);
        art.pine = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 12);
        b.ellipse(14, 8, 12, 4, 1);
        b.ellipse(10, 7, 5, 2, 2);
        art.drift = gs::uploadMipped(vdp, b);
    }
    art.endWord = words(vdp, "END", 2, 1, 3);
    art.levelWord = words(vdp, "LEVEL", 2, 5, 2);
}

}  // namespace mush
