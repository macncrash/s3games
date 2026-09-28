#include "game/art.h"

#include <cstdint>

namespace lotdoor {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* c, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? c[i] : 0);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{2, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.shared(px);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

gs::Bitmap gateArt() {
    gs::Bitmap b(48, 200);
    b.rect(6, 0, 6, 200, 2);
    b.rect(36, 0, 6, 200, 2);
    for (int y = 4; y < 196; y += 8) b.rect(8, float(y), 32, 3, 3);
    for (int x = 14; x < 36; x += 8) b.rect(float(x), 0, 2, 200, 4);
    b.rect(0, 0, 8, 200, 1);
    for (int y = 10; y < 190; y += 28) {
        b.rect(0, float(y), 14, 6, 5);
        b.ellipse(6, float(y + 3), 2.4f, 2.4f, 6);
    }
    b.rect(40, 88, 8, 24, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap cartArt() {
    gs::Bitmap b(40, 26);
    b.rect(2, 8, 18, 12, 2);
    b.rect(4, 10, 12, 6, 3);
    b.rect(18, 11, 18, 5, 4);
    b.rect(18, 12, 16, 2, 5);
    b.ellipse(8, 20, 4, 4, 6);
    b.ellipse(16, 20, 4, 4, 6);
    b.ellipse(36, 13, 2.2f, 2.2f, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap sedanArt() {
    gs::Bitmap b(56, 24);
    b.poly({{2, 16}, {8, 12}, {16, 8}, {34, 8}, {42, 12}, {54, 14}, {54, 18}, {4, 18}}, 2);
    b.poly({{18, 9}, {32, 9}, {36, 13}, {16, 13}}, 3);
    b.rect(6, 14, 8, 3, 4);
    b.rect(40, 13, 10, 3, 5);
    b.ellipse(14, 18, 4, 4, 6);
    b.ellipse(42, 18, 4, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap truckArt() {
    gs::Bitmap b(72, 32);
    b.rect(4, 6, 40, 18, 2);
    b.rect(8, 8, 32, 8, 3);
    b.poly({{44, 12}, {52, 8}, {66, 10}, {68, 16}, {66, 22}, {44, 22}}, 4);
    b.rect(54, 12, 8, 6, 5);
    b.ellipse(16, 24, 5, 5, 6);
    b.ellipse(32, 24, 5, 5, 6);
    b.ellipse(58, 24, 5, 5, 6);
    b.rect(2, 12, 6, 6, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap flareArt() {
    gs::Bitmap b(14, 6);
    b.rect(0, 1, 10, 4, 1);
    b.rect(8, 2, 6, 2, 2);
    b.ellipse(12, 3, 2, 2, 3);
    return b;
}

gs::Bitmap sparkArt() {
    gs::Bitmap b(18, 14);
    b.ellipse(9, 8, 7, 4, 1);
    b.ellipse(5, 5, 3, 3, 2);
    b.ellipse(13, 4, 2, 3, 3);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(16, 48);
    b.rect(6, 10, 4, 38, 1);
    b.ellipse(8, 8, 6, 5, 2);
    b.ellipse(8, 7, 3, 2, 3);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(15, 15, 13), gs::rgb4(7, 7, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 2)};
    const uint16_t gate[] = {0, gs::rgb4(2, 2, 2), gs::rgb4(6, 6, 7), gs::rgb4(8, 8, 9), gs::rgb4(5, 6, 6),
                             gs::rgb4(10, 9, 4), gs::rgb4(4, 3, 1), gs::rgb4(12, 4, 3)};
    const uint16_t cart[] = {0, gs::rgb4(2, 2, 3), gs::rgb4(9, 8, 3), gs::rgb4(13, 11, 4), gs::rgb4(5, 5, 6),
                             gs::rgb4(12, 12, 10), gs::rgb4(2, 2, 2), gs::rgb4(15, 12, 4)};
    const uint16_t sedan[] = {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 5, 9), gs::rgb4(8, 11, 14), gs::rgb4(15, 13, 4),
                              gs::rgb4(14, 4, 3), gs::rgb4(1, 1, 1)};
    const uint16_t truck[] = {0, gs::rgb4(2, 1, 1), gs::rgb4(8, 3, 2), gs::rgb4(12, 8, 4), gs::rgb4(4, 4, 5),
                              gs::rgb4(10, 12, 13), gs::rgb4(1, 1, 1), gs::rgb4(14, 12, 3)};
    const uint16_t flare[] = {0, gs::rgb4(15, 8, 2), gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 12)};
    const uint16_t lamp[] = {0, gs::rgb4(4, 4, 5), gs::rgb4(14, 10, 3), gs::rgb4(15, 15, 10)};
    const uint16_t gold[] = {0, gs::rgb4(15, 13, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 2, 0)};
    const uint16_t alert[] = {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(3, 0, 0)};
    const uint16_t lot[] = {gs::rgb4(2, 2, 2), gs::rgb4(3, 3, 3), gs::rgb4(4, 4, 4), gs::rgb4(5, 5, 5),
                            gs::rgb4(8, 8, 7), gs::rgb4(3, 3, 2), gs::rgb4(2, 2, 3), gs::rgb4(6, 6, 5),
                            gs::rgb4(7, 7, 6), gs::rgb4(9, 9, 7), gs::rgb4(4, 4, 3), gs::rgb4(5, 5, 4),
                            gs::rgb4(10, 10, 8), gs::rgb4(1, 1, 1), gs::rgb4(6, 5, 3), gs::rgb4(11, 10, 6)};
    setPal(vdp, PAL_HUD, hud, 16);
    setPal(vdp, PAL_GATE, gate, 8);
    setPal(vdp, PAL_CART, cart, 8);
    setPal(vdp, PAL_SEDAN, sedan, 7);
    setPal(vdp, PAL_TRUCK, truck, 8);
    setPal(vdp, PAL_FLARE, flare, 4);
    setPal(vdp, PAL_LAMP, lamp, 4);
    setPal(vdp, PAL_GOLD, gold, 16);
    setPal(vdp, PAL_ALERT, alert, 16);
    setPal(vdp, PAL_LOT, lot, 16);
    vdp.setFogColor(gs::rgb4(2, 2, 3));

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.cart = gs::uploadMipped(vdp, cartArt());
    art.sedan = gs::uploadMipped(vdp, sedanArt());
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.flare = gs::uploadMipped(vdp, flareArt());
    art.spark = gs::uploadMipped(vdp, sparkArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace lotdoor
