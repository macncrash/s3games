#include "game/art.h"

#include <initializer_list>

namespace boardbell {
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

gs::Bitmap deskArt() {
    gs::Bitmap b(280, 168);
    b.rect(0, 0, 280, 168, 1);
    b.rect(6, 6, 268, 156, 2);
    b.rect(10, 14, 78, 140, 3);
    b.rect(192, 14, 78, 140, 3);
    for (int i = 0; i < 6; i++) {
        int y = 22 + i * 22;
        b.ellipse(36.f, float(y + 8), 7.f, 7.f, 4);
        b.rect(188, y, 14, 14, 5);
        b.rect(200, y + 4, 52, 6, 6);
    }
    b.rect(96, 148, 88, 8, 7);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7.f, 7.f, 6.f, 6.f, 1);
    b.ellipse(7.f, 7.f, 3.2f, 3.2f, 2);
    b.ellipse(5.f, 5.f, 1.2f, 1.f, 3);
    return b;
}

gs::Bitmap plugArt() {
    gs::Bitmap b(10, 16);
    b.rect(2, 0, 6, 8, 1);
    b.rect(3, 8, 2, 6, 2);
    b.rect(5, 8, 2, 6, 2);
    b.rect(1, 2, 8, 3, 3);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(18, 16);
    b.rect(8, 0, 2, 3, 1);
    b.ellipse(9.f, 8.f, 7.f, 6.f, 2);
    b.rect(3, 8, 12, 5, 0);
    b.ellipse(9.f, 8.f, 5.f, 3.2f, 3);
    b.ellipse(9.f, 13.f, 1.4f, 1.4f, 4);
    return b;
}

gs::Bitmap badgeArt() {
    gs::Bitmap b(8, 8);
    b.rect(1, 1, 6, 6, 1);
    b.rect(3, 0, 2, 8, 2);
    b.rect(0, 3, 8, 2, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_DESK,
           {0, gs::rgb4(2, 2, 4), gs::rgb4(5, 4, 3), gs::rgb4(3, 3, 5), gs::rgb4(1, 1, 2), gs::rgb4(8, 7, 4),
            gs::rgb4(4, 6, 5), gs::rgb4(9, 7, 3)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(10, 6, 1), gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_PLUG, {0, gs::rgb4(2, 2, 3), gs::rgb4(12, 11, 8), gs::rgb4(14, 4, 3)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(6, 5, 2), gs::rgb4(12, 9, 2), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_CORD, {0, gs::rgb4(12, 3, 3), gs::rgb4(15, 8, 4)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 4), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 11), gs::rgb4(1, 4, 1));
    textPal(vdp, PAL_DEAD, gs::rgb4(15, 8, 6), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(1, 2, 2));

    loadFont(vdp, art);
    art.desk = gs::uploadImage(vdp, deskArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
    art.plug = gs::uploadImage(vdp, plugArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.badge = gs::uploadImage(vdp, badgeArt());
}

}  // namespace boardbell
