#include "game/art.h"

#include <initializer_list>
#include <string>

namespace alley {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap wallArt() {
    Bitmap b(40, 200);
    b.rect(0, 0, 40, 200, 2);
    b.rect(4, 0, 32, 200, 1);
    for (int row = 0; row < 16; row++) {
        int y = 4 + row * 12;
        int shift = (row & 1) ? 8 : 0;
        for (int x = -8 + shift; x < 36; x += 16) b.rect(x, y, 14, 8, 3);
        b.rect(2, y + 9, 36, 2, 4);
    }
    for (int y = 28; y < 170; y += 48) {
        b.rect(10, y, 16, 22, 6);
        b.rect(12, y + 2, 12, 10, 5);
        b.rect(12, y + 14, 12, 6, 7);
    }
    b.rect(8, 168, 22, 28, 8);
    b.outline(8, false);
    return b;
}

Bitmap escapeArt() {
    Bitmap b(36, 160);
    for (int i = 0; i < 5; i++) {
        int y = 8 + i * 30;
        b.rect(2, y, 32, 3, 1);
        b.rect(4, y + 6, 28, 2, 2);
        b.rect(4, y + 12, 28, 2, 2);
        b.rect(6, y, 2, 22, 3);
        b.rect(28, y, 2, 22, 3);
    }
    b.rect(14, 0, 4, 160, 3);
    b.outline(8, false);
    return b;
}

Bitmap dumpsterArt() {
    Bitmap b(48, 36);
    b.poly({{4, 10}, {44, 10}, {48, 32}, {0, 32}}, 2);
    b.rect(6, 14, 36, 14, 1);
    b.rect(8, 6, 32, 6, 3);
    b.rect(18, 16, 12, 8, 4);
    b.rect(4, 30, 8, 4, 5);
    b.rect(36, 30, 8, 4, 5);
    b.outline(8, false);
    return b;
}

Bitmap watchArt(bool swing) {
    Bitmap b(40, 78);
    b.ellipse(20, 12, 8, 9, 4);
    b.rect(14, 4, 12, 4, 1);
    b.rect(16, 8, 8, 3, 5);
    b.rect(12, 20, 16, 22, 2);
    b.rect(14, 22, 12, 8, 3);
    b.rect(10, 24, 4, 12, 1);
    b.rect(26, 24, 4, 12, 1);
    b.rect(14, 42, 5, 22, 2);
    b.rect(22, 42, 5, 22, 2);
    b.rect(13, 62, 7, 5, 6);
    b.rect(21, 62, 7, 5, 6);
    if (!swing) {
        b.rect(30, 28, 4, 18, 7);
        b.ellipse(32, 48, 5, 6, 5);
        b.rect(31, 46, 2, 3, 3);
    } else {
        b.rect(6, 18, 28, 3, 7);
        b.ellipse(34, 18, 4, 4, 6);
    }
    b.outline(8, false);
    return b;
}

Bitmap hoodArt(int frame) {
    Bitmap b(32, 64);
    b.poly({{16, 2}, {6, 18}, {26, 18}}, 1);
    b.ellipse(16, 20, 7, 6, 3);
    b.set(13, 19, 8);
    b.set(19, 19, 8);
    b.rect(10, 26, 12, 16, 2);
    int step = frame ? 3 : 0;
    b.rect(11, 42, 4, 16 - step, 1);
    b.rect(18, 42 + step, 4, 16 - step, 1);
    b.rect(10, 56, 6, 4, 6);
    b.rect(17, 56 - step, 6, 4, 6);
    b.line(24, 30, 30, 16 + step, 5, 2.0f);
    b.outline(8, false);
    return b;
}

Bitmap bruiserArt() {
    Bitmap b(44, 72);
    b.ellipse(22, 14, 9, 8, 3);
    b.rect(12, 22, 20, 22, 1);
    b.rect(8, 24, 6, 16, 2);
    b.rect(30, 20, 4, 28, 5);
    b.rect(14, 44, 6, 20, 2);
    b.rect(24, 44, 6, 20, 2);
    b.rect(13, 62, 8, 5, 6);
    b.rect(23, 62, 8, 5, 6);
    b.outline(8, false);
    return b;
}

Bitmap barrelArt() {
    Bitmap b(56, 48);
    b.ellipse(28, 22, 20, 14, 1);
    b.ellipse(28, 22, 12, 8, 2);
    b.rect(10, 18, 36, 4, 3);
    b.ellipse(14, 38, 6, 6, 4);
    b.ellipse(42, 38, 6, 6, 4);
    b.rect(8, 36, 12, 6, 5);
    b.rect(36, 36, 12, 6, 5);
    b.outline(8, false);
    return b;
}

Bitmap stickArt() {
    Bitmap b(28, 8);
    b.rect(0, 3, 22, 2, 1);
    b.ellipse(24, 4, 3, 3, 2);
    return b;
}

Bitmap lanternArt() {
    Bitmap b(12, 16);
    b.rect(4, 0, 4, 3, 1);
    b.poly({{2, 4}, {10, 4}, {9, 14}, {3, 14}}, 2);
    b.rect(5, 6, 2, 5, 3);
    return b;
}

Bitmap lampArt(int frame) {
    Bitmap b(16, 20);
    b.ellipse(8, 12, 6, 6, frame ? 1 : 2);
    b.ellipse(8, 12, 3, 3, 3);
    b.rect(7, 0, 2, 6, 4);
    return b;
}

Bitmap poleArt() {
    Bitmap b(8, 48);
    b.rect(3, 0, 2, 48, 1);
    b.rect(1, 4, 6, 3, 2);
    return b;
}

Bitmap bellArt() {
    Bitmap b(28, 24);
    b.rect(12, 0, 4, 4, 1);
    b.poly({{4, 6}, {24, 6}, {22, 18}, {6, 18}}, 2);
    b.ellipse(14, 18, 8, 4, 3);
    b.ellipse(14, 16, 3, 3, 4);
    b.outline(8, false);
    return b;
}

Bitmap cordArt() {
    Bitmap b(6, 70);
    b.rect(2, 0, 2, 70, 1);
    return b;
}

Bitmap signArt() {
    Bitmap b(64, 18);
    b.rect(0, 2, 64, 14, 1);
    b.rect(2, 4, 60, 10, 2);
    const char* word = "ALLEY";
    gs::TextStyle st{1, 3, 0, 0, 1};
    Bitmap letters = gs::textBitmap(word, st);
    b.blit(letters, 8, 5);
    b.outline(8, false);
    return b;
}

Bitmap steamArt() {
    Bitmap b(20, 16);
    b.ellipse(6, 10, 5, 4, 1);
    b.ellipse(13, 6, 4, 3, 2);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(36, 10);
    b.ellipse(18, 5, 16, 4, 1);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
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
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(14, 14, 15);
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_HUD, {0, ink, gs::rgb4(5, 6, 8), gs::rgb4(12, 8, 14), gs::rgb4(14, 5, 4), gs::rgb4(6, 13, 10),
                          gs::rgb4(8, 12, 14), gs::rgb4(3, 3, 5), 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BRICK, {0, gs::rgb4(9, 4, 3), gs::rgb4(6, 3, 3), gs::rgb4(4, 2, 2), gs::rgb4(3, 2, 2),
                            gs::rgb4(12, 10, 6), gs::rgb4(2, 3, 5), gs::rgb4(14, 12, 8), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WATCH, {0, gs::rgb4(3, 4, 6), gs::rgb4(2, 5, 8), gs::rgb4(8, 10, 12), gs::rgb4(12, 9, 7),
                            gs::rgb4(15, 13, 6), gs::rgb4(2, 2, 3), gs::rgb4(10, 8, 4), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_HOOD, {0, gs::rgb4(2, 2, 3), gs::rgb4(5, 5, 7), gs::rgb4(12, 9, 7), gs::rgb4(8, 3, 3),
                           gs::rgb4(12, 12, 13), gs::rgb4(4, 3, 3), gs::rgb4(9, 8, 6), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(10, 8, 4), gs::rgb4(3, 3, 4),
                           gs::rgb4(12, 9, 5), gs::rgb4(6, 4, 2), gs::rgb4(2, 2, 2), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_NEON, {0, gs::rgb4(4, 2, 6), gs::rgb4(14, 4, 12), gs::rgb4(15, 14, 8), gs::rgb4(6, 6, 8),
                           gs::rgb4(8, 14, 14), gs::rgb4(12, 6, 3), gs::rgb4(10, 10, 12), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(6, 5, 4), gs::rgb4(12, 10, 5), gs::rgb4(14, 12, 7), gs::rgb4(8, 14, 8),
                           gs::rgb4(4, 8, 6), gs::rgb4(10, 8, 4), gs::rgb4(15, 15, 12), shadow, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(4, 5, 7), gs::rgb4(8, 9, 12), gs::rgb4(14, 14, 15), gs::rgb4(10, 6, 3),
                            gs::rgb4(3, 8, 10), gs::rgb4(6, 3, 8), gs::rgb4(2, 2, 4), shadow, 0, 0, 0, 0, 0, 0, shadow});

    const uint16_t field[16] = {
        0,
        gs::rgb4(2, 2, 3), gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 4),
        gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 2),
        gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4),
        gs::rgb4(5, 4, 6), gs::rgb4(8, 3, 8), gs::rgb4(3, 6, 7),
        gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 3), gs::rgb4(7, 7, 8),
        gs::rgb4(12, 10, 4), gs::rgb4(6, 6, 7),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    loadFont(vdp, art);
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.escape = gs::uploadMipped(vdp, escapeArt());
    art.dumpster = gs::uploadMipped(vdp, dumpsterArt());
    art.watch[0] = gs::uploadMipped(vdp, watchArt(false));
    art.watch[1] = gs::uploadMipped(vdp, watchArt(true));
    art.hood[0] = gs::uploadMipped(vdp, hoodArt(0));
    art.hood[1] = gs::uploadMipped(vdp, hoodArt(1));
    art.bruiser = gs::uploadMipped(vdp, bruiserArt());
    art.barrel = gs::uploadMipped(vdp, barrelArt());
    art.stick = gs::uploadMipped(vdp, stickArt());
    art.lantern = gs::uploadMipped(vdp, lanternArt());
    art.lamp[0] = gs::uploadMipped(vdp, lampArt(0));
    art.lamp[1] = gs::uploadMipped(vdp, lampArt(1));
    art.pole = gs::uploadMipped(vdp, poleArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.cord = gs::uploadMipped(vdp, cordArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.steam = gs::uploadMipped(vdp, steamArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(1, 1, 3));
}

}  // namespace alley
