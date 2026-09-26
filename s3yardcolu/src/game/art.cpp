#include "game/art.h"

#include <initializer_list>
#include <string>

namespace ycol {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
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

Bitmap truckArt() {
    Bitmap b(60, 72);
    b.rect(16, 2, 28, 8, 4);
    b.rect(18, 4, 24, 3, 1);
    b.rect(14, 10, 32, 16, 2);
    b.rect(18, 13, 10, 9, 6);
    b.rect(32, 13, 10, 9, 6);
    b.rect(28, 13, 4, 9, 3);
    b.rect(12, 26, 36, 8, 3);
    b.rect(20, 28, 20, 3, 7);
    b.rect(16, 34, 28, 12, 2);
    b.rect(18, 36, 24, 3, 1);
    b.ellipse(16, 52, 7, 7, 5);
    b.ellipse(44, 52, 7, 7, 5);
    b.ellipse(16, 52, 3, 3, 8);
    b.ellipse(44, 52, 3, 3, 8);
    b.rect(8, 46, 44, 5, 3);
    b.rect(6, 40, 6, 16, 4);
    b.rect(48, 40, 6, 16, 4);
    b.rect(10, 58, 8, 4, 7);
    b.rect(42, 58, 8, 4, 7);
    b.outline(15, false);
    return b;
}

Bitmap muleArt() {
    Bitmap b(64, 56);
    b.rect(24, 2, 16, 5, 7);
    b.ellipse(32, 4, 4, 3, 8);
    b.rect(14, 7, 36, 14, 2);
    b.rect(18, 10, 12, 8, 6);
    b.rect(34, 10, 12, 8, 6);
    b.rect(30, 10, 4, 8, 3);
    b.rect(10, 21, 44, 8, 1);
    b.rect(12, 29, 40, 6, 4);
    b.rect(22, 31, 20, 3, 8);
    b.ellipse(16, 42, 8, 8, 5);
    b.ellipse(48, 42, 8, 8, 5);
    b.ellipse(16, 42, 3, 3, 1);
    b.ellipse(48, 42, 3, 3, 1);
    b.rect(8, 36, 6, 12, 3);
    b.rect(50, 36, 6, 12, 3);
    b.rect(28, 46, 8, 4, 7);
    b.outline(15, false);
    return b;
}

Bitmap hulkArt() {
    Bitmap b(78, 36);
    b.poly({{8, 16}, {18, 6}, {60, 6}, {70, 16}, {70, 24}, {8, 24}}, 2);
    b.rect(10, 16, 58, 6, 1);
    b.rect(16, 8, 14, 7, 6);
    b.rect(46, 9, 12, 6, 6);
    b.rect(14, 22, 50, 6, 3);
    b.rect(6, 18, 8, 8, 4);
    b.rect(64, 18, 8, 8, 4);
    b.rect(12, 26, 10, 5, 5);
    b.rect(56, 26, 10, 5, 5);
    b.rect(30, 27, 16, 3, 7);
    b.rect(20, 12, 6, 3, 8);
    b.rect(50, 12, 5, 3, 8);
    b.outline(15, false);
    return b;
}

Bitmap magnetArt() {
    Bitmap b(36, 28);
    b.ellipse(18, 14, 14, 10, 2);
    b.ellipse(18, 14, 8, 5, 3);
    b.rect(16, 2, 4, 8, 4);
    b.rect(8, 12, 6, 4, 1);
    b.rect(22, 12, 6, 4, 1);
    b.outline(15, false);
    return b;
}

Bitmap pennantArt() {
    Bitmap b(16, 26);
    b.rect(7, 2, 3, 22, 3);
    b.poly({{10, 4}, {15, 8}, {10, 13}}, 1);
    b.outline(15, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(18, 72);
    b.rect(6, 4, 6, 58, 1);
    b.rect(7, 4, 2, 58, 2);
    for (int i = 0; i < 6; ++i) b.rect(5, 8 + i * 8, 8, 4, (i & 1) ? 3 : 1);
    b.rect(3, 60, 12, 8, 4);
    b.rect(4, 2, 10, 6, 5);
    b.outline(15, false);
    return b;
}

Bitmap beamArt() {
    Bitmap b(48, 10);
    for (int i = 0; i < 8; ++i) b.rect(i * 6, 1, 6, 8, (i & 1) ? 3 : 1);
    return b;
}

Bitmap cableArt() {
    Bitmap b(6, 20);
    b.rect(2, 0, 2, 20, 1);
    b.rect(1, 4, 4, 2, 2);
    b.rect(1, 12, 4, 2, 2);
    return b;
}

Bitmap markArt() {
    Bitmap b(40, 10);
    b.rect(0, 3, 40, 4, 1);
    for (int i = 0; i < 5; ++i) b.poly({{float(i * 8 + 1), 8}, {float(i * 8 + 4), 1}, {float(i * 8 + 7), 8}}, 2);
    return b;
}

Bitmap lampArt() {
    Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 2, 2, 2);
    b.outline(15, false);
    return b;
}

Bitmap scrapArt() {
    Bitmap b(64, 40);
    b.ellipse(18, 22, 14, 10, 2);
    b.ellipse(36, 18, 16, 11, 3);
    b.ellipse(48, 24, 12, 8, 4);
    b.rect(10, 16, 18, 6, 1);
    b.rect(30, 12, 16, 5, 5);
    b.rect(22, 26, 14, 4, 6);
    b.outline(15, false);
    return b;
}

Bitmap stackArt() {
    Bitmap b(56, 48);
    b.rect(6, 30, 44, 8, 3);
    b.rect(10, 20, 38, 8, 2);
    b.rect(14, 10, 30, 8, 4);
    b.rect(18, 4, 20, 6, 1);
    b.rect(12, 32, 10, 3, 5);
    b.rect(34, 22, 8, 3, 6);
    b.outline(15, false);
    return b;
}

Bitmap drumsArt() {
    Bitmap b(48, 40);
    b.ellipse(14, 22, 10, 12, 4);
    b.ellipse(30, 16, 10, 12, 2);
    b.ellipse(24, 28, 10, 12, 5);
    b.rect(6, 16, 16, 3, 1);
    b.rect(22, 10, 16, 3, 1);
    b.rect(16, 22, 16, 3, 6);
    b.outline(15, false);
    return b;
}

Bitmap shackArt() {
    Bitmap b(56, 48);
    b.poly({{4, 16}, {28, 4}, {52, 16}}, 5);
    b.rect(6, 16, 44, 26, 2);
    b.rect(10, 20, 14, 12, 6);
    b.rect(30, 22, 14, 20, 3);
    b.rect(34, 28, 6, 4, 1);
    b.rect(8, 40, 40, 4, 4);
    b.outline(15, false);
    return b;
}

Bitmap signArt() {
    Bitmap b(72, 52);
    b.rect(32, 28, 6, 20, 4);
    b.rect(4, 4, 64, 26, 2);
    b.rect(4, 4, 64, 5, 5);
    Bitmap word = gs::textBitmap("YARD", gs::TextStyle{2, 3, 0, 0, 1});
    b.blit(word, 12, 12);
    b.outline(15, false);
    return b;
}

Bitmap fenceArt() {
    Bitmap b(48, 28);
    b.rect(4, 6, 3, 20, 2);
    b.rect(22, 6, 3, 20, 2);
    b.rect(40, 6, 3, 20, 2);
    b.rect(4, 8, 40, 3, 1);
    b.rect(4, 16, 40, 3, 3);
    b.outline(15, false);
    return b;
}

Bitmap balerArt() {
    Bitmap b(52, 46);
    b.rect(6, 10, 40, 26, 2);
    b.rect(8, 12, 36, 8, 3);
    b.rect(14, 22, 16, 12, 4);
    b.rect(34, 18, 8, 14, 1);
    b.rect(4, 34, 44, 6, 5);
    b.rect(18, 4, 8, 8, 6);
    b.outline(15, false);
    return b;
}

Bitmap dustArt() {
    Bitmap b(28, 16);
    b.ellipse(14, 9, 12, 5, 1);
    b.ellipse(8, 8, 5, 3, 2);
    b.ellipse(18, 11, 4, 3, 3);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(40, 12);
    b.ellipse(20, 6, 16, 4, 1);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(70, 24);
    b.ellipse(22, 14, 16, 7, 1);
    b.ellipse(40, 11, 18, 8, 2);
    b.ellipse(54, 15, 12, 6, 1);
    return b;
}

Bitmap sunArt() {
    Bitmap b(26, 26);
    b.ellipse(13, 13, 9, 9, 1);
    b.ellipse(10, 10, 3, 3, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(2, 1, 1);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11), gs::rgb4(8, 7, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), gs::rgb4(15, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), gs::rgb4(15, 10, 6), gs::rgb4(6, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(8, 15, 6), gs::rgb4(14, 15, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_TRUCK, {0, gs::rgb4(12, 13, 8), gs::rgb4(6, 8, 4), gs::rgb4(3, 4, 2), gs::rgb4(8, 9, 5), gs::rgb4(1, 1, 1),
                            gs::rgb4(8, 12, 13), gs::rgb4(14, 13, 8), gs::rgb4(10, 8, 4), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_MULE, {0, gs::rgb4(15, 13, 4), gs::rgb4(13, 10, 2), gs::rgb4(6, 4, 1), gs::rgb4(3, 3, 3), gs::rgb4(1, 1, 1),
                           gs::rgb4(9, 12, 14), gs::rgb4(15, 8, 2), gs::rgb4(14, 14, 12), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HULK, {0, gs::rgb4(12, 7, 4), gs::rgb4(9, 4, 2), gs::rgb4(4, 2, 1), gs::rgb4(7, 7, 7), gs::rgb4(2, 2, 2),
                           gs::rgb4(6, 8, 9), gs::rgb4(14, 8, 3), gs::rgb4(13, 12, 8), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_YARD, {0, gs::rgb4(12, 10, 7), gs::rgb4(7, 6, 5), gs::rgb4(4, 3, 3), gs::rgb4(10, 5, 2), gs::rgb4(5, 5, 4),
                           gs::rgb4(8, 7, 3), gs::rgb4(3, 5, 6), gs::rgb4(14, 11, 4), 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GANTRY, {0, gs::rgb4(14, 12, 3), gs::rgb4(15, 14, 8), gs::rgb4(2, 2, 2), gs::rgb4(5, 4, 3), gs::rgb4(8, 2, 1),
                             gs::rgb4(15, 6, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(15, 14, 10), gs::rgb4(12, 9, 4), gs::rgb4(3, 2, 1), gs::rgb4(6, 4, 2), gs::rgb4(12, 3, 2),
                           gs::rgb4(8, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(13, 11, 8), gs::rgb4(9, 8, 6), gs::rgb4(5, 4, 3), gs::rgb4(15, 8, 3), 0, 0, 0, 0, 0, 0, 0,
                         0, 0, 0, 0, ink});
    setPal(vdp, PAL_MAGNET, {0, gs::rgb4(15, 4, 2), gs::rgb4(10, 2, 2), gs::rgb4(3, 2, 2), gs::rgb4(8, 8, 9), gs::rgb4(14, 12, 6),
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_DUSK, {0, gs::rgb4(15, 12, 7), gs::rgb4(12, 8, 5), gs::rgb4(8, 6, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_HAZARD, {0, gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(5, 4, 2),
        gs::rgb4(3, 3, 2),
        gs::rgb4(6, 5, 3),
        gs::rgb4(4, 4, 3),
        gs::rgb4(2, 2, 2),
        gs::rgb4(3, 3, 3),
        gs::rgb4(2, 2, 2),
        gs::rgb4(5, 4, 3),
        gs::rgb4(4, 4, 4),
        gs::rgb4(7, 6, 3),
        gs::rgb4(2, 3, 3),
        gs::rgb4(3, 4, 4),
        gs::rgb4(4, 5, 5),
        gs::rgb4(13, 10, 3),
        gs::rgb4(6, 5, 4),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    art.truck = gs::uploadMipped(vdp, truckArt());
    art.mule = gs::uploadMipped(vdp, muleArt());
    art.hulk = gs::uploadMipped(vdp, hulkArt());
    art.magnet = gs::uploadMipped(vdp, magnetArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.scrap = gs::uploadMipped(vdp, scrapArt());
    art.stack = gs::uploadMipped(vdp, stackArt());
    art.drums = gs::uploadMipped(vdp, drumsArt());
    art.shack = gs::uploadMipped(vdp, shackArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.fence = gs::uploadMipped(vdp, fenceArt());
    art.baler = gs::uploadMipped(vdp, balerArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    loadFont(vdp, art);
}

}  // namespace ycol
