#include "game/art.h"

#include <initializer_list>

namespace memorymark {
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

gs::Bitmap backArt() {
    gs::Bitmap b(36, 48);
    b.rect(0, 0, 36, 48, 1);
    b.rect(2, 2, 32, 44, 2);
    b.rect(14, 18, 8, 12, 3);
    return b;
}

gs::Bitmap faceBar() {
    gs::Bitmap b(36, 48);
    b.rect(0, 0, 36, 48, 1);
    b.rect(16, 8, 4, 32, 2);
    b.rect(10, 8, 16, 3, 3);
    return b;
}

gs::Bitmap faceRing() {
    gs::Bitmap b(36, 48);
    b.rect(0, 0, 36, 48, 1);
    b.ellipse(18.f, 24.f, 12.f, 14.f, 2);
    b.ellipse(18.f, 24.f, 6.f, 7.f, 1);
    return b;
}

gs::Bitmap faceChev() {
    gs::Bitmap b(36, 48);
    b.rect(0, 0, 36, 48, 1);
    b.line(8.f, 16.f, 18.f, 32.f, 2, 2.f);
    b.line(18.f, 32.f, 28.f, 16.f, 2, 2.f);
    b.line(10.f, 12.f, 18.f, 22.f, 3, 1.5f);
    b.line(18.f, 22.f, 26.f, 12.f, 3, 1.5f);
    return b;
}

gs::Bitmap faceCross() {
    gs::Bitmap b(36, 48);
    b.rect(0, 0, 36, 48, 1);
    b.rect(16, 8, 4, 32, 2);
    b.rect(8, 22, 20, 4, 2);
    b.rect(8, 22, 4, 4, 3);
    return b;
}

gs::Bitmap stampArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14.f, 14.f, 13.f, 13.f, 1);
    b.ellipse(14.f, 14.f, 8.f, 8.f, 2);
    b.rect(12, 6, 4, 16, 3);
    b.rect(6, 12, 16, 4, 3);
    return b;
}

gs::Bitmap handArt() {
    gs::Bitmap b(18, 16);
    b.ellipse(9.f, 10.f, 7.f, 5.f, 1);
    b.rect(3.f, 2.f, 2.f, 6.f, 1);
    b.rect(6.f, 1.f, 2.f, 6.f, 1);
    b.rect(9.f, 2.f, 2.f, 6.f, 1);
    b.rect(12.f, 3.f, 2.f, 5.f, 2);
    return b;
}

gs::Bitmap cursorArt() {
    gs::Bitmap b(12, 8);
    b.poly({{0.f, 0.f}, {12.f, 0.f}, {6.f, 8.f}}, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_BACK, {0, gs::rgb4(1, 2, 6), gs::rgb4(2, 4, 10), gs::rgb4(12, 10, 3)});
    setPal(vdp, PAL_FACE, {0, gs::rgb4(14, 13, 10), gs::rgb4(2, 2, 5), gs::rgb4(10, 4, 2)});
    setPal(vdp, PAL_STAMP, {0, gs::rgb4(14, 11, 2), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_HAND, {0, gs::rgb4(13, 8, 5), gs::rgb4(8, 4, 3)});
    setPal(vdp, PAL_CURSOR, {0, gs::rgb4(15, 14, 6)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 13, 11), gs::rgb4(3, 2, 4));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(5, 3, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4), gs::rgb4(4, 1, 1));
    textPal(vdp, PAL_OK, gs::rgb4(8, 15, 8), gs::rgb4(1, 4, 2));
    loadFont(vdp, art);
    art.back = gs::uploadImage(vdp, backArt());
    art.face[0] = gs::uploadImage(vdp, faceBar());
    art.face[1] = gs::uploadImage(vdp, faceRing());
    art.face[2] = gs::uploadImage(vdp, faceChev());
    art.face[3] = gs::uploadImage(vdp, faceCross());
    art.stamp = gs::uploadImage(vdp, stampArt());
    art.hand = gs::uploadImage(vdp, handArt());
    art.cursor = gs::uploadImage(vdp, cursorArt());
}

}  // namespace memorymark
