#include "game/art.h"

#include <string>

namespace bikeplat {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 2));
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

// pose: 0 nose-down, 1 level, 2 nose-up. Wheels are drawn apart from the frame.
gs::Bitmap bodyArt(int pose) {
    gs::Bitmap b(84, 52);
    int dy = (pose - 1) * 6;
    b.line(18, 34, 42, 18 + dy, 1, 2.2f);
    b.line(42, 18 + dy, 64, 34, 1, 2.2f);
    b.line(18, 34, 64, 34, 2, 2.2f);
    b.line(64, 34, 70, 40, 1, 2.2f);
    b.line(18, 34, 16, 40, 2, 2.f);
    b.line(36, 18 + dy, 50, 17 + dy, 3, 2.4f);
    b.line(58, 22 + dy / 2, 70, 18, 2, 1.8f);
    b.ellipse(44, 10 + dy, 5, 5, 4);
    b.rect(40, 6 + dy, 8, 3, 5);
    b.line(44, 15 + dy, 40, 28 + dy / 2, 6, 2.4f);
    b.line(42, 20 + dy, 62, 22, 6, 1.8f);
    b.line(40, 28, 30, 36, 6, 1.8f);
    b.line(40, 28, 50, 36, 7, 1.6f);
    b.outline(8, false);
    return b.cropToContent(1);
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 1);
    b.ellipse(14, 14, 9, 9, 2);
    b.ellipse(14, 14, 2, 2, 3);
    b.line(14, 4, 14, 24, 3, 1.1f);
    b.line(4, 14, 24, 14, 3, 1.1f);
    b.line(7, 7, 21, 21, 3, 1.f);
    b.line(7, 21, 21, 7, 3, 1.f);
    return b;
}

gs::Bitmap plankArt() {
    gs::Bitmap b(40, 12);
    b.rect(0, 0, 40, 12, 1);
    for (int x = 0; x < 40; x++) {
        b.set(x, 0, 2);
        b.set(x, 11, 3);
        if (x % 8 == 0)
            for (int y = 0; y < 12; y++) b.set(x, y, 4);
    }
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(28, 10);
    for (int x = 0; x < 28; x++) {
        int c = ((x / 4) & 1) ? 1 : 2;
        for (int y = 0; y < 10; y++) b.set(x, y, c);
    }
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 48);
    b.rect(4, 0, 4, 48, 1);
    b.rect(5, 2, 1, 44, 2);
    b.rect(2, 0, 8, 4, 3);
    return b;
}

gs::Bitmap pathArt() {
    gs::Bitmap b(32, 16);
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 32; x++) {
            uint32_t h = hash2(x, y);
            int c = 1 + int(h % 3);
            if (y < 2) c = 4;
            b.set(x, y, c);
        }
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(40, 56);
    b.rect(17, 30, 6, 26, 1);
    b.ellipse(20, 20, 16, 16, 2);
    b.ellipse(14, 18, 6, 5, 3);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(48, 18);
    b.ellipse(16, 10, 12, 6, 1);
    b.ellipse(30, 9, 14, 7, 1);
    b.ellipse(24, 8, 8, 5, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 7, 7, 1);
    b.ellipse(10, 10, 4, 4, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 36);
    b.rect(6, 10, 2, 26, 1);
    b.rect(2, 2, 10, 8, 2);
    b.rect(4, 4, 6, 4, 3);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 12);
    b.ellipse(8, 6, 7, 4, 1);
    b.ellipse(5, 6, 3, 2, 2);
    return b;
}

gs::Bitmap signArt() {
    gs::Bitmap b(36, 16);
    b.rect(1, 1, 34, 14, 1);
    b.rect(3, 3, 30, 10, 2);
    b.rect(1, 1, 34, 2, 3);
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
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 13));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 11, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 8));

    setPal(vdp, PAL_BIKE,
           {0, gs::rgb4(2, 6, 4), gs::rgb4(4, 10, 6), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 5), gs::rgb4(14, 3, 2),
            gs::rgb4(13, 9, 6), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_RIDER,
           {0, gs::rgb4(2, 6, 4), gs::rgb4(4, 10, 6), gs::rgb4(8, 5, 2), gs::rgb4(12, 8, 5), gs::rgb4(14, 12, 3),
            gs::rgb4(13, 9, 6), gs::rgb4(12, 3, 3), gs::rgb4(6, 4, 3)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(1, 1, 1), gs::rgb4(6, 7, 7), gs::rgb4(12, 12, 11)});
    setPal(vdp, PAL_DECK, {0, gs::rgb4(9, 7, 4), gs::rgb4(13, 11, 7), gs::rgb4(5, 4, 2), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_PATH, {0, gs::rgb4(6, 6, 5), gs::rgb4(8, 8, 6), gs::rgb4(4, 4, 3), gs::rgb4(10, 10, 8)});
    setPal(vdp, PAL_POST, {0, gs::rgb4(5, 5, 4), gs::rgb4(8, 8, 6), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(5, 3, 2), gs::rgb4(3, 8, 3), gs::rgb4(5, 11, 4)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 3), gs::rgb4(2, 2, 1), gs::rgb4(14, 4, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 4, 4), gs::rgb4(12, 10, 4), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(10, 9, 7), gs::rgb4(13, 12, 9)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(2, 5, 3), gs::rgb4(14, 14, 10), gs::rgb4(8, 12, 6)});

    loadFont(vdp, art);
    for (int i = 0; i < 3; i++) art.body[i] = gs::uploadMipped(vdp, bodyArt(i));
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.path = gs::uploadMipped(vdp, pathArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.sign = gs::uploadMipped(vdp, signArt());
}

}  // namespace bikeplat
