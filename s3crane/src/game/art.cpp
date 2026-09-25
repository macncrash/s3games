#include "game/art.h"

#include "game/world.h"

#include <string>

namespace crane {
namespace {

void textPal(gs::VDP& v, int pal, uint16_t ink) {
    v.setColor(pal * 16 + 1, ink);
    v.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

void paintPals(gs::VDP& vdp) {
    textPal(vdp, PAL_WHITE, gs::rgb4(15, 15, 15));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3));
    textPal(vdp, PAL_GREEN, gs::rgb4(7, 15, 6));
    textPal(vdp, PAL_BANNER, gs::rgb4(15, 14, 8));

    const uint16_t yard[16] = {
        0,
        gs::rgb4(4, 8, 13),   // sky top
        gs::rgb4(6, 11, 15),  // sky
        gs::rgb4(11, 14, 15), // haze
        gs::rgb4(15, 15, 15), // cloud
        gs::rgb4(2, 5, 10),   // water deep
        gs::rgb4(3, 8, 13),   // water
        gs::rgb4(9, 14, 15),  // foam
        gs::rgb4(9, 9, 8),    // pier
        gs::rgb4(5, 5, 4),    // pier shadow
        gs::rgb4(11, 3, 2),   // hull
        gs::rgb4(7, 2, 2),    // hull dark
        gs::rgb4(15, 12, 2),  // mark
        gs::rgb4(8, 9, 10),   // steel
        gs::rgb4(12, 13, 14), // steel light
        gs::rgb4(2, 2, 3),    // ink
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_YARD * 16 + i, yard[i]);

    auto steel = [&](int pal, uint16_t win, uint16_t stripe) {
        vdp.setColor(pal * 16 + 1, gs::rgb4(13, 14, 15));
        vdp.setColor(pal * 16 + 2, gs::rgb4(8, 9, 11));
        vdp.setColor(pal * 16 + 3, gs::rgb4(4, 5, 6));
        vdp.setColor(pal * 16 + 4, win);
        vdp.setColor(pal * 16 + 5, gs::rgb4(6, 8, 10));
        vdp.setColor(pal * 16 + 6, stripe);
        vdp.setColor(pal * 16 + 7, gs::rgb4(1, 1, 2));
        vdp.setColor(pal * 16 + 8, gs::rgb4(12, 8, 5));
    };
    steel(PAL_CRANE, gs::rgb4(10, 14, 15), gs::rgb4(15, 12, 2));
    steel(PAL_SET, gs::rgb4(8, 15, 8), gs::rgb4(6, 14, 5));

    vdp.setColor(PAL_HOOK * 16 + 1, gs::rgb4(14, 14, 15));
    vdp.setColor(PAL_HOOK * 16 + 2, gs::rgb4(5, 6, 7));
    vdp.setColor(PAL_HOOK * 16 + 3, gs::rgb4(15, 12, 2));
    vdp.setColor(PAL_HOOK * 16 + 4, gs::rgb4(2, 2, 3));

    vdp.setColor(PAL_WOOD * 16 + 1, gs::rgb4(14, 11, 6));
    vdp.setColor(PAL_WOOD * 16 + 2, gs::rgb4(11, 8, 4));
    vdp.setColor(PAL_WOOD * 16 + 3, gs::rgb4(7, 5, 2));
    vdp.setColor(PAL_WOOD * 16 + 4, gs::rgb4(9, 9, 10));
    vdp.setColor(PAL_WOOD * 16 + 5, gs::rgb4(3, 3, 3));
    vdp.setColor(PAL_WOOD * 16 + 6, gs::rgb4(8, 2, 2));

    vdp.setColor(PAL_CABLE * 16 + 1, gs::rgb4(3, 3, 4));
    vdp.setColor(PAL_CABLE * 16 + 2, gs::rgb4(8, 8, 9));

