#include "game/art.h"

#include <initializer_list>

namespace redoubtbann {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

Bitmap soldier(bool leaping) {
    Bitmap b(22, 40);
    b.ellipse(11, 6, 5, 5, 1);
    b.rect(8, 2, 6, 3, 5);
    b.rect(9, 10, 5, 3, 1);
    b.rect(6, 13, 10, 12, 2);
    b.rect(5, 15, 3, 8, 3);
    b.rect(14, 15, 3, 8, 3);
    if (leaping) {
        b.rect(6, 25, 5, 4, 4);
        b.rect(12, 24, 7, 3, 4);
        b.rect(16, 21, 4, 3, 4);
    } else {
        b.rect(7, 25, 3, 10, 4);
        b.rect(12, 25, 3, 10, 4);
        b.rect(6, 34, 4, 3, 6);
        b.rect(12, 34, 4, 3, 6);
    }
    b.rect(10, 14, 2, 8, 6);
    b.set(9, 6, 7);
    b.set(13, 6, 7);
    return b;
}

Bitmap foeArt() {
    Bitmap b(22, 38);
    b.ellipse(11, 6, 5, 5, 1);
    b.rect(7, 1, 8, 3, 5);
    b.rect(5, 13, 12, 12, 2);
    b.rect(3, 15, 3, 7, 3);
    b.rect(16, 15, 3, 7, 3);
    b.rect(7, 25, 3, 10, 4);
    b.rect(12, 25, 3, 10, 4);
    b.rect(4, 16, 14, 2, 6);
    b.set(9, 6, 7);
    b.set(13, 6, 7);
    return b;
}

Bitmap bannerArt() {
    Bitmap b(26, 36);
    b.rect(4, 2, 2, 32, 4);
    b.rect(6, 4, 16, 12, 1);
    b.rect(6, 16, 16, 4, 2);
    b.rect(8, 6, 4, 8, 3);
    b.rect(14, 6, 4, 8, 3);
    b.ellipse(5, 34, 3, 2, 5);
    return b;
}

Bitmap staffArt() {
    Bitmap b(14, 48);
    b.rect(6, 2, 2, 42, 1);
    b.rect(4, 40, 6, 4, 2);
    b.ellipse(7, 4, 3, 3, 3);
    return b;
}

Bitmap bagArt() {
    Bitmap b(30, 22);
    b.rect(2, 6, 26, 8, 2);
    b.rect(4, 2, 22, 6, 3);
    b.rect(3, 14, 24, 6, 1);
    b.rect(6, 8, 6, 3, 4);
    b.rect(16, 4, 6, 3, 4);
    return b;
}

Bitmap worksArt() {
    Bitmap b(72, 48);
    b.poly({{2, 46}, {10, 22}, {62, 18}, {70, 46}}, 2);
    b.rect(16, 20, 28, 16, 3);
    b.rect(22, 12, 10, 10, 4);
    b.rect(8, 30, 12, 8, 1);
    b.rect(48, 28, 14, 8, 5);
    b.rect(30, 24, 8, 6, 1);
    return b;
}

Bitmap puffArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 6, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 0)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 0, 0)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 15, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(0, 3, 1)});
    setPal(vdp, PAL_HERO,
           {0, gs::rgb4(12, 8, 5), gs::rgb4(4, 6, 3), gs::rgb4(2, 4, 2), gs::rgb4(3, 3, 2), gs::rgb4(5, 6, 4),
            gs::rgb4(8, 7, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FOE,
           {0, gs::rgb4(10, 7, 5), gs::rgb4(6, 3, 2), gs::rgb4(4, 2, 2), gs::rgb4(2, 2, 2), gs::rgb4(3, 2, 2),
            gs::rgb4(10, 8, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BANNER,
           {0, gs::rgb4(12, 2, 2), gs::rgb4(14, 12, 3), gs::rgb4(15, 14, 10), gs::rgb4(5, 4, 2), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_EARTH,
           {0, gs::rgb4(5, 4, 2), gs::rgb4(7, 6, 3), gs::rgb4(4, 5, 3), gs::rgb4(6, 6, 5), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(12, 10, 4)});
    setPal(vdp, PAL_BURST, {0, gs::rgb4(14, 12, 6), gs::rgb4(15, 8, 3)});
    vdp.setFogColor(gs::rgb4(6, 7, 8));
    loadFont(vdp, art);
    art.stand = gs::uploadMipped(vdp, soldier(false));
    art.hop = gs::uploadMipped(vdp, soldier(true));
    art.foe = gs::uploadMipped(vdp, foeArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.staff = gs::uploadMipped(vdp, staffArt());
    art.bag = gs::uploadMipped(vdp, bagArt());
    art.works = gs::uploadMipped(vdp, worksArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
}

}  // namespace redoubtbann
