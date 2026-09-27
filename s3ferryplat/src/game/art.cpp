#include "game/art.h"

#include <string>

namespace ferryplat {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 2, 3));
}

uint32_t hash2(int x, int y) {
    uint32_t h = uint32_t(x) * 374761393u + uint32_t(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

gs::Bitmap ferryArt() {
    gs::Bitmap b(168, 64);
    // Hull, white band, red boot topping, bow rake.
    b.poly({{8, 36}, {18, 48}, {148, 50}, {156, 40}, {150, 28}, {14, 28}}, 1);
    b.poly({{16, 30}, {148, 31}, {154, 40}, {150, 48}, {20, 46}}, 2);
    b.rect(22, 28, 120, 6, 3);
    b.poly({{148, 28}, {162, 22}, {158, 34}, {148, 36}}, 1);
    b.rect(24, 46, 118, 3, 4);
    // Deck and wheelhouse.
    b.rect(36, 18, 78, 12, 5);
    b.rect(48, 8, 36, 12, 6);
    b.rect(54, 11, 8, 6, 7);
    b.rect(66, 11, 8, 6, 7);
    b.rect(40, 22, 10, 6, 7);
    b.rect(56, 22, 10, 6, 7);
    b.rect(72, 22, 10, 6, 7);
    b.rect(88, 22, 10, 6, 7);
    // Funnel.
    b.rect(96, 4, 14, 16, 8);
    b.rect(96, 4, 14, 4, 9);
    b.ellipse(103, 3, 6, 3, 10);
    // Cars on the open deck.
    b.rect(28, 22, 10, 6, 11);
    b.rect(112, 22, 12, 6, 12);
    b.rect(126, 23, 10, 5, 11);
    // Bow door seam and name band.
    b.line(150, 28, 156, 38, 13, 1.5f);
    b.rect(60, 36, 28, 4, 9);
    b.outline(13, false);
    return b.cropToContent(1);
}

gs::Bitmap plankArt() {
    gs::Bitmap b(40, 14);
    b.rect(0, 0, 40, 14, 1);
    for (int x = 0; x < 40; x++) {
        b.set(x, 0, 2);
        b.set(x, 13, 3);
        if (x % 10 == 0)
            for (int y = 0; y < 14; y++) b.set(x, y, 4);
    }
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(32, 14);
    for (int x = 0; x < 32; x++) {
        int c = ((x / 4) & 1) ? 1 : 2;
        for (int y = 0; y < 14; y++) b.set(x, y, c);
    }
    return b;
}

gs::Bitmap pileArt() {
    gs::Bitmap b(16, 48);
    b.rect(5, 0, 6, 48, 1);
    b.rect(6, 2, 2, 44, 2);
    b.rect(3, 0, 10, 4, 3);
    for (int y = 10; y < 46; y += 8) b.rect(4, y, 8, 2, 3);
    return b;
}

gs::Bitmap waterArt() {
    gs::Bitmap b(32, 24);
    for (int y = 0; y < 24; y++)
        for (int x = 0; x < 32; x++) {
            uint32_t h = hash2(x, y);
            int c = 1 + int(h % 3);
            if ((y + x / 4) % 6 == 0) c = 4;
            if (y < 2) c = 5;
            b.set(x, y, c);
        }
    return b;
}

gs::Bitmap hillArt() {
    gs::Bitmap b(96, 40);
    b.poly({{0, 38}, {14, 22}, {30, 28}, {48, 8}, {66, 20}, {82, 12}, {96, 38}}, 1);
    b.poly({{44, 14}, {54, 8}, {64, 16}, {50, 18}}, 2);
    b.rect(0, 34, 96, 6, 3);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(64, 24);
    b.ellipse(16, 14, 12, 7, 1);
    b.ellipse(32, 10, 16, 9, 1);
    b.ellipse(48, 14, 12, 7, 1);
    b.ellipse(30, 14, 12, 5, 2);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 7, 7, 1);
    b.ellipse(14, 14, 4, 4, 2);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(40, 32);
    b.poly({{2, 14}, {20, 2}, {38, 14}}, 1);
    b.rect(4, 14, 32, 16, 2);
    b.rect(16, 20, 8, 10, 3);
    b.rect(8, 18, 6, 5, 4);
    b.rect(26, 18, 6, 5, 4);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 26);
    b.rect(6, 8, 2, 18, 3);
    b.ellipse(7, 7, 4, 4, 1);
    b.ellipse(7, 7, 2, 2, 2);
    return b;
}

