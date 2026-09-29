#include "game/art.h"

#include <initializer_list>

namespace subboom {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap paintSub() {
    gs::Bitmap b(112, 48);
    b.ellipse(52, 26, 40, 12, 2);
    b.ellipse(50, 24, 36, 9, 1);
    b.rect(70, 16, 22, 8, 3);
    b.rect(74, 10, 12, 10, 3);
    b.rect(76, 8, 8, 6, 4);
    b.ellipse(78, 24, 5, 4, 6);
    b.ellipse(78, 24, 2.2f, 2.0f, 7);
    b.ellipse(58, 23, 3.2f, 2.6f, 6);
    b.ellipse(44, 23, 2.4f, 2.0f, 5);
    b.rect(14, 22, 10, 3, 4);
    b.rect(8, 16, 3, 16, 8);
    b.rect(11, 18, 2, 12, 8);
    b.rect(86, 20, 8, 4, 4);
    b.poly({{96, 18}, {108, 24}, {96, 30}}, 3);
    b.outline(9, false);
    return b;
}

gs::Bitmap paintDrive() {
    gs::Bitmap b(64, 28);
    b.rect(6, 6, 52, 16, 1);
    b.ellipse(8, 14, 6, 8, 1);
    b.ellipse(56, 14, 6, 8, 1);
    b.rect(18, 6, 4, 16, 3);
    b.rect(30, 6, 4, 16, 3);
    b.rect(42, 6, 4, 16, 3);
    b.rect(10, 9, 44, 4, 2);
    b.rect(48, 11, 6, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap paintBoom() {
    gs::Bitmap b(72, 16);
    b.rect(0, 4, 72, 8, 1);
    b.rect(0, 6, 72, 3, 2);
    for (int x = 4; x < 70; x += 10) b.rect(x, 2, 3, 12, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap paintPile() {
    gs::Bitmap b(16, 72);
    b.rect(4, 0, 8, 72, 1);
    b.rect(6, 0, 3, 72, 2);
    for (int y = 6; y < 68; y += 10) b.rect(3, y, 10, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap paintRock() {
    gs::Bitmap b(40, 36);
    b.poly({{2, 34}, {8, 14}, {16, 22}, {22, 6}, {30, 16}, {38, 34}}, 1);
    b.poly({{8, 34}, {14, 20}, {22, 12}, {28, 22}, {34, 34}}, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap paintKelp() {
    gs::Bitmap b(16, 48);
    b.poly({{8, 46}, {4, 30}, {10, 22}, {3, 10}, {8, 2}, {12, 12}, {7, 24}, {13, 34}, {8, 46}}, 1);
    b.poly({{8, 46}, {7, 28}, {9, 14}, {8, 4}, {10, 16}, {9, 30}}, 2);
    return b;
}

gs::Bitmap paintBub() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 4.5f, 4.5f, 1);
    b.ellipse(6, 6, 2.4f, 2.4f, 0);
    b.set(4, 4, 2);
    return b;
}

gs::Bitmap paintFish() {
    gs::Bitmap b(28, 14);
    b.ellipse(12, 7, 9, 5, 1);
    b.poly({{20, 7}, {26, 2}, {26, 12}}, 1);
    b.ellipse(7, 6, 1.3f, 1.3f, 2);
    return b;
}

gs::Bitmap paintLamp() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 3, 3, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp);
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
        art.font[c - 32] = t;
    }
}

gs::Mipped words(gs::VDP& vdp, const char* text, int scale) {
    gs::TextStyle st{scale, 1, 2, 15, 1};
    return gs::uploadMipped(vdp, gs::textBitmap(text, st));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(1, 2, 3);
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(6, 10, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SUB,
           {0, gs::rgb4(10, 12, 11), gs::rgb4(4, 6, 6), gs::rgb4(13, 14, 12), gs::rgb4(6, 8, 9), gs::rgb4(2, 3, 3),
            gs::rgb4(8, 14, 15), gs::rgb4(15, 15, 12), gs::rgb4(7, 8, 7), ink, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_DRIVE,
           {0, gs::rgb4(12, 9, 3), gs::rgb4(15, 13, 6), gs::rgb4(6, 5, 3), gs::rgb4(14, 4, 3), ink, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0});
    setPal(vdp, PAL_BOOM,
           {0, gs::rgb4(8, 8, 7), gs::rgb4(12, 12, 10), gs::rgb4(4, 5, 4), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_ROCK,
           {0, gs::rgb4(3, 4, 5), gs::rgb4(5, 6, 7), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BUB, {0, gs::rgb4(10, 14, 15), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FISH, {0, gs::rgb4(12, 8, 3), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WIN,
           {0, gs::rgb4(12, 15, 10), gs::rgb4(2, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_ALERT,
           {0, gs::rgb4(15, 8, 6), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    art.sub = gs::uploadMipped(vdp, paintSub());
    art.drive = gs::uploadMipped(vdp, paintDrive());
    art.boom = gs::uploadMipped(vdp, paintBoom());
    art.pile = gs::uploadMipped(vdp, paintPile());
    art.rock = gs::uploadMipped(vdp, paintRock());
    art.kelp = gs::uploadMipped(vdp, paintKelp());
    art.bub = gs::uploadMipped(vdp, paintBub());
    art.fish = gs::uploadMipped(vdp, paintFish());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.title = words(vdp, "SUB BOOM", 3);
    art.delivered = words(vdp, "DRIVE ON THE BOOM", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.offLeg = words(vdp, "OFF THE LEG", 2);
    art.out = words(vdp, "LEG RAN OUT", 2);
    art.paused = words(vdp, "PAUSED", 2);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(1, 3, 6));
}

}  // namespace subboom
