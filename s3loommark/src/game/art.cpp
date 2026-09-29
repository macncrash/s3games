#include "game/art.h"

#include <cstdint>
#include <initializer_list>

namespace loommark {
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

bool speck(int x, int y, unsigned mask) {
    uint32_t h = uint32_t(x) * 2246822519u ^ uint32_t(y) * 3266489917u;
    h ^= h >> 16;
    return (h & mask) == 0;
}

gs::Bitmap frameArt() {
    gs::Bitmap b(204, 210);
    b.rect(14, 18, 18, 168, 2);
    b.rect(172, 18, 18, 168, 2);
    b.rect(14, 18, 6, 168, 3);
    b.rect(184, 18, 6, 168, 1);
    b.rect(10, 22, 184, 16, 2);
    b.rect(10, 22, 184, 4, 3);
    b.rect(10, 34, 184, 3, 1);
    b.rect(10, 158, 184, 18, 2);
    b.rect(10, 158, 184, 4, 3);
    b.rect(10, 172, 184, 4, 1);
    b.ellipse(102.f, 186.f, 78.f, 9.f, 1);
    b.ellipse(102.f, 184.f, 78.f, 5.f, 2);
    b.rect(6, 192, 34, 8, 1);
    b.rect(164, 192, 34, 8, 1);
    b.rect(8, 190, 30, 3, 3);
    b.rect(166, 190, 30, 3, 3);
    for (int y = 40; y < 156; y += 3)
        if (speck(4, y, 3u)) b.set(20, y, 4);
    for (int x = 28; x < 176; x++)
        if (speck(x, 9, 5u)) b.set(x, 28, 4);
    b.rect(96, 14, 12, 8, 5);
    b.rect(99, 8, 6, 8, 5);
    return b;
}

gs::Bitmap warpArt() {
    gs::Bitmap b(3, 120);
    b.rect(1, 0, 1, 120, 1);
    b.rect(0, 0, 1, 120, 2);
    for (int y = 0; y < 120; y += 4) b.set(2, y, 3);
    return b;
}

gs::Bitmap creamArt() {
    gs::Bitmap b(16, 6);
    b.rect(0, 0, 16, 6, 4);
    b.rect(0, 0, 16, 1, 5);
    b.rect(0, 5, 16, 1, 3);
    for (int x = 1; x < 16; x += 3) b.rect(float(x), 1, 1, 4, 6);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(10, 8);
    b.rect(0, 1, 10, 6, 7);
    b.rect(0, 1, 10, 2, 8);
    b.rect(0, 5, 10, 2, 9);
    b.rect(4, 2, 2, 4, 8);
    return b;
}

gs::Bitmap shuttleArt() {
    gs::Bitmap b(26, 10);
    b.ellipse(13.f, 5.f, 12.f, 4.2f, 1);
    b.ellipse(13.f, 4.4f, 9.f, 2.4f, 2);
    b.rect(6, 4, 14, 2, 3);
    b.ellipse(8.f, 5.f, 2.2f, 2.2f, 4);
    b.ellipse(18.f, 5.f, 2.2f, 2.2f, 4);
    b.rect(12, 3, 2, 4, 5);
    return b;
}

gs::Bitmap reedArt() {
    gs::Bitmap b(132, 10);
    b.rect(0, 0, 132, 3, 1);
    b.rect(0, 7, 132, 3, 1);
    for (int x = 2; x < 130; x += 4) b.rect(float(x), 1, 1, 8, 2);
    return b;
}

gs::Bitmap heddleArt() {
    gs::Bitmap b(8, 14);
    b.rect(3, 0, 2, 14, 1);
    b.rect(1, 5, 6, 4, 2);
    b.set(3, 6, 0);
    b.set(4, 6, 0);
    b.set(3, 7, 0);
    b.set(4, 7, 0);
    return b;
}

gs::Bitmap skeinArt() {
    gs::Bitmap b(18, 16);
    b.ellipse(9.f, 8.f, 8.f, 6.5f, 1);
    b.ellipse(9.f, 7.2f, 5.f, 3.4f, 2);
    b.line(3, 4, 15, 12, 3, 1.2f);
    b.line(4, 12, 14, 4, 3, 1.2f);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    b.set(0, 0, 0);
    b.set(3, 0, 0);
    b.set(0, 3, 0);
    b.set(3, 3, 0);
    return b;
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
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 4), gs::rgb4(6, 4, 2), gs::rgb4(10, 10, 11)});
    setPal(vdp, PAL_CLOTH,
           {0, gs::rgb4(12, 11, 8), gs::rgb4(15, 14, 11), gs::rgb4(8, 7, 5), gs::rgb4(13, 12, 9), gs::rgb4(15, 14, 12),
            gs::rgb4(9, 8, 6), gs::rgb4(11, 2, 2), gs::rgb4(15, 6, 5), gs::rgb4(8, 1, 1)});
    setPal(vdp, PAL_SHUTTLE,
           {0, gs::rgb4(5, 3, 1), gs::rgb4(10, 7, 3), gs::rgb4(14, 12, 8), gs::rgb4(9, 1, 2), gs::rgb4(14, 4, 4)});
    textPal(vdp, PAL_INK, gs::rgb4(15, 14, 12), gs::rgb4(2, 1, 1));
    textPal(vdp, PAL_TITLE, gs::rgb4(15, 12, 5), gs::rgb4(3, 2, 1));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 13, 6), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 6, 4), gs::rgb4(3, 0, 0));
    textPal(vdp, PAL_WIN, gs::rgb4(15, 14, 8), gs::rgb4(3, 2, 0));
    textPal(vdp, PAL_DIM, gs::rgb4(8, 7, 6), gs::rgb4(2, 1, 1));
    setPal(vdp, PAL_YARN, {0, gs::rgb4(12, 3, 3), gs::rgb4(15, 8, 6), gs::rgb4(8, 1, 2)});

    loadFont(vdp, art);
    art.frame = gs::uploadImage(vdp, frameArt());
    art.warp = gs::uploadImage(vdp, warpArt());
    art.cream = gs::uploadImage(vdp, creamArt());
    art.mark = gs::uploadImage(vdp, markArt());
    art.shuttle = gs::uploadImage(vdp, shuttleArt());
    art.reed = gs::uploadImage(vdp, reedArt());
    art.heddle = gs::uploadImage(vdp, heddleArt());
    art.skein = gs::uploadImage(vdp, skeinArt());
    art.dot = gs::uploadImage(vdp, dotArt());
    art.logo = gs::uploadImage(vdp, gs::textBitmap("LOOMMARK", {2, 1, 2, 0, 1}));
    art.fin = gs::uploadImage(vdp, gs::textBitmap("FINISHED", {2, 1, 2, 0, 1}));
}

}  // namespace loommark