gs::Bitmap signArt() {
    gs::Bitmap b(22, 28);
    b.rect(10, 12, 2, 16, 3);
    b.rect(2, 2, 18, 12, 1);
    b.rect(4, 4, 14, 8, 2);
    return b;
}

gs::Bitmap gullArt(int frame) {
    gs::Bitmap b(28, 14);
    float dip = frame ? 3.f : 0.f;
    b.poly({{1, 7}, {12, 5 + dip}, {14, 7}, {12, 8}}, 1);
    b.poly({{27, 7}, {16, 5 + dip}, {14, 7}, {16, 8}}, 1);
    b.ellipse(14, 7, 2, 2, 2);
    return b;
}

gs::Bitmap foamArt() {
    gs::Bitmap b(20, 10);
    b.ellipse(10, 6, 8, 3, 1);
    b.ellipse(6, 5, 3, 2, 2);
    return b;
}

gs::Bitmap wakeArt() {
    gs::Bitmap b(36, 10);
    b.ellipse(18, 5, 16, 3, 1);
    b.ellipse(10, 5, 6, 2, 2);
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
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 4));
    textPal(vdp, PAL_BAD, gs::rgb4(15, 5, 3));
    textPal(vdp, PAL_GOOD, gs::rgb4(6, 15, 8));

    setPal(vdp, PAL_SHIP,
           {0, gs::rgb4(2, 4, 8), gs::rgb4(14, 14, 13), gs::rgb4(15, 15, 15), gs::rgb4(12, 2, 2), gs::rgb4(9, 10, 11),
            gs::rgb4(13, 14, 15), gs::rgb4(6, 10, 13), gs::rgb4(12, 3, 2), gs::rgb4(15, 12, 3), gs::rgb4(4, 4, 4),
            gs::rgb4(3, 6, 10), gs::rgb4(10, 8, 3), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_PIER, {0, gs::rgb4(10, 7, 4), gs::rgb4(13, 10, 6), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(2, 6, 11), gs::rgb4(3, 8, 13), gs::rgb4(1, 5, 9), gs::rgb4(8, 12, 14),
                            gs::rgb4(10, 14, 15)});
    setPal(vdp, PAL_PILE, {0, gs::rgb4(6, 6, 6), gs::rgb4(9, 9, 8), gs::rgb4(4, 4, 4)});
    setPal(vdp, PAL_HILL, {0, gs::rgb4(5, 7, 9), gs::rgb4(12, 13, 14), gs::rgb4(3, 6, 5)});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 15), gs::rgb4(14, 14, 15), gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 11)});
    setPal(vdp, PAL_END, {0, gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 1), gs::rgb4(13, 3, 2), gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_SHED, {0, gs::rgb4(8, 3, 2), gs::rgb4(12, 10, 7), gs::rgb4(3, 3, 4), gs::rgb4(8, 12, 14)});
    setPal(vdp, PAL_FOAM, {0, gs::rgb4(14, 15, 15), gs::rgb4(10, 13, 14)});
    setPal(vdp, PAL_GULL, {0, gs::rgb4(15, 15, 15), gs::rgb4(15, 10, 3)});
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(3, 5, 3), gs::rgb4(14, 14, 10), gs::rgb4(5, 4, 3)});
    setPal(vdp, PAL_WAKE, {0, gs::rgb4(12, 14, 15), gs::rgb4(15, 15, 15)});

    loadFont(vdp, art);
    art.ferry = gs::uploadMipped(vdp, ferryArt());
    art.plank = gs::uploadMipped(vdp, plankArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
    art.pile = gs::uploadMipped(vdp, pileArt());
    art.water = gs::uploadMipped(vdp, waterArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.sign = gs::uploadMipped(vdp, signArt());
    art.gull[0] = gs::uploadMipped(vdp, gullArt(0));
    art.gull[1] = gs::uploadMipped(vdp, gullArt(1));
    art.foam = gs::uploadMipped(vdp, foamArt());
    art.wake = gs::uploadMipped(vdp, wakeArt());
}

}  // namespace ferryplat
