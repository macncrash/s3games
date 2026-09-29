#include "game/art.h"

#include <initializer_list>

namespace submark {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap paintSub() {
    gs::Bitmap b(120, 40);
    b.ellipse(58, 22, 46, 11, 2);
    b.ellipse(56, 20, 42, 8, 1);
    b.rect(62, 12, 16, 10, 3);
    b.rect(66, 6, 8, 10, 3);
    b.rect(67, 4, 6, 4, 4);
    b.rect(40, 16, 28, 3, 5);
    b.ellipse(48, 20, 4.2f, 3.2f, 6);
    b.ellipse(48, 20, 2.0f, 1.6f, 7);
    b.ellipse(36, 20, 2.2f, 1.8f, 6);
    b.rect(10, 18, 12, 4, 4);
    b.rect(6, 14, 4, 12, 8);
    b.poly({{100, 14}, {114, 20}, {100, 28}}, 3);
    b.rect(92, 18, 6, 4, 4);
    b.outline(9, false);
    return b;
}

gs::Bitmap paintPad() {
    gs::Bitmap b(96, 18);
    b.rect(2, 4, 92, 10, 1);
    b.rect(2, 6, 92, 4, 2);
    for (int x = 6; x < 90; x += 8) b.rect(x, 4, 2, 10, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap paintCross() {
    gs::Bitmap b(36, 36);
    b.ellipse(18, 18, 15, 15, 1);
    b.ellipse(18, 18, 11, 11, 0);
    b.rect(16, 4, 4, 28, 2);
    b.rect(4, 16, 28, 4, 2);
    b.ellipse(18, 18, 3, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap paintPylon() {
    gs::Bitmap b(28, 80);
    b.rect(10, 8, 8, 70, 1);
    b.rect(12, 8, 3, 70, 2);
    b.poly({{4, 18}, {14, 4}, {24, 18}}, 3);
    b.rect(6, 18, 16, 6, 3);
    for (int y = 28; y < 74; y += 10) b.rect(8, y, 12, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap paintSand() {
    gs::Bitmap b(48, 20);
    b.poly({{0, 18}, {8, 8}, {16, 16}, {26, 6}, {36, 14}, {48, 8}, {48, 20}}, 1);
    b.poly({{0, 20}, {12, 14}, {24, 18}, {40, 12}, {48, 20}}, 2);
    return b;
}

gs::Bitmap paintKelp() {
    gs::Bitmap b(14, 40);
    b.poly({{7, 38}, {3, 26}, {9, 18}, {2, 8}, {7, 2}, {11, 12}, {6, 20}, {12, 30}, {7, 38}}, 1);
    b.poly({{7, 36}, {6, 22}, {8, 12}, {7, 4}}, 2);
    return b;
}

gs::Bitmap paintBub() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 0);
    b.set(3, 3, 2);
    return b;
}

gs::Bitmap paintFish() {
    gs::Bitmap b(24, 12);
    b.ellipse(10, 6, 8, 4, 1);
    b.poly({{16, 6}, {22, 2}, {22, 10}}, 2);
    b.ellipse(6, 5, 1.2f, 1.2f, 3);
    return b;
}

gs::Bitmap paintLamp() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2.4f, 2.4f, 2);
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
    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 15, 15), gs::rgb4(5, 9, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SUB,
           {0, gs::rgb4(11, 13, 12), gs::rgb4(3, 5, 6), gs::rgb4(14, 14, 11), gs::rgb4(5, 7, 8), gs::rgb4(14, 11, 3),
            gs::rgb4(7, 14, 15), gs::rgb4(15, 15, 13), gs::rgb4(6, 7, 6), ink, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MARK,
           {0, gs::rgb4(12, 11, 6), gs::rgb4(15, 14, 8), gs::rgb4(15, 6, 3), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_END,
           {0, gs::rgb4(9, 4, 3), gs::rgb4(13, 8, 5), gs::rgb4(15, 12, 4), ink, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BED, {0, gs::rgb4(6, 5, 3), gs::rgb4(9, 8, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_KELP, {0, gs::rgb4(2, 7, 4), gs::rgb4(4, 10, 5), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BUB, {0, gs::rgb4(9, 14, 15), gs::rgb4(15, 15, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_WIN,
           {0, gs::rgb4(11, 15, 9), gs::rgb4(2, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 2, 1)});
    setPal(vdp, PAL_ALERT,
           {0, gs::rgb4(15, 7, 5), gs::rgb4(4, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 1, 1)});
    setPal(vdp, PAL_FISH, {0, gs::rgb4(13, 9, 4), gs::rgb4(8, 5, 2), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    art.sub = gs::uploadMipped(vdp, paintSub());
    art.pad = gs::uploadMipped(vdp, paintPad());
    art.cross = gs::uploadMipped(vdp, paintCross());
    art.pylon = gs::uploadMipped(vdp, paintPylon());
    art.sand = gs::uploadMipped(vdp, paintSand());
    art.kelp = gs::uploadMipped(vdp, paintKelp());
    art.bub = gs::uploadMipped(vdp, paintBub());
    art.fish = gs::uploadMipped(vdp, paintFish());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    art.title = words(vdp, "SUB MARK", 3);
    art.set = words(vdp, "SET ON THE MARK", 2);
    art.missed = words(vdp, "MISSED THE END", 2);
    art.off = words(vdp, "OFF THE MARK", 2);
    art.ran = words(vdp, "LEG RAN OUT", 2);
    art.hard = words(vdp, "SET TOO HARD", 2);
    art.paused = words(vdp, "PAUSED", 2);
    loadFont(vdp, art);
    vdp.setFogColor(gs::rgb4(1, 3, 5));
}

}  // namespace submark
