#include "game/art.h"

#include <initializer_list>
#include <string>

namespace tablegold {
namespace {

constexpr float kL = 56.f;
constexpr float kR = 264.f;
constexpr float kT = 64.f;
constexpr float kB = 168.f;
constexpr float kML = 124.f;
constexpr float kMR = 196.f;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void inkPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t shadow) {
    setPal(vdp, pal, {0, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, ink, shadow});
}

gs::Bitmap mallet(bool you) {
    gs::Bitmap b(32, 32);
    b.ellipse(16, 16, 12, 12, 1);
    b.ellipse(16, 16, 9, 9, 2);
    b.ellipse(12, 12, 4, 3, 3);
    b.ellipse(16, 16, 3, 3, 4);
    if (you) b.poly({{16, 5}, {21, 12}, {11, 12}}, 5);
    else b.poly({{16, 27}, {21, 20}, {11, 20}}, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap puckArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 7, 7, 1);
    b.ellipse(9, 9, 5, 5, 2);
    b.ellipse(7, 6, 2, 2, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap paintTable() {
    gs::Bitmap b(gs::SCREEN_W, gs::SCREEN_H);
    b.rect(0, 0, gs::SCREEN_W, gs::SCREEN_H, 1);
    b.rect(36, 40, 248, 152, 2);
    b.rect(44, 48, 232, 136, 3);
    b.rect(kL, kT, kR - kL, kB - kT, 4);
    for (int y = int(kT); y < int(kB); y += 6)
        if ((y / 6) & 1) b.rect(kL, float(y), kR - kL, 2, 5);

    b.ellipse(160, (kT + kB) * 0.5f, 18, 18, 6);
    b.ellipse(160, (kT + kB) * 0.5f, 15, 15, 4);
    b.rect(kL, (kT + kB) * 0.5f - 1, 160 - 20 - kL, 2, 6);
    b.rect(180, (kT + kB) * 0.5f - 1, kR - 180, 2, 6);

    b.rect(kML, kT - 14, kMR - kML, 14, 7);
    b.rect(kML + 4, kT - 10, kMR - kML - 8, 8, 8);
    b.rect(kML, kB, kMR - kML, 14, 9);
    b.rect(kML + 4, kB + 3, kMR - kML - 8, 8, 10);

    b.rect(kML - 4, kT - 2, 5, 6, 11);
    b.rect(kMR - 1, kT - 2, 5, 6, 11);
    b.rect(kML - 4, kB - 4, 5, 6, 11);
    b.rect(kMR - 1, kB - 4, 5, 6, 11);

    b.ellipse(48, 48, 3, 3, 12);
    b.ellipse(272, 48, 3, 3, 12);
    b.ellipse(48, 184, 3, 3, 12);
    b.ellipse(272, 184, 3, 3, 12);

    gs::TextStyle brand{2, 13, 0, 0, 1};
    gs::Bitmap mark = gs::textBitmap("S3", brand);
    b.blit(mark, int(160 - mark.w * 0.5f), int((kT + kB) * 0.5f - mark.h * 0.5f));
    return b;
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    inkPal(vdp, PAL_INK, gs::rgb4(14, 15, 15), shadow);
    inkPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 2), shadow);
    inkPal(vdp, PAL_CREAM, gs::rgb4(15, 14, 10), shadow);
    inkPal(vdp, PAL_WORD, gs::rgb4(15, 15, 15), shadow);
    inkPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 9), shadow);
    inkPal(vdp, PAL_BAD, gs::rgb4(15, 4, 4), shadow);
    inkPal(vdp, PAL_SHADOW, gs::rgb4(0, 0, 0), shadow);

    setPal(vdp, PAL_YOU, {0, gs::rgb4(10, 6, 1), gs::rgb4(15, 11, 2), gs::rgb4(15, 15, 8), gs::rgb4(5, 3, 1),
                          gs::rgb4(15, 14, 6), gs::rgb4(2, 1, 0), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(6, 1, 2), gs::rgb4(13, 2, 3), gs::rgb4(15, 8, 7), gs::rgb4(3, 1, 1),
                           gs::rgb4(15, 12, 10), gs::rgb4(1, 0, 1), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_PUCK, {0, gs::rgb4(3, 3, 4), gs::rgb4(13, 14, 15), gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 2), 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, shadow});

    setPal(vdp, PAL_ICE,
           {0, gs::rgb4(1, 2, 5), gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(9, 13, 15), gs::rgb4(6, 11, 14),
            gs::rgb4(12, 3, 3), gs::rgb4(4, 8, 12), gs::rgb4(15, 12, 3), gs::rgb4(3, 6, 10), gs::rgb4(12, 8, 2),
            gs::rgb4(14, 15, 15), gs::rgb4(15, 13, 6), gs::rgb4(2, 4, 7), 0, 0});

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, paintTable(), PAL_ICE);

    art.malletYou = gs::uploadMipped(vdp, mallet(true));
    art.malletThem = gs::uploadMipped(vdp, mallet(false));
    art.puck = gs::uploadMipped(vdp, puckArt());

    gs::Bitmap sh(24, 10);
    sh.ellipse(12, 5, 10, 3, 1);
    art.shadow = gs::uploadImage(vdp, sh);
    gs::Bitmap blot(8, 8);
    blot.rect(0, 0, 8, 8, 1);
    art.blot = gs::uploadImage(vdp, blot);
}

}  // namespace tablegold
