#include "game/art.h"

#include <string>

namespace lotc {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        ++i;
    }
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void textPal(gs::VDP& vdp, int pal, uint16_t ink) {
    for (int i = 0; i < 16; ++i) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 1, ink);
    vdp.setColor(pal * 16 + 15, gs::rgb4(2, 1, 1));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{3, 1, 0, 15, 1};
    for (int c = 32; c < 128; ++c) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 5; ++x)
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

gs::Bitmap vanArt() {
    gs::Bitmap b(44, 58);
    b.rect(10, 2, 24, 10, 1);
    b.rect(13, 4, 8, 5, 5);
    b.rect(23, 4, 8, 5, 5);
    b.rect(8, 12, 28, 28, 2);
    b.rect(11, 16, 8, 10, 6);
    b.rect(25, 16, 8, 10, 6);
    b.rect(20, 15, 4, 12, 3);
    b.rect(6, 40, 32, 6, 4);
    b.ellipse(14, 46, 5, 5, 8);
    b.ellipse(30, 46, 5, 5, 8);
    b.rect(12, 44, 4, 4, 9);
    b.rect(28, 44, 4, 4, 9);
    b.rect(18, 48, 8, 3, 7);
    b.rect(4, 18, 4, 16, 10);
    b.rect(36, 18, 4, 16, 10);
    b.outline(15, false);
    return b;
}

