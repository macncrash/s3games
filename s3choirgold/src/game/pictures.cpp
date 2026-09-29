#include "game/pictures.h"

#include <initializer_list>

namespace choirgold {
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
    vdp.setColor(pal * 16 + 15, edge);
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

gs::Bitmap naveArt() {
    gs::Bitmap b(320, 100);
    b.rect(0, 40, 320, 60, 1);
    b.rect(0, 36, 320, 6, 2);
    for (int i = 0; i < 3; i++) {
        int cx = 56 + i * 104;
        b.rect(cx - 28, 8, 56, 70, 3);
        b.ellipse(float(cx), 18.f, 26.f, 16.f, 3);
        b.rect(cx - 18, 22, 36, 40, 4);
        b.ellipse(float(cx), 28.f, 16.f, 10.f, 4);
        b.rect(cx - 2, 8, 4, 62, 5);
    }
    b.rect(8, 86, 304, 8, 2);
    return b;
}

gs::Bitmap singerArt(int kind) {
    gs::Bitmap b(36, 52);
    const int cx = 18;
    int robe = kind == 2 ? 14 : kind == 1 ? 12 : 10;
    b.ellipse(float(cx), 48.f, float(robe) * 0.45f, 3.f, 6);
    b.poly({{float(cx - robe / 2), 22},
            {float(cx + robe / 2), 22},
            {float(cx + robe / 2 - 2), 48},
            {float(cx - robe / 2 + 2), 48}},
           2);
    b.poly({{float(cx - robe / 2 + 2), 24}, {float(cx - 1), 24}, {float(cx - 1), 46}, {float(cx - robe / 2 + 4), 46}}, 1);
    b.rect(cx - 3, 18, 6, 6, 3);
    b.ellipse(float(cx), 12.f, 6.f, 7.f, 3);
    if (kind == 0) b.ellipse(float(cx), 7.f, 7.f, 4.f, 4);
    else if (kind == 1) {
        b.rect(cx - 8, 8, 3, 14, 4);
        b.rect(cx + 5, 8, 3, 14, 4);
        b.ellipse(float(cx), 7.f, 7.f, 4.f, 4);
    } else {
        b.rect(cx - 6, 6, 12, 4, 4);
        b.ellipse(float(cx), 16.f, 5.f, 3.f, 4);
    }
    b.set(cx - 3, 12, 5);
    b.set(cx + 2, 12, 5);
    b.rect(cx - 2, 16, 4, 2, 7);
    return b;
}

gs::Bitmap noteArt() {
    gs::Bitmap b(14, 16);
    b.ellipse(5.f, 12.f, 4.f, 3.f, 1);
    b.rect(8, 2, 2, 11, 1);
    b.rect(8, 2, 5, 2, 1);
    return b;
}

gs::Bitmap diamondArt() {
    gs::Bitmap b(11, 11);
    b.poly({{5.f, 0.f}, {10.f, 5.f}, {5.f, 10.f}, {0.f, 5.f}}, 1);
    b.poly({{5.f, 2.f}, {8.f, 5.f}, {5.f, 8.f}, {2.f, 5.f}}, 2);
    return b;
}

gs::Bitmap barArt() {
    gs::Bitmap b(2, 8);
    b.rect(0, 0, 2, 8, 1);
    return b;
}

gs::Bitmap staffArt() {
    gs::Bitmap b(8, 1);
    b.rect(0, 0, 8, 1, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_INK, gs::rgb4(14, 13, 11), gs::rgb4(2, 1, 3));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3), gs::rgb4(3, 1, 0));
    textPal(vdp, PAL_CREAM, gs::rgb4(15, 14, 11), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 14, 12), gs::rgb4(1, 2, 2));
    setPal(vdp, PAL_NAVE,
           {0, gs::rgb4(3, 2, 5), gs::rgb4(6, 4, 3), gs::rgb4(8, 7, 9), gs::rgb4(4, 6, 10), gs::rgb4(12, 10, 6)});
    setPal(vdp, PAL_ROBE,
           {0, gs::rgb4(4, 2, 6), gs::rgb4(7, 3, 8), gs::rgb4(13, 9, 7), gs::rgb4(5, 3, 2), gs::rgb4(1, 1, 2),
            gs::rgb4(2, 2, 3), gs::rgb4(8, 2, 3)});
    setPal(vdp, PAL_NOTE, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_WAX, {0, gs::rgb4(14, 12, 9), gs::rgb4(15, 15, 13)});

    loadFont(vdp, art);
    art.nave = gs::uploadImage(vdp, naveArt());
    for (int i = 0; i < 3; i++) art.singer[i] = gs::uploadImage(vdp, singerArt(i));
    art.note = gs::uploadImage(vdp, noteArt());
    art.diamond = gs::uploadImage(vdp, diamondArt());
    art.bar = gs::uploadImage(vdp, barArt());
    art.staff = gs::uploadImage(vdp, staffArt());
}

}  // namespace choirgold
