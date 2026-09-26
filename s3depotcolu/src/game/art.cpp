#include "game/art.h"

#include <initializer_list>
#include <string>

namespace dcol {
namespace {

using gs::Bitmap;

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

Bitmap tankerArt() {
    Bitmap b(72, 68);
    b.rect(16, 4, 40, 8, 3);
    b.rect(22, 6, 28, 4, 6);
    b.ellipse(36, 28, 28, 16, 2);
    b.ellipse(36, 28, 22, 11, 3);
    b.rect(10, 24, 52, 6, 8);
    b.rect(10, 30, 52, 4, 5);
    b.rect(14, 42, 44, 8, 4);
    b.rect(20, 44, 12, 4, 6);
    b.rect(40, 44, 12, 4, 1);
    b.ellipse(18, 56, 8, 8, 5);
    b.ellipse(54, 56, 8, 8, 5);
    b.ellipse(18, 56, 3, 3, 1);
    b.ellipse(54, 56, 3, 3, 1);
    b.rect(8, 46, 6, 4, 7);
    b.rect(58, 46, 6, 4, 7);
    b.outline(15, false);
    return b;
}

Bitmap truckArt() {
    Bitmap b(64, 74);
    b.poly({{14, 10}, {50, 10}, {46, 2}, {18, 2}}, 1);
    b.rect(12, 10, 40, 16, 2);
    b.rect(18, 13, 28, 7, 6);
    b.rect(14, 26, 36, 16, 4);
    b.rect(18, 29, 12, 8, 3);
    b.rect(34, 29, 12, 8, 7);
    b.rect(10, 42, 44, 8, 3);
    b.ellipse(16, 58, 8, 8, 5);
    b.ellipse(48, 58, 8, 8, 5);
    b.ellipse(16, 58, 3, 3, 1);
    b.ellipse(48, 58, 3, 3, 8);
    b.rect(6, 40, 6, 4, 7);
    b.rect(52, 40, 6, 4, 7);
    b.outline(15, false);
    return b;
}

Bitmap pennantArt() {
    Bitmap b(18, 28);
    b.rect(8, 4, 3, 22, 3);
    b.poly({{11, 6}, {17, 10}, {11, 15}}, 2);
    b.outline(15, false);
    return b;
}

Bitmap warehouseArt() {
    Bitmap b(104, 72);
    b.poly({{4, 22}, {52, 6}, {100, 22}, {100, 28}, {4, 28}}, 4);
    b.rect(8, 26, 88, 40, 2);
    b.rect(14, 32, 28, 28, 3);
    b.rect(48, 34, 10, 12, 7);
    b.rect(64, 34, 10, 12, 7);
    b.rect(80, 34, 10, 12, 7);
    b.rect(8, 22, 88, 4, 5);
    b.rect(46, 8, 8, 8, 8);
    b.outline(15, false);
    return b;
}

Bitmap doorArt() {
    Bitmap b(36, 48);
    b.rect(2, 2, 32, 44, 6);
    for (int i = 0; i < 5; ++i) b.rect(6, 6 + i * 8, 24, 3, 1);
    b.rect(16, 22, 4, 8, 8);
    b.outline(15, false);
    return b;
}

Bitmap apronArt() {
    Bitmap b(72, 16);
    for (int i = 0; i < 9; ++i) b.rect(i * 8, 2, 4, 12, (i & 1) ? 8 : 1);
    return b;
}

Bitmap tankArt() {
    Bitmap b(64, 48);
    b.ellipse(32, 22, 26, 14, 1);
    b.ellipse(32, 22, 18, 9, 2);
    b.rect(8, 20, 48, 4, 3);
    b.rect(14, 34, 6, 10, 5);
    b.rect(44, 34, 6, 10, 5);
    b.rect(30, 6, 4, 10, 3);
    b.outline(15, false);
    return b;
}

Bitmap drumsArt() {
    Bitmap b(48, 40);
    b.ellipse(14, 22, 10, 12, 4);
    b.ellipse(30, 16, 10, 12, 4);
    b.ellipse(24, 28, 10, 12, 5);
    b.rect(6, 18, 16, 3, 1);
    b.rect(22, 12, 16, 3, 1);
    b.outline(15, false);
    return b;
}

Bitmap cratesArt() {
    Bitmap b(48, 40);
    b.rect(4, 16, 22, 18, 3);
    b.rect(22, 8, 22, 18, 2);
    b.rect(8, 20, 14, 3, 1);
    b.rect(26, 12, 14, 3, 1);
    b.rect(26, 8, 3, 18, 6);
    b.outline(15, false);
    return b;
}

Bitmap craneArt() {
    Bitmap b(72, 80);
    b.rect(10, 18, 8, 54, 6);
    b.line(14, 20, 64, 10, 6, 3);
    b.line(14, 28, 58, 16, 3, 2);
    b.rect(60, 8, 6, 8, 8);
    b.line(63, 16, 63, 36, 5, 1);
    b.rect(6, 68, 16, 6, 5);
    b.outline(15, false);
    return b;
}

Bitmap shackArt() {
    Bitmap b(52, 48);
    b.poly({{4, 16}, {26, 4}, {48, 16}}, 4);
    b.rect(6, 16, 40, 26, 2);
    b.rect(10, 20, 12, 10, 7);
    b.rect(28, 24, 12, 18, 3);
    b.rect(22, 8, 6, 6, 8);
    b.outline(15, false);
    return b;
}

Bitmap signArt() {
    Bitmap b(84, 44);
    b.rect(6, 4, 72, 26, 1);
    b.rect(6, 4, 72, 4, 4);
    gs::TextStyle st{2, 2, 0, 0, 1};
    Bitmap word = gs::textBitmap("DEPOT", st);
    b.blit(word, (b.w - word.w) / 2, 10);
    b.rect(38, 30, 8, 12, 3);
    b.outline(15, false);
    return b;
}

Bitmap stripeArt() {
    Bitmap b(28, 12);
    for (int i = 0; i < 4; ++i) b.rect(i * 7, 1, 7, 10, (i & 1) ? 2 : 1);
    b.outline(4, false);
    return b;
}

Bitmap lampArt() {
    Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(6, 6, 2, 2, 2);
    b.outline(15, false);
    return b;
}

Bitmap postArt() {
    Bitmap b(14, 56);
    b.rect(4, 2, 6, 50, 3);
    b.rect(2, 48, 10, 6, 5);
    b.rect(3, 8, 8, 3, 1);
    b.outline(15, false);
    return b;
}

Bitmap blockArt() {
    Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    return b;
}

Bitmap watchArt() {
    Bitmap b(40, 64);
    b.ellipse(20, 10, 8, 8, 2);
    b.rect(14, 6, 12, 4, 3);
    b.poly({{20, 16}, {10, 24}, {8, 42}, {32, 42}, {30, 24}}, 1);
    b.rect(16, 26, 8, 10, 4);
    b.rect(28, 28, 8, 4, 6);
    b.ellipse(36, 30, 4, 4, 7);
    b.rect(12, 42, 6, 14, 5);
    b.rect(22, 42, 6, 14, 5);
    b.rect(10, 54, 10, 4, 3);
    b.rect(20, 54, 10, 4, 3);
    b.outline(15, false);
    return b;
}

Bitmap dustArt() {
    Bitmap b(28, 18);
    b.ellipse(14, 10, 12, 6, 1);
    b.ellipse(8, 8, 6, 4, 2);
    b.ellipse(18, 12, 5, 3, 3);
    return b;
}

Bitmap shadowArt() {
    Bitmap b(36, 12);
    b.ellipse(18, 6, 16, 4, 3);
    return b;
}

Bitmap cloudArt() {
    Bitmap b(72, 26);
    b.ellipse(22, 15, 16, 8, 2);
    b.ellipse(40, 12, 18, 9, 1);
    b.ellipse(56, 16, 12, 6, 2);
    return b;
}

Bitmap sunArt() {
    Bitmap b(28, 28);
    b.ellipse(14, 14, 10, 10, 1);
    b.ellipse(11, 11, 4, 4, 2);
    return b;
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 14, 13), gs::rgb4(8, 8, 9), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 4), gs::rgb4(10, 7, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 3), gs::rgb4(15, 12, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 15, 7), gs::rgb4(14, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_TANKER, {0, gs::rgb4(14, 13, 8), gs::rgb4(8, 9, 4), gs::rgb4(4, 5, 2), gs::rgb4(5, 6, 4),
                             gs::rgb4(1, 1, 1), gs::rgb4(8, 12, 14), gs::rgb4(15, 12, 4), gs::rgb4(15, 10, 2), 0, 0, 0,
                             0, 0, 0, ink});
    setPal(vdp, PAL_TRUCK, {0, gs::rgb4(12, 13, 9), gs::rgb4(7, 8, 5), gs::rgb4(3, 4, 2), gs::rgb4(4, 5, 4),
                            gs::rgb4(1, 1, 1), gs::rgb4(9, 12, 14), gs::rgb4(15, 14, 10), gs::rgb4(12, 3, 2), 0, 0, 0,
                            0, 0, 0, ink});
    setPal(vdp, PAL_DEPOT, {0, gs::rgb4(12, 11, 9), gs::rgb4(8, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(10, 5, 3),
                            gs::rgb4(6, 3, 2), gs::rgb4(9, 9, 10), gs::rgb4(14, 12, 6), gs::rgb4(15, 10, 3), 0, 0, 0, 0,
                            0, 0, ink});
    setPal(vdp, PAL_BOOM, {0, gs::rgb4(14, 3, 2), gs::rgb4(14, 13, 9), gs::rgb4(5, 5, 6), gs::rgb4(1, 1, 1), 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_YARD, {0, gs::rgb4(11, 12, 13), gs::rgb4(6, 7, 8), gs::rgb4(8, 4, 2), gs::rgb4(4, 6, 10),
                           gs::rgb4(8, 8, 5), gs::rgb4(13, 11, 3), gs::rgb4(2, 2, 2), gs::rgb4(15, 12, 4), 0, 0, 0, 0, 0,
                           0, ink});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(14, 12, 8), gs::rgb4(3, 2, 2), gs::rgb4(5, 4, 3), gs::rgb4(12, 3, 2),
                           gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 13, 8), gs::rgb4(12, 10, 6), gs::rgb4(6, 5, 4), gs::rgb4(15, 5, 3),
                         gs::rgb4(6, 14, 7), gs::rgb4(15, 15, 13), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_COAT, {0, gs::rgb4(9, 8, 6), gs::rgb4(12, 9, 6), gs::rgb4(4, 3, 3), gs::rgb4(7, 6, 4),
                           gs::rgb4(2, 2, 2), gs::rgb4(13, 10, 4), gs::rgb4(15, 12, 4), gs::rgb4(8, 12, 14), 0, 0, 0, 0,
                           0, 0, ink});
    setPal(vdp, PAL_WHITE, {0, gs::rgb4(14, 14, 13), gs::rgb4(8, 8, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_NIGHT, {0, gs::rgb4(15, 10, 4), gs::rgb4(12, 8, 5), gs::rgb4(6, 6, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, ink});

    const uint16_t road[16] = {
        0,
        gs::rgb4(5, 5, 3),
        gs::rgb4(3, 3, 2),
        gs::rgb4(6, 6, 4),
        gs::rgb4(4, 4, 3),
        gs::rgb4(2, 2, 2),
        gs::rgb4(3, 3, 3),
        gs::rgb4(2, 2, 2),
        gs::rgb4(5, 5, 4),
        gs::rgb4(4, 4, 4),
        gs::rgb4(6, 6, 5),
        gs::rgb4(2, 3, 4),
        gs::rgb4(3, 4, 5),
        gs::rgb4(4, 5, 6),
        gs::rgb4(14, 11, 3),
        gs::rgb4(6, 6, 5),
    };
    for (int i = 0; i < 16; ++i) vdp.setColor(PAL_ROAD * 16 + i, road[i]);

    art.tanker = gs::uploadMipped(vdp, tankerArt());
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.warehouse = gs::uploadMipped(vdp, warehouseArt());
    art.door = gs::uploadMipped(vdp, doorArt());
    art.apron = gs::uploadMipped(vdp, apronArt());
    art.tank = gs::uploadMipped(vdp, tankArt());
    art.drums = gs::uploadMipped(vdp, drumsArt());
    art.crates = gs::uploadMipped(vdp, cratesArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.shack = gs::uploadMipped(vdp, shackArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.block = gs::uploadMipped(vdp, blockArt());
    art.watch = gs::uploadMipped(vdp, watchArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    loadFont(vdp, art);
}

}  // namespace dcol
