#include "game/art.h"

namespace cway {
namespace {

void setPal(gs::VDP& vdp, int pal, const uint16_t* cs, int n) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, i < n ? cs[i] : 0);
}

gs::Bitmap truckArt() {
    gs::Bitmap b(64, 56);
    b.rect(10, 28, 44, 22, 2);
    b.rect(14, 10, 36, 20, 1);
    b.rect(18, 14, 12, 10, 4);
    b.rect(34, 14, 12, 10, 4);
    b.rect(8, 46, 10, 8, 3);
    b.rect(46, 46, 10, 8, 3);
    b.rect(26, 34, 12, 6, 5);
    b.rect(22, 6, 20, 6, 6);
    b.outline(7, false);
    return b;
}

gs::Bitmap chainArt() {
    gs::Bitmap b(96, 16);
    for (int i = 0; i < 8; i++) {
        b.ellipse(8.f + i * 12.f, 8.f, 6.f, 5.f, 1);
        b.ellipse(8.f + i * 12.f, 8.f, 3.f, 2.5f, 0);
    }
    b.outline(2, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(16, 48);
    b.rect(5, 4, 6, 40, 1);
    b.rect(3, 2, 10, 6, 2);
    b.rect(4, 40, 8, 6, 3);
    b.outline(4, false);
    return b;
}

gs::Bitmap keeperArt() {
    gs::Bitmap b(32, 48);
    b.ellipse(16, 8, 6, 6, 3);
    b.rect(12, 14, 8, 16, 1);
    b.rect(6, 16, 6, 12, 2);
    b.rect(20, 16, 6, 12, 2);
    b.rect(11, 30, 4, 14, 4);
    b.rect(17, 30, 4, 14, 4);
    b.rect(8, 18, 16, 4, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(20, 40);
    b.rect(8, 14, 4, 22, 2);
    b.ellipse(10, 8, 7, 6, 1);
    b.ellipse(10, 8, 3, 2, 3);
    b.rect(6, 34, 8, 4, 4);
    b.outline(2, false);
    return b;
}

void loadFont(gs::VDP& vdp, Art& a) {
    gs::TextStyle big;
    big.scale = 2;
    big.color = 1;
    big.outline = 2;
    for (int c = 32; c < 127; c++) {
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {0, gs::rgb4(14, 14, 12), gs::rgb4(6, 6, 7), gs::rgb4(15, 13, 6), gs::rgb4(15, 5, 3),
                            gs::rgb4(8, 14, 7), gs::rgb4(1, 1, 2)};
    const uint16_t truck[] = {0, gs::rgb4(9, 10, 8), gs::rgb4(6, 7, 5), gs::rgb4(2, 2, 2), gs::rgb4(8, 12, 14),
                              gs::rgb4(12, 4, 2), gs::rgb4(14, 12, 3), gs::rgb4(1, 1, 1)};
    const uint16_t chain[] = {0, gs::rgb4(12, 12, 11), gs::rgb4(4, 4, 5), gs::rgb4(15, 13, 4)};
    const uint16_t keeper[] = {0, gs::rgb4(4, 6, 8), gs::rgb4(3, 4, 6), gs::rgb4(12, 9, 6), gs::rgb4(2, 2, 3),
                               gs::rgb4(14, 12, 3), gs::rgb4(1, 1, 1)};
    const uint16_t lamp[] = {0, gs::rgb4(15, 14, 6), gs::rgb4(5, 5, 5), gs::rgb4(15, 15, 12), gs::rgb4(3, 3, 3)};
    const uint16_t stone[] = {0, gs::rgb4(8, 8, 7), gs::rgb4(11, 10, 8), gs::rgb4(5, 5, 4), gs::rgb4(2, 2, 2)};
    const uint16_t ok[] = {0, gs::rgb4(8, 14, 7), gs::rgb4(3, 6, 3)};
    const uint16_t alert[] = {0, gs::rgb4(15, 5, 3), gs::rgb4(8, 2, 2)};
    setPal(vdp, PAL_HUD, hud, 7);
    setPal(vdp, PAL_TRUCK, truck, 8);
    setPal(vdp, PAL_CHAIN, chain, 4);
    setPal(vdp, PAL_KEEPER, keeper, 7);
    setPal(vdp, PAL_LAMP, lamp, 5);
    setPal(vdp, PAL_STONE, stone, 5);
    setPal(vdp, PAL_OK, ok, 3);
    setPal(vdp, PAL_ALERT, alert, 3);

    const uint16_t field[16] = {
        0,
        gs::rgb4(4, 5, 4), gs::rgb4(3, 4, 3), gs::rgb4(6, 6, 5),
        gs::rgb4(7, 7, 6), gs::rgb4(5, 5, 4),
        gs::rgb4(6, 6, 6), gs::rgb4(4, 4, 5),
        gs::rgb4(8, 8, 7), gs::rgb4(5, 5, 5), gs::rgb4(9, 9, 8),
        gs::rgb4(1, 3, 6), gs::rgb4(2, 4, 7), gs::rgb4(5, 8, 10),
        gs::rgb4(14, 13, 8), gs::rgb4(10, 10, 9),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, field[i]);
    vdp.setFogColor(gs::rgb4(6, 7, 9));

    loadFont(vdp, art);
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.chain = gs::uploadMipped(vdp, chainArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.keeper = gs::uploadMipped(vdp, keeperArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
}

}  // namespace cway
