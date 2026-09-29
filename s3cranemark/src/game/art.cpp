#include "game/art.h"

#include "game/world.h"

#include <string>

namespace cranemark {
namespace {

void textPal(gs::VDP& v, int pal, uint16_t ink) {
    v.setColor(pal * 16 + 1, ink);
    v.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void paintPals(gs::VDP& vdp) {
    textPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 14));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 11, 3));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(6, 15, 7));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 13, 6));
    textPal(vdp, PAL_END, gs::rgb4(15, 6, 4));
    vdp.setColor(PAL_END * 16 + 2, gs::rgb4(8, 7, 6));

    const uint16_t yard[16] = {
        0,
        gs::rgb4(3, 5, 10),    // dawn sky
        gs::rgb4(8, 7, 12),    // sky
        gs::rgb4(14, 10, 7),   // horizon
        gs::rgb4(15, 13, 10),  // cloud
        gs::rgb4(1, 3, 7),     // water deep
        gs::rgb4(2, 6, 10),    // water
        gs::rgb4(8, 12, 13),   // foam
        gs::rgb4(7, 6, 5),     // pier
        gs::rgb4(4, 3, 3),     // pier shadow
        gs::rgb4(4, 6, 8),     // hull
        gs::rgb4(2, 4, 6),     // hull dark
        gs::rgb4(15, 12, 2),   // mark paint
        gs::rgb4(10, 11, 12),  // steel
        gs::rgb4(14, 14, 13),  // steel light
        gs::rgb4(2, 1, 2),     // ink
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_YARD * 16 + i, yard[i]);

    auto cab = [&](int pal, uint16_t glass, uint16_t stripe) {
        vdp.setColor(pal * 16 + 1, gs::rgb4(14, 14, 13));
        vdp.setColor(pal * 16 + 2, gs::rgb4(7, 8, 9));
        vdp.setColor(pal * 16 + 3, gs::rgb4(3, 4, 5));
        vdp.setColor(pal * 16 + 4, glass);
        vdp.setColor(pal * 16 + 5, gs::rgb4(5, 6, 7));
        vdp.setColor(pal * 16 + 6, stripe);
        vdp.setColor(pal * 16 + 7, gs::rgb4(1, 1, 2));
        vdp.setColor(pal * 16 + 8, gs::rgb4(12, 7, 3));
    };
    cab(PAL_CRANE, gs::rgb4(8, 13, 15), gs::rgb4(15, 8, 2));
    cab(PAL_SET, gs::rgb4(7, 15, 8), gs::rgb4(5, 14, 4));

    vdp.setColor(PAL_HOOK * 16 + 1, gs::rgb4(13, 13, 14));
    vdp.setColor(PAL_HOOK * 16 + 2, gs::rgb4(5, 5, 6));
    vdp.setColor(PAL_HOOK * 16 + 3, gs::rgb4(15, 10, 2));
    vdp.setColor(PAL_HOOK * 16 + 4, gs::rgb4(2, 2, 3));

    vdp.setColor(PAL_WOOD * 16 + 1, gs::rgb4(13, 9, 4));
    vdp.setColor(PAL_WOOD * 16 + 2, gs::rgb4(9, 6, 3));
    vdp.setColor(PAL_WOOD * 16 + 3, gs::rgb4(5, 3, 2));
    vdp.setColor(PAL_WOOD * 16 + 4, gs::rgb4(12, 12, 11));
    vdp.setColor(PAL_WOOD * 16 + 5, gs::rgb4(2, 2, 2));
    vdp.setColor(PAL_WOOD * 16 + 6, gs::rgb4(15, 12, 2));

    vdp.setColor(PAL_CABLE * 16 + 1, gs::rgb4(2, 2, 3));
    vdp.setColor(PAL_CABLE * 16 + 2, gs::rgb4(9, 9, 8));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& art) {
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
        art.font[c - 32] = t;
    }
}

gs::Bitmap crateArt() {
    gs::Bitmap b(26, 20);
    b.rect(0, 0, 26, 20, 3);
    b.rect(1, 1, 24, 18, 2);
    b.rect(2, 2, 22, 7, 1);
    b.rect(1, 11, 24, 3, 6);
    b.rect(2, 3, 2, 2, 5);
    b.rect(22, 3, 2, 2, 5);
    b.rect(2, 15, 2, 2, 5);
    b.rect(22, 15, 2, 2, 5);
    gs::TextStyle st{1, 4, 0, 0, 1};
    gs::Bitmap num = gs::textBitmap("M", st);
    b.blit(num, 10, 2);
    return b;
}

gs::Bitmap trolleyArt() {
    gs::Bitmap b(40, 26);
    b.rect(3, 0, 8, 5, 3);
    b.rect(29, 0, 8, 5, 3);
    b.rect(5, 1, 4, 3, 1);
    b.rect(31, 1, 4, 3, 1);
    b.rect(0, 4, 40, 5, 2);
    b.rect(0, 4, 40, 1, 1);
    b.rect(6, 6, 6, 3, 6);
    b.rect(8, 9, 24, 15, 5);
    b.rect(11, 11, 12, 8, 4);
    b.rect(24, 11, 6, 8, 6);
    b.rect(13, 13, 3, 2, 7);
    b.rect(17, 22, 4, 4, 3);
    b.rect(10, 22, 2, 4, 8);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(16, 16);
    b.rect(6, 0, 4, 4, 1);
    b.rect(7, 3, 2, 5, 2);
    b.poly({{2, 8}, {14, 8}, {12, 12}, {4, 12}}, 3);
    b.rect(3, 12, 10, 3, 4);
    b.rect(5, 9, 2, 2, 1);
    b.rect(9, 9, 2, 2, 1);
    return b;
}

