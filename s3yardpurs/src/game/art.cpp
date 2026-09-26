#include "game/art.h"

#include <algorithm>
#include <string>

namespace ypurs {
namespace {

using gs::Bitmap;

void blank(gs::VDP& v, int pal) {
    for (int i = 0; i < 16; ++i) v.setColor(pal * 16 + i, 0);
}

void ink(gs::VDP& v, int pal, int i, uint16_t c) { v.setColor(pal * 16 + i, c); }

Bitmap loaderArt() {
    Bitmap b(52, 70);
    b.ellipse(12, 50, 10, 14, 5);
    b.ellipse(40, 50, 10, 14, 5);
    b.ellipse(12, 50, 4, 6, 6);
    b.ellipse(40, 50, 4, 6, 6);
    b.rect(14, 36, 24, 22, 2);
    b.rect(16, 52, 20, 8, 3);
    b.rect(16, 18, 20, 22, 2);
    b.rect(18, 20, 16, 12, 4);
    b.rect(20, 22, 6, 4, 9);
    b.rect(11, 14, 5, 24, 7);
    b.rect(11, 12, 5, 4, 1);
    b.line(16, 34, 8, 16, 3, 3.f);
    b.line(36, 34, 44, 16, 3, 3.f);
    b.rect(4, 10, 44, 8, 10);
    b.rect(6, 12, 40, 3, 2);
    b.rect(8, 8, 6, 4, 10);
    b.rect(38, 8, 6, 4, 10);
    b.rect(16, 56, 5, 3, 1);
    b.rect(31, 56, 5, 3, 1);
    b.outline(8, false);
    return b;
}

Bitmap muleArt() {
    Bitmap b(44, 56);
    b.rect(7, 4, 30, 20, 4);
    b.rect(9, 6, 26, 5, 9);
    b.rect(9, 18, 26, 16, 2);
    b.rect(12, 20, 20, 8, 6);
    b.rect(14, 21, 6, 3, 1);
    b.rect(8, 32, 28, 12, 2);
    b.rect(18, 34, 8, 6, 7);
    b.rect(10, 35, 6, 4, 1);
    b.rect(28, 35, 6, 4, 1);
    b.rect(6, 42, 32, 5, 9);
    b.rect(2, 34, 8, 18, 5);
    b.rect(34, 34, 8, 18, 5);
    b.rect(4, 40, 4, 6, 6);
    b.rect(36, 40, 4, 6, 6);
    b.outline(8, false);
    return b;
}

Bitmap welderArt() {
    Bitmap b(40, 54);
    b.ellipse(13, 16, 6, 12, 4);
    b.ellipse(24, 16, 6, 12, 4);
    b.rect(11, 5, 4, 4, 6);
    b.rect(22, 5, 4, 4, 9);
    b.rect(6, 26, 28, 14, 2);
    b.rect(8, 28, 24, 4, 3);
    b.line(28, 28, 34, 12, 3, 2.5f);
    b.rect(31, 8, 6, 6, 6);
    b.rect(33, 4, 3, 5, 1);
    b.ellipse(10, 44, 6, 6, 5);
    b.ellipse(30, 44, 6, 6, 5);
    b.ellipse(10, 44, 2, 2, 6);
    b.ellipse(30, 44, 2, 2, 6);
    b.outline(8, false);
    return b;
}

Bitmap crusherArt() {
    Bitmap b(58, 52);
    b.poly({{6, 20}, {29, 4}, {52, 20}}, 6);
    b.rect(8, 18, 42, 16, 2);
    b.rect(14, 22, 30, 10, 3);
    for (int x = 16; x < 42; x += 6) b.rect(x, 20, 3, 5, 4);
    b.rect(10, 32, 38, 10, 2);
    b.rect(12, 34, 12, 4, 1);
    b.rect(4, 36, 10, 12, 5);
    b.rect(44, 36, 10, 12, 5);
    b.rect(6, 38, 6, 3, 6);
    b.rect(6, 44, 6, 3, 6);
    b.rect(46, 38, 6, 3, 6);
    b.rect(46, 44, 6, 3, 6);
    b.outline(8, false);
    return b;
}

Bitmap balerArt() {
    Bitmap b(46, 54);
    b.rect(8, 8, 30, 32, 2);
    b.rect(8, 20, 30, 6, 4);
    b.rect(14, 12, 18, 8, 6);
    b.rect(12, 34, 6, 10, 7);
    b.rect(28, 34, 6, 10, 7);
    b.rect(6, 40, 34, 6, 3);
    b.ellipse(12, 48, 5, 5, 5);
    b.ellipse(34, 48, 5, 5, 5);
    b.rect(20, 3, 6, 6, 1);
    b.outline(8, false);
    return b;
}

Bitmap purseArt() {
    Bitmap b(28, 32);
    b.rect(12, 2, 4, 7, 6);
    b.ellipse(14, 18, 11, 10, 2);
    b.poly({{5, 15}, {14, 7}, {23, 15}}, 3);
    b.rect(11, 14, 6, 5, 5);
    b.rect(13, 15, 2, 2, 1);
    b.line(8, 20, 8, 26, 4, 1.f);
    b.line(20, 20, 20, 26, 4, 1.f);
    b.outline(8, false);
    return b;
}

Bitmap pileArt() {
    Bitmap b(40, 28);
    b.ellipse(20, 18, 16, 8, 4);
    b.rect(8, 10, 14, 8, 2);
    b.rect(18, 6, 10, 10, 6);
    b.poly({{6, 16}, {14, 6}, {18, 16}}, 3);
    b.rect(24, 12, 8, 6, 1);
    b.outline(8, false);
    return b;
}

Bitmap drumArt() {
    Bitmap b(22, 28);
    b.ellipse(11, 8, 8, 4, 6);
    b.rect(3, 8, 16, 14, 6);
    b.ellipse(11, 22, 8, 4, 2);
    b.rect(3, 12, 16, 3, 4);
    b.outline(8, false);
    return b;
}

Bitmap shackArt() {
    Bitmap b(48, 40);
    b.poly({{4, 16}, {24, 4}, {44, 16}}, 5);
    b.rect(6, 16, 36, 20, 7);
    b.rect(20, 24, 10, 12, 3);
    b.rect(10, 20, 8, 8, 9);
    b.rect(30, 20, 8, 8, 1);
    b.outline(8, false);
    return b;
}

Bitmap fenceArt() {
    Bitmap b(28, 24);
    b.rect(2, 4, 3, 18, 2);
    b.rect(23, 4, 3, 18, 2);
    b.rect(2, 8, 24, 3, 3);
    b.rect(2, 16, 24, 3, 3);
    b.outline(8, false);
    return b;
}

Bitmap craneArt() {
    Bitmap b(90, 36);
    b.rect(8, 16, 6, 18, 2);
    b.rect(72, 20, 5, 14, 2);
    b.rect(8, 16, 68, 4, 3);
    b.line(14, 16, 40, 4, 2, 2.f);
    b.line(40, 4, 70, 16, 2, 2.f);
    b.rect(38, 2, 4, 6, 1);
    return b;
}

Bitmap postArt() {
    Bitmap b(10, 36);
    b.rect(3, 2, 4, 30, 2);
    b.rect(1, 30, 8, 4, 3);
    b.outline(8, false);
    return b;
}

Bitmap bulbArt() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 5, 1);
    b.ellipse(7, 7, 3, 2, 2);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 40);
    b.rect(6, 12, 4, 24, 2);
    b.ellipse(8, 8, 6, 5, 9);
    b.ellipse(8, 8, 3, 2, 1);
    b.rect(2, 34, 12, 3, 3);
    b.outline(8, false);
    return b;
}

