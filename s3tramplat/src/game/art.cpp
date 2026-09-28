#include "game/art.h"

#include <initializer_list>

namespace tramplat {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

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

gs::Bitmap tramArt() {
    gs::Bitmap b(128, 48);
    b.rect(2, 14, 124, 22, 1);
    b.rect(6, 8, 116, 8, 2);
    b.rect(4, 10, 8, 6, 3);
    b.rect(116, 10, 8, 6, 3);
    for (int i = 0; i < 5; i++) b.rect(14.f + i * 16.f, 16, 12, 10, 4);
    b.rect(54, 16, 18, 18, 5);
    b.rect(58, 18, 4, 14, 6);
    b.rect(66, 18, 2, 14, 6);
    b.rect(2, 34, 124, 4, 7);
    b.rect(48, 12, 32, 3, 8);
    b.ellipse(28, 40, 6, 4, 9);
    b.ellipse(100, 40, 6, 4, 9);
    return b;
}

gs::Bitmap bogieArt() {
    gs::Bitmap b(22, 16);
    b.rect(1, 2, 20, 4, 1);
    b.ellipse(6, 10, 5, 5, 2);
    b.ellipse(16, 10, 5, 5, 2);
    b.ellipse(6, 10, 2, 2, 3);
    b.ellipse(16, 10, 2, 2, 3);
    return b;
}

gs::Bitmap panArt() {
    gs::Bitmap b(28, 22);
    b.line(4, 20, 14, 4, 1, 1.5f);
    b.line(24, 20, 14, 4, 1, 1.5f);
    b.rect(6, 2, 16, 3, 2);
    b.rect(12, 18, 4, 4, 3);
    return b;
}

gs::Bitmap stopArt() {
    gs::Bitmap b(140, 36);
    b.rect(0, 8, 140, 16, 1);
    b.rect(0, 4, 140, 5, 2);
    b.rect(0, 24, 140, 12, 3);
    for (int i = 0; i < 9; i++) b.rect(4.f + i * 15.f, 10, 8, 10, (i & 1) ? 4 : 5);
    b.rect(0, 0, 3, 36, 6);
    b.rect(137, 0, 3, 36, 6);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(10, 24);
    b.rect(2, 0, 6, 24, 1);
    for (int y = 0; y < 24; y += 6) b.rect(3, float(y), 4, 3, 2);
    return b;
}

gs::Bitmap shelterArt() {
    gs::Bitmap b(64, 40);
    b.poly({{2, 12}, {32, 2}, {62, 12}}, 1);
    b.rect(4, 12, 56, 3, 2);
    b.rect(8, 15, 3, 22, 3);
    b.rect(53, 15, 3, 22, 3);
    b.rect(14, 18, 36, 12, 4);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 10, 10, 2);
    b.rect(13, 6, 2, 9, 3);
    b.rect(13, 13, 7, 2, 3);
    b.ellipse(14, 14, 2, 2, 4);
    return b;
}

gs::Bitmap poleArt() {
    gs::Bitmap b(10, 48);
    b.rect(4, 6, 2, 38, 1);
    b.ellipse(5, 5, 4, 4, 2);
    b.rect(1, 42, 8, 4, 3);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(80, 28);
    b.rect(2, 10, 76, 12, 1);
    b.rect(6, 6, 68, 6, 2);
    for (int i = 0; i < 4; i++) b.rect(10.f + i * 16.f, 12, 10, 6, 3);
    b.rect(2, 20, 76, 3, 4);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(14, 14, 13));
    textPal(vdp, PAL_CREAM, gs::rgb4(15, 13, 6));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 14, 7));
    loadFont(vdp, art);

    setPal(vdp, PAL_TRAM,
           {0, gs::rgb4(14, 12, 8), gs::rgb4(12, 3, 3), gs::rgb4(15, 14, 8), gs::rgb4(6, 10, 13),
            gs::rgb4(4, 5, 6), gs::rgb4(15, 12, 2), gs::rgb4(8, 2, 2), gs::rgb4(15, 15, 12),
            gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_STOP,
           {0, gs::rgb4(9, 9, 10), gs::rgb4(13, 12, 8), gs::rgb4(5, 5, 6), gs::rgb4(7, 9, 11),
            gs::rgb4(4, 6, 8), gs::rgb4(12, 10, 4)});
    setPal(vdp, PAL_WIRE, {0, gs::rgb4(4, 4, 5), gs::rgb4(11, 11, 12), gs::rgb4(6, 5, 4)});
    setPal(vdp, PAL_TOWN,
           {0, gs::rgb4(5, 6, 8), gs::rgb4(14, 12, 4), gs::rgb4(3, 3, 4), gs::rgb4(8, 11, 13)});
    setPal(vdp, PAL_CLOCK,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(14, 13, 10), gs::rgb4(12, 2, 2), gs::rgb4(15, 12, 3)});

    art.tram = gs::uploadMipped(vdp, tramArt());
    art.bogie = gs::uploadMipped(vdp, bogieArt());
    art.pan = gs::uploadMipped(vdp, panArt());
    art.stop = gs::uploadMipped(vdp, stopArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.shelter = gs::uploadMipped(vdp, shelterArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    vdp.setFogColor(gs::rgb4(6, 8, 11));
}

}  // namespace tramplat
