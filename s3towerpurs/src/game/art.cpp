#include "game/art.h"

#include <algorithm>
#include <string>

namespace tpurs {
namespace {

using gs::Bitmap;

void blank(gs::VDP& v, int pal) {
    for (int i = 0; i < 16; ++i) v.setColor(pal * 16 + i, 0);
}

void ink(gs::VDP& v, int pal, int i, uint16_t c) { v.setColor(pal * 16 + i, c); }

Bitmap cabArt() {
    Bitmap b(44, 36);
    b.rect(6, 10, 32, 16, 2);
    b.rect(10, 12, 14, 6, 4);
    b.rect(12, 13, 6, 3, 9);
    b.rect(4, 22, 36, 6, 3);
    b.rect(2, 18, 6, 10, 5);
    b.rect(36, 18, 6, 10, 5);
    b.rect(8, 26, 6, 6, 6);
    b.rect(30, 26, 6, 6, 6);
    b.rect(18, 4, 8, 8, 7);
    b.outline(8, false);
    return b;
}

Bitmap crawlerArt() {
    Bitmap b(48, 32);
    b.rect(8, 8, 32, 14, 2);
    b.rect(12, 10, 10, 5, 4);
    b.rect(4, 16, 40, 8, 3);
    b.rect(2, 14, 8, 12, 5);
    b.rect(38, 14, 8, 12, 5);
    b.rect(20, 2, 8, 8, 7);
    b.outline(8, false);
    return b;
}

Bitmap haulerArt() {
    Bitmap b(52, 34);
    b.rect(6, 8, 28, 14, 2);
    b.rect(32, 12, 14, 10, 3);
    b.rect(8, 10, 10, 5, 4);
    b.rect(4, 20, 44, 6, 5);
    b.ellipse(12, 28, 5, 5, 6);
    b.ellipse(40, 28, 5, 5, 6);
    b.outline(8, false);
    return b;
}

Bitmap scoutArt() {
    Bitmap b(36, 30);
    b.rect(8, 8, 20, 12, 2);
    b.rect(10, 10, 8, 4, 4);
    b.rect(4, 16, 28, 6, 3);
    b.ellipse(10, 24, 4, 4, 5);
    b.ellipse(26, 24, 4, 4, 5);
    b.rect(16, 2, 4, 7, 7);
    b.outline(8, false);
    return b;
}

Bitmap towerArt() {
    Bitmap b(40, 96);
    b.rect(10, 18, 20, 70, 2);
    b.rect(6, 10, 28, 12, 3);
    b.rect(14, 2, 12, 10, 4);
    b.rect(16, 0, 8, 4, 9);
    for (int y = 28; y < 80; y += 14) {
        b.rect(14, y, 5, 7, 5);
        b.rect(22, y, 5, 7, 5);
    }
    b.rect(4, 84, 32, 8, 3);
    b.outline(8, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 28);
    b.rect(7, 10, 2, 14, 2);
    b.ellipse(8, 7, 5, 5, 9);
    b.ellipse(8, 7, 2, 2, 1);
    b.rect(3, 22, 10, 3, 3);
    return b;
}

Bitmap puffArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 9, 6, 5, 2);
    b.ellipse(10, 7, 4, 3, 1);
    return b;
}

