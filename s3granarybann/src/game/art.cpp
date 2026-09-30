#include "game/art.h"

namespace granary {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 8);
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 2;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

gs::Bitmap farmerArt(int step) {
    gs::Bitmap b(40, 56);
    int leg = step ? 6 : 0;
    b.ellipse(20, 14, 7, 7, 1);
    b.rect(12, 6, 16, 5, 4);
    b.rect(26, 8, 7, 3, 4);
    b.rect(14, 20, 12, 16, 3);
    b.rect(15, 22, 10, 6, 7);
    b.rect(10, 22, 5, 12, 2);
    b.rect(25, 22, 5, 12, 2);
    b.rect(15, 36, 5, 12 + (step ? 0 : 2), 5);
    b.rect(21, 36, 5, 12 + leg / 2, 5);
    b.rect(14, 46 + (step ? 0 : 2), 6, 3, 6);
    b.rect(21, 46 + leg / 3, 6, 3, 6);
    b.set(17, 13, 6);
    b.set(22, 13, 6);
    b.outline(6, false);
    (void)leg;
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(36, 64);
    b.rect(16, 4, 3, 56, 4);
    b.rect(8, 8, 22, 28, 1);
    b.rect(8, 8, 22, 6, 2);
    b.rect(8, 20, 22, 4, 3);
    b.rect(14, 12, 4, 16, 5);
    b.ellipse(17, 6, 5, 3, 3);
    b.outline(6, false);
    return b;
}

gs::Bitmap barnArt() {
    gs::Bitmap b(120, 100);
    b.poly({{8, 42}, {60, 6}, {112, 42}}, 1);
    b.poly({{18, 40}, {60, 14}, {102, 40}}, 2);
    b.rect(16, 40, 88, 52, 8);
    b.rect(16, 40, 88, 8, 3);
    for (int x = 20; x < 100; x += 14) b.rect(x, 48, 3, 40, 3);
    b.rect(16, 84, 88, 10, 5);
    b.rect(70, 58, 24, 34, 6);
    b.rect(74, 62, 16, 8, 7);
    b.rect(46, 52, 14, 12, 7);
    b.rect(48, 54, 10, 8, 9);
    b.rect(22, 56, 12, 16, 4);
    b.outline(3, false);
    return b;
}

gs::Bitmap crowArt() {
    gs::Bitmap b(28, 16);
    b.ellipse(12, 8, 8, 4, 1);
    b.poly({{18, 6}, {26, 4}, {20, 9}}, 1);
    b.poly({{6, 8}, {2, 3}, {10, 7}}, 2);
    b.poly({{8, 9}, {4, 14}, {12, 9}}, 2);
    b.set(16, 7, 3);
    return b;
}

gs::Bitmap sackArt() {
    gs::Bitmap b(24, 28);
    b.ellipse(12, 16, 9, 10, 1);
    b.ellipse(12, 8, 5, 4, 2);
    b.line(8, 14, 16, 18, 3, 1.5f);
    b.outline(3, false);
    return b;
}

void groundTiles(gs::VDP& vdp, Art& a) {
    uint8_t wheat[64] = {};
    uint8_t dirt[64] = {};
    for (int i = 0; i < 64; i++) {
        wheat[i] = 1;
        dirt[i] = 4;
    }
    wheat[2] = 2;
    wheat[10] = 2;
    wheat[19] = 3;
    wheat[27] = 2;
    wheat[36] = 3;
    wheat[44] = 2;
    wheat[53] = 3;
    wheat[60] = 2;
    dirt[9] = 5;
    dirt[22] = 5;
    dirt[41] = 5;
    dirt[55] = 1;
    gs::TileAlloc tiles(vdp, 1);
    a.wheat = tiles.alloc(1);
    vdp.loadTile(a.wheat, wheat);
    a.dirt = tiles.alloc(1);
    vdp.loadTile(a.dirt, dirt);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 14);
    const uint16_t shade = gs::rgb4(2, 2, 3);
    setPal(vdp, PAL_HUD, {0, ink, shade, gs::rgb4(15, 12, 4), gs::rgb4(8, 14, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FARM,
           {0, gs::rgb4(13, 9, 6), gs::rgb4(4, 6, 10), gs::rgb4(6, 8, 12), gs::rgb4(8, 5, 2), gs::rgb4(3, 3, 4),
            gs::rgb4(1, 1, 1), gs::rgb4(12, 12, 10), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BANN,
           {0, gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 1), gs::rgb4(14, 11, 3), gs::rgb4(6, 4, 2), gs::rgb4(15, 14, 10),
            gs::rgb4(1, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BARN,
           {0, gs::rgb4(12, 9, 3), gs::rgb4(8, 6, 2), gs::rgb4(5, 3, 2), gs::rgb4(7, 4, 2), gs::rgb4(6, 6, 6),
            gs::rgb4(2, 1, 1), gs::rgb4(10, 12, 13), gs::rgb4(13, 11, 8), gs::rgb4(4, 8, 12), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(10, 8, 3), gs::rgb4(7, 5, 2), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_CROW, {0, gs::rgb4(2, 2, 3), gs::rgb4(4, 4, 5), gs::rgb4(14, 10, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_GROUND,
           {0, gs::rgb4(7, 10, 3), gs::rgb4(10, 12, 4), gs::rgb4(5, 7, 2), gs::rgb4(9, 7, 3), gs::rgb4(7, 5, 2), 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0});

    groundTiles(vdp, art);
    loadFont(vdp, art);
    art.farmer[0] = gs::uploadMipped(vdp, farmerArt(0));
    art.farmer[1] = gs::uploadMipped(vdp, farmerArt(1));
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.barn = gs::uploadMipped(vdp, barnArt());
    art.crow = gs::uploadMipped(vdp, crowArt());
    art.sack = gs::uploadMipped(vdp, sackArt());

    vdp.setFogColor(gs::rgb4(8, 10, 12));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int t = y < 88 ? y : 88;
        int r = 3 + t * 8 / 88;
        int g = 6 + t * 6 / 88;
        int b = 12 - t * 4 / 88;
        if (b < 4) b = 4;
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
}

}  // namespace granary
