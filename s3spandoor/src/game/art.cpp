#include "game/art.h"

#include <initializer_list>

namespace spandoor {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap gateArt() {
    Bitmap b(72, 128);
    b.rect(4, 4, 64, 120, 3);
    for (int i = 0; i < 4; i++) {
        b.rect(8.f, 8.f + i * 28, 56, 22, (i & 1) ? 2 : 1);
        b.rect(8.f, 8.f + i * 28, 56, 3, 4);
    }
    for (int y = 14; y < 118; y += 12) {
        b.ellipse(14, float(y), 2.2f, 2.2f, 5);
        b.ellipse(58, float(y), 2.2f, 2.2f, 5);
    }
    b.rect(30, 52, 12, 28, 6);
    b.ellipse(36, 66, 3.2f, 3.2f, 5);
    b.outline(7, false);
    return b;
}

Bitmap towerArt() {
    Bitmap b(36, 176);
    b.rect(6, 4, 24, 168, 2);
    b.rect(4, 2, 28, 8, 4);
    for (int y = 16; y < 160; y += 14) {
        b.rect(8, float(y), 20, 3, 1);
        b.rect(8, float(y + 6), 8, 3, 5);
        b.rect(20, float(y + 6), 8, 3, 5);
    }
    b.rect(14, 150, 8, 18, 6);
    b.outline(7, false);
    return b;
}

Bitmap deckArt() {
    Bitmap b(220, 28);
    b.rect(0, 4, 220, 20, 3);
    for (int i = 0; i < 11; i++) {
        b.rect(float(2 + i * 20), 6, 16, 14, (i & 1) ? 2 : 1);
        b.rect(float(2 + i * 20), 6, 16, 2, 4);
    }
    b.rect(0, 22, 220, 4, 6);
    return b;
}

Bitmap cableArt() {
    Bitmap b(90, 48);
    b.line(2, 4, 86, 42, 1, 2.4f);
    b.line(2, 8, 86, 46, 2, 1.2f);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 28);
    b.rect(6, 10, 4, 16, 3);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

Bitmap watchArt(bool lean) {
    Bitmap b(40, 64);
    b.ellipse(20, 10, 7, 7, 4);
    b.rect(14, 4, 12, 4, 5);
    b.rect(lean ? 10.f : 14.f, 18, 14, 22, 1);
    b.rect(lean ? 8.f : 12.f, 20, 14, 6, 2);
    b.rect(lean ? 6.f : 4.f, 22, 10, 5, 1);
    b.rect(lean ? 24.f : 26.f, 22, 10, 5, 1);
    b.rect(14, 40, 5, 16, 3);
    b.rect(22, 40, 5, 16, 3);
    b.rect(12, 54, 8, 4, 6);
    b.rect(22, 54, 8, 4, 6);
    b.outline(7, false);
    return b;
}

Bitmap wagonArt() {
    Bitmap b(78, 44);
    b.rect(8, 8, 58, 20, 1);
    b.rect(8, 8, 58, 4, 2);
    b.rect(14, 14, 12, 8, 3);
    b.rect(32, 14, 12, 8, 3);
    b.rect(50, 14, 10, 8, 3);
    b.ellipse(22, 32, 7, 7, 4);
    b.ellipse(56, 32, 7, 7, 4);
    b.ellipse(22, 32, 3, 3, 5);
    b.ellipse(56, 32, 3, 3, 5);
    b.outline(6, false);
    return b;
}

Bitmap pinArt() {
    Bitmap b(18, 36);
    b.rect(6, 4, 6, 28, 1);
    b.rect(2, 2, 14, 6, 2);
    b.rect(7, 8, 4, 20, 3);
    return b;
}

Bitmap streakArt() {
    Bitmap b(22, 4);
    b.rect(0, 1, 22, 2, 1);
    b.rect(0, 1, 6, 2, 2);
    return b;
}

Bitmap chipArt() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 10, 12), gs::rgb4(15, 13, 8), gs::rgb4(6, 12, 10),
                          gs::rgb4(12, 14, 15), shadow, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_STEEL, {0, gs::rgb4(9, 10, 12), gs::rgb4(6, 7, 9), gs::rgb4(4, 5, 7), gs::rgb4(13, 14, 15),
                            gs::rgb4(15, 12, 6), gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 5, 8), gs::rgb4(3, 8, 11), gs::rgb4(1, 3, 6), gs::rgb4(8, 12, 14),
                            gs::rgb4(4, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 8), gs::rgb4(4, 4, 6), gs::rgb4(12, 8, 5),
                           gs::rgb4(2, 2, 3), gs::rgb4(6, 5, 4), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 10, 3), gs::rgb4(5, 5, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, shadow});
    setPal(vdp, PAL_WAGON, {0, gs::rgb4(8, 4, 3), gs::rgb4(12, 7, 4), gs::rgb4(10, 12, 13), gs::rgb4(2, 2, 3),
                            gs::rgb4(14, 12, 8), gs::rgb4(3, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_CABLE, {0, gs::rgb4(11, 12, 13), gs::rgb4(6, 7, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WARN, {0, gs::rgb4(15, 10, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});

    art.gate = gs::uploadMipped(vdp, gateArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.deck = gs::uploadMipped(vdp, deckArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.watch = gs::uploadMipped(vdp, watchArt(false));
    art.watchLean = gs::uploadMipped(vdp, watchArt(true));
    art.wagon = gs::uploadMipped(vdp, wagonArt());
    art.pin = gs::uploadMipped(vdp, pinArt());
    art.streak = gs::uploadMipped(vdp, streakArt());
    art.chip = gs::uploadMipped(vdp, chipArt());
    loadFont(vdp, art);
}

}  // namespace spandoor
