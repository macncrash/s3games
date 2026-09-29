#include "game/art.h"

#include <initializer_list>

namespace cliffmark {
namespace {

void setPal(gs::VDP& v, int pal, const uint16_t* cols) {
    for (int i = 0; i < 16; i++) v.setColor(pal * 16 + i, cols[i]);
}

void loadFont(gs::VDP& vdp, Art& art) {
    gs::TileAlloc tiles(vdp, 1);
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

gs::Bitmap cartArt() {
    gs::Bitmap b(88, 52);
    b.rect(10, 24, 64, 14, 1);
    b.rect(18, 14, 34, 12, 2);
    b.rect(24, 16, 10, 6, 3);
    b.rect(48, 18, 16, 6, 4);
    b.rect(6, 34, 12, 12, 5);
    b.rect(66, 34, 12, 12, 5);
    b.ellipse(12, 40, 5, 5, 6);
    b.ellipse(72, 40, 5, 5, 6);
    b.rect(62, 22, 10, 6, 7);
    b.line(8, 24, 76, 24, 8, 1);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(14, 64);
    b.rect(5, 8, 4, 52, 1);
    b.rect(2, 2, 10, 8, 2);
    b.rect(3, 56, 8, 6, 3);
    return b;
}

gs::Bitmap cragArt() {
    gs::Bitmap b(40, 72);
    b.poly({{4, 70}, {14, 22}, {22, 36}, {34, 4}, {38, 70}}, 1);
    b.poly({{10, 70}, {18, 34}, {26, 44}, {32, 16}, {36, 70}}, 2);
    return b;
}

gs::Bitmap paintArt() {
    gs::Bitmap b(48, 20);
    b.rect(2, 4, 44, 12, 1);
    b.rect(6, 7, 36, 6, 2);
    b.rect(20, 2, 8, 16, 3);
    return b;
}

gs::Bitmap ribbonArt() {
    gs::Bitmap b(72, 16);
    b.rect(0, 3, 72, 10, 1);
    for (int x = 0; x < 72; x += 12) b.rect(float(x), 3, 6, 10, 2);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    for (int p = 0; p < gs::NUM_PALETTES; p++)
        for (int i = 0; i < 16; i++) vdp.setColor(p * 16 + i, 0);

    const uint16_t shadow = gs::rgb4(1, 1, 2);
    uint16_t hud[16] = {};
    hud[1] = gs::rgb4(15, 15, 14);
    hud[2] = gs::rgb4(8, 13, 15);
    hud[3] = gs::rgb4(15, 13, 4);
    hud[4] = gs::rgb4(15, 5, 3);
    hud[5] = gs::rgb4(4, 14, 6);
    hud[6] = gs::rgb4(8, 10, 6);
    hud[15] = shadow;
    uint16_t cart[16] = {};
    cart[1] = gs::rgb4(11, 7, 3);
    cart[2] = gs::rgb4(4, 6, 8);
    cart[3] = gs::rgb4(12, 14, 15);
    cart[4] = gs::rgb4(15, 11, 3);
    cart[5] = gs::rgb4(2, 2, 2);
    cart[6] = gs::rgb4(9, 9, 8);
    cart[7] = gs::rgb4(14, 6, 3);
    cart[8] = gs::rgb4(13, 12, 8);
    cart[15] = shadow;
    uint16_t post[16] = {};
    post[1] = gs::rgb4(8, 8, 7);
    post[2] = gs::rgb4(14, 12, 3);
    post[3] = gs::rgb4(4, 3, 3);
    post[15] = shadow;
    uint16_t crag[16] = {};
    crag[1] = gs::rgb4(6, 5, 4);
    crag[2] = gs::rgb4(4, 3, 3);
    crag[15] = shadow;
    uint16_t paint[16] = {};
    paint[1] = gs::rgb4(14, 12, 3);
    paint[2] = gs::rgb4(15, 15, 12);
    paint[3] = gs::rgb4(12, 4, 3);
    paint[15] = shadow;
    uint16_t ribbon[16] = {};
    ribbon[1] = gs::rgb4(12, 3, 3);
    ribbon[2] = gs::rgb4(15, 14, 12);
    ribbon[15] = shadow;
    uint16_t shelf[16] = {};
    shelf[1] = gs::rgb4(5, 5, 4);
    shelf[2] = gs::rgb4(3, 3, 3);
    shelf[3] = gs::rgb4(8, 7, 6);
    shelf[4] = gs::rgb4(6, 5, 4);
    shelf[5] = gs::rgb4(4, 4, 3);
    shelf[6] = gs::rgb4(8, 7, 6);
    shelf[7] = gs::rgb4(6, 5, 4);
    shelf[8] = gs::rgb4(9, 8, 7);
    shelf[9] = gs::rgb4(5, 5, 4);
    shelf[10] = gs::rgb4(7, 6, 5);
    shelf[14] = gs::rgb4(12, 10, 6);
    shelf[15] = gs::rgb4(10, 9, 8);
    uint16_t mark[16] = {};
    mark[1] = gs::rgb4(12, 10, 3);
    mark[2] = gs::rgb4(8, 6, 2);
    mark[3] = gs::rgb4(15, 13, 5);
    mark[4] = gs::rgb4(10, 8, 3);
    mark[5] = gs::rgb4(7, 5, 2);
    mark[6] = gs::rgb4(14, 12, 4);
    mark[7] = gs::rgb4(9, 7, 2);
    mark[8] = gs::rgb4(15, 14, 6);
    mark[9] = gs::rgb4(6, 5, 2);
    mark[10] = gs::rgb4(13, 11, 4);
    mark[14] = gs::rgb4(15, 15, 12);
    mark[15] = gs::rgb4(12, 4, 3);

    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_CART, cart);
    setPal(vdp, PAL_POST, post);
    setPal(vdp, PAL_CRAG, crag);
    setPal(vdp, PAL_PAINT, paint);
    setPal(vdp, PAL_RIBBON, ribbon);
    setPal(vdp, PAL_SHELF, shelf);
    setPal(vdp, PAL_MARK, mark);
    vdp.setFogColor(gs::rgb4(6, 7, 9));
    loadFont(vdp, art);
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.crag = gs::uploadMipped(vdp, cragArt());
    art.paint = gs::uploadMipped(vdp, paintArt());
    art.ribbon = gs::uploadMipped(vdp, ribbonArt());
}

}  // namespace cliffmark