Bitmap chevArt() {
    Bitmap b(30, 10);
    b.rect(0, 3, 30, 4, 1);
    b.rect(0, 0, 4, 10, 1);
    b.rect(26, 0, 4, 10, 1);
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
    b.line(10, 2, 2, 10, 3, 1.5f);
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

Bitmap starArt() {
    Bitmap b(5, 5);
    b.set(2, 0, 1);
    b.set(2, 1, 1);
    b.set(2, 2, 1);
    b.set(2, 3, 1);
    b.set(2, 4, 1);
    b.set(0, 2, 1);
    b.set(1, 2, 1);
    b.set(3, 2, 1);
    b.set(4, 2, 1);
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
    const uint16_t dark = gs::rgb4(1, 1, 1);
    auto textPal = [&](int pal, uint16_t inkC) {
        blank(vdp, pal);
        ink(vdp, pal, 1, inkC);
        ink(vdp, pal, 15, dark);
        ink(vdp, pal, 8, dark);
    };

    textPal(PAL_HUD, gs::rgb4(14, 13, 11));

    textPal(PAL_YOU, gs::rgb4(15, 13, 2));
    ink(vdp, PAL_YOU, 2, gs::rgb4(13, 10, 1));
    ink(vdp, PAL_YOU, 3, gs::rgb4(8, 6, 1));
    ink(vdp, PAL_YOU, 4, gs::rgb4(5, 8, 9));
    ink(vdp, PAL_YOU, 5, gs::rgb4(2, 2, 2));
    ink(vdp, PAL_YOU, 6, gs::rgb4(7, 7, 6));
    ink(vdp, PAL_YOU, 7, gs::rgb4(4, 4, 4));
    ink(vdp, PAL_YOU, 9, gs::rgb4(15, 15, 12));
    ink(vdp, PAL_YOU, 10, gs::rgb4(12, 12, 10));

    textPal(PAL_MULE, gs::rgb4(15, 15, 12));
    ink(vdp, PAL_MULE, 2, gs::rgb4(6, 7, 3));
    ink(vdp, PAL_MULE, 3, gs::rgb4(3, 4, 2));
    ink(vdp, PAL_MULE, 4, gs::rgb4(5, 5, 4));
    ink(vdp, PAL_MULE, 5, gs::rgb4(2, 2, 2));
    ink(vdp, PAL_MULE, 6, gs::rgb4(5, 7, 8));
    ink(vdp, PAL_MULE, 7, gs::rgb4(7, 3, 2));
    ink(vdp, PAL_MULE, 9, gs::rgb4(9, 9, 8));

    textPal(PAL_WELD, gs::rgb4(12, 15, 15));
    ink(vdp, PAL_WELD, 2, gs::rgb4(8, 9, 10));
    ink(vdp, PAL_WELD, 3, gs::rgb4(4, 5, 6));
    ink(vdp, PAL_WELD, 4, gs::rgb4(6, 7, 4));
    ink(vdp, PAL_WELD, 5, gs::rgb4(2, 2, 2));
    ink(vdp, PAL_WELD, 6, gs::rgb4(8, 4, 2));
    ink(vdp, PAL_WELD, 9, gs::rgb4(10, 6, 3));

    textPal(PAL_CRUSH, gs::rgb4(14, 5, 2));
    ink(vdp, PAL_CRUSH, 2, gs::rgb4(9, 3, 2));
    ink(vdp, PAL_CRUSH, 3, gs::rgb4(5, 2, 1));
    ink(vdp, PAL_CRUSH, 4, gs::rgb4(12, 12, 10));
    ink(vdp, PAL_CRUSH, 5, gs::rgb4(3, 3, 3));
    ink(vdp, PAL_CRUSH, 6, gs::rgb4(6, 6, 6));
    ink(vdp, PAL_CRUSH, 9, gs::rgb4(7, 2, 1));

    textPal(PAL_BALE, gs::rgb4(11, 14, 6));
    ink(vdp, PAL_BALE, 2, gs::rgb4(4, 7, 4));
    ink(vdp, PAL_BALE, 3, gs::rgb4(2, 4, 2));
    ink(vdp, PAL_BALE, 4, gs::rgb4(12, 10, 2));
    ink(vdp, PAL_BALE, 5, gs::rgb4(7, 8, 7));
    ink(vdp, PAL_BALE, 6, gs::rgb4(1, 1, 1));
    ink(vdp, PAL_BALE, 7, gs::rgb4(6, 6, 7));

    textPal(PAL_PURSE, gs::rgb4(14, 11, 3));
    ink(vdp, PAL_PURSE, 2, gs::rgb4(7, 4, 2));
    ink(vdp, PAL_PURSE, 3, gs::rgb4(4, 2, 1));
    ink(vdp, PAL_PURSE, 4, gs::rgb4(10, 8, 5));
    ink(vdp, PAL_PURSE, 5, gs::rgb4(15, 14, 8));
    ink(vdp, PAL_PURSE, 6, gs::rgb4(5, 3, 2));
    ink(vdp, PAL_PURSE, 9, gs::rgb4(15, 13, 8));

    textPal(PAL_PROP, gs::rgb4(10, 10, 9));
    ink(vdp, PAL_PROP, 2, gs::rgb4(6, 6, 6));
    ink(vdp, PAL_PROP, 3, gs::rgb4(3, 3, 3));
    ink(vdp, PAL_PROP, 4, gs::rgb4(8, 4, 2));
    ink(vdp, PAL_PROP, 5, gs::rgb4(7, 5, 3));
    ink(vdp, PAL_PROP, 6, gs::rgb4(4, 5, 3));
    ink(vdp, PAL_PROP, 7, gs::rgb4(5, 4, 3));
    ink(vdp, PAL_PROP, 9, gs::rgb4(15, 12, 4));

    textPal(PAL_FX, gs::rgb4(12, 12, 11));
    ink(vdp, PAL_FX, 2, gs::rgb4(8, 8, 7));
    ink(vdp, PAL_FX, 3, gs::rgb4(5, 5, 5));
    ink(vdp, PAL_FX, 4, gs::rgb4(14, 14, 12));

    textPal(PAL_LAMP, gs::rgb4(15, 12, 3));
    ink(vdp, PAL_LAMP, 2, gs::rgb4(15, 15, 10));

    blank(vdp, PAL_SHOCK);
    for (int i = 1; i < 16; ++i) {
        int g = std::max(4, 15 - i / 2);
        ink(vdp, PAL_SHOCK, i, gs::rgb4(15, g, 3));
    }
    ink(vdp, PAL_SHOCK, 1, gs::rgb4(15, 15, 14));
    ink(vdp, PAL_SHOCK, 15, dark);

    const uint16_t road[16] = {
        0,
        gs::rgb4(3, 4, 2), gs::rgb4(2, 2, 1), gs::rgb4(4, 4, 2),
        gs::rgb4(4, 3, 2), gs::rgb4(2, 2, 1),
        gs::rgb4(5, 4, 3), gs::rgb4(3, 2, 2),
        gs::rgb4(6, 5, 4), gs::rgb4(2, 2, 2), gs::rgb4(6, 5, 3),
        gs::rgb4(2, 3, 4), gs::rgb4(3, 4, 5), gs::rgb4(4, 5, 6),
        gs::rgb4(10, 7, 2), gs::rgb4(7, 6, 4),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    loadFont(vdp, art);
    art.loader = gs::uploadMipped(vdp, loaderArt());
    art.mule = gs::uploadMipped(vdp, muleArt());
    art.welder = gs::uploadMipped(vdp, welderArt());
    art.crusher = gs::uploadMipped(vdp, crusherArt());
    art.baler = gs::uploadMipped(vdp, balerArt());
    art.purse = gs::uploadMipped(vdp, purseArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.shack = gs::uploadMipped(vdp, shackArt());
    art.fence = gs::uploadMipped(vdp, fenceArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.bulb = gs::uploadMipped(vdp, bulbArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.moon = gs::uploadMipped(vdp, moonArt());
    art.star = gs::uploadMipped(vdp, starArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(4, 3, 3));
}

}  // namespace ypurs
