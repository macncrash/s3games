#include "game/art.h"

#include <algorithm>
#include <string>

namespace apurs {
namespace {

using gs::Bitmap;

void blank(gs::VDP& v, int pal) {
    for (int i = 0; i < 16; ++i) v.setColor(pal * 16 + i, 0);
}

void ink(gs::VDP& v, int pal, int i, uint16_t c) { v.setColor(pal * 16 + i, c); }

Bitmap barrowArt() {
    Bitmap b(48, 58);
    b.rect(8, 22, 32, 16, 2);
    b.rect(10, 24, 28, 6, 4);
    b.rect(12, 26, 10, 3, 9);
    b.ellipse(14, 42, 7, 7, 5);
    b.ellipse(34, 42, 7, 7, 5);
    b.ellipse(14, 42, 3, 3, 6);
    b.ellipse(34, 42, 3, 3, 6);
    b.rect(6, 10, 6, 18, 3);
    b.rect(36, 10, 6, 18, 3);
    b.rect(6, 8, 36, 5, 7);
    b.rect(18, 14, 12, 8, 1);
    b.outline(8, false);
    return b;
}

Bitmap cartArt() {
    Bitmap b(40, 50);
    b.rect(6, 8, 28, 22, 2);
    b.rect(8, 10, 24, 8, 4);
    b.rect(10, 12, 8, 4, 9);
    b.rect(6, 28, 28, 8, 3);
    b.ellipse(12, 40, 6, 6, 5);
    b.ellipse(28, 40, 6, 6, 5);
    b.rect(18, 4, 4, 6, 7);
    b.outline(8, false);
    return b;
}

Bitmap milkArt() {
    Bitmap b(46, 52);
    b.rect(8, 14, 30, 18, 2);
    b.rect(10, 16, 26, 6, 1);
    b.rect(12, 18, 8, 3, 9);
    b.rect(6, 30, 34, 8, 3);
    b.rect(4, 8, 8, 10, 6);
    b.rect(34, 8, 8, 10, 6);
    b.ellipse(12, 42, 6, 6, 5);
    b.ellipse(34, 42, 6, 6, 5);
    b.outline(8, false);
    return b;
}

Bitmap vanArt() {
    Bitmap b(56, 48);
    b.rect(6, 10, 44, 22, 2);
    b.rect(10, 12, 16, 10, 4);
    b.rect(28, 12, 16, 10, 4);
    b.rect(12, 14, 6, 4, 9);
    b.rect(8, 30, 40, 8, 3);
    b.rect(2, 22, 6, 14, 5);
    b.rect(48, 22, 6, 14, 5);
    b.ellipse(14, 40, 6, 6, 6);
    b.ellipse(42, 40, 6, 6, 6);
    b.rect(24, 4, 8, 7, 7);
    b.outline(8, false);
    return b;
}

Bitmap doorArt() {
    Bitmap b(36, 56);
    b.rect(2, 4, 32, 50, 3);
    b.rect(6, 8, 24, 42, 2);
    b.rect(8, 12, 8, 12, 4);
    b.rect(20, 12, 8, 12, 4);
    b.rect(8, 28, 8, 16, 5);
    b.rect(20, 28, 8, 16, 5);
    b.ellipse(26, 30, 2, 2, 9);
    b.outline(8, false);
    return b;
}

Bitmap brickArt() {
    Bitmap b(28, 48);
    b.rect(0, 0, 28, 48, 2);
    for (int y = 2; y < 46; y += 8) {
        int off = ((y / 8) & 1) ? 6 : 0;
        for (int x = -10 + off; x < 28; x += 12) b.rect(x, y, 10, 6, 3);
    }
    b.rect(0, 44, 28, 4, 5);
    return b;
}

Bitmap lampArt() {
    Bitmap b(14, 40);
    b.rect(6, 12, 3, 24, 2);
    b.ellipse(7, 8, 5, 5, 9);
    b.ellipse(7, 8, 2, 2, 1);
    b.rect(2, 34, 10, 3, 3);
    b.outline(8, false);
    return b;
}

Bitmap binArt() {
    Bitmap b(22, 28);
    b.rect(3, 6, 16, 18, 2);
    b.rect(5, 8, 12, 4, 4);
    b.rect(2, 4, 18, 3, 3);
    b.outline(8, false);
    return b;
}

Bitmap pipeArt() {
    Bitmap b(12, 36);
    b.rect(3, 2, 6, 30, 2);
    b.rect(1, 28, 10, 5, 3);
    b.outline(8, false);
    return b;
}

Bitmap chevArt() {
    Bitmap b(28, 10);
    b.rect(0, 3, 28, 4, 1);
    b.rect(0, 0, 4, 10, 1);
    b.rect(24, 0, 4, 10, 1);
    return b;
}

Bitmap puffArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 6, 5, 2);
    b.ellipse(10, 7, 4, 3, 1);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(12, 12);
    b.line(1, 6, 11, 6, 1, 2.f);
    b.line(6, 1, 6, 11, 1, 2.f);
    b.line(2, 2, 10, 10, 2, 1.5f);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

Bitmap dripArt() {
    Bitmap b(6, 10);
    b.ellipse(3, 6, 2, 3, 1);
    b.set(3, 1, 1);
    b.set(3, 2, 1);
    return b;
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
    const uint16_t dark = gs::rgb4(1, 1, 2);
    auto textPal = [&](int pal, uint16_t inkC) {
        blank(vdp, pal);
        ink(vdp, pal, 1, inkC);
        ink(vdp, pal, 15, dark);
        ink(vdp, pal, 8, dark);
    };

    textPal(PAL_HUD, gs::rgb4(13, 12, 11));

    textPal(PAL_YOU, gs::rgb4(14, 12, 3));
    ink(vdp, PAL_YOU, 2, gs::rgb4(10, 7, 2));
    ink(vdp, PAL_YOU, 3, gs::rgb4(6, 4, 2));
    ink(vdp, PAL_YOU, 4, gs::rgb4(4, 7, 9));
    ink(vdp, PAL_YOU, 5, gs::rgb4(2, 2, 2));
    ink(vdp, PAL_YOU, 6, gs::rgb4(8, 8, 7));
    ink(vdp, PAL_YOU, 7, gs::rgb4(5, 4, 3));
    ink(vdp, PAL_YOU, 9, gs::rgb4(15, 15, 12));

    textPal(PAL_CART, gs::rgb4(12, 8, 4));
    ink(vdp, PAL_CART, 2, gs::rgb4(8, 5, 3));
    ink(vdp, PAL_CART, 3, gs::rgb4(4, 3, 2));
    ink(vdp, PAL_CART, 4, gs::rgb4(11, 11, 9));
    ink(vdp, PAL_CART, 5, gs::rgb4(2, 2, 2));
    ink(vdp, PAL_CART, 6, gs::rgb4(6, 6, 6));
    ink(vdp, PAL_CART, 7, gs::rgb4(3, 3, 3));
    ink(vdp, PAL_CART, 9, gs::rgb4(15, 14, 8));

    textPal(PAL_FLOAT, gs::rgb4(12, 14, 15));
    ink(vdp, PAL_FLOAT, 2, gs::rgb4(6, 8, 10));
    ink(vdp, PAL_FLOAT, 3, gs::rgb4(3, 4, 6));
    ink(vdp, PAL_FLOAT, 4, gs::rgb4(9, 12, 13));
    ink(vdp, PAL_FLOAT, 5, gs::rgb4(2, 2, 3));
    ink(vdp, PAL_FLOAT, 6, gs::rgb4(14, 14, 12));
    ink(vdp, PAL_FLOAT, 9, gs::rgb4(15, 15, 14));

    textPal(PAL_VAN, gs::rgb4(12, 4, 3));
    ink(vdp, PAL_VAN, 2, gs::rgb4(7, 2, 2));
    ink(vdp, PAL_VAN, 3, gs::rgb4(4, 2, 2));
    ink(vdp, PAL_VAN, 4, gs::rgb4(5, 7, 9));
    ink(vdp, PAL_VAN, 5, gs::rgb4(2, 2, 2));
    ink(vdp, PAL_VAN, 6, gs::rgb4(8, 8, 8));
    ink(vdp, PAL_VAN, 7, gs::rgb4(5, 5, 5));
    ink(vdp, PAL_VAN, 9, gs::rgb4(15, 13, 6));

    textPal(PAL_DOOR, gs::rgb4(11, 8, 4));
    ink(vdp, PAL_DOOR, 2, gs::rgb4(6, 4, 2));
    ink(vdp, PAL_DOOR, 3, gs::rgb4(3, 2, 2));
    ink(vdp, PAL_DOOR, 4, gs::rgb4(12, 10, 5));
    ink(vdp, PAL_DOOR, 5, gs::rgb4(2, 2, 1));
    ink(vdp, PAL_DOOR, 9, gs::rgb4(15, 12, 4));

    textPal(PAL_BRICK, gs::rgb4(9, 5, 4));
    ink(vdp, PAL_BRICK, 2, gs::rgb4(7, 3, 3));
    ink(vdp, PAL_BRICK, 3, gs::rgb4(5, 3, 3));
    ink(vdp, PAL_BRICK, 4, gs::rgb4(4, 4, 4));
    ink(vdp, PAL_BRICK, 5, gs::rgb4(2, 2, 2));
    ink(vdp, PAL_BRICK, 9, gs::rgb4(14, 11, 4));

    textPal(PAL_FX, gs::rgb4(11, 11, 12));
    ink(vdp, PAL_FX, 2, gs::rgb4(7, 7, 8));
    ink(vdp, PAL_FX, 3, gs::rgb4(4, 4, 5));

    textPal(PAL_LAMP, gs::rgb4(15, 12, 3));
    ink(vdp, PAL_LAMP, 2, gs::rgb4(15, 15, 10));
    ink(vdp, PAL_LAMP, 9, gs::rgb4(15, 14, 6));

    blank(vdp, PAL_SHOCK);
    for (int i = 1; i < 16; ++i) ink(vdp, PAL_SHOCK, i, gs::rgb4(15, std::max(4, 15 - i / 2), 3));
    ink(vdp, PAL_SHOCK, 1, gs::rgb4(15, 15, 14));
    ink(vdp, PAL_SHOCK, 15, dark);

    const uint16_t road[16] = {
        0,
        gs::rgb4(3, 3, 3), gs::rgb4(2, 2, 2), gs::rgb4(4, 4, 4),
        gs::rgb4(5, 4, 3), gs::rgb4(2, 2, 3),
        gs::rgb4(4, 4, 5), gs::rgb4(3, 3, 4),
        gs::rgb4(6, 6, 6), gs::rgb4(2, 2, 2), gs::rgb4(5, 5, 6),
        gs::rgb4(2, 3, 4), gs::rgb4(3, 4, 5), gs::rgb4(4, 5, 6),
        gs::rgb4(8, 7, 4), gs::rgb4(6, 6, 5),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.barrow = gs::uploadMipped(vdp, barrowArt());
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.milk = gs::uploadMipped(vdp, milkArt());
    art.van = gs::uploadMipped(vdp, vanArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.brick = gs::uploadMipped(vdp, brickArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.bin = gs::uploadMipped(vdp, binArt());
    art.pipe = gs::uploadMipped(vdp, pipeArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.drip = gs::uploadMipped(vdp, dripArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(2, 2, 3));
}

}  // namespace apurs
