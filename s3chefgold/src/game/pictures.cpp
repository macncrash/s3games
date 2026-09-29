#include "game/pictures.h"

namespace chefgold {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? c[i] : 0);
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

gs::Bitmap chefArt() {
    gs::Bitmap b(28, 40);
    b.rect(8, 2, 12, 6, 2);
    b.rect(6, 6, 16, 4, 1);
    b.ellipse(14, 14, 6, 6, 3);
    b.set(11, 13, 4);
    b.set(17, 13, 4);
    b.rect(12, 16, 4, 1, 5);
    b.rect(6, 20, 16, 14, 6);
    b.rect(10, 22, 8, 8, 1);
    b.rect(4, 22, 4, 10, 6);
    b.rect(20, 22, 4, 10, 6);
    b.rect(8, 34, 5, 6, 7);
    b.rect(15, 34, 5, 6, 7);
    b.outline(4, false);
    return b;
}

gs::Bitmap panArt() {
    gs::Bitmap b(40, 16);
    b.ellipse(18, 9, 16, 6, 2);
    b.ellipse(18, 8, 12, 3, 1);
    b.rect(30, 7, 8, 3, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap flameArt() {
    gs::Bitmap b(16, 14);
    b.poly({{8, 1}, {13, 13}, {3, 13}}, 1);
    b.poly({{8, 5}, {11, 13}, {5, 13}}, 2);
    return b;
}

gs::Bitmap goldDish() {
    gs::Bitmap b(28, 18);
    b.ellipse(14, 13, 12, 4, 3);
    b.ellipse(14, 11, 9, 3, 1);
    b.ellipse(14, 8, 7, 4, 2);
    b.rect(10, 6, 8, 2, 4);
    b.outline(5, false);
    return b;
}

gs::Bitmap creamDish() {
    gs::Bitmap b(28, 18);
    b.ellipse(14, 13, 12, 4, 3);
    b.ellipse(14, 10, 8, 5, 1);
    b.ellipse(14, 8, 5, 3, 2);
    b.outline(5, false);
    return b;
}

gs::Bitmap ticketArt() {
    gs::Bitmap b(22, 16);
    b.rect(1, 1, 20, 14, 1);
    b.rect(3, 4, 12, 2, 2);
    b.rect(3, 8, 8, 2, 3);
    b.outline(4, false);
    return b;
}

}  // namespace

void buildPictures(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(15, 14, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(4, 2, 1)};
    const uint16_t gold[] = {0, gs::rgb4(15, 12, 3), gs::rgb4(10, 7, 1)};
    const uint16_t alert[] = {0, gs::rgb4(15, 3, 2), gs::rgb4(8, 1, 1)};
    const uint16_t paper[] = {0, gs::rgb4(14, 12, 9), gs::rgb4(8, 6, 4)};
    const uint16_t steel[] = {0, gs::rgb4(10, 11, 12), gs::rgb4(6, 7, 8), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 1)};
    const uint16_t food[] = {0, gs::rgb4(15, 13, 6), gs::rgb4(12, 6, 2), gs::rgb4(13, 13, 12), gs::rgb4(4, 10, 3),
                             gs::rgb4(2, 1, 1)};
    const uint16_t chef[] = {0, gs::rgb4(15, 15, 14), gs::rgb4(15, 15, 15), gs::rgb4(13, 9, 6), gs::rgb4(2, 1, 1),
                             gs::rgb4(10, 3, 3), gs::rgb4(15, 15, 15), gs::rgb4(3, 3, 4)};
    const uint16_t fire[] = {0, gs::rgb4(15, 10, 2), gs::rgb4(15, 4, 1)};
    setPal(vdp, PAL_HUD, hud, 16);
    setPal(vdp, PAL_GOLD, gold, 3);
    setPal(vdp, PAL_ALERT, alert, 3);
    setPal(vdp, PAL_PAPER, paper, 3);
    setPal(vdp, PAL_STEEL, steel, 5);
    setPal(vdp, PAL_FOOD, food, 6);
    setPal(vdp, PAL_CHEF, chef, 8);
    setPal(vdp, PAL_FIRE, fire, 3);
    loadFont(vdp, art);
    art.chef = gs::uploadMipped(vdp, chefArt());
    art.pan = gs::uploadMipped(vdp, panArt());
    art.flame = gs::uploadMipped(vdp, flameArt());
    art.goldDish = gs::uploadMipped(vdp, goldDish());
    art.creamDish = gs::uploadMipped(vdp, creamDish());
    art.ticket = gs::uploadMipped(vdp, ticketArt());
    gs::Bitmap solid(4, 4);
    solid.rect(0, 0, 4, 4, 1);
    art.solid = gs::uploadMipped(vdp, solid);
}

}  // namespace chefgold
