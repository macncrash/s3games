#include "game/art.h"

#include <initializer_list>

namespace lanternbell {
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

gs::Bitmap postArt() {
    gs::Bitmap b(10, 70);
    b.rect(3.f, 0.f, 4.f, 64.f, 1);
    b.rect(1.f, 62.f, 8.f, 6.f, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(22, 28);
    b.poly({{11.f, 1.f}, {18.f, 8.f}, {4.f, 8.f}}, 1);
    b.rect(4.f, 8.f, 14.f, 14.f, 2);
    b.rect(6.f, 10.f, 10.f, 10.f, 3);
    b.rect(9.f, 22.f, 4.f, 5.f, 1);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(8, 12);
    b.poly({{4.f, 0.f}, {7.f, 7.f}, {4.f, 11.f}, {1.f, 7.f}}, 1);
    b.ellipse(4.f, 7.f, 1.4f, 2.2f, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(18, 20);
    b.rect(8.f, 0.f, 2.f, 3.f, 2);
    b.poly({{4.f, 5.f}, {14.f, 5.f}, {16.f, 16.f}, {2.f, 16.f}}, 1);
    b.rect(2.f, 15.f, 14.f, 3.f, 3);
    return b;
}

gs::Bitmap carryArt() {
    gs::Bitmap b(16, 22);
    b.rect(6.f, 0.f, 4.f, 3.f, 1);
    b.rect(3.f, 3.f, 10.f, 12.f, 2);
    b.rect(5.f, 5.f, 6.f, 8.f, 3);
    b.rect(7.f, 15.f, 2.f, 5.f, 1);
    return b;
}

gs::Bitmap moonArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11.f, 11.f, 10.f, 10.f, 1);
    b.ellipse(15.f, 8.f, 7.f, 7.f, 0);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_YARD, {0, gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(6, 5, 4), gs::rgb4(3, 4, 6), gs::rgb4(8, 10, 12)});
    setPal(vdp, PAL_FLAME, {0, gs::rgb4(15, 10, 2), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(14, 12, 4), gs::rgb4(8, 7, 3), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(8, 5, 3), gs::rgb4(10, 7, 3), gs::rgb4(15, 12, 4)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 15, 14), gs::rgb4(1, 1, 2));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_GREEN, gs::rgb4(5, 15, 7), gs::rgb4(1, 2, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4), gs::rgb4(2, 0, 0));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 14, 6), gs::rgb4(3, 1, 1));
    textPal(vdp, PAL_LEAVE, gs::rgb4(15, 15, 12), gs::rgb4(1, 4, 1));
    setPal(vdp, PAL_MOON, {0, gs::rgb4(14, 14, 11)});

    loadFont(vdp, art);
    art.post = gs::uploadImage(vdp, postArt());
    art.lamp = gs::uploadImage(vdp, lampArt());
    art.flame = gs::uploadImage(vdp, flameArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    gs::Bitmap clap(5, 8);
    clap.rect(2.f, 0.f, 1.f, 5.f, 1);
    clap.ellipse(2.f, 6.f, 2.f, 2.f, 2);
    art.clapper = gs::uploadImage(vdp, clap);
    art.carry = gs::uploadImage(vdp, carryArt());
    art.moon = gs::uploadImage(vdp, moonArt());
    gs::Bitmap star(3, 3);
    star.set(1, 0, 1);
    star.set(0, 1, 1);
    star.set(1, 1, 1);
    star.set(2, 1, 1);
    star.set(1, 2, 1);
    art.star = gs::uploadImage(vdp, star);
    art.title = gs::uploadImage(vdp, gs::textBitmap("S3 LANTERNBELL", {2, 1, 2, 0, 1}));
    art.leave = gs::uploadImage(vdp, gs::textBitmap("BELL", {2, 1, 2, 0, 1}));
}

}  // namespace lanternbell
