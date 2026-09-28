#include "game/art.h"

namespace markettape {
namespace {

void ink(gs::VDP& v, int p, int r, int g, int b) {
    uint16_t c[16] = {};
    c[1] = gs::rgb4(r, g, b);
    c[15] = gs::rgb4(2, 1, 2);
    for (int i = 0; i < 16; i++) v.setColor(p * 16 + i, c[i]);
}

void paint(gs::VDP& v, int p, int body, int trim, int shade) {
    uint16_t c[16] = {};
    c[1] = gs::rgb4(1, 1, 2);
    c[2] = body;
    c[3] = trim;
    c[4] = shade;
    c[5] = gs::rgb4(15, 14, 10);
    c[6] = gs::rgb4(8, 5, 2);
    for (int i = 0; i < 16; i++) v.setColor(p * 16 + i, c[i]);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        a.font[c - 32] = tiles.shared(px);
    }
}

gs::Bitmap awningArt() {
    gs::Bitmap b(96, 28);
    for (int i = 0; i < 6; i++) {
        int c = (i & 1) ? 2 : 3;
        b.rect(float(4 + i * 15), 4, 15, 12, c);
        b.ellipse(float(11 + i * 15), 18, 8, 6, c);
    }
    b.rect(2, 2, 92, 3, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap clerkArt() {
    gs::Bitmap b(28, 48);
    b.ellipse(14, 10, 7, 7, 5);
    b.rect(10, 8, 3, 2, 1);
    b.rect(16, 8, 3, 2, 1);
    b.rect(8, 18, 12, 14, 2);
    b.rect(6, 18, 3, 10, 5);
    b.rect(19, 18, 3, 10, 5);
    b.rect(10, 32, 3, 12, 6);
    b.rect(15, 32, 3, 12, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(36, 22);
    b.rect(2, 4, 32, 16, 2);
    b.rect(2, 8, 32, 2, 3);
    b.rect(2, 14, 32, 2, 3);
    b.rect(12, 4, 2, 16, 3);
    b.rect(22, 4, 2, 16, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap drawerArt() {
    gs::Bitmap b(120, 36);
    b.rect(2, 2, 116, 32, 6);
    b.rect(6, 6, 108, 24, 4);
    b.rect(54, 14, 12, 6, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap roundFruit(int body, int leaf) {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 12, 7, 7, body);
    b.rect(9, 3, 2, 4, leaf);
    b.ellipse(13, 5, 3, 2, leaf);
    b.outline(1, false);
    return b;
}

gs::Bitmap loafArt() {
    gs::Bitmap b(24, 16);
    b.ellipse(12, 9, 10, 6, 2);
    b.rect(6, 6, 12, 2, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap bunArt() {
    gs::Bitmap b(20, 16);
    b.ellipse(10, 9, 7, 5, 2);
    b.rect(6, 8, 2, 2, 3);
    b.rect(12, 8, 2, 2, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap pearArt() {
    gs::Bitmap b(18, 24);
    b.ellipse(9, 16, 6, 6, 2);
    b.ellipse(9, 9, 4, 4, 2);
    b.rect(8, 2, 2, 4, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap figArt() {
    gs::Bitmap b(16, 18);
    b.ellipse(8, 11, 5, 6, 2);
    b.rect(7, 3, 2, 3, 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap arrowArt() {
    gs::Bitmap b(12, 10);
    b.poly({{6, 9}, {1, 2}, {11, 2}}, 2);
    b.outline(1, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    ink(vdp, PAL_HUD, 15, 15, 14);
    ink(vdp, PAL_OK, 4, 14, 5);
    ink(vdp, PAL_BAD, 14, 3, 2);
    ink(vdp, PAL_INK, 15, 13, 4);
    paint(vdp, PAL_STALL, gs::rgb4(12, 3, 3), gs::rgb4(15, 14, 11), gs::rgb4(6, 2, 2));
    paint(vdp, PAL_CLERK, gs::rgb4(3, 6, 12), gs::rgb4(15, 12, 8), gs::rgb4(2, 3, 6));
    paint(vdp, PAL_APPLE, gs::rgb4(13, 2, 2), gs::rgb4(3, 10, 3), gs::rgb4(8, 1, 1));
    paint(vdp, PAL_PLUM, gs::rgb4(8, 2, 10), gs::rgb4(3, 9, 3), gs::rgb4(4, 1, 6));
    paint(vdp, PAL_LOAF, gs::rgb4(12, 8, 3), gs::rgb4(14, 12, 6), gs::rgb4(7, 4, 1));
    paint(vdp, PAL_BUN, gs::rgb4(14, 11, 6), gs::rgb4(10, 6, 2), gs::rgb4(8, 5, 2));
    paint(vdp, PAL_PEAR, gs::rgb4(10, 13, 3), gs::rgb4(3, 8, 2), gs::rgb4(5, 7, 1));
    paint(vdp, PAL_FIG, gs::rgb4(6, 2, 8), gs::rgb4(3, 8, 3), gs::rgb4(3, 1, 4));
    paint(vdp, PAL_TAPE, gs::rgb4(14, 13, 8), gs::rgb4(2, 2, 3), gs::rgb4(9, 8, 4));
    paint(vdp, PAL_WOOD, gs::rgb4(10, 6, 2), gs::rgb4(14, 11, 5), gs::rgb4(6, 3, 1));
    vdp.setColor(PAL_WOOD * 16 + 4, gs::rgb4(4, 2, 1));

    art.awning = gs::uploadMipped(vdp, awningArt());
    art.clerk = gs::uploadMipped(vdp, clerkArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.drawer = gs::uploadMipped(vdp, drawerArt());
    art.apple = gs::uploadMipped(vdp, roundFruit(2, 3));
    art.plum = gs::uploadMipped(vdp, roundFruit(2, 3));
    art.loaf = gs::uploadMipped(vdp, loafArt());
    art.bun = gs::uploadMipped(vdp, bunArt());
    art.pear = gs::uploadMipped(vdp, pearArt());
    art.fig = gs::uploadMipped(vdp, figArt());
    art.arrow = gs::uploadMipped(vdp, arrowArt());

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        int sky = y < 78 ? (10 - y / 16) : 0;
        if (y < 78) vdp.lineBackdrop[y] = gs::rgb4(6, 8 + (y < 40 ? 2 : 0), 12 - y / 20);
        else if (y < 150) vdp.lineBackdrop[y] = gs::rgb4(9, 6, 3);
        else vdp.lineBackdrop[y] = gs::rgb4(5, 4, 3);
        (void)sky;
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(6, 5, 4));
}

}  // namespace markettape
