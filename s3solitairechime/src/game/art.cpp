#include "game/art.h"

#include <initializer_list>

namespace solitairechime {
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
    vdp.setColor(pal * 16 + 15, gs::rgb4(0, 2, 1));
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

gs::Bitmap feltArt() {
    gs::Bitmap b(312, 200);
    b.rect(0, 0, 312, 200, 1);
    b.rect(6, 6, 300, 188, 2);
    b.ellipse(156.f, 130.f, 130.f, 50.f, 3);
    return b;
}

gs::Bitmap cardArt() {
    gs::Bitmap b(26, 36);
    b.rect(0, 0, 26, 36, 1);
    b.rect(2, 2, 22, 32, 2);
    b.rect(3, 3, 7, 6, 3);
    b.ellipse(13.f, 22.f, 6.f, 7.f, 4);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5.f, 5.f, 4.f, 4.f, 1);
    b.ellipse(5.f, 5.f, 2.f, 2.f, 2);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(28, 24);
    b.rect(12, 0, 4, 4, 1);
    b.ellipse(14.f, 12.f, 12.f, 9.f, 2);
    b.rect(3, 16, 22, 4, 3);
    b.ellipse(14.f, 11.f, 6.f, 4.f, 4);
    return b;
}

gs::Bitmap clapperArt() {
    gs::Bitmap b(8, 8);
    b.rect(3, 0, 2, 3, 1);
    b.ellipse(4.f, 5.f, 3.f, 3.f, 2);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(40, 40);
    b.ellipse(20.f, 20.f, 18.f, 18.f, 1);
    b.ellipse(20.f, 20.f, 15.f, 15.f, 2);
    b.rect(18, 6, 4, 3, 3);
    b.rect(18, 31, 4, 3, 3);
    b.rect(6, 18, 3, 4, 3);
    b.rect(31, 18, 3, 4, 3);
    b.ellipse(20.f, 20.f, 2.f, 2.f, 4);
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(4, 12);
    b.rect(1, 0, 2, 12, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_FELT,
           {0, gs::rgb4(0, 3, 2), gs::rgb4(1, 7, 3), gs::rgb4(2, 11, 5), gs::rgb4(0, 2, 1)});
    setPal(vdp, PAL_CARD, {0, gs::rgb4(5, 3, 2), gs::rgb4(15, 14, 11), gs::rgb4(12, 3, 3), gs::rgb4(14, 10, 3)});
    setPal(vdp, PAL_PIP, {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 12, 8)});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(8, 6, 2), gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 6), gs::rgb4(12, 8, 2)});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(6, 5, 3), gs::rgb4(15, 14, 10), gs::rgb4(2, 2, 2), gs::rgb4(12, 4, 2), gs::rgb4(1, 1, 1)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 12), gs::rgb4(0, 2, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 12, 3), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 10), gs::rgb4(0, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 6, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(0, 2, 2));

    loadFont(vdp, art);
    art.felt = gs::uploadImage(vdp, feltArt());
    art.card = gs::uploadImage(vdp, cardArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.bell = gs::uploadImage(vdp, bellArt());
    art.clapper = gs::uploadImage(vdp, clapperArt());
    art.clock = gs::uploadImage(vdp, clockArt());
    art.hand = gs::uploadImage(vdp, handArt());
}

}  // namespace solitairechime
