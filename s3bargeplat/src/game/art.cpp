#include "game/art.h"

#include <string>

namespace bargeplat {
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

gs::Bitmap hullArt() {
    gs::Bitmap b(180, 48);
    b.poly({{6, 22}, {16, 38}, {164, 40}, {172, 28}, {160, 16}, {12, 16}}, 1);
    b.poly({{18, 20}, {158, 20}, {168, 30}, {160, 38}, {20, 36}}, 2);
    b.rect(20, 16, 136, 5, 3);
    b.rect(22, 34, 132, 3, 4);
    b.rect(28, 8, 22, 8, 5);
    b.rect(54, 6, 18, 10, 5);
    b.rect(58, 8, 6, 5, 6);
    b.rect(96, 10, 28, 6, 7);
    b.line(160, 16, 170, 28, 8, 1.5f);
    b.outline(8, false);
    return b.cropToContent(1);
}

gs::Bitmap crateArt() {
    gs::Bitmap b(22, 18);
    b.rect(1, 2, 20, 14, 1);
    b.rect(1, 2, 20, 3, 2);
    b.line(1, 9, 20, 9, 3, 1.f);
    b.line(11, 2, 11, 15, 3, 1.f);
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
    gs::Bitmap b(28, 12);
    for (int x = 0; x < 28; x++) {
        int c = ((x / 4) & 1) ? 1 : 2;
        for (int y = 0; y < 12; y++) b.set(x, y, c);
    }
    return b;
}

gs::Bitmap pileArt() {
    gs::Bitmap b(14, 52);
    b.rect(4, 0, 6, 52, 1);
    b.rect(5, 2, 2, 48, 2);
    b.rect(2, 0, 10, 4, 3);
    for (int y = 12; y < 50; y += 9) b.rect(3, y, 8, 2, 3);
    return b;
}

gs::Bitmap waterArt() {
    gs::Bitmap b(32, 24);
    for (int y = 0; y < 24; y++)
        for (int x = 0; x < 32; x++) {
            uint32_t h = hash2(x, y);
            int c = 1 + int(h % 3);
            if ((y + x / 5) % 7 == 0) c = 4;
            if (y < 2) c = 5;
            b.set(x, y, c);
        }
    return b;
}

gs::Bitmap bankArt() {
    gs::Bitmap b(96, 36);
    b.poly({{0, 34}, {10, 20}, {28, 24}, {46, 10}, {64, 18}, {80, 12}, {96, 34}}, 1);
    b.rect(0, 28, 96, 8, 2);
    b.rect(0, 32, 96, 4, 3);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(60, 22);
    b.ellipse(16, 13, 11, 6, 1);
    b.ellipse(32, 9, 15, 8, 1);
    b.ellipse(46, 13, 11, 6, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 7, 7, 1);
    b.ellipse(13, 13, 3, 3, 2);
    return b;
}

gs::Bitmap craneArt() {
    gs::Bitmap b(48, 40);
    b.rect(6, 28, 10, 12, 1);
    b.rect(8, 18, 6, 12, 2);
    b.poly({{10, 20}, {44, 6}, {44, 10}, {12, 24}}, 3);
    b.line(42, 8, 42, 22, 4, 1.2f);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 24);
    b.rect(5, 8, 2, 16, 3);
    b.ellipse(6, 6, 4, 3, 1);
    b.ellipse(6, 6, 2, 1, 2);
    return b;
}

gs::Bitmap foamArt() {
    gs::Bitmap b(18, 8);
    b.ellipse(9, 5, 7, 2, 1);
    b.ellipse(5, 4, 3, 2, 2);
    return b;
}

gs::Bitmap wakeArt() {
    gs::Bitmap b(34, 8);
    b.ellipse(16, 4, 14, 3, 1);
    b.ellipse(8, 4, 5, 2, 2);
    return b;
}

gs::Bitmap birdArt(int flap) {
    gs::Bitmap b(18, 10);
    if (flap) {
        b.line(1, 7, 8, 3, 1, 1.2f);
        b.line(8, 3, 16, 7, 1, 1.2f);
    } else {
        b.line(1, 4, 8, 5, 1, 1.2f);
        b.line(8, 5, 16, 3, 1, 1.2f);
    }
    b.set(8, 5, 2);
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

    setPal(vdp, PAL_HULL,
           {0, gs::rgb4(4, 5, 4), gs::rgb4(7, 8, 6), gs::rgb4(11, 9, 5), gs::rgb4(8, 2, 2), gs::rgb4(5, 6, 7),
            gs::rgb4(9, 12, 14), gs::rgb4(9, 6, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_QUAY, {0, gs::rgb4(9, 7, 4), gs::rgb4(13, 11, 7), gs::rgb4(5, 4, 2), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(1, 5, 8), gs::rgb4(2, 7, 10), gs::rgb4(1, 4, 6), gs::rgb4(6, 11, 12),
                            gs::rgb4(9, 13, 14)});
    setPal(vdp, PAL_PILE, {0, gs::rgb4(5, 5, 4), gs::rgb4(8, 8, 6), gs::rgb4(3, 3, 2)});
    setPal(vdp, PAL_BANK, {0, gs::rgb4(4, 7, 4), gs::rgb4(3, 6, 3), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 13, 6), gs::rgb4(14, 14, 12)});
    setPal(vdp, PAL_MARK, {0, gs::rgb4(15, 14, 4), gs::rgb4(2, 2, 1), gs::rgb4(14, 4, 2), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_CRANE, {0, gs::rgb4(6, 6, 6), gs::rgb4(10, 10, 9), gs::rgb4(12, 8, 3), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(13, 15, 14), gs::rgb4(8, 12, 12)});
    setPal(vdp, PAL_BIRD, {0, gs::rgb4(14, 14, 13), gs::rgb4(15, 10, 3)});
    setPal(vdp, PAL_CRATE, {0, gs::rgb4(11, 8, 4), gs::rgb4(14, 12, 7), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(10, 14, 14), gs::rgb4(15, 15, 15)});

    loadFont(vdp, art);
    art.hull = gs::uploadMipped(vdp, hullArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.bank = gs::uploadMipped(vdp, bankArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.crane = gs::uploadMipped(vdp, craneArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
    art.bird[0] = gs::uploadMipped(vdp, birdArt(0));
    art.bird[1] = gs::uploadMipped(vdp, birdArt(1));
}

}  // namespace bargeplat
