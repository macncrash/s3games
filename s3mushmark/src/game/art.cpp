#include "game/art.h"

#include <initializer_list>

namespace mushmark {
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const auto C = gs::rgb4;
    setPal(vdp, PAL_HUD, {C(0, 0, 0), C(15, 15, 14), C(12, 10, 7), C(8, 14, 10), C(14, 5, 4), C(9, 12, 15), C(5, 6, 7), C(0, 0, 0),
                          C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(0, 0, 0), C(2, 2, 3)});
    setPal(vdp, PAL_SNOW, {C(0, 0, 0), C(15, 15, 15), C(11, 13, 14), C(7, 9, 11), C(4, 5, 7), C(13, 14, 15)});
    setPal(vdp, PAL_TEAM, {C(0, 0, 0), C(11, 7, 3), C(13, 3, 2), C(5, 3, 2), C(15, 14, 10), C(2, 2, 2), C(14, 14, 15), C(8, 5, 3), C(15, 12, 6)});
    setPal(vdp, PAL_PINE, {C(0, 0, 0), C(2, 7, 3), C(1, 4, 2), C(9, 11, 6), C(6, 4, 2)});
    setPal(vdp, PAL_MARK, {C(0, 0, 0), C(14, 2, 2), C(15, 14, 12), C(8, 1, 1), C(15, 11, 3), C(4, 1, 1)});
    setPal(vdp, PAL_GOLD, {C(0, 0, 0), C(15, 13, 3), C(9, 6, 1), C(15, 15, 12), C(6, 4, 1)});
    setPal(vdp, PAL_RIVAL, {C(0, 0, 0), C(4, 6, 12), C(14, 14, 15), C(8, 9, 11), C(2, 3, 6)});
    vdp.setFogColor(C(12, 13, 15));

    loadFont(vdp, art);

    {
        gs::Bitmap b(80, 26);
        b.line(4, 16, 74, 16, 3, 2.4f);
        b.line(6, 19, 72, 21, 8, 1.3f);
        b.ellipse(30, 11, 18, 6, 1);
        b.rect(18, 7, 24, 6, 2);
        b.rect(20, 4, 9, 4, 4);
        b.line(12, 14, 8, 22, 3, 1.7f);
        b.line(52, 14, 64, 22, 3, 1.7f);
        b.ellipse(70, 18, 3, 3, 5);
        art.sled = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(22, 32);
        b.ellipse(11, 7, 5, 5, 4);
        b.rect(8, 3, 6, 3, 2);
        b.rect(7, 12, 9, 11, 1);
        b.line(9, 23, 6, 31, 3, 1.7f);
        b.line(14, 23, 17, 31, 3, 1.7f);
        b.line(7, 15, 2, 12, 8, 1.4f);
        b.rect(2, 10, 4, 2, 6);
        art.musher = gs::uploadMipped(vdp, b);
    }
    for (int f = 0; f < 3; f++) {
        gs::Bitmap b(40, 20);
        int a = (f == 1) ? 3 : 0;
        int bleg = (f == 2) ? 3 : 0;
        b.ellipse(18, 10, 11, 4.4f, 1);
        b.ellipse(31, 8, 4.5f, 3.4f, 6);
        b.rect(29, 3, 2, 3, 5);
        b.rect(34, 3, 2, 3, 5);
        b.line(12, 13, 8, 18 - a, 3, 1.6f);
        b.line(22, 13, 24, 18 - bleg, 3, 1.6f);
        b.line(6, 9, 1, 7, 3, 1.3f);
        b.ellipse(33, 8, 1.1f, 1.1f, 5);
        art.dog[f] = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(64, 22);
        b.rect(2, 8, 60, 8, 1);
        b.rect(2, 14, 60, 4, 5);
        b.line(4, 6, 60, 16, 4, 2.2f);
        b.line(60, 6, 4, 16, 4, 2.2f);
        b.rect(28, 2, 8, 4, 2);
        art.paint = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(8, 36);
        b.rect(3, 6, 2, 28, 3);
        b.poly({{4, 2}, {7, 8}, {1, 8}}, 4);
        art.stake = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(28, 22);
        b.rect(2, 2, 2, 18, 3);
        b.poly({{4, 3}, {24, 8}, {4, 13}}, 1);
        b.line(6, 6, 20, 8, 2, 1.2f);
        art.flag = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(30, 48);
        b.poly({{15, 1}, {28, 24}, {2, 24}}, 1);
        b.poly({{15, 12}, {26, 38}, {4, 38}}, 2);
        b.rect(12, 38, 6, 8, 4);
        b.ellipse(10, 18, 2, 1.4f, 3);
        art.pine = gs::uploadMipped(vdp, b);
    }
    {
        gs::Bitmap b(24, 10);
        b.ellipse(12, 6, 11, 3.2f, 1);
        b.ellipse(8, 5, 4, 2.2f, 5);
        art.drift = gs::uploadMipped(vdp, b);
    }
    {
        gs::TextStyle st{2, 1, 4, 0, 1};
        art.wordMark = gs::uploadMipped(vdp, gs::textBitmap("MARK", st));
    }
}

}  // namespace mushmark
