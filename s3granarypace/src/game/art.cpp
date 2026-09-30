#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace granary {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t edge) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, edge);
}

gs::Bitmap barnArt() {
    gs::Bitmap b(108, 132);
    for (int i = 0; i < 28; ++i) b.rect(float(6 + i), float(36 - i), float(96 - 2 * i), 2, (i & 3) == 0 ? 3 : 1);
    b.rect(10, 36, 88, 88, 2);
    b.rect(10, 36, 88, 6, 4);
    for (int y = 48; y < 118; y += 10) b.rect(14, float(y), 80, 2, 5);
    b.rect(40, 70, 28, 54, 6);
    b.rect(40, 70, 28, 4, 3);
    b.rect(52, 74, 3, 50, 7);
    b.rect(18, 48, 16, 14, 8);
    b.rect(74, 48, 16, 14, 8);
    b.rect(24, 54, 4, 4, 9);
    b.rect(80, 54, 4, 4, 9);
    b.rect(46, 8, 16, 14, 2);
    b.rect(50, 4, 8, 6, 3);
    b.rect(51, 12, 6, 6, 9);
    b.rect(8, 118, 18, 12, 4);
    b.rect(82, 116, 16, 14, 3);
    return b;
}

void manBody(gs::Bitmap& b, int leg) {
    b.rect(6, 0, 14, 4, 4);
    b.ellipse(13.f, 8.f, 5.f, 5.f, 1);
    b.rect(11, 7, 3, 2, 5);
    b.rect(8, 13, 10, 14, 2);
    b.rect(8, 13, 10, 3, 3);
    b.rect(5, 16, 3, 10, 2);
    b.rect(18, 16, 3, 10, 2);
    if (leg == 0) {
        b.rect(8, 26, 4, 12, 6);
        b.rect(14, 26, 4, 8, 6);
        b.rect(7, 36, 6, 3, 7);
        b.rect(14, 32, 6, 3, 7);
    } else {
        b.rect(8, 26, 4, 8, 6);
        b.rect(14, 26, 4, 12, 6);
        b.rect(6, 32, 6, 3, 7);
        b.rect(13, 36, 6, 3, 7);
    }
}

gs::Bitmap manArt(int leg) {
    gs::Bitmap b(26, 42);
    manBody(b, leg);
    return b;
}

gs::Bitmap gunArt() {
    gs::Bitmap b(56, 14);
    b.rect(0, 5, 40, 4, 1);
    b.rect(0, 5, 40, 1, 2);
    b.rect(36, 3, 8, 8, 3);
    b.rect(44, 6, 12, 2, 2);
    b.rect(8, 9, 8, 5, 4);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8.f, 8.f, 7.f, 7.f, 1);
    b.ellipse(8.f, 8.f, 3.f, 3.f, 2);
    return b;
}

gs::Bitmap stalkArt() {
    gs::Bitmap b(7, 22);
    b.rect(3, 6, 1, 16, 1);
    b.ellipse(3.5f, 5.f, 3.f, 4.f, 2);
    b.ellipse(3.5f, 4.f, 1.4f, 1.6f, 3);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(16, 12);
    b.ellipse(8.f, 7.f, 7.f, 4.5f, 1);
    b.ellipse(8.f, 6.f, 4.f, 2.2f, 2);
    b.rect(7, 2, 2, 5, 3);
    return b;
}

gs::Bitmap bootArt() {
    gs::Bitmap b(10, 6);
    b.ellipse(4.f, 3.f, 3.5f, 2.2f, 1);
    b.rect(5, 2, 4, 2, 2);
    return b;
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BARN,
           {0, gs::rgb4(8, 3, 2), gs::rgb4(11, 6, 3), gs::rgb4(6, 2, 1), gs::rgb4(13, 9, 5), gs::rgb4(7, 4, 2),
            gs::rgb4(3, 2, 1), gs::rgb4(5, 4, 3), gs::rgb4(4, 6, 8), gs::rgb4(12, 12, 8)});
    setPal(vdp, PAL_MAN,
           {0, gs::rgb4(12, 8, 5), gs::rgb4(4, 5, 7), gs::rgb4(7, 8, 10), gs::rgb4(3, 2, 1), gs::rgb4(2, 2, 2),
            gs::rgb4(3, 3, 4), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_GUN, {0, gs::rgb4(4, 4, 4), gs::rgb4(9, 9, 8), gs::rgb4(6, 5, 3), gs::rgb4(3, 2, 1), gs::rgb4(15, 13, 6),
                          gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_WHEAT, {0, gs::rgb4(8, 7, 2), gs::rgb4(13, 11, 3), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(8, 6, 3), gs::rgb4(12, 9, 5)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 12), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 5), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 7, 5), gs::rgb4(1, 1, 1));

    loadFont(vdp, art);
    art.barn = gs::uploadImage(vdp, barnArt());
    art.manA = gs::uploadImage(vdp, manArt(0));
    art.manB = gs::uploadImage(vdp, manArt(1));
    art.gun = gs::uploadImage(vdp, gunArt());
    art.flash = gs::uploadImage(vdp, flashArt());
    art.stalk = gs::uploadImage(vdp, stalkArt());
    art.sack = gs::uploadImage(vdp, sackArt());
    art.boot = gs::uploadImage(vdp, bootArt());
}

}  // namespace granary
