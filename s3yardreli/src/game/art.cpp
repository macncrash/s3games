#include "game/art.h"

#include <cmath>
#include <string>

namespace yard {
namespace {

using gs::Bitmap;

void blank(gs::VDP& v, int pal) {
    for (int i = 0; i < 16; i++) v.setColor(pal * 16 + i, 0);
}

void ink(gs::VDP& v, int pal, int i, uint16_t c) { v.setColor(pal * 16 + i, c); }

Bitmap cabArt() {
    Bitmap b(46, 72);
    b.rect(4, 16, 38, 50, 4);
    b.rect(8, 8, 30, 12, 2);
    b.rect(10, 22, 26, 16, 9);
    b.rect(12, 24, 10, 6, 1);
    b.rect(14, 44, 16, 18, 3);
    b.rect(6, 62, 34, 6, 5);
    b.rect(20, 2, 3, 8, 1);
    b.rect(16, 48, 4, 6, 6);
    b.outline(8, false);
    return b;
}

Bitmap shackArt() {
    Bitmap b(52, 64);
    b.rect(6, 22, 40, 38, 4);
    b.poly({{4, 24}, {26, 6}, {48, 24}}, 2);
    b.rect(20, 38, 12, 22, 9);
    b.rect(10, 28, 10, 10, 5);
    b.rect(32, 28, 10, 10, 7);
    b.rect(34, 30, 6, 6, 1);
    b.rect(8, 56, 36, 4, 3);
    b.outline(8, false);
    return b;
}

Bitmap beamArt() {
    Bitmap b(280, 16);
    b.rect(0, 4, 280, 8, 4);
    b.rect(0, 4, 280, 2, 1);
    b.rect(0, 10, 280, 2, 5);
    for (int x = 0; x < 280; x += 14) {
        b.rect(x, 6, 7, 4, 6);
        b.rect(x + 7, 6, 7, 4, 7);
    }
    return b;
}

Bitmap magnetArt() {
    Bitmap b(40, 32);
    b.rect(8, 4, 24, 8, 3);
    b.rect(10, 6, 20, 4, 1);
    b.rect(4, 12, 8, 16, 6);
    b.rect(28, 12, 8, 16, 6);
    b.rect(6, 14, 4, 12, 7);
    b.rect(30, 14, 4, 12, 7);
    b.rect(12, 12, 16, 4, 2);
    b.ellipse(20, 5, 3, 3, 4);
    b.outline(8, false);
    return b;
}

Bitmap cableArt() {
    Bitmap b(4, 40);
    for (int y = 0; y < 40; y++) {
        int x = 1 + int(std::sin(y * 0.45f) > 0);
        b.set(x, y, 1);
        if (x + 1 < 4) b.set(x + 1, y, 2);
    }
    return b;
}

Bitmap bellArt() {
    Bitmap b(28, 32);
    b.rect(10, 1, 8, 3, 1);
    b.rect(13, 4, 2, 4, 2);
    b.poly({{6, 10}, {22, 10}, {25, 24}, {3, 24}}, 2);
    b.poly({{9, 12}, {19, 12}, {21, 22}, {7, 22}}, 1);
    b.rect(4, 24, 20, 3, 3);
    b.ellipse(14, 27, 2, 2, 4);
    b.outline(8, false);
    return b;
}

Bitmap handArt(int step) {
    Bitmap b(20, 28);
    b.ellipse(10, 7, 5, 5, 4);
    b.rect(6, 3, 8, 3, 5);
    b.rect(5, 12, 10, 10, 1);
    b.rect(6, 22, 3, 5, 3);
    b.rect(11, 22, 3, 5, 3);
    if (step) b.rect(14, 13, 4, 3, 7);
    else b.rect(2, 14, 4, 3, 7);
    b.outline(8, false);
    return b;
}

Bitmap dogArt(int step) {
    Bitmap b(36, 32);
    b.ellipse(18, 14, 12, 8, 1);
    b.ellipse(10, 8, 4, 5, 2);
    b.ellipse(26, 8, 4, 5, 2);
    b.ellipse(18, 16, 7, 6, 3);
    b.set(15, 15, 4);
    b.set(21, 15, 4);
    b.ellipse(18, 19, 2, 2, 2);
    int a = step ? 2 : 0;
    b.rect(8, 20, 4, 8, 2);
    b.rect(14, 20 + a, 4, 8 - a, 1);
    b.rect(20, 20 + (step ? 0 : 2), 4, 8, 2);
    b.rect(26, 20, 4, 8, 1);
    b.rect(12, 18, 12, 3, 6);
    b.outline(8, false);
    return b;
}

Bitmap scrapArt(int step) {
    Bitmap b(40, 64);
    b.ellipse(20, 12, 7, 7, 4);
    b.rect(13, 4, 14, 5, 6);
    b.set(17, 12, 8);
    b.set(23, 12, 8);
    b.rect(12, 18, 16, 20, 1);
    b.rect(14, 20, 12, 14, 2);
    b.rect(7, 20, 6, 14, 2);
    b.rect(27, 20, 6, 12, 1);
    b.line(31, 22, 37, 8, 5, 2.0f);
    b.rect(34, 4, 4, 6, 5);
    int a = step ? 3 : 0;
    b.rect(13, 38, 6, 18 - a, 3);
    b.rect(22, 38 + a, 6, 18 - a, 3);
    b.rect(12, 54, 8, 5, 8);
    b.rect(21, 54 - (step ? 2 : 0), 8, 5, 8);
    b.outline(8, false);
    return b;
}

Bitmap reliefArt(int step) {
    Bitmap b(28, 52);
    b.ellipse(14, 8, 6, 6, 4);
    b.rect(9, 3, 10, 4, 5);
    b.rect(8, 14, 12, 16, 1);
    b.rect(6, 16, 4, 10, 2);
    int a = step ? 2 : 0;
    b.rect(9, 30, 4, 14 - a, 3);
    b.rect(15, 30 + a, 4, 14 - a, 3);
    b.rect(8, 44, 6, 4, 8);
    b.rect(14, 44 - a, 6, 4, 8);
    b.line(18, 18, 24, 10, 7, 2.0f);
    b.ellipse(24, 8, 3, 3, 9);
    b.outline(8, false);
    return b;
}

Bitmap wreckArt() {
    Bitmap b(70, 40);
    b.poly({{10, 12}, {60, 12}, {66, 20}, {4, 20}}, 7);
    b.rect(8, 18, 54, 10, 1);
    b.rect(6, 24, 58, 6, 2);
    b.rect(14, 14, 14, 8, 4);
    b.rect(40, 14, 14, 8, 4);
    b.rect(32, 20, 6, 6, 3);
    b.ellipse(16, 30, 7, 7, 5);
    b.ellipse(54, 32, 6, 6, 5);
    b.ellipse(16, 30, 2, 2, 3);
    b.ellipse(54, 32, 2, 2, 3);
    b.rect(4, 16, 6, 4, 6);
    b.rect(60, 16, 6, 4, 6);
    b.rect(22, 8, 10, 5, 2);
    b.outline(8, false);
    return b;
}

Bitmap pileArt(int kind) {
    Bitmap b(48, 60);
    int top = kind ? 2 : 1;
    int mid = kind ? 3 : 2;
    int low = kind ? 1 : 3;
    b.rect(8, 8, 32, 14, top);
    b.rect(12, 10, 10, 6, 5);
    b.rect(4, 22, 40, 14, mid);
    b.rect(10, 24, 12, 6, 4);
    b.rect(6, 36, 36, 14, low);
    b.rect(28, 38, 10, 6, 5);
    b.ellipse(14, 52, 5, 5, 8);
    b.ellipse(34, 50, 5, 5, 8);
    b.outline(8, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(14, 44);
    b.rect(6, 12, 2, 30, 9);
    b.rect(3, 4, 8, 8, 7);
    b.rect(5, 6, 4, 4, 1);
    b.rect(4, 40, 6, 3, 3);
    b.outline(8, false);
    return b;
}

Bitmap drumArt() {
    Bitmap b(16, 22);
    b.ellipse(8, 5, 6, 3, 6);
    b.rect(2, 5, 12, 12, 6);
    b.ellipse(8, 17, 6, 3, 10);
    b.rect(2, 8, 12, 2, 1);
    b.rect(2, 13, 12, 2, 8);
    b.outline(8, false);
    return b;
}

Bitmap padArt() {
    Bitmap b(22, 10);
    b.ellipse(11, 5, 10, 4, 6);
    b.ellipse(11, 5, 4, 2, 7);
    return b;
}

Bitmap linkArt() {
    Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(16, 16);
    b.line(8, 1, 8, 14, 1, 1.2f);
    b.line(1, 8, 14, 8, 2, 1.2f);
    b.line(3, 3, 13, 13, 4, 1.0f);
    b.line(13, 3, 3, 13, 3, 1.0f);
    return b;
}

Bitmap puffArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 7, 5, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

Bitmap smokeArt() {
    Bitmap b(20, 16);
    b.ellipse(8, 10, 6, 4, 1);
    b.ellipse(13, 8, 5, 4, 2);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

Bitmap moonArt() {
    Bitmap b(18, 18);
    b.ellipse(8, 9, 7, 7, 4);
    b.ellipse(12, 7, 5, 5, 0);
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
    const uint16_t dark = gs::rgb4(1, 1, 2);
    blank(vdp, PAL_HUD);
    ink(vdp, PAL_HUD, 1, gs::rgb4(14, 13, 11));
    ink(vdp, PAL_HUD, 15, dark);

    blank(vdp, PAL_CRANE);
    ink(vdp, PAL_CRANE, 1, gs::rgb4(12, 13, 14));
    ink(vdp, PAL_CRANE, 2, gs::rgb4(7, 8, 9));
    ink(vdp, PAL_CRANE, 3, gs::rgb4(4, 5, 6));
    ink(vdp, PAL_CRANE, 4, gs::rgb4(10, 5, 2));
    ink(vdp, PAL_CRANE, 5, gs::rgb4(6, 3, 1));
    ink(vdp, PAL_CRANE, 6, gs::rgb4(14, 12, 2));
    ink(vdp, PAL_CRANE, 7, gs::rgb4(1, 1, 1));
    ink(vdp, PAL_CRANE, 8, dark);
    ink(vdp, PAL_CRANE, 9, gs::rgb4(6, 10, 12));

    blank(vdp, PAL_HAND);
    ink(vdp, PAL_HAND, 1, gs::rgb4(5, 7, 4));
    ink(vdp, PAL_HAND, 2, gs::rgb4(3, 4, 2));
    ink(vdp, PAL_HAND, 3, gs::rgb4(2, 2, 3));
    ink(vdp, PAL_HAND, 4, gs::rgb4(12, 8, 5));
    ink(vdp, PAL_HAND, 5, gs::rgb4(7, 5, 2));
    ink(vdp, PAL_HAND, 7, gs::rgb4(14, 12, 4));
    ink(vdp, PAL_HAND, 8, dark);
    ink(vdp, PAL_HAND, 9, gs::rgb4(15, 14, 8));

    blank(vdp, PAL_SCRAP);
    ink(vdp, PAL_SCRAP, 1, gs::rgb4(13, 4, 3));
    ink(vdp, PAL_SCRAP, 2, gs::rgb4(7, 2, 2));
    ink(vdp, PAL_SCRAP, 3, gs::rgb4(3, 3, 5));
    ink(vdp, PAL_SCRAP, 4, gs::rgb4(11, 8, 6));
    ink(vdp, PAL_SCRAP, 5, gs::rgb4(9, 9, 10));
    ink(vdp, PAL_SCRAP, 6, gs::rgb4(2, 2, 3));
    ink(vdp, PAL_SCRAP, 8, dark);
    ink(vdp, PAL_SCRAP, 15, dark);

    blank(vdp, PAL_DOG);
    ink(vdp, PAL_DOG, 1, gs::rgb4(6, 5, 4));
    ink(vdp, PAL_DOG, 2, gs::rgb4(3, 2, 2));
    ink(vdp, PAL_DOG, 3, gs::rgb4(9, 7, 5));
    ink(vdp, PAL_DOG, 4, gs::rgb4(14, 12, 3));
    ink(vdp, PAL_DOG, 6, gs::rgb4(12, 3, 2));
    ink(vdp, PAL_DOG, 8, dark);

    blank(vdp, PAL_WRECK);
    ink(vdp, PAL_WRECK, 1, gs::rgb4(9, 4, 2));
    ink(vdp, PAL_WRECK, 2, gs::rgb4(5, 2, 1));
    ink(vdp, PAL_WRECK, 3, gs::rgb4(9, 9, 10));
    ink(vdp, PAL_WRECK, 4, gs::rgb4(4, 6, 8));
    ink(vdp, PAL_WRECK, 5, gs::rgb4(2, 2, 2));
    ink(vdp, PAL_WRECK, 6, gs::rgb4(14, 13, 8));
    ink(vdp, PAL_WRECK, 7, gs::rgb4(12, 7, 3));
    ink(vdp, PAL_WRECK, 8, dark);

    blank(vdp, PAL_BELL);
    ink(vdp, PAL_BELL, 1, gs::rgb4(10, 14, 7));
    ink(vdp, PAL_BELL, 2, gs::rgb4(12, 9, 4));
    ink(vdp, PAL_BELL, 3, gs::rgb4(6, 4, 2));
    ink(vdp, PAL_BELL, 4, gs::rgb4(5, 5, 6));
    ink(vdp, PAL_BELL, 8, dark);
    ink(vdp, PAL_BELL, 15, dark);

    blank(vdp, PAL_FX);
    ink(vdp, PAL_FX, 1, gs::rgb4(15, 14, 8));
    ink(vdp, PAL_FX, 2, gs::rgb4(14, 7, 2));
    ink(vdp, PAL_FX, 3, gs::rgb4(10, 3, 1));
    ink(vdp, PAL_FX, 4, gs::rgb4(15, 15, 12));
    ink(vdp, PAL_FX, 8, dark);
    ink(vdp, PAL_FX, 15, dark);

    blank(vdp, PAL_PROP);
    ink(vdp, PAL_PROP, 1, gs::rgb4(5, 6, 8));
    ink(vdp, PAL_PROP, 2, gs::rgb4(8, 3, 2));
    ink(vdp, PAL_PROP, 3, gs::rgb4(3, 6, 3));
    ink(vdp, PAL_PROP, 4, gs::rgb4(8, 5, 3));
    ink(vdp, PAL_PROP, 5, gs::rgb4(8, 9, 10));
    ink(vdp, PAL_PROP, 6, gs::rgb4(10, 8, 3));
    ink(vdp, PAL_PROP, 7, gs::rgb4(15, 13, 6));
    ink(vdp, PAL_PROP, 8, dark);
    ink(vdp, PAL_PROP, 9, gs::rgb4(3, 3, 3));
    ink(vdp, PAL_PROP, 10, gs::rgb4(6, 5, 2));

    const uint16_t field[16] = {
        0,
        gs::rgb4(5, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(6, 5, 3),
        gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 1),
        gs::rgb4(6, 5, 4), gs::rgb4(4, 3, 3),
        gs::rgb4(8, 7, 5), gs::rgb4(3, 3, 2), gs::rgb4(7, 6, 3),
        gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1), gs::rgb4(4, 4, 5),
        gs::rgb4(9, 6, 2), gs::rgb4(10, 8, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    loadFont(vdp, art);
    art.cab = gs::uploadMipped(vdp, cabArt());
    art.shack = gs::uploadMipped(vdp, shackArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.magnet = gs::uploadMipped(vdp, magnetArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.hand[0] = gs::uploadMipped(vdp, handArt(0));
    art.hand[1] = gs::uploadMipped(vdp, handArt(1));
    art.dog[0] = gs::uploadMipped(vdp, dogArt(0));
    art.dog[1] = gs::uploadMipped(vdp, dogArt(1));
    art.scrap[0] = gs::uploadMipped(vdp, scrapArt(0));
    art.scrap[1] = gs::uploadMipped(vdp, scrapArt(1));
    art.relief[0] = gs::uploadMipped(vdp, reliefArt(0));
    art.relief[1] = gs::uploadMipped(vdp, reliefArt(1));
    art.wreck = gs::uploadMipped(vdp, wreckArt());
    art.pile[0] = gs::uploadMipped(vdp, pileArt(0));
    art.pile[1] = gs::uploadMipped(vdp, pileArt(1));
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.pad = gs::uploadMipped(vdp, padArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.smoke = gs::uploadMipped(vdp, smokeArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.moon = gs::uploadMipped(vdp, moonArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.setFogColor(gs::rgb4(4, 3, 2));
}

}  // namespace yard
