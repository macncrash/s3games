#include "art.h"

namespace combine {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink, uint16_t dim) {
    vdp.setColor(pal * 16 + 0, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 2, dim);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

gs::Bitmap bodyArt() {
    gs::Bitmap b(150, 124);
    b.poly({{62, 2}, {88, 2}, {102, 30}, {48, 30}}, 3);
    b.poly({{66, 6}, {84, 6}, {96, 26}, {54, 26}}, 2);
    b.rect(38, 16, 40, 34, 3);
    b.rect(40, 14, 36, 8, 1);
    b.rect(44, 26, 28, 16, 7);
    b.rect(46, 28, 12, 5, 1);
    b.ellipse(58, 14, 4, 3, 9);
    b.rect(34, 8, 5, 16, 6);
    b.rect(32, 6, 9, 4, 11);
    b.rect(30, 36, 92, 58, 2);
    b.rect(34, 38, 84, 10, 1);
    b.rect(78, 52, 30, 20, 8);
    b.rect(80, 54, 26, 6, 1);
    b.rect(36, 78, 80, 8, 3);
    b.line(112, 48, 142, 26, 11, 4);
    b.ellipse(144, 24, 6, 5, 11);
    b.ellipse(144, 24, 3, 2, 6);
    b.rect(108, 70, 8, 32, 6);
    for (int i = 0; i < 6; i++) b.rect(108, 74 + i * 5, 8, 2, 11);
    b.rect(52, 96, 52, 7, 6);
    b.ellipse(36, 108, 22, 16, 10);
    b.ellipse(36, 108, 11, 8, 14);
    b.ellipse(36, 108, 4, 3, 11);
    b.ellipse(114, 108, 22, 16, 10);
    b.ellipse(114, 108, 11, 8, 14);
    b.ellipse(114, 108, 4, 3, 11);
    b.outline(15, false);
    return b;
}

gs::Bitmap headerArt() {
    gs::Bitmap b(120, 14);
    b.rect(4, 3, 112, 7, 4);
    b.rect(4, 9, 112, 4, 5);
    b.rect(0, 1, 6, 12, 6);
    b.rect(114, 1, 6, 12, 6);
    b.rect(8, 1, 104, 2, 6);
    for (int i = 0; i < 5; i++) b.rect(18 + i * 18, 2, 2, 8, 14);
    b.outline(15, false);
    return b;
}

gs::Bitmap fillArt() {
    gs::Bitmap b(96, 8);
    b.rect(0, 1, 96, 6, 2);
    b.rect(0, 0, 96, 3, 1);
    return b;
}

gs::Bitmap batArt() {
    gs::Bitmap b(4, 12);
    b.rect(1, 0, 2, 12, 6);
    return b;
}

gs::Bitmap earArt() {
    gs::Bitmap b(14, 32);
    b.line(7, 31, 7, 14, 3, 2);
    b.ellipse(7, 12, 4, 9, 2);
    b.ellipse(7, 11, 2, 6, 1);
    b.line(7, 10, 2, 1, 4, 1);
    b.line(7, 8, 7, 0, 4, 1);
    b.line(7, 10, 12, 1, 4, 1);
    return b;
}

gs::Bitmap chaffArt() {
    gs::Bitmap b(8, 8);
    b.line(1, 6, 6, 1, 1, 1);
    b.set(2, 3, 2);
    b.set(5, 4, 4);
    return b;
}

gs::Bitmap lampArt(int c) {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, c);
    b.ellipse(5, 5, 2, 2, 3);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(48, 68);
    b.rect(21, 38, 7, 28, 4);
    b.ellipse(24, 28, 18, 16, 2);
    b.ellipse(14, 26, 10, 10, 1);
    b.ellipse(32, 24, 11, 9, 3);
    b.outline(5, false);
    return b;
}

gs::Bitmap baleArt() {
    gs::Bitmap b(36, 22);
    b.rect(2, 4, 32, 15, 2);
    b.rect(4, 6, 28, 4, 1);
    b.line(2, 12, 34, 12, 5, 1);
    b.line(12, 4, 12, 19, 5, 1);
    b.line(24, 4, 24, 19, 5, 1);
    b.outline(6, false);
    return b;
}

