#include "game/art.h"

#include <cstdint>

namespace rail {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t c[16]) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

gs::Bitmap cartArt(bool duck) {
    gs::Bitmap b(48, duck ? 22 : 36);
    int base = duck ? 4 : 16;
    b.rect(4, base, 34, 14, 2);
    b.rect(8, base - 8, 16, 8, 3);
    b.rect(10, base - 6, 6, 4, 1);
    b.rect(18, base - 6, 4, 4, 1);
    b.rect(30, base + 2, 10, 6, 4);
    b.ellipse(12, base + 14, 5, 5, 5);
    b.ellipse(32, base + 14, 5, 5, 5);
    b.ellipse(12, base + 14, 2, 2, 1);
    b.ellipse(32, base + 14, 2, 2, 1);
    b.rect(2, base + 4, 4, 3, 6);
    return b;
}

gs::Bitmap rivalArt() {
    gs::Bitmap b(48, 32);
    b.rect(6, 12, 30, 12, 2);
    b.rect(10, 6, 14, 7, 3);
    b.rect(12, 8, 5, 3, 1);
    b.ellipse(14, 24, 4, 4, 5);
    b.ellipse(30, 24, 4, 4, 5);
    b.rect(32, 14, 8, 5, 4);
    return b;
}

gs::Bitmap driveArt() {
    gs::Bitmap b(28, 28);
    b.ellipse(14, 14, 12, 12, 2);
    b.ellipse(14, 14, 7, 7, 3);
    b.ellipse(14, 14, 3, 3, 1);
    b.rect(13, 2, 2, 6, 4);
    b.rect(13, 20, 2, 6, 4);
    b.rect(2, 13, 6, 2, 4);
    b.rect(20, 13, 6, 2, 4);
    return b;
}

gs::Bitmap boomArt() {
    gs::Bitmap b(72, 80);
    b.rect(8, 20, 8, 56, 2);
    b.rect(4, 70, 20, 6, 3);
    b.rect(12, 16, 48, 6, 4);
    b.rect(52, 22, 4, 28, 5);
    b.rect(46, 48, 16, 5, 3);
    b.rect(50, 53, 3, 14, 1);
    b.rect(16, 28, 4, 10, 6);
    return b;
}

gs::Bitmap beamArt() {
    gs::Bitmap b(20, 48);
    b.rect(2, 0, 16, 6, 2);
    b.rect(8, 6, 4, 42, 3);
    b.rect(4, 40, 12, 4, 4);
    return b;
}

gs::Bitmap sleeperArt() {
    gs::Bitmap b(18, 8);
    b.rect(0, 1, 18, 6, 2);
    b.rect(2, 2, 3, 4, 3);
    b.rect(13, 2, 3, 4, 3);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(32, 6);
    b.rect(0, 0, 32, 3, 1);
    b.rect(0, 3, 32, 3, 2);
    return b;
}

gs::Bitmap hillArt() {
    gs::Bitmap b(96, 40);
    b.poly({{0, 40}, {20, 16}, {40, 28}, {62, 6}, {96, 40}}, 2);
    b.poly({{8, 40}, {28, 22}, {48, 32}, {70, 14}, {96, 40}}, 3);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(12, 12);
    b.line(6, 0, 6, 12, 1, 2);
    b.line(0, 6, 12, 6, 1, 2);
    b.ellipse(6, 6, 2, 2, 2);
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
    uint16_t hud[16] = {z, gs::rgb4(15, 15, 15), gs::rgb4(10, 10, 12), gs::rgb4(15, 12, 3), gs::rgb4(15, 4, 3),
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
    body(PAL_CART, gs::rgb4(14, 14, 12), gs::rgb4(4, 8, 14), gs::rgb4(8, 12, 15), gs::rgb4(15, 10, 2), gs::rgb4(2, 2, 3),
         gs::rgb4(15, 6, 1));
    body(PAL_RIVAL, gs::rgb4(14, 12, 10), gs::rgb4(12, 3, 2), gs::rgb4(15, 6, 4), gs::rgb4(8, 8, 9), gs::rgb4(2, 2, 3),
         gs::rgb4(15, 12, 2));
    body(PAL_DRIVE, gs::rgb4(15, 14, 6), gs::rgb4(12, 9, 2), gs::rgb4(15, 12, 4), gs::rgb4(6, 5, 2), gs::rgb4(15, 15, 12),
         gs::rgb4(3, 3, 2));
    body(PAL_BOOM, gs::rgb4(12, 12, 13), gs::rgb4(6, 7, 8), gs::rgb4(4, 5, 6), gs::rgb4(15, 11, 2), gs::rgb4(10, 8, 3),
         gs::rgb4(15, 4, 2));
    body(PAL_IRON, gs::rgb4(11, 11, 12), gs::rgb4(6, 5, 4), gs::rgb4(9, 7, 4), gs::rgb4(14, 8, 2), gs::rgb4(3, 3, 4),
         gs::rgb4(8, 8, 8));
    body(PAL_HILL, gs::rgb4(4, 8, 3), gs::rgb4(3, 6, 3), gs::rgb4(5, 9, 4), gs::rgb4(8, 7, 4), z, z);

    art.cart = gs::uploadMipped(vdp, cartArt(false));
    art.cartDuck = gs::uploadMipped(vdp, cartArt(true));
    art.rival = gs::uploadMipped(vdp, rivalArt());
    art.drive = gs::uploadMipped(vdp, driveArt());
    art.boom = gs::uploadMipped(vdp, boomArt());
    art.beam = gs::uploadMipped(vdp, beamArt());
    art.sleeper = gs::uploadMipped(vdp, sleeperArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.hill = gs::uploadMipped(vdp, hillArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    loadFont(vdp, art);
}

}  // namespace rail
