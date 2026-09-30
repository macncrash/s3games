#include "game/art.h"

namespace rail {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

gs::Bitmap cartArt() {
    gs::Bitmap b(52, 34);
    b.rect(6, 14, 32, 10, 2);
    b.rect(10, 6, 18, 9, 3);
    b.rect(12, 8, 6, 4, 1);
    b.rect(20, 8, 6, 4, 1);
    b.rect(34, 16, 10, 5, 4);
    b.rect(2, 17, 6, 4, 6);
    b.ellipse(14, 26, 6, 6, 5);
    b.ellipse(36, 26, 6, 6, 5);
    b.ellipse(14, 26, 2, 2, 1);
    b.ellipse(36, 26, 2, 2, 1);
    b.rect(8, 12, 8, 3, 4);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(48, 30);
    b.rect(6, 12, 28, 9, 2);
    b.rect(10, 6, 14, 7, 3);
    b.rect(12, 8, 5, 3, 1);
    b.rect(30, 14, 8, 4, 4);
    b.ellipse(14, 22, 5, 5, 5);
    b.ellipse(30, 22, 5, 5, 5);
    b.ellipse(14, 22, 2, 2, 1);
    b.ellipse(30, 22, 2, 2, 1);
    return b;
}

gs::Bitmap sleeperArt() {
    gs::Bitmap b(20, 8);
    b.rect(0, 1, 20, 6, 2);
    b.rect(1, 2, 3, 4, 3);
    b.rect(16, 2, 3, 4, 3);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(36, 6);
    b.rect(0, 0, 36, 2, 1);
    b.rect(0, 2, 36, 2, 2);
    b.rect(0, 4, 36, 2, 3);
    return b;
}

gs::Bitmap hillArt() {
    gs::Bitmap b(100, 36);
    b.poly({{0, 36}, {18, 18}, {36, 26}, {58, 8}, {78, 22}, {100, 36}}, 2);
    b.poly({{10, 36}, {30, 22}, {52, 30}, {74, 16}, {100, 36}}, 3);
    return b;
}

gs::Bitmap markArt() {
    gs::Bitmap b(16, 28);
    b.rect(0, 8, 16, 20, 2);
    b.rect(2, 10, 12, 4, 1);
    b.rect(2, 16, 12, 4, 3);
    b.rect(2, 22, 12, 4, 1);
    b.rect(6, 0, 4, 8, 4);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(22, 56);
    b.rect(9, 10, 4, 46, 2);
    b.rect(4, 48, 14, 5, 3);
    b.rect(2, 2, 18, 10, 4);
    b.rect(4, 4, 14, 6, 1);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(14, 10);
    b.line(0, 8, 6, 2, 1, 2);
    b.line(4, 9, 12, 3, 2, 2);
    b.ellipse(7, 4, 2, 2, 3);
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
                if (g && g[y * 5 + x]) {
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    const uint16_t z = 0;
    uint16_t hud[16] = {z, gs::rgb4(15, 15, 14), gs::rgb4(9, 9, 10), gs::rgb4(15, 12, 3), gs::rgb4(15, 4, 3),
                        gs::rgb4(4, 14, 6), gs::rgb4(6, 8, 15), gs::rgb4(5, 5, 6), z, z, z, z, z, z, z, shadow};
    setPal(vdp, PAL_HUD, hud);
    uint16_t amber[16] = {z, gs::rgb4(15, 12, 2), gs::rgb4(15, 8, 1), gs::rgb4(15, 15, 8), z, z, z, z, z, z, z, z, z, z, z, shadow};
    setPal(vdp, PAL_AMBER, amber);
    uint16_t red[16] = {z, gs::rgb4(15, 3, 2), gs::rgb4(10, 1, 1), gs::rgb4(15, 10, 8), z, z, z, z, z, z, z, z, z, z, z, shadow};
    setPal(vdp, PAL_RED, red);
    uint16_t green[16] = {z, gs::rgb4(6, 15, 5), gs::rgb4(2, 9, 3), gs::rgb4(14, 15, 10), z, z, z, z, z, z, z, z, z, z, z, shadow};
    setPal(vdp, PAL_GREEN, green);

    auto body = [&](int pal, uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint16_t f) {
        uint16_t p[16] = {z, a, b, c, d, e, f, z, z, z, z, z, z, z, z, shadow};
        setPal(vdp, pal, p);
    };
    body(PAL_CART, gs::rgb4(15, 14, 8), gs::rgb4(12, 8, 2), gs::rgb4(8, 10, 14), gs::rgb4(15, 11, 3), gs::rgb4(2, 2, 3),
         gs::rgb4(14, 5, 2));
    body(PAL_RIVAL, gs::rgb4(14, 12, 11), gs::rgb4(10, 2, 3), gs::rgb4(14, 6, 6), gs::rgb4(6, 6, 7), gs::rgb4(2, 2, 3),
         gs::rgb4(12, 10, 4));
    body(PAL_MARK, gs::rgb4(15, 15, 12), gs::rgb4(15, 13, 2), gs::rgb4(4, 4, 5), gs::rgb4(12, 4, 2), gs::rgb4(8, 8, 9), z);
    body(PAL_IRON, gs::rgb4(12, 12, 13), gs::rgb4(7, 6, 5), gs::rgb4(4, 4, 5), gs::rgb4(10, 8, 4), gs::rgb4(3, 3, 4),
         gs::rgb4(9, 9, 10));
    body(PAL_HILL, gs::rgb4(5, 8, 4), gs::rgb4(3, 6, 4), gs::rgb4(7, 9, 5), gs::rgb4(8, 7, 4), z, z);
    body(PAL_POST, gs::rgb4(14, 14, 12), gs::rgb4(5, 5, 6), gs::rgb4(8, 7, 5), gs::rgb4(13, 3, 2), gs::rgb4(15, 12, 3), z);

    art.cart = gs::uploadMipped(vdp, cartArt());
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.sleeper = gs::uploadMipped(vdp, sleeperArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.mark = gs::uploadMipped(vdp, markArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    loadFont(vdp, art);
}

}  // namespace rail
