#include "game/art.h"

#include <initializer_list>

namespace solitairetape {
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
    gs::Bitmap b(160, 112);
    b.rect(0, 0, 160, 112, 1);
    b.rect(4, 4, 152, 104, 2);
    b.ellipse(80.f, 70.f, 62.f, 28.f, 3);
    b.rect(68, 6, 24, 18, 4);
    return b;
}

gs::Bitmap cardArt() {
    gs::Bitmap b(22, 32);
    b.rect(0, 0, 22, 32, 1);
    b.rect(1, 1, 20, 30, 2);
    b.rect(2, 2, 6, 5, 3);
    b.ellipse(11.f, 20.f, 5.f, 6.f, 4);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(8, 8);
    b.ellipse(4.f, 4.f, 3.f, 3.f, 1);
    b.ellipse(4.f, 4.f, 1.5f, 1.5f, 2);
    return b;
}

gs::Bitmap reelArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14.f, 14.f, 13.f, 13.f, 1);
    b.ellipse(14.f, 14.f, 8.f, 8.f, 2);
    b.ellipse(14.f, 14.f, 3.f, 3.f, 3);
    b.rect(13, 2, 2, 8, 4);
    return b;
}

gs::Bitmap slotArt() {
    gs::Bitmap b(28, 16);
    b.rect(0, 0, 28, 16, 1);
    b.rect(2, 2, 24, 12, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_FELT,
           {0, gs::rgb4(0, 4, 2), gs::rgb4(1, 8, 4), gs::rgb4(2, 12, 5), gs::rgb4(4, 5, 2), gs::rgb4(8, 6, 2)});
    setPal(vdp, PAL_CARD, {0, gs::rgb4(4, 3, 2), gs::rgb4(15, 14, 12), gs::rgb4(12, 2, 2), gs::rgb4(14, 11, 3)});
    setPal(vdp, PAL_PIP, {0, gs::rgb4(13, 2, 3), gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_REEL, {0, gs::rgb4(3, 3, 4), gs::rgb4(10, 8, 5), gs::rgb4(2, 2, 2), gs::rgb4(14, 12, 6)});
    setPal(vdp, PAL_SLOT, {0, gs::rgb4(5, 4, 2), gs::rgb4(2, 6, 3)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 12), gs::rgb4(0, 2, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 12, 4), gs::rgb4(2, 1, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 15, 10), gs::rgb4(0, 3, 1));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 6, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_HINT, gs::rgb4(10, 15, 12), gs::rgb4(0, 2, 2));

    loadFont(vdp, art);
    art.felt = gs::uploadImage(vdp, feltArt());
    art.card = gs::uploadImage(vdp, cardArt());
    art.pip = gs::uploadImage(vdp, pipArt());
    art.reel = gs::uploadImage(vdp, reelArt());
    art.slot = gs::uploadImage(vdp, slotArt());
}

}  // namespace solitairetape
