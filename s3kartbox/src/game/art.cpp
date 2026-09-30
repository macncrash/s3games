#include "game/art.h"

#include <string>

namespace kartbox {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1));
}

gs::Bitmap bodyArt() {
    gs::Bitmap b(96, 40);
    b.rect(10, 22, 70, 10, 1);
    b.rect(14, 18, 46, 6, 2);
    b.rect(48, 14, 16, 10, 3);
    b.rect(18, 12, 22, 8, 4);
    b.rect(22, 14, 12, 4, 5);
    b.rect(62, 16, 14, 8, 6);
    b.ellipse(78, 20, 6, 5, 6);
    b.rect(8, 24, 8, 6, 2);
    b.rect(70, 26, 18, 4, 7);
    b.line(16, 22, 78, 22, 8, 1.2f);
    b.outline(9, false);
    return b.cropToContent(1);
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 12, 12, 1);
    b.ellipse(13, 13, 8, 8, 2);
    b.ellipse(13, 13, 3, 3, 3);
    b.line(13, 3, 13, 23, 3, 1.2f);
    b.line(3, 13, 23, 13, 3, 1.2f);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 56);
    b.rect(3, 4, 4, 52, 1);
    b.rect(2, 0, 6, 8, 2);
    for (int y = 10; y < 52; y += 8) b.rect(3, y, 4, 4, 3);
    return b;
}

gs::Bitmap slabArt() {
    gs::Bitmap b(32, 14);
    for (int y = 0; y < 14; y++)
        for (int x = 0; x < 32; x++) {
            int c = 1 + ((x / 4 + y / 3) & 1);
            if (y == 0) c = 3;
            if (y > 11) c = 4;
            b.set(x, y, c);
        }
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(24, 8);
    for (int x = 0; x < 24; x++) {
        int c = ((x / 4) & 1) ? 1 : 2;
        for (int y = 0; y < 8; y++) b.set(x, y, c);
    }
    return b;
}

gs::Bitmap coneArt() {
    gs::Bitmap b(18, 28);
    b.poly({{9, 2}, {16, 24}, {2, 24}}, 1);
    b.rect(1, 23, 16, 4, 2);
    b.rect(6, 10, 6, 3, 3);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(44, 16);
    b.ellipse(14, 9, 10, 5, 1);
    b.ellipse(28, 8, 12, 6, 1);
    b.ellipse(22, 7, 7, 4, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 5, 7, 4, 1);
    b.ellipse(5, 5, 3, 2, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{2, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 7));
    setPal(vdp, PAL_KART,
           {0, gs::rgb4(13, 3, 2), gs::rgb4(15, 6, 3), gs::rgb4(8, 8, 9), gs::rgb4(4, 5, 7), gs::rgb4(12, 14, 15),
            gs::rgb4(3, 3, 3), gs::rgb4(10, 8, 4), gs::rgb4(15, 13, 6), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(2, 2, 2), gs::rgb4(6, 6, 7), gs::rgb4(13, 12, 8)});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(14, 12, 4), gs::rgb4(15, 8, 2), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_YARD, {0, gs::rgb4(7, 7, 6), gs::rgb4(9, 9, 8), gs::rgb4(12, 12, 10), gs::rgb4(4, 4, 3)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(14, 14, 15), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_CONE, {0, gs::rgb4(15, 7, 1), gs::rgb4(4, 4, 4), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_TAPE, {0, gs::rgb4(15, 14, 2), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(10, 9, 7), gs::rgb4(13, 12, 9)});

    art.body = gs::uploadMipped(vdp, bodyArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.slab = gs::uploadMipped(vdp, slabArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.cone = gs::uploadMipped(vdp, coneArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    loadFont(vdp, art);
}

}  // namespace kartbox
