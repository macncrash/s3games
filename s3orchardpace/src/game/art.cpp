#include "game/art.h"

#include <cmath>
#include <initializer_list>
#include <string>

namespace orchard {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

gs::Bitmap picker(int step) {
    gs::Bitmap b(36, 72);
    b.ellipse(18, 11, 7, 8, 4);
    b.ellipse(18, 7, 8, 3, 5);
    b.rect(12, 16, 12, 4, 5);
    b.set(15, 12, 8);
    b.set(21, 12, 8);
    b.poly({{8, 22}, {28, 22}, {30, 46}, {6, 46}}, 2);
    b.poly({{8, 22}, {18, 22}, {16, 46}, {6, 46}}, 1);
    b.rect(16, 24, 4, 16, 3);
    b.ellipse(10, 40, 4, 4, 6);
    b.line(11, 26, 5, 38, 4, 3.f);
    b.line(25, 26, 31, 36, 4, 3.f);
    if (step == 0) {
        b.rect(11, 44, 6, 20, 3);
        b.rect(20, 44, 6, 16, 7);
        b.rect(9, 62, 9, 4, 8);
        b.rect(19, 58, 8, 4, 8);
    } else {
        b.rect(11, 44, 6, 16, 7);
        b.rect(20, 44, 6, 20, 3);
        b.rect(10, 58, 8, 4, 8);
        b.rect(18, 62, 9, 4, 8);
    }
    b.outline(9, false);
    return b;
}

gs::Bitmap fallen() {
    gs::Bitmap b(78, 28);
    b.ellipse(12, 14, 6, 5, 4);
    b.ellipse(12, 10, 7, 3, 5);
    b.poly({{20, 8}, {66, 12}, {70, 22}, {18, 20}}, 2);
    b.ellipse(58, 16, 5, 4, 6);
    b.rect(64, 18, 10, 4, 8);
    b.outline(9, false);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(48, 84);
    b.rect(21, 40, 6, 40, 1);
    b.rect(23, 42, 2, 36, 2);
    b.ellipse(24, 28, 20, 22, 3);
    b.ellipse(16, 24, 10, 12, 4);
    b.ellipse(32, 22, 9, 10, 5);
    b.ellipse(18, 18, 3, 3, 6);
    b.ellipse(30, 30, 3, 3, 6);
    b.ellipse(24, 14, 3, 3, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap appleArt() {
    gs::Bitmap b(14, 16);
    b.ellipse(7, 9, 6, 6, 1);
    b.ellipse(5, 8, 2, 2, 2);
    b.rect(6, 2, 2, 4, 3);
    b.line(8, 3, 11, 1, 3, 1.f);
    return b;
}

gs::Bitmap basketArt() {
    gs::Bitmap b(22, 16);
    b.poly({{2, 4}, {20, 4}, {17, 15}, {5, 15}}, 1);
    b.rect(2, 3, 18, 3, 2);
    b.ellipse(8, 6, 2, 2, 3);
    b.ellipse(13, 7, 2, 2, 3);
    b.line(6, 4, 6, 1, 2, 1.f);
    b.line(16, 4, 16, 1, 2, 1.f);
    b.line(6, 1, 16, 1, 2, 1.f);
    return b;
}

gs::Bitmap shedArt() {
    gs::Bitmap b(40, 44);
    b.poly({{2, 16}, {20, 4}, {38, 16}}, 2);
    b.rect(4, 16, 32, 26, 1);
    b.rect(16, 24, 8, 18, 3);
    b.rect(8, 22, 6, 6, 4);
    b.rect(26, 22, 6, 6, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(10, 36);
    b.rect(4, 4, 2, 32, 1);
    b.poly({{5, 0}, {10, 8}, {0, 8}}, 2);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 5, 5, 0);
    b.rect(8, 1, 2, 4, 2);
    b.rect(8, 13, 2, 4, 2);
    b.rect(1, 8, 4, 2, 2);
    b.rect(13, 8, 4, 2, 2);
    return b;
}

gs::Bitmap slingArt() {
    gs::Bitmap b(16, 40);
    b.line(3, 36, 8, 8, 1, 2.f);
    b.line(13, 36, 8, 8, 1, 2.f);
    b.line(4, 10, 12, 10, 2, 2.f);
    b.ellipse(8, 12, 3, 2, 3);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    return b;
}

gs::Bitmap flareArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 2);
    b.ellipse(9, 9, 3, 3, 1);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(5, 5, 4, 2, 1);
    b.ellipse(12, 4, 3, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(24, 8);
    b.ellipse(12, 4, 10, 3, 1);
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
    const uint16_t ink = gs::rgb4(1, 2, 1);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 14, 12), gs::rgb4(8, 9, 6), gs::rgb4(3, 4, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 9), gs::rgb4(5, 3, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 9, 6), gs::rgb4(6, 1, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(5, 14, 6), gs::rgb4(12, 15, 10), gs::rgb4(1, 5, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_LEAF, {0, gs::rgb4(5, 3, 1), gs::rgb4(8, 5, 2), gs::rgb4(2, 8, 2), gs::rgb4(4, 11, 3), gs::rgb4(6, 13, 4),
                           gs::rgb4(14, 3, 2), gs::rgb4(1, 4, 1), 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_PICK, {0, gs::rgb4(3, 7, 3), gs::rgb4(2, 5, 8), gs::rgb4(1, 3, 2), gs::rgb4(13, 10, 7), gs::rgb4(6, 4, 2),
                           gs::rgb4(12, 3, 2), gs::rgb4(8, 6, 3), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 9), 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SIGHT, {0, gs::rgb4(14, 12, 4), gs::rgb4(15, 15, 12), gs::rgb4(6, 5, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_APPLE, {0, gs::rgb4(14, 2, 2), gs::rgb4(15, 8, 6), gs::rgb4(2, 8, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_BARK, {0, gs::rgb4(7, 4, 2), gs::rgb4(10, 7, 3), gs::rgb4(4, 2, 1), gs::rgb4(13, 11, 8), gs::rgb4(2, 1, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_FX, {0, gs::rgb4(12, 11, 7), gs::rgb4(15, 12, 4), gs::rgb4(8, 7, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(10, 13, 15), gs::rgb4(14, 14, 12), gs::rgb4(6, 9, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});
    setPal(vdp, PAL_POST, {0, gs::rgb4(9, 8, 6), gs::rgb4(14, 3, 2), gs::rgb4(4, 3, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ink});

    const uint16_t row[16] = {
        0,
        gs::rgb4(4, 6, 2), gs::rgb4(3, 5, 2), gs::rgb4(6, 5, 2),
        gs::rgb4(5, 4, 2), gs::rgb4(7, 6, 3),
        gs::rgb4(2, 4, 1), gs::rgb4(8, 7, 3),
        gs::rgb4(3, 3, 2), gs::rgb4(9, 8, 4), gs::rgb4(1, 3, 1),
        gs::rgb4(6, 7, 3), gs::rgb4(4, 4, 2), gs::rgb4(10, 8, 4),
        gs::rgb4(2, 2, 1), gs::rgb4(7, 8, 4),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROW * 16 + i, row[i]);

    loadFont(vdp, art);
    art.step[0] = gs::uploadMipped(vdp, picker(0));
    art.step[1] = gs::uploadMipped(vdp, picker(1));
    art.down = gs::uploadMipped(vdp, fallen());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.apple = gs::uploadMipped(vdp, appleArt());
    art.basket = gs::uploadMipped(vdp, basketArt());
    art.shed = gs::uploadMipped(vdp, shedArt());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.sling = gs::uploadMipped(vdp, slingArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flare = gs::uploadMipped(vdp, flareArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());

    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(8, 10, 12));
}

}  // namespace orchard
