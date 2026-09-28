#include "game/art.h"

#include <string>

namespace lotp {
namespace {

using gs::Bitmap;

void blank(gs::VDP& v, int pal) {
    for (int i = 0; i < 16; ++i) v.setColor(pal * 16 + i, 0);
}

void ink(gs::VDP& v, int pal, int i, uint16_t c) { v.setColor(pal * 16 + i, c); }

void wheels(Bitmap& b, int y0, int y1) {
    b.rect(1, y0, 5, 8, 4);
    b.rect(22, y0, 5, 8, 4);
    b.rect(1, y1, 5, 8, 4);
    b.rect(22, y1, 5, 8, 4);
    b.rect(2, y0 + 2, 3, 4, 5);
    b.rect(23, y0 + 2, 3, 4, 5);
}

Bitmap sedanArt() {
    Bitmap b(28, 48);
    b.rect(6, 4, 16, 40, 1);
    b.rect(7, 6, 14, 8, 3);
    b.rect(8, 28, 12, 8, 3);
    b.rect(10, 2, 8, 3, 6);
    b.rect(9, 42, 4, 3, 6);
    b.rect(15, 42, 4, 3, 6);
    wheels(b, 8, 32);
    b.outline(8, false);
    return b;
}

Bitmap vanArt() {
    Bitmap b(32, 52);
    b.rect(5, 2, 22, 46, 1);
    b.rect(7, 6, 18, 14, 3);
    b.rect(8, 24, 16, 10, 2);
    b.rect(11, 44, 4, 4, 6);
    b.rect(17, 44, 4, 4, 6);
    b.rect(1, 10, 5, 10, 4);
    b.rect(26, 10, 5, 10, 4);
    b.rect(1, 32, 5, 10, 4);
    b.rect(26, 32, 5, 10, 4);
    b.outline(8, false);
    return b;
}

Bitmap truckArt() {
    Bitmap b(34, 56);
    b.rect(6, 2, 22, 20, 1);
    b.rect(8, 5, 18, 8, 3);
    b.rect(4, 22, 26, 30, 2);
    b.rect(8, 26, 18, 4, 6);
    b.rect(2, 8, 5, 10, 4);
    b.rect(27, 8, 5, 10, 4);
    b.rect(2, 36, 6, 12, 4);
    b.rect(26, 36, 6, 12, 4);
    b.outline(8, false);
    return b;
}

Bitmap wagonArt() {
    Bitmap b(30, 50);
    b.rect(5, 6, 20, 38, 1);
    b.rect(7, 10, 16, 10, 3);
    b.rect(7, 24, 16, 8, 3);
    b.rect(8, 38, 14, 4, 2);
    b.rect(9, 2, 5, 4, 6);
    b.rect(16, 2, 5, 4, 6);
    b.rect(1, 12, 5, 8, 4);
    b.rect(24, 12, 5, 8, 4);
    b.rect(1, 32, 5, 8, 4);
    b.rect(24, 32, 5, 8, 4);
    b.outline(8, false);
    return b;
}

Bitmap wreckArt() {
    Bitmap b(36, 28);
    b.rect(4, 6, 28, 16, 1);
    b.rect(8, 4, 10, 6, 2);
    b.rect(18, 16, 12, 8, 3);
    b.rect(2, 10, 6, 8, 4);
    b.rect(28, 8, 6, 8, 4);
    b.line(6, 8, 30, 20, 5, 2.f);
    b.outline(8, false);
    return b;
}

Bitmap stripeArt() {
    Bitmap b(4, 64);
    b.rect(1, 0, 2, 64, 1);
    return b;
}

Bitmap lampArt() {
    Bitmap b(10, 40);
    b.rect(4, 8, 2, 32, 2);
    b.ellipse(5, 6, 4, 4, 3);
    b.rect(2, 36, 6, 3, 4);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(9, 9);
    b.line(4, 0, 4, 8, 1, 1.f);
    b.line(0, 4, 8, 4, 1, 1.f);
    b.set(2, 2, 2);
    b.set(6, 2, 2);
    b.set(2, 6, 2);
    b.set(6, 6, 2);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(28, 10);
    b.ellipse(14, 5, 13, 4, 1);
    return b;
}

void paintCar(gs::VDP& v, int pal, uint16_t body, uint16_t shade, uint16_t glass) {
    blank(v, pal);
    ink(v, pal, 1, body);
    ink(v, pal, 2, shade);
    ink(v, pal, 3, glass);
    ink(v, pal, 4, gs::rgb4(1, 1, 1));
    ink(v, pal, 5, gs::rgb4(6, 6, 6));
    ink(v, pal, 6, gs::rgb4(15, 14, 8));
    ink(v, pal, 8, gs::rgb4(1, 1, 2));
    ink(v, pal, 15, gs::rgb4(1, 1, 1));
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y) {
            for (int x = 0; x < 5; ++x) {
                if (!g[y * 5 + x]) continue;
                px[y * 8 + x + 1] = 1;
                if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
            }
        }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    blank(vdp, PAL_HUD);
    ink(vdp, PAL_HUD, 1, gs::rgb4(14, 13, 11));
    ink(vdp, PAL_HUD, 15, gs::rgb4(1, 1, 2));

    paintCar(vdp, PAL_YOU, gs::rgb4(14, 10, 2), gs::rgb4(9, 6, 1), gs::rgb4(6, 10, 12));
    paintCar(vdp, PAL_RED, gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(4, 6, 8));
    paintCar(vdp, PAL_VAN, gs::rgb4(3, 6, 12), gs::rgb4(2, 3, 7), gs::rgb4(8, 12, 14));
    paintCar(vdp, PAL_TRUCK, gs::rgb4(3, 10, 4), gs::rgb4(2, 6, 2), gs::rgb4(7, 11, 10));
    paintCar(vdp, PAL_WAGON, gs::rgb4(13, 13, 12), gs::rgb4(8, 8, 8), gs::rgb4(5, 8, 10));
    paintCar(vdp, PAL_WRECK, gs::rgb4(4, 4, 4), gs::rgb4(3, 2, 2), gs::rgb4(6, 3, 2));

    blank(vdp, PAL_LOT);
    ink(vdp, PAL_LOT, 1, gs::rgb4(14, 14, 12));
    ink(vdp, PAL_LOT, 2, gs::rgb4(5, 5, 6));
    ink(vdp, PAL_LOT, 3, gs::rgb4(15, 13, 4));
    ink(vdp, PAL_LOT, 4, gs::rgb4(3, 3, 3));
    ink(vdp, PAL_LOT, 15, gs::rgb4(1, 1, 2));

    blank(vdp, PAL_FX);
    ink(vdp, PAL_FX, 1, gs::rgb4(15, 12, 4));
    ink(vdp, PAL_FX, 2, gs::rgb4(15, 6, 2));

    art.sedan = gs::uploadMipped(vdp, sedanArt());
    art.van = gs::uploadMipped(vdp, vanArt());
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.wagon = gs::uploadMipped(vdp, wagonArt());
    art.wreck = gs::uploadMipped(vdp, wreckArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    loadFont(vdp, art);
}

}  // namespace lotp
