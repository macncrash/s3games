#include "game/art.h"

#include <initializer_list>

namespace mailpass {
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

gs::Bitmap hoodArt() {
    gs::Bitmap b(220, 72);
    b.poly({{8, 30}, {48, 8}, {172, 8}, {212, 30}, {220, 72}, {0, 72}}, 1);
    b.rect(16, 34, 188, 22, 2);
    b.rect(24, 38, 48, 12, 3);
    b.rect(148, 38, 48, 12, 3);
    b.rect(78, 14, 64, 14, 4);
    b.rect(84, 18, 52, 6, 5);
    b.rect(96, 40, 28, 8, 6);
    b.rect(0, 58, 220, 14, 7);
    for (int i = 0; i < 10; i++) b.rect(8.f + i * 21.f, 60, 12, 6, i % 2 ? 8 : 9);
    b.rect(6, 24, 18, 6, 10);
    b.rect(196, 24, 18, 6, 10);
    b.rect(100, 48, 20, 6, 11);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(40, 72);
    b.poly({{20, 2}, {36, 28}, {28, 26}, {38, 48}, {26, 46}, {34, 62}, {6, 62}, {14, 46}, {2, 48}, {12, 26}, {4, 28}}, 1);
    b.rect(16, 60, 8, 10, 2);
    b.poly({{20, 8}, {28, 26}, {12, 26}}, 3);
    return b;
}

gs::Bitmap cairnArt() {
    gs::Bitmap b(36, 40);
    b.poly({{4, 36}, {10, 22}, {6, 18}, {16, 8}, {24, 14}, {30, 10}, {34, 36}}, 1);
    b.rect(12, 16, 8, 6, 2);
    b.rect(8, 28, 16, 6, 3);
    return b;
}

gs::Bitmap signArt() {
    gs::Bitmap b(48, 56);
    b.rect(22, 22, 4, 32, 1);
    b.rect(4, 6, 40, 18, 2);
    b.rect(8, 10, 32, 4, 3);
    b.rect(8, 16, 20, 4, 3);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(32, 40);
    b.ellipse(16, 24, 12, 14, 1);
    b.poly({{10, 14}, {16, 2}, {22, 14}}, 2);
    b.rect(12, 8, 8, 6, 3);
    return b;
}

gs::Bitmap archArt() {
    gs::Bitmap b(96, 48);
    b.rect(2, 8, 10, 40, 1);
    b.rect(84, 8, 10, 40, 1);
    b.poly({{2, 16}, {48, 2}, {94, 16}, {84, 18}, {48, 8}, {12, 18}}, 2);
    b.rect(18, 14, 60, 4, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    vdp.setFogColor(gs::rgb4(6, 7, 9));
    textPal(vdp, PAL_HUD, gs::rgb4(14, 14, 13));
    textPal(vdp, PAL_ICE, gs::rgb4(8, 12, 15));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 4));
    textPal(vdp, PAL_GOOD, gs::rgb4(8, 15, 8));
    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(12, 4, 3), gs::rgb4(14, 6, 4), gs::rgb4(6, 10, 14), gs::rgb4(2, 2, 3), gs::rgb4(10, 12, 14),
            gs::rgb4(15, 12, 3), gs::rgb4(4, 4, 5), gs::rgb4(15, 14, 6), gs::rgb4(3, 3, 4), gs::rgb4(15, 15, 12),
            gs::rgb4(9, 2, 2)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(1, 6, 2), gs::rgb4(5, 3, 1), gs::rgb4(3, 9, 4)});
    setPal(vdp, PAL_CAIRN, {0, gs::rgb4(8, 8, 9), gs::rgb4(12, 12, 13), gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(6, 4, 2), gs::rgb4(14, 12, 4), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_SACK, {0, gs::rgb4(10, 8, 4), gs::rgb4(7, 5, 2), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(5, 5, 6), gs::rgb4(7, 7, 8), gs::rgb4(9, 9, 10), gs::rgb4(3, 3, 4), gs::rgb4(12, 13, 14),
            gs::rgb4(8, 9, 11), gs::rgb4(4, 5, 6), gs::rgb4(2, 2, 3), gs::rgb4(11, 12, 13), gs::rgb4(6, 6, 7),
            gs::rgb4(1, 1, 2), gs::rgb4(13, 14, 15), gs::rgb4(9, 10, 12), gs::rgb4(14, 14, 15)});
    loadFont(vdp, art);
    art.hood = gs::uploadMipped(vdp, hoodArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.cairn = gs::uploadMipped(vdp, cairnArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.arch = gs::uploadMipped(vdp, archArt());
}

}  // namespace mailpass