gs::Bitmap legArt() {
    gs::Bitmap b(44, 100);
    b.rect(16, 22, 12, 70, 2);
    b.rect(18, 24, 4, 66, 7);
    b.rect(6, 10, 32, 16, 3);
    b.poly({{4, 22}, {22, 2}, {40, 22}}, 4);
    b.rect(2, 86, 40, 10, 5);
    b.line(8, 28, 16, 78, 6, 2);
    b.outline(1, false);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(80, 22);
    b.ellipse(40, 11, 36, 8, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(34, 34);
    b.ellipse(17, 17, 13, 13, 1);
    b.ellipse(17, 17, 7, 7, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(70, 26);
    b.ellipse(22, 16, 16, 8, 1);
    b.ellipse(40, 13, 18, 10, 1);
    b.ellipse(54, 16, 12, 7, 2);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp);
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
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_HUD, gs::rgb4(15, 15, 15), gs::rgb4(10, 10, 12));
    textPal(vdp, PAL_AMBER, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 2));
    textPal(vdp, PAL_RED, gs::rgb4(15, 4, 3), gs::rgb4(10, 2, 2));
    textPal(vdp, PAL_GREEN, gs::rgb4(8, 15, 5), gs::rgb4(4, 10, 3));

    setPal(vdp, PAL_RIG,
           {0, gs::rgb4(15, 14, 10), gs::rgb4(13, 2, 1), gs::rgb4(8, 1, 1), gs::rgb4(15, 12, 2), gs::rgb4(11, 7, 1),
            gs::rgb4(3, 3, 4), gs::rgb4(6, 10, 13), gs::rgb4(15, 11, 3), gs::rgb4(15, 8, 1), gs::rgb4(2, 2, 2),
            gs::rgb4(12, 13, 14), gs::rgb4(9, 7, 4), gs::rgb4(5, 5, 6), gs::rgb4(4, 4, 5), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_WHEAT,
           {0, gs::rgb4(15, 13, 4), gs::rgb4(13, 9, 2), gs::rgb4(8, 10, 3), gs::rgb4(15, 14, 8), gs::rgb4(9, 7, 3),
            gs::rgb4(6, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(10, 14, 4), gs::rgb4(6, 10, 3), gs::rgb4(3, 7, 2), gs::rgb4(8, 5, 2), gs::rgb4(2, 2, 1), 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_CLOUD, {0, gs::rgb4(15, 15, 15), gs::rgb4(12, 13, 15), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_YARD,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(11, 11, 10), gs::rgb4(8, 8, 8), gs::rgb4(12, 3, 2), gs::rgb4(10, 10, 9),
            gs::rgb4(12, 13, 14), gs::rgb4(14, 14, 13), 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(6, 15, 4), gs::rgb4(15, 3, 2), gs::rgb4(15, 15, 14), gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 12, 5), gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    const uint16_t field[16] = {
        0,
        gs::rgb4(12, 11, 7),
        gs::rgb4(8, 8, 5),
        gs::rgb4(7, 8, 4),
        gs::rgb4(15, 14, 7),
        gs::rgb4(12, 10, 4),
        gs::rgb4(15, 12, 3),
        gs::rgb4(13, 9, 2),
        gs::rgb4(12, 8, 2),
        gs::rgb4(11, 8, 2),
        gs::rgb4(15, 14, 6),
        0,
        0,
        0,
        gs::rgb4(15, 15, 9),
        gs::rgb4(8, 5, 1),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_FIELD * 16 + i, field[i]);

    art.body = gs::uploadMipped(vdp, bodyArt());
    art.header = gs::uploadMipped(vdp, headerArt());
    art.fill = gs::uploadMipped(vdp, fillArt());
    art.bat = gs::uploadMipped(vdp, batArt());
    art.ear = gs::uploadMipped(vdp, earArt());
    art.chaff = gs::uploadMipped(vdp, chaffArt());
    art.lampOn = gs::uploadMipped(vdp, lampArt(1));
    art.lampOff = gs::uploadMipped(vdp, lampArt(2));
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.bale = gs::uploadMipped(vdp, baleArt());
    art.leg = gs::uploadMipped(vdp, legArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    loadFont(vdp, art);
}

}  // namespace combine
