#include "game/art.h"

#include <string>

namespace mailplat {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap vanArt() {
    gs::Bitmap b(96, 40);
    b.poly({{4, 22}, {10, 32}, {86, 32}, {92, 22}, {88, 14}, {62, 12}, {54, 6}, {22, 6}, {14, 14}}, 1);
    b.rect(16, 14, 68, 14, 2);
    b.rect(24, 8, 26, 8, 3);
    b.rect(28, 10, 8, 5, 4);
    b.rect(40, 10, 6, 5, 4);
    b.rect(8, 18, 10, 8, 5);
    b.rect(70, 16, 16, 8, 6);
    b.rect(4, 28, 88, 4, 7);
    b.line(54, 6, 62, 14, 8, 1.2f);
    b.outline(8, false);
    return b.cropToContent(1);
}

gs::Bitmap wheelArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 4, 4, 2);
    b.ellipse(9, 9, 1.5f, 1.5f, 3);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(16, 18);
    b.ellipse(8, 10, 6, 7, 1);
    b.rect(5, 2, 6, 4, 2);
    b.line(5, 6, 11, 6, 3, 1.f);
    return b;
}

gs::Bitmap dockArt() {
    gs::Bitmap b(48, 36);
    b.rect(0, 8, 48, 28, 1);
    b.rect(0, 8, 48, 5, 2);
    b.rect(0, 30, 48, 6, 3);
    for (int x = 6; x < 48; x += 10) b.rect(x, 16, 2, 12, 4);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(32, 8);
    for (int x = 0; x < 32; x++) {
        int c = ((x / 4) & 1) ? 1 : 2;
        for (int y = 0; y < 8; y++) b.set(x, y, c);
    }
    return b;
}

gs::Bitmap roadArt() {
    gs::Bitmap b(32, 16);
    b.rect(0, 0, 32, 16, 1);
    for (int x = 0; x < 32; x += 8) b.rect(x, 6, 4, 2, 2);
    b.rect(0, 0, 32, 2, 3);
    return b;
}

gs::Bitmap officeArt() {
    gs::Bitmap b(70, 48);
    b.rect(4, 10, 62, 38, 1);
    b.rect(4, 10, 62, 6, 2);
    b.rect(10, 20, 12, 10, 3);
    b.rect(28, 20, 12, 10, 3);
    b.rect(46, 20, 12, 10, 3);
    b.rect(28, 34, 14, 14, 4);
    b.rect(30, 8, 10, 4, 5);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 28);
    b.rect(5, 8, 2, 20, 1);
    b.ellipse(6, 6, 4, 3, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(48, 16);
    b.ellipse(14, 9, 10, 5, 1);
    b.ellipse(28, 7, 12, 6, 1);
    b.ellipse(38, 10, 8, 4, 1);
    return b;
}

gs::Bitmap birdArt(int flap) {
    gs::Bitmap b(16, 8);
    if (flap) {
        b.line(1, 6, 8, 2, 1, 1.2f);
        b.line(8, 2, 15, 6, 1, 1.2f);
    } else {
        b.line(1, 3, 8, 5, 1, 1.2f);
        b.line(8, 5, 15, 3, 1, 1.2f);
    }
    b.set(8, 4, 2);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(20, 16);
    b.rect(1, 1, 18, 14, 1);
    b.line(1, 8, 18, 8, 2, 1.f);
    b.line(10, 1, 10, 14, 2, 1.f);
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
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 15, 8));

    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(14, 11, 2), gs::rgb4(15, 13, 5), gs::rgb4(8, 10, 12), gs::rgb4(12, 14, 15),
            gs::rgb4(12, 3, 2), gs::rgb4(6, 4, 2), gs::rgb4(13, 2, 2), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_DOCK, {0, gs::rgb4(8, 8, 7), gs::rgb4(12, 12, 10), gs::rgb4(5, 5, 4), gs::rgb4(6, 6, 5)});
    setPal(vdp, PAL_ROAD, {0, gs::rgb4(4, 4, 4), gs::rgb4(14, 12, 3), gs::rgb4(6, 6, 6)});
    setPal(vdp, PAL_SACK, {0, gs::rgb4(10, 7, 3), gs::rgb4(13, 10, 5), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_OFFICE, {0, gs::rgb4(9, 6, 5), gs::rgb4(12, 8, 6), gs::rgb4(6, 9, 12), gs::rgb4(4, 3, 3),
                             gs::rgb4(14, 4, 3)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 3), gs::rgb4(14, 3, 2)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(5, 5, 5), gs::rgb4(15, 14, 6)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(2, 2, 2), gs::rgb4(8, 8, 8), gs::rgb4(12, 11, 8)});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(3, 3, 4), gs::rgb4(14, 10, 3)});
    setPal(vdp, PAL_BOX, {0, gs::rgb4(11, 8, 4), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_STRIPE, {0, gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 2)});

    loadFont(vdp, art);
    art.van = gs::uploadMipped(vdp, vanArt());
    art.wheel = gs::uploadMipped(vdp, wheelArt());
    art.sack = gs::uploadMipped(vdp, sackArt());
    art.dock = gs::uploadMipped(vdp, dockArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.road = gs::uploadMipped(vdp, roadArt());
    art.office = gs::uploadMipped(vdp, officeArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(0));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(1));
    art.crate = gs::uploadMipped(vdp, crateArt());
}

}  // namespace mailplat