gs::Bitmap shopperArt() {
    gs::Bitmap b(40, 28);
    b.rect(8, 4, 24, 7, 1);
    b.rect(11, 5, 7, 4, 5);
    b.rect(22, 5, 7, 4, 5);
    b.rect(6, 11, 28, 8, 2);
    b.rect(4, 18, 32, 4, 4);
    b.ellipse(11, 20, 4, 4, 8);
    b.ellipse(29, 20, 4, 4, 8);
    b.rect(16, 12, 8, 3, 7);
    b.rect(18, 8, 4, 2, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 52);
    b.rect(4, 2, 4, 44, 2);
    b.rect(5, 2, 2, 44, 1);
    b.rect(2, 2, 8, 6, 4);
    b.rect(1, 46, 10, 4, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap armArt() {
    gs::Bitmap b(16, 8);
    b.rect(0, 2, 16, 4, 1);
    b.rect(2, 3, 4, 2, 2);
    b.rect(8, 3, 4, 2, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 56);
    b.rect(7, 10, 3, 40, 2);
    b.rect(3, 4, 10, 8, 1);
    b.rect(5, 6, 6, 4, 4);
    b.rect(4, 48, 8, 4, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap boothArt() {
    gs::Bitmap b(36, 40);
    b.rect(4, 6, 28, 6, 3);
    b.rect(6, 12, 24, 22, 1);
    b.rect(9, 16, 8, 10, 5);
    b.rect(19, 16, 8, 8, 4);
    b.rect(8, 34, 20, 4, 2);
    b.rect(14, 4, 8, 4, 6);
    b.outline(15, false);
    return b;
}

gs::Bitmap coneArt() {
    gs::Bitmap b(16, 24);
    b.rect(6, 2, 4, 4, 1);
    b.rect(4, 6, 8, 6, 2);
    b.rect(2, 12, 12, 6, 1);
    b.rect(1, 18, 14, 4, 3);
    b.outline(15, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(28, 40);
    b.ellipse(14, 12, 10, 10, 1);
    b.ellipse(10, 16, 6, 6, 2);
    b.ellipse(18, 15, 5, 5, 3);
    b.rect(12, 22, 4, 14, 4);
    b.outline(15, false);
    return b;
}

gs::Bitmap puffArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 5, 4, 1);
    b.ellipse(5, 6, 2, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(22, 8);
    b.ellipse(11, 4, 9, 3, 1);
    return b;
}

gs::Bitmap signArt() {
    gs::Bitmap b(28, 18);
    b.rect(2, 2, 24, 12, 1);
    b.rect(4, 4, 8, 8, 2);
    b.rect(14, 4, 10, 3, 3);
    b.rect(12, 14, 4, 4, 4);
    b.outline(15, false);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    textPal(vdp, PAL_TEXT, gs::rgb4(14, 13, 11));
    textPal(vdp, PAL_GOLD, gs::rgb4(15, 12, 3));
    textPal(vdp, PAL_ALERT, gs::rgb4(15, 4, 2));
    textPal(vdp, PAL_GOOD, gs::rgb4(5, 14, 6));

    setPal(vdp, PAL_VAN,
           {0, gs::rgb4(14, 10, 2), gs::rgb4(11, 7, 1), gs::rgb4(6, 4, 1), gs::rgb4(3, 3, 3), gs::rgb4(8, 11, 13),
            gs::rgb4(4, 6, 8), gs::rgb4(15, 13, 4), gs::rgb4(1, 1, 1), gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 4),
            gs::rgb4(2, 2, 2), gs::rgb4(12, 8, 2), gs::rgb4(9, 6, 2), gs::rgb4(15, 14, 8), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_SHOP,
           {0, gs::rgb4(4, 8, 13), gs::rgb4(2, 5, 9), gs::rgb4(1, 3, 6), gs::rgb4(8, 8, 7), gs::rgb4(10, 13, 15),
            gs::rgb4(6, 8, 10), gs::rgb4(14, 12, 3), gs::rgb4(1, 1, 1), gs::rgb4(5, 6, 7), gs::rgb4(3, 3, 4),
            gs::rgb4(2, 2, 2), gs::rgb4(7, 10, 14), gs::rgb4(3, 6, 11), gs::rgb4(12, 14, 15), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_BOOM,
           {0, gs::rgb4(12, 12, 11), gs::rgb4(7, 7, 6), gs::rgb4(4, 4, 4), gs::rgb4(14, 3, 2), gs::rgb4(15, 14, 8),
            gs::rgb4(9, 9, 8), gs::rgb4(5, 5, 5), gs::rgb4(2, 2, 2), gs::rgb4(10, 6, 2), gs::rgb4(6, 6, 5),
            gs::rgb4(1, 1, 1), gs::rgb4(13, 8, 3), gs::rgb4(8, 8, 7), gs::rgb4(15, 15, 12), gs::rgb4(3, 3, 3)});
    setPal(vdp, PAL_BOOTH,
           {0, gs::rgb4(10, 8, 6), gs::rgb4(6, 5, 4), gs::rgb4(12, 6, 3), gs::rgb4(3, 5, 8), gs::rgb4(8, 12, 14),
            gs::rgb4(14, 10, 3), gs::rgb4(4, 3, 2), gs::rgb4(9, 7, 5), gs::rgb4(2, 2, 2), gs::rgb4(11, 9, 7),
            gs::rgb4(1, 1, 1), gs::rgb4(7, 6, 4), gs::rgb4(13, 11, 8), gs::rgb4(5, 4, 3), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(15, 14, 6), gs::rgb4(8, 8, 7), gs::rgb4(4, 4, 4), gs::rgb4(15, 12, 4), gs::rgb4(6, 6, 5),
            gs::rgb4(3, 3, 3), gs::rgb4(12, 10, 4), gs::rgb4(2, 2, 2), gs::rgb4(10, 9, 5), gs::rgb4(5, 5, 4),
            gs::rgb4(1, 1, 1), gs::rgb4(14, 11, 3), gs::rgb4(7, 6, 3), gs::rgb4(15, 15, 10), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_STRIPE,
           {0, gs::rgb4(14, 8, 2), gs::rgb4(15, 14, 8), gs::rgb4(4, 4, 4), gs::rgb4(12, 5, 1), gs::rgb4(8, 4, 1),
            gs::rgb4(3, 3, 3), gs::rgb4(11, 9, 4), gs::rgb4(6, 5, 2), gs::rgb4(2, 2, 2), gs::rgb4(13, 10, 3),
            gs::rgb4(1, 1, 1), gs::rgb4(9, 6, 2), gs::rgb4(15, 12, 5), gs::rgb4(7, 3, 1), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_FX,
           {0, gs::rgb4(12, 11, 10), gs::rgb4(8, 8, 7), gs::rgb4(6, 6, 5), gs::rgb4(4, 4, 3), gs::rgb4(10, 9, 8),
            gs::rgb4(3, 3, 3), gs::rgb4(9, 8, 7), gs::rgb4(5, 5, 4), gs::rgb4(11, 10, 9), gs::rgb4(2, 2, 2),
            gs::rgb4(1, 1, 1), gs::rgb4(7, 6, 5), gs::rgb4(13, 12, 10), gs::rgb4(14, 13, 11), gs::rgb4(2, 2, 1)});
    setPal(vdp, PAL_ARM,
           {0, gs::rgb4(15, 12, 2), gs::rgb4(2, 2, 2), gs::rgb4(15, 15, 12), gs::rgb4(12, 4, 1), gs::rgb4(8, 6, 1),
            gs::rgb4(4, 4, 3), gs::rgb4(14, 8, 2), gs::rgb4(6, 5, 2), gs::rgb4(10, 8, 2), gs::rgb4(3, 3, 2),
            gs::rgb4(1, 1, 1), gs::rgb4(13, 10, 2), gs::rgb4(9, 7, 1), gs::rgb4(15, 14, 6), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_SIGN,
           {0, gs::rgb4(2, 6, 12), gs::rgb4(15, 15, 14), gs::rgb4(14, 10, 2), gs::rgb4(6, 6, 5), gs::rgb4(3, 4, 6),
            gs::rgb4(8, 10, 13), gs::rgb4(1, 3, 6), gs::rgb4(12, 12, 11), gs::rgb4(4, 5, 7), gs::rgb4(10, 8, 3),
            gs::rgb4(1, 1, 1), gs::rgb4(5, 8, 12), gs::rgb4(13, 13, 12), gs::rgb4(9, 7, 2), gs::rgb4(2, 2, 2)});
    setPal(vdp, PAL_TREE,
           {0, gs::rgb4(3, 8, 3), gs::rgb4(2, 6, 2), gs::rgb4(5, 10, 4), gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2),
            gs::rgb4(1, 4, 1), gs::rgb4(7, 9, 4), gs::rgb4(3, 5, 2), gs::rgb4(8, 6, 3), gs::rgb4(2, 3, 1),
            gs::rgb4(1, 1, 1), gs::rgb4(4, 7, 3), gs::rgb4(6, 8, 3), gs::rgb4(5, 4, 2), gs::rgb4(2, 2, 1)});

    int r = PAL_ROAD * 16;
    vdp.setColor(r + 0, 0);
    vdp.setColor(r + 1, gs::rgb4(5, 5, 5));
    vdp.setColor(r + 2, gs::rgb4(3, 4, 3));
    vdp.setColor(r + 3, gs::rgb4(7, 7, 6));
    vdp.setColor(r + 4, gs::rgb4(6, 6, 5));
    vdp.setColor(r + 5, gs::rgb4(4, 4, 3));
    vdp.setColor(r + 6, gs::rgb4(2, 2, 2));
    vdp.setColor(r + 7, gs::rgb4(8, 8, 7));
    vdp.setColor(r + 8, gs::rgb4(4, 4, 4));
    vdp.setColor(r + 9, gs::rgb4(1, 1, 1));
    vdp.setColor(r + 10, gs::rgb4(3, 3, 3));
    vdp.setColor(r + 11, gs::rgb4(9, 8, 3));
    vdp.setColor(r + 12, gs::rgb4(12, 11, 4));
    vdp.setColor(r + 13, gs::rgb4(8, 7, 3));
    vdp.setColor(r + 14, gs::rgb4(14, 13, 6));
    vdp.setColor(r + 15, gs::rgb4(6, 6, 6));
    vdp.setFogColor(gs::rgb4(8, 5, 4));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.van = gs::uploadMipped(vdp, vanArt());
    art.shopper = gs::uploadMipped(vdp, shopperArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.arm = gs::uploadMipped(vdp, armArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.booth = gs::uploadMipped(vdp, boothArt());
    art.cone = gs::uploadMipped(vdp, coneArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.puff = gs::uploadMipped(vdp, puffArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.sign = gs::uploadMipped(vdp, signArt());
}

}  // namespace lotc
