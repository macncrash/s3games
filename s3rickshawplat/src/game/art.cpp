#include "game/art.h"

#include <string>

namespace rickplat {
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

gs::Bitmap cabArt() {
    gs::Bitmap b(92, 52);
    b.poly({{8, 40}, {84, 40}, {86, 34}, {78, 28}, {14, 28}}, 1);
    b.rect(16, 14, 58, 16, 2);
    b.poly({{16, 14}, {28, 4}, {68, 4}, {74, 14}}, 3);
    b.rect(30, 8, 22, 8, 4);
    b.rect(18, 18, 16, 10, 5);
    b.rect(40, 18, 20, 10, 5);
    b.line(10, 40, 82, 40, 6, 2.f);
    b.rect(12, 36, 8, 6, 7);
    b.rect(70, 36, 10, 6, 7);
    b.outline(8, false);
    return b.cropToContent(1);
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 1);
    b.ellipse(11, 11, 7, 7, 2);
    b.ellipse(11, 11, 2, 2, 3);
    b.line(11, 2, 11, 20, 3, 1.f);
    b.line(2, 11, 20, 11, 3, 1.f);
    return b;
}

gs::Bitmap pullArt(int step) {
    gs::Bitmap b(28, 36);
    b.ellipse(14, 6, 5, 5, 1);
    b.rect(12, 11, 5, 12, 2);
    b.line(12, 14, 4, 22, 2, 1.6f);
    b.line(17, 14, 24, 20, 2, 1.6f);
    if (step) {
        b.line(13, 22, 8, 34, 3, 1.8f);
        b.line(16, 22, 22, 34, 3, 1.8f);
    } else {
        b.line(13, 22, 16, 34, 3, 1.8f);
        b.line(16, 22, 10, 33, 3, 1.8f);
    }
    b.rect(10, 33, 5, 2, 4);
    b.rect(18, 33, 5, 2, 4);
    return b;
}

gs::Bitmap riderArt() {
    gs::Bitmap b(20, 22);
    b.ellipse(10, 5, 4, 4, 1);
    b.rect(7, 9, 7, 8, 2);
    b.rect(5, 16, 11, 4, 3);
    return b;
}

gs::Bitmap stoneArt() {
    gs::Bitmap b(36, 16);
    b.rect(0, 0, 36, 16, 1);
    for (int x = 0; x < 36; x++) {
        b.set(x, 0, 2);
        b.set(x, 15, 3);
        if (x % 9 == 0)
            for (int y = 0; y < 16; y++) b.set(x, y, 4);
    }
    return b;
}

gs::Bitmap lipArt() {
    gs::Bitmap b(12, 8);
    b.rect(0, 2, 12, 6, 1);
    b.rect(0, 0, 12, 3, 2);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(24, 8);
    for (int x = 0; x < 24; x++) {
        int c = ((x / 4) & 1) ? 1 : 2;
        for (int y = 0; y < 8; y++) b.set(x, y, c);
    }
    return b;
}

gs::Bitmap wallArt() {
    gs::Bitmap b(48, 40);
    b.rect(0, 8, 48, 32, 1);
    b.poly({{0, 10}, {8, 2}, {24, 8}, {40, 2}, {48, 10}}, 2);
    for (int y = 14; y < 38; y += 8)
        for (int x = 4; x < 44; x += 10) b.rect(x, y, 6, 4, 3);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(40, 48);
    b.rect(17, 22, 6, 26, 1);
    b.ellipse(20, 16, 16, 14, 2);
    b.ellipse(12, 18, 8, 7, 3);
    b.ellipse(28, 14, 7, 6, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 36);
    b.rect(5, 8, 2, 28, 1);
    b.rect(2, 2, 8, 8, 2);
    b.rect(4, 4, 4, 4, 3);
    return b;
}

gs::Bitmap awnArt() {
    gs::Bitmap b(40, 14);
    b.poly({{0, 12}, {4, 2}, {36, 2}, {40, 12}}, 1);
    for (int x = 4; x < 36; x += 8) b.line(float(x), 2, float(x), 12, 2, 1.f);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(14, 9, 12, 6, 1);
    b.ellipse(26, 8, 10, 5, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 6, 6, 1);
    b.ellipse(9, 9, 3, 3, 2);
    return b;
}

gs::Bitmap birdArt(int flap) {
    gs::Bitmap b(16, 8);
    if (flap) {
        b.line(1, 6, 8, 2, 1, 1.2f);
        b.line(8, 2, 15, 6, 1, 1.2f);
    } else {
        b.line(1, 3, 8, 5, 1, 1.2f);
        b.line(8, 5, 15, 2, 1, 1.2f);
    }
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 10);
    for (int y = 0; y < 10; y++)
        for (int x = 0; x < 16; x++)
            if ((hash2(x, y) % 5) == 0) b.set(x, y, 1 + int(hash2(x + 3, y) % 2));
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

    setPal(vdp, PAL_CAB,
           {0, gs::rgb4(12, 3, 2), gs::rgb4(14, 8, 3), gs::rgb4(10, 2, 2), gs::rgb4(6, 10, 13), gs::rgb4(3, 6, 8),
            gs::rgb4(2, 2, 2), gs::rgb4(8, 6, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_STONE,
           {0, gs::rgb4(8, 8, 7), gs::rgb4(12, 12, 10), gs::rgb4(5, 5, 4), gs::rgb4(6, 6, 5)});
    setPal(vdp, PAL_ROAD,
           {0, gs::rgb4(6, 6, 5), gs::rgb4(4, 4, 3), gs::rgb4(8, 7, 5), gs::rgb4(9, 8, 4)});
    setPal(vdp, PAL_WALL,
           {0, gs::rgb4(11, 8, 5), gs::rgb4(13, 6, 4), gs::rgb4(7, 5, 3)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(6, 4, 2), gs::rgb4(2, 8, 3), gs::rgb4(4, 11, 4)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(14, 14, 15), gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 12, 2), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_MAN,
           {0, gs::rgb4(12, 8, 5), gs::rgb4(2, 5, 10), gs::rgb4(3, 3, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(10, 9, 6), gs::rgb4(8, 7, 5)});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(3, 3, 3), gs::rgb4(12, 10, 4), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_AWN, {0, gs::rgb4(12, 2, 3), gs::rgb4(15, 14, 12)});

    art.cab = gs::uploadMipped(vdp, cabArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.pull[0] = gs::uploadMipped(vdp, pullArt(0));
    art.pull[1] = gs::uploadMipped(vdp, pullArt(1));
    art.rider = gs::uploadMipped(vdp, riderArt());
    art.stone = gs::uploadMipped(vdp, stoneArt());
    art.lip = gs::uploadMipped(vdp, lipArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.wall = gs::uploadMipped(vdp, wallArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.awn = gs::uploadMipped(vdp, awnArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(0));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(1));
    art.dust = gs::uploadMipped(vdp, dustArt());
    loadFont(vdp, art);
}

}  // namespace rickplat
