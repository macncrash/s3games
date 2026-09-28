#include "game/art.h"

#include <initializer_list>

namespace paradegold {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap meArt() {
    gs::Bitmap b(14, 28);
    b.rect(5, 0, 4, 3, 5);
    b.rect(4, 2, 6, 2, 4);
    b.ellipse(7, 7, 3.4f, 3.0f, 3);
    b.set(6, 7, 6);
    b.set(9, 7, 6);
    b.rect(6, 10, 2, 2, 2);
    b.rect(4, 12, 6, 8, 1);
    b.rect(5, 13, 2, 6, 2);
    b.rect(2, 13, 2, 6, 1);
    b.rect(10, 13, 2, 6, 1);
    b.rect(4, 20, 2, 6, 7);
    b.rect(8, 20, 2, 6, 7);
    b.rect(3, 25, 3, 2, 6);
    b.rect(8, 25, 3, 2, 6);
    return b;
}

gs::Bitmap wagonArt() {
    gs::Bitmap b(44, 20);
    b.rect(2, 6, 40, 9, 1);
    b.rect(2, 4, 40, 3, 2);
    b.poly({{4, 4}, {12, 4}, {8, 0}}, 3);
    b.poly({{16, 4}, {26, 4}, {21, 0}}, 4);
    b.poly({{30, 4}, {40, 4}, {35, 0}}, 3);
    b.rect(6, 8, 8, 5, 5);
    b.rect(18, 8, 8, 5, 4);
    b.rect(30, 8, 8, 5, 5);
    b.ellipse(10, 16, 3.2f, 3.0f, 6);
    b.ellipse(34, 16, 3.2f, 3.0f, 6);
    b.ellipse(10, 16, 1.3f, 1.3f, 7);
    b.ellipse(34, 16, 1.3f, 1.3f, 7);
    return b;
}

gs::Bitmap coinArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6.2f, 6.2f, 2);
    b.ellipse(7, 7, 4.6f, 4.6f, 1);
    b.poly({{7, 3}, {8.4f, 6}, {11.4f, 6.2f}, {9, 8.2f}, {10, 11.4f}, {7, 9.6f}, {4, 11.4f}, {5, 8.2f}, {2.6f, 6.2f},
            {5.6f, 6}},
           3);
    return b;
}

gs::Bitmap creamArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5.2f, 5.2f, 2);
    b.ellipse(6, 6, 3.2f, 3.2f, 1);
    b.rect(5, 3, 2, 6, 3);
    return b;
}

gs::Bitmap flagArt() {
    gs::Bitmap b(12, 20);
    b.rect(1, 0, 2, 20, 3);
    b.poly({{3, 2}, {11, 5}, {3, 9}}, 1);
    b.poly({{4, 3}, {9, 5}, {4, 7}}, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 6);
    b.ellipse(8, 3, 7.f, 2.f, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
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
        a.font[c - 32] = t;
    }
}

gs::Image phrase(gs::VDP& vdp, const char* s) {
    return gs::uploadImage(vdp, gs::textBitmap(s, {2, 1, 2, 0, 1}));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 13);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(3, 3, 4), gs::rgb4(8, 8, 9)});
    setPal(vdp, PAL_ME, {0, gs::rgb4(12, 2, 3), gs::rgb4(15, 14, 10), gs::rgb4(13, 9, 6), gs::rgb4(15, 12, 2),
                         gs::rgb4(4, 2, 8), gs::rgb4(2, 2, 2), gs::rgb4(3, 4, 9)});
    setPal(vdp, PAL_WAGON, {0, gs::rgb4(11, 3, 4), gs::rgb4(15, 13, 8), gs::rgb4(14, 4, 5), gs::rgb4(4, 7, 13),
                            gs::rgb4(15, 14, 6), gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(15, 14, 10), gs::rgb4(12, 9, 5), gs::rgb4(8, 6, 3), gs::rgb4(15, 15, 13)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 2), gs::rgb4(10, 7, 1), gs::rgb4(15, 15, 12), gs::rgb4(6, 4, 0)});
    setPal(vdp, PAL_BUNT, {0, gs::rgb4(13, 2, 3), gs::rgb4(15, 14, 12), gs::rgb4(5, 3, 2)});
    setPal(vdp, PAL_CROWD, {0, gs::rgb4(7, 5, 8), gs::rgb4(4, 6, 9), gs::rgb4(11, 8, 5), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(6, 8, 12), gs::rgb4(9, 11, 13)});
    loadFont(vdp, art);
    art.me = gs::uploadMipped(vdp, meArt());
    art.wagon = gs::uploadMipped(vdp, wagonArt());
    art.coin = gs::uploadMipped(vdp, coinArt());
    art.cream = gs::uploadMipped(vdp, creamArt());
    art.flag = gs::uploadMipped(vdp, flagArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.logo = phrase(vdp, "PARADE GOLD");
    art.done = phrase(vdp, "GOLD DOUBLE");
}

}  // namespace paradegold