Bitmap sparkArt() {
    Bitmap b(14, 14);
    b.line(1, 7, 13, 7, 1, 2.f);
    b.line(7, 1, 7, 13, 1, 2.f);
    b.line(2, 2, 12, 12, 2, 1.5f);
    b.line(12, 2, 2, 12, 2, 1.5f);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
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

    textPal(PAL_HUD, gs::rgb4(13, 12, 10));

    textPal(PAL_YOU, gs::rgb4(14, 12, 3));
    ink(vdp, PAL_YOU, 2, gs::rgb4(9, 7, 2));
    ink(vdp, PAL_YOU, 3, gs::rgb4(5, 4, 2));
    ink(vdp, PAL_YOU, 4, gs::rgb4(6, 8, 10));
    ink(vdp, PAL_YOU, 5, gs::rgb4(3, 3, 3));
    ink(vdp, PAL_YOU, 6, gs::rgb4(2, 2, 2));
    ink(vdp, PAL_YOU, 7, gs::rgb4(8, 6, 3));
    ink(vdp, PAL_YOU, 9, gs::rgb4(15, 15, 12));

    textPal(PAL_RED, gs::rgb4(13, 4, 3));
    ink(vdp, PAL_RED, 2, gs::rgb4(8, 2, 2));
    ink(vdp, PAL_RED, 3, gs::rgb4(4, 2, 2));
    ink(vdp, PAL_RED, 4, gs::rgb4(10, 8, 5));
    ink(vdp, PAL_RED, 5, gs::rgb4(3, 2, 2));
    ink(vdp, PAL_RED, 6, gs::rgb4(2, 2, 2));
    ink(vdp, PAL_RED, 7, gs::rgb4(6, 5, 4));
    ink(vdp, PAL_RED, 9, gs::rgb4(15, 12, 6));

    textPal(PAL_TEAL, gs::rgb4(5, 11, 11));
    ink(vdp, PAL_TEAL, 2, gs::rgb4(3, 7, 8));
    ink(vdp, PAL_TEAL, 3, gs::rgb4(2, 4, 5));
    ink(vdp, PAL_TEAL, 4, gs::rgb4(10, 12, 11));
    ink(vdp, PAL_TEAL, 5, gs::rgb4(2, 3, 3));
    ink(vdp, PAL_TEAL, 6, gs::rgb4(1, 2, 2));
    ink(vdp, PAL_TEAL, 7, gs::rgb4(6, 6, 5));
    ink(vdp, PAL_TEAL, 9, gs::rgb4(14, 15, 12));

    textPal(PAL_STONE, gs::rgb4(9, 8, 7));
    ink(vdp, PAL_STONE, 2, gs::rgb4(6, 5, 5));
    ink(vdp, PAL_STONE, 3, gs::rgb4(4, 3, 3));
    ink(vdp, PAL_STONE, 4, gs::rgb4(8, 7, 5));
    ink(vdp, PAL_STONE, 5, gs::rgb4(3, 4, 6));
    ink(vdp, PAL_STONE, 9, gs::rgb4(15, 13, 6));

    textPal(PAL_LAMP, gs::rgb4(15, 13, 4));
    ink(vdp, PAL_LAMP, 2, gs::rgb4(8, 7, 5));
    ink(vdp, PAL_LAMP, 3, gs::rgb4(4, 3, 2));
    ink(vdp, PAL_LAMP, 9, gs::rgb4(15, 15, 12));

    textPal(PAL_FX, gs::rgb4(10, 10, 11));
    ink(vdp, PAL_FX, 2, gs::rgb4(6, 6, 7));

    blank(vdp, PAL_SPARK);
    for (int i = 1; i < 16; ++i) ink(vdp, PAL_SPARK, i, gs::rgb4(15, std::max(3, 14 - i), 2));
    ink(vdp, PAL_SPARK, 1, gs::rgb4(15, 15, 13));
    ink(vdp, PAL_SPARK, 15, dark);

    const uint16_t yard[16] = {
        0,
        gs::rgb4(3, 3, 2), gs::rgb4(2, 2, 2), gs::rgb4(4, 4, 3),
        gs::rgb4(5, 4, 3), gs::rgb4(2, 3, 2),
        gs::rgb4(4, 5, 3), gs::rgb4(3, 3, 2),
        gs::rgb4(6, 5, 4), gs::rgb4(2, 2, 1), gs::rgb4(5, 5, 4),
        gs::rgb4(3, 3, 2), gs::rgb4(4, 4, 3), gs::rgb4(5, 4, 3),
        gs::rgb4(7, 6, 3), gs::rgb4(6, 5, 4),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_YARD * 16 + i, yard[i]);

    loadFont(vdp, art);
    art.cab = gs::uploadMipped(vdp, cabArt());
    art.crawler = gs::uploadMipped(vdp, crawlerArt());
    art.hauler = gs::uploadMipped(vdp, haulerArt());
    art.scout = gs::uploadMipped(vdp, scoutArt());
    art.tower = gs::uploadMipped(vdp, towerArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(2, 2, 3));
}

}  // namespace tpurs
