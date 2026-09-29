#include "game/art.h"

#include "console/gfx.h"

namespace memchime {
namespace {

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

gs::Bitmap cardBack() {
    gs::Bitmap b(32, 44);
    b.rect(0, 0, 32, 44, 1);
    b.rect(2, 2, 28, 40, 2);
    b.rect(6, 8, 20, 28, 3);
    b.rect(10, 14, 12, 16, 2);
    b.rect(14, 18, 4, 8, 5);
    return b;
}

gs::Bitmap paper() {
    gs::Bitmap b(32, 44);
    b.rect(0, 0, 32, 44, 1);
    b.rect(2, 2, 28, 40, 4);
    b.rect(3, 3, 8, 3, 11);
    return b;
}

gs::Bitmap bellFace() {
    gs::Bitmap b = paper();
    b.ellipse(16, 22, 9, 8, 5);
    b.rect(15, 12, 2, 8, 5);
    b.rect(10, 28, 12, 3, 6);
    b.ellipse(16, 30, 2, 2, 7);
    return b;
}

gs::Bitmap starFace() {
    gs::Bitmap b = paper();
    b.line(16, 10, 19, 18, 8, 2);
    b.line(19, 18, 28, 18, 8, 2);
    b.line(28, 18, 21, 23, 8, 2);
    b.line(21, 23, 24, 32, 8, 2);
    b.line(24, 32, 16, 26, 8, 2);
    b.line(16, 26, 8, 32, 8, 2);
    b.line(8, 32, 11, 23, 8, 2);
    b.line(11, 23, 4, 18, 8, 2);
    b.line(4, 18, 13, 18, 8, 2);
    b.line(13, 18, 16, 10, 8, 2);
    b.set(16, 20, 11);
    return b;
}

gs::Bitmap moonFace() {
    gs::Bitmap b = paper();
    b.ellipse(18, 24, 9, 11, 9);
    b.ellipse(22, 22, 7, 9, 4);
    b.set(12, 16, 11);
    return b;
}

gs::Bitmap keyFace() {
    gs::Bitmap b = paper();
    b.ellipse(13, 16, 6, 6, 10);
    b.ellipse(13, 16, 3, 3, 4);
    b.rect(17, 14, 10, 4, 10);
    b.rect(22, 18, 3, 5, 10);
    b.rect(26, 18, 3, 7, 6);
    return b;
}

gs::Bitmap cursor() {
    gs::Bitmap b(36, 48);
    b.rect(0, 0, 36, 3, 14);
    b.rect(0, 45, 36, 3, 14);
    b.rect(0, 0, 3, 48, 14);
    b.rect(33, 0, 3, 48, 14);
    return b;
}

gs::Bitmap tower() {
    gs::Bitmap b(28, 48);
    b.rect(11, 0, 6, 8, 5);
    b.rect(6, 8, 16, 5, 6);
    b.rect(8, 13, 12, 24, 2);
    b.ellipse(14, 22, 5, 5, 4);
    b.rect(13, 17, 2, 6, 7);
    b.rect(11, 22, 4, 2, 7);
    b.rect(4, 37, 20, 11, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_CARD * 16 + i, 0);
    vdp.setColor(PAL_CARD * 16 + 1, gs::rgb4(1, 1, 3));
    vdp.setColor(PAL_CARD * 16 + 2, gs::rgb4(2, 3, 8));
    vdp.setColor(PAL_CARD * 16 + 3, gs::rgb4(4, 6, 12));
    vdp.setColor(PAL_CARD * 16 + 4, gs::rgb4(14, 12, 9));
    vdp.setColor(PAL_CARD * 16 + 5, gs::rgb4(15, 12, 3));
    vdp.setColor(PAL_CARD * 16 + 6, gs::rgb4(12, 7, 3));
    vdp.setColor(PAL_CARD * 16 + 7, gs::rgb4(12, 2, 2));
    vdp.setColor(PAL_CARD * 16 + 8, gs::rgb4(15, 14, 4));
    vdp.setColor(PAL_CARD * 16 + 9, gs::rgb4(13, 14, 15));
    vdp.setColor(PAL_CARD * 16 + 10, gs::rgb4(10, 8, 4));
    vdp.setColor(PAL_CARD * 16 + 11, gs::rgb4(15, 15, 13));
    vdp.setColor(PAL_CARD * 16 + 14, gs::rgb4(15, 13, 6));

    textPal(vdp, PAL_CREAM, gs::rgb4(14, 12, 9));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_DIM, gs::rgb4(7, 8, 11));
    textPal(vdp, PAL_LEAF, gs::rgb4(6, 13, 7));
    loadFont(vdp, art);

    art.back = gs::uploadImage(vdp, cardBack());
    art.face[0] = gs::uploadImage(vdp, bellFace());
    art.face[1] = gs::uploadImage(vdp, starFace());
    art.face[2] = gs::uploadImage(vdp, moonFace());
    art.face[3] = gs::uploadImage(vdp, keyFace());
    art.cursor = gs::uploadImage(vdp, cursor());
    art.tower = gs::uploadImage(vdp, tower());
}

}  // namespace memchime