gs::Bitmap linkArt() {
    gs::Bitmap b(4, 6);
    b.rect(1, 0, 2, 6, 1);
    b.set(0, 2, 2);
    b.set(3, 3, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(26, 8);
    b.ellipse(13, 4, 11, 3, 1);
    return b;
}

gs::Bitmap pennantArt() {
    gs::Bitmap b(10, 16);
    b.rect(1, 0, 2, 16, 2);
    b.poly({{3, 2}, {9, 5}, {3, 8}}, 1);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(16, 7);
    b.line(0, 4, 7, 1, 1, 1);
    b.line(7, 1, 15, 5, 1, 1);
    b.set(7, 2, 1);
    return b;
}

gs::Bitmap yardPicture() {
    gs::Bitmap b(320, 224);
    for (int y = 0; y < int(WATER_Y); y++) {
        int c = y < 40 ? 1 : y < 100 ? 2 : 3;
        b.rect(0, float(y), 320, 1, c);
    }
    b.ellipse(48, 36, 22, 8, 4);
    b.ellipse(70, 32, 14, 6, 4);
    b.ellipse(210, 28, 18, 7, 4);
    b.ellipse(250, 44, 10, 10, 12);
    b.ellipse(250, 44, 5, 5, 3);

    b.rect(0, WATER_Y, 320, 224 - WATER_Y, 6);
    for (int y = int(WATER_Y); y < 224; y += 6) b.rect(0, float(y), 320, 2, (y / 6) % 2 ? 5 : 7);

    b.poly({{SHIP_L + 10, DECK_Y + 14},
            {SHIP_R - 2, DECK_Y + 14},
            {SHIP_R - 10, 222},
            {SHIP_L + 4, 222},
            {SHIP_L - 8, DECK_Y + 50}},
           10);
    b.rect(SHIP_L + 18, DECK_Y + 26, SHIP_R - SHIP_L - 48, 7, 11);
    for (int i = 0; i < 3; i++) b.ellipse(SHIP_L + 40 + i * 28, DECK_Y + 42, 4, 4, 3);
    b.rect(SHIP_L, DECK_Y, SHIP_R - SHIP_L, 16, 14);
    b.rect(MARK_X - MARK_HALF, DECK_Y, MARK_HALF * 2, 16, 12);
    b.rect(MARK_X - MARK_HALF, DECK_Y, 2, 16, 15);
    b.rect(MARK_X + MARK_HALF - 2, DECK_Y, 2, 16, 15);
    b.rect(MARK_X - 2, DECK_Y, 4, 16, 15);
    gs::TextStyle ink{1, 15, 0, 0, 1};
    gs::Bitmap word = gs::textBitmap("MARK", ink);
    b.blit(word, int(MARK_X) - word.w / 2, int(DECK_Y) + 4);

    b.rect(276, 112, 32, 40, 11);
    b.rect(280, 118, 10, 8, 12);
    b.rect(294, 118, 8, 8, 12);
    b.rect(286, 92, 12, 22, 10);
    b.rect(282, 86, 20, 7, 15);

    for (int i = 0; i < 3; i++) b.rect(PIER_L + 14 + i * 32, PIER_Y + 8, 7, 224 - (PIER_Y + 8), 9);
    b.rect(PIER_L, PIER_Y, PIER_R - PIER_L, 10, 8);
    b.rect(PIER_L, PIER_Y, PIER_R - PIER_L, 2, 14);
    b.rect(PIER_L, PIER_Y + 10, PIER_R - PIER_L, 14, 9);
    gs::Bitmap pier = gs::textBitmap("PIER", ink);
    b.blit(pier, int(PIER_L) + 8, int(PIER_Y) + 12);

    b.rect(140, WATER_Y - 8, 3, 10, 15);
    b.ellipse(141, WATER_Y + 3, 6, 4, 12);

    b.rect(12, 16, 10, PIER_Y - 8, 13);
    b.rect(300, 16, 8, DECK_Y - 8, 13);
    for (int i = 0; i < 5; i++) {
        float y = 36.f + i * 22.f;
        b.line(14, y, 20, y + 12, 15, 1);
        b.line(20, y, 14, y + 12, 15, 1);
    }
    b.rect(10, 16, 300, 14, 13);
    b.rect(10, 16, 300, 3, 14);
    for (int i = 0; i < 8; i++) b.rect(22 + i * 5, 24, 2, 5, i % 2 ? 12 : 15);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    paintPals(vdp);
    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, yardPicture(), PAL_YARD);

    art.trolley = gs::uploadMipped(vdp, trolleyArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.link = gs::uploadMipped(vdp, linkArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.pennant = gs::uploadMipped(vdp, pennantArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.crate = gs::uploadMipped(vdp, crateArt());

    gs::TextStyle banner{3, 1, 15, 0, 1};
    art.banner = gs::uploadMipped(vdp, gs::textBitmap("CRANE MARK", banner));
}

}  // namespace cranemark