    vdp.setColor(PAL_SPLASH * 16 + 1, gs::rgb4(12, 15, 15));
    vdp.setColor(PAL_SPLASH * 16 + 2, gs::rgb4(7, 12, 15));
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

gs::Bitmap crateArt(char digit) {
    gs::Bitmap b(28, 22);
    b.rect(0, 0, 28, 22, 3);
    b.rect(1, 1, 26, 20, 2);
    b.rect(2, 2, 24, 8, 1);
    b.rect(1, 13, 26, 3, 4);
    b.rect(3, 4, 2, 2, 5);
    b.rect(23, 4, 2, 2, 5);
    b.rect(3, 16, 2, 2, 5);
    b.rect(23, 16, 2, 2, 5);
    gs::TextStyle st{1, 6, 0, 0, 1};
    gs::Bitmap num = gs::textBitmap(std::string(1, digit), st);
    b.blit(num, 12, 3);
    return b;
}

gs::Bitmap trolleyArt() {
    gs::Bitmap b(36, 24);
    b.rect(2, 0, 9, 5, 3);
    b.rect(25, 0, 9, 5, 3);
    b.rect(4, 1, 5, 3, 1);
    b.rect(27, 1, 5, 3, 1);
    b.rect(0, 4, 36, 5, 2);
    b.rect(0, 4, 36, 1, 1);
    b.rect(7, 9, 22, 13, 5);
    b.rect(10, 11, 10, 7, 4);
    b.rect(21, 11, 5, 7, 6);
    b.rect(12, 12, 4, 2, 7);
    b.set(13, 14, 8);
    b.set(14, 14, 8);
    b.rect(16, 20, 4, 4, 3);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(16, 18);
    b.rect(6, 0, 4, 5, 1);
    b.rect(7, 4, 2, 5, 2);
    b.poly({{2, 9}, {14, 9}, {12, 14}, {4, 14}}, 3);
    b.rect(3, 14, 10, 3, 4);
    b.rect(5, 11, 2, 2, 1);
    b.rect(9, 11, 2, 2, 1);
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
    gs::Bitmap b(28, 8);
    b.ellipse(14, 4, 12, 3, 1);
    return b;
}

gs::Bitmap splashArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(20, 11, 16, 5, 1);
    b.ellipse(8, 8, 6, 3, 2);
    b.ellipse(32, 8, 6, 3, 2);
    b.line(20, 2, 20, 8, 1, 1);
    return b;
}

gs::Bitmap arrowArt() {
    gs::Bitmap b(12, 10);
    b.poly({{6, 9}, {1, 1}, {11, 1}}, 1);
    return b;
}

gs::Bitmap birdArt() {
    gs::Bitmap b(18, 8);
    b.line(0, 5, 8, 1, 1, 1);
    b.line(8, 1, 17, 6, 1, 1);
    b.set(8, 2, 1);
    b.set(8, 3, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(8, 14);
    b.ellipse(4, 4, 3, 3, 3);
    b.rect(3, 6, 2, 8, 2);
    return b;
}

gs::Bitmap yardPicture() {
    gs::Bitmap b(320, 224);
    for (int y = 0; y < int(WATER_Y); y++) {
        int c = y < 48 ? 1 : y < 110 ? 2 : 3;
        b.rect(0, float(y), 320, 1, c);
    }
    b.ellipse(200, 48, 11, 11, 12);
    b.ellipse(200, 48, 6, 6, 4);
    b.ellipse(46, 58, 26, 10, 4);
    b.ellipse(68, 54, 18, 8, 4);
    b.ellipse(130, 78, 22, 9, 4);
    b.ellipse(270, 70, 20, 8, 4);

    b.rect(0, WATER_Y, 320, 224 - WATER_Y, 6);
    for (int y = int(WATER_Y); y < 224; y += 5) b.rect(0, float(y), 320, 2, (y / 5) % 2 ? 5 : 7);

    // Hull, then the deck and the painted mark on its face.
    b.poly({{SHIP_L + 16, DECK_Y + 16},
            {SHIP_R - 4, DECK_Y + 16},
            {SHIP_R - 8, 222},
            {SHIP_L + 6, 222},
            {SHIP_L - 6, DECK_Y + 48}},
           10);
    b.rect(SHIP_L + 20, DECK_Y + 28, SHIP_R - SHIP_L - 40, 8, 11);
    for (int i = 0; i < 4; i++) b.ellipse(SHIP_L + 36 + i * 22, DECK_Y + 46, 4, 4, 3);
    b.rect(SHIP_L, DECK_Y, SHIP_R - SHIP_L, 18, 14);
    b.rect(MARK_X - MARK_HALF, DECK_Y, MARK_HALF * 2, 18, 12);
    b.rect(MARK_X - MARK_HALF, DECK_Y, MARK_HALF * 2, 2, 15);
    gs::TextStyle ink{1, 15, 0, 0, 1};
    gs::Bitmap mark = gs::textBitmap("MARK", ink);
    b.blit(mark, int(MARK_X) - mark.w / 2, int(DECK_Y) + 3);

    // Cabin and funnel, clear of the mark.
    b.rect(268, 108, 40, 40, 11);
    b.rect(274, 116, 12, 10, 12);
    b.rect(292, 116, 10, 10, 12);
    b.rect(286, 78, 16, 32, 10);
    b.rect(282, 72, 24, 8, 15);
    gs::TextStyle plate{2, 12, 0, 0, 1};
    gs::Bitmap name = gs::textBitmap("S3", plate);
    b.blit(name, int(SHIP_L) + 24, int(DECK_Y) + 32);

    // Pier slab, face, and pilings.
    for (int i = 0; i < 4; i++) b.rect(DOCK_L + 12 + i * 30, DOCK_Y + 8, 8, 224 - (DOCK_Y + 8), 9);
    b.rect(DOCK_L, DOCK_Y, DOCK_R - DOCK_L, 12, 8);
    b.rect(DOCK_L, DOCK_Y, DOCK_R - DOCK_L, 3, 14);
    b.rect(DOCK_L, DOCK_Y + 12, DOCK_R - DOCK_L, 16, 9);
    gs::Bitmap pier = gs::textBitmap("PIER", ink);
    b.blit(pier, int(DOCK_L) + 10, int(DOCK_Y) + 16);

    // Buoy in the gap between pier and ship.
    b.rect(150, WATER_Y - 10, 4, 12, 15);
    b.ellipse(152, WATER_Y + 4, 7, 5, 12);

    // Gantry towers and the rail the trolley rides.
    b.rect(10, 20, 12, DOCK_Y - 12, 13);
    b.rect(304, 20, 10, DECK_Y - 12, 13);
    for (int i = 0; i < 6; i++) {
        float y = 40 + i * 20;
        b.line(12, y, 20, y + 12, 15, 1);
        b.line(20, y, 12, y + 12, 15, 1);
        b.line(306, y, 312, y + 10, 15, 1);
    }
    b.rect(8, 20, 308, 16, 13);
    b.rect(8, 20, 308, 3, 14);
    for (int i = 0; i < 7; i++) b.rect(18 + i * 6, 30, 3, 5, i % 2 ? 12 : 15);
    for (int i = 0; i < 7; i++) b.rect(262 + i * 6, 30, 3, 5, i % 2 ? 12 : 15);
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
    art.splash = gs::uploadMipped(vdp, splashArt());
    art.arrow = gs::uploadMipped(vdp, arrowArt());
    art.bird = gs::uploadMipped(vdp, birdArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    for (int i = 0; i < 3; i++) art.crate[i] = gs::uploadMipped(vdp, crateArt(char('1' + i)));

    gs::TextStyle banner{4, 1, 15, 0, 1};
    art.banner = gs::uploadMipped(vdp, gs::textBitmap("S3 CRANE", banner));
}

}  // namespace crane
