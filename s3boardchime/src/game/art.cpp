#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace boardchime {
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

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8 && x + 2 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap deskArt() {
    gs::Bitmap b(300, 150);
    b.rect(0, 0, 300, 150, 1);
    b.rect(8, 8, 284, 134, 2);
    b.rect(16, 16, 86, 112, 3);
    b.rect(198, 16, 86, 112, 3);
    for (int i = 0; i < 6; i++) {
        int y = 24 + i * 17;
        b.ellipse(42.f, float(y + 6), 6.f, 6.f, 4);
        b.rect(210, y, 12, 12, 5);
        b.rect(226, y + 3, 46, 6, 6);
    }
    b.rect(112, 126, 76, 6, 7);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8.f, 8.f, 7.f, 7.f, 1);
    b.ellipse(8.f, 8.f, 4.f, 4.f, 2);
    b.ellipse(6.f, 6.f, 1.4f, 1.2f, 3);
    return b;
}

gs::Bitmap plugArt() {
    gs::Bitmap b(12, 18);
    b.rect(2, 0, 8, 8, 1);
    b.rect(3, 8, 2, 8, 2);
    b.rect(7, 8, 2, 8, 2);
    b.rect(1, 2, 10, 3, 3);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(46, 46);
    b.ellipse(23.f, 23.f, 22.f, 22.f, 1);
    b.ellipse(23.f, 23.f, 18.f, 18.f, 2);
    b.ellipse(23.f, 23.f, 2.f, 2.f, 3);
    for (int i = 0; i < 12; i++) {
        float a = float(i) * 0.5236f - 1.5708f;
        int x = int(23.f + std::cos(a) * 15.f);
        int y = int(23.f + std::sin(a) * 15.f);
        b.rect(x, y, 2, 2, i % 3 == 0 ? 4 : 3);
    }
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(3, 12);
    b.rect(1, 0, 1, 12, 1);
    b.rect(0, 10, 3, 2, 1);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2.f, 2.f, 1.6f, 1.6f, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_DESK,
           {0, gs::rgb4(3, 2, 4), gs::rgb4(6, 5, 4), gs::rgb4(4, 3, 6), gs::rgb4(2, 2, 3), gs::rgb4(9, 8, 5),
            gs::rgb4(5, 7, 6), gs::rgb4(10, 8, 3)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(12, 7, 1), gs::rgb4(15, 13, 4), gs::rgb4(15, 15, 11)});
    setPal(vdp, PAL_PLUG, {0, gs::rgb4(3, 3, 4), gs::rgb4(13, 12, 9), gs::rgb4(14, 5, 3)});
    setPal(vdp, PAL_CLOCK, {0, gs::rgb4(8, 6, 2), gs::rgb4(14, 12, 8), gs::rgb4(4, 3, 2), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_CORD, {0, gs::rgb4(13, 4, 3)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 13, 5), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 11), gs::rgb4(1, 3, 1));
    textPal(vdp, PAL_DEAD, gs::rgb4(15, 8, 6), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(1, 2, 2));

    loadFont(vdp, art);
    art.desk = gs::uploadImage(vdp, deskArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
    art.plug = gs::uploadImage(vdp, plugArt());
    art.clock = gs::uploadImage(vdp, clockArt());
    art.hand = gs::uploadImage(vdp, handArt());
    art.bead = gs::uploadImage(vdp, beadArt());
}

}  // namespace boardchime
