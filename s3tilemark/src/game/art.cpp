#include "game/art.h"

#include <initializer_list>

namespace tilemark {
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

gs::Bitmap plateArt() {
    gs::Bitmap b(36, 40);
    b.rect(0, 0, 36, 40, 1);
    b.rect(2, 2, 32, 36, 2);
    b.rect(4, 4, 28, 32, 3);
    return b;
}

gs::Bitmap faceBar() {
    gs::Bitmap b(28, 32);
    b.rect(12, 2, 4, 28, 4);
    b.rect(6, 4, 16, 4, 5);
    b.rect(6, 24, 16, 4, 5);
    return b;
}

gs::Bitmap faceRing() {
    gs::Bitmap b(28, 32);
    b.ellipse(14.f, 16.f, 11.f, 12.f, 4);
    b.ellipse(14.f, 16.f, 5.f, 6.f, 3);
    return b;
}

gs::Bitmap faceDiamond() {
    gs::Bitmap b(28, 32);
    b.poly({{14.f, 2.f}, {26.f, 16.f}, {14.f, 30.f}, {2.f, 16.f}}, 5);
    b.poly({{14.f, 8.f}, {20.f, 16.f}, {14.f, 24.f}, {8.f, 16.f}}, 4);
    return b;
}

gs::Bitmap faceCross() {
    gs::Bitmap b(28, 32);
    b.rect(12, 2, 4, 28, 4);
    b.rect(2, 14, 24, 4, 4);
    b.rect(12, 14, 4, 4, 5);
    return b;
}

gs::Bitmap stampArt() {
    gs::Bitmap b(26, 26);
    b.ellipse(13.f, 13.f, 12.f, 12.f, 1);
    b.ellipse(13.f, 13.f, 7.f, 7.f, 2);
    b.rect(11, 5, 4, 16, 3);
    b.rect(5, 11, 16, 4, 3);
    return b;
}

gs::Bitmap cursorArt() {
    gs::Bitmap b(14, 8);
    b.poly({{0.f, 8.f}, {7.f, 0.f}, {14.f, 8.f}}, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TILE, {0, gs::rgb4(6, 5, 4), gs::rgb4(10, 8, 6), gs::rgb4(14, 12, 9), gs::rgb4(3, 6, 10),
                           gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(4, 3, 3), gs::rgb4(8, 6, 5), gs::rgb4(12, 10, 8), gs::rgb4(2, 3, 6),
                           gs::rgb4(9, 6, 2)});
    setPal(vdp, PAL_STAMP, {0, gs::rgb4(14, 11, 2), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 8)});
    setPal(vdp, PAL_CURSOR, {0, gs::rgb4(15, 14, 6)});
    textPal(vdp, PAL_INK, gs::rgb4(14, 13, 11), gs::rgb4(3, 2, 4));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 4), gs::rgb4(5, 3, 1));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 5, 4), gs::rgb4(4, 1, 1));
    textPal(vdp, PAL_OK, gs::rgb4(8, 15, 8), gs::rgb4(1, 4, 2));
    loadFont(vdp, art);
    art.plate = gs::uploadImage(vdp, plateArt());
    art.face[0] = gs::uploadImage(vdp, faceBar());
    art.face[1] = gs::uploadImage(vdp, faceRing());
    art.face[2] = gs::uploadImage(vdp, faceDiamond());
    art.face[3] = gs::uploadImage(vdp, faceCross());
    art.stamp = gs::uploadImage(vdp, stampArt());
    art.cursor = gs::uploadImage(vdp, cursorArt());
}

}  // namespace tilemark
