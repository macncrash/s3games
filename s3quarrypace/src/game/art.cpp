#include "game/art.h"

#include <initializer_list>

namespace quarrypace {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) vdp.setColor(pal * 16 + i++, c);
}

gs::Bitmap blaster(int step) {
    gs::Bitmap b(28, 48);
    b.rect(9, 1, 10, 4, 5);
    b.rect(10, 5, 8, 7, 4);
    b.rect(11, 7, 3, 2, 8);
    b.rect(7, 12, 14, 15, 1);
    b.rect(9, 14, 6, 8, 7);
    b.rect(5, 13, 3, 11, 2);
    b.rect(20, 14, 3, 9, 2);
    int lx = step ? 8 : 11;
    int rx = step ? 16 : 13;
    b.rect(lx, 27, 4, 15, 6);
    b.rect(rx, 27, 4, 15, 11);
    b.rect(lx - 1, 40, 6, 4, 3);
    b.rect(rx - 1, 40, 6, 4, 3);
    b.rect(21, 18, 5, 2, 9);
    return b;
}

gs::Bitmap fallenMan() {
    gs::Bitmap b(48, 18);
    b.ellipse(24, 11, 18, 5, 1);
    b.rect(4, 5, 10, 8, 4);
    b.rect(8, 3, 8, 3, 5);
    b.rect(28, 8, 14, 4, 7);
    return b;
}

gs::Bitmap rockFace() {
    gs::Bitmap b(36, 80);
    b.poly({{2, 10}, {18, 2}, {34, 16}, {32, 78}, {4, 76}}, 1);
    b.poly({{8, 22}, {20, 14}, {28, 30}, {16, 40}}, 2);
    b.rect(10, 48, 12, 18, 3);
    b.rect(6, 68, 20, 6, 4);
    return b;
}

gs::Bitmap derrickArt() {
    gs::Bitmap b(40, 56);
    b.line(4, 52, 20, 4, 1, 2);
    b.line(36, 52, 20, 4, 2, 2);
    b.line(8, 36, 32, 36, 3, 2);
    b.line(12, 22, 28, 22, 3, 2);
    b.rect(18, 2, 4, 8, 4);
    b.rect(17, 48, 6, 8, 5);
    return b;
}

gs::Bitmap quarrySign() {
    gs::Bitmap b(52, 18);
    b.rect(0, 2, 52, 14, 1);
    b.rect(3, 5, 46, 8, 2);
    b.rect(24, 14, 4, 4, 3);
    return b;
}

gs::Bitmap workLamp() {
    gs::Bitmap b(14, 20);
    b.rect(6, 0, 2, 5, 2);
    b.poly({{2, 6}, {12, 6}, {10, 16}, {4, 16}}, 1);
    b.rect(5, 8, 4, 5, 3);
    return b;
}

gs::Bitmap stakeArt(int which) {
    gs::Bitmap b(14, 28);
    b.rect(6, 8, 2, 20, 2);
    int cap = which == 2 ? 3 : 1;
    b.ellipse(7, 6, 5, 5, cap);
    b.rect(6, 4, 2, 3, 4);
    if (which == 1) b.rect(3, 14, 8, 2, 5);
    return b;
}

gs::Bitmap truckArt() {
    gs::Bitmap b(48, 26);
    b.rect(4, 8, 28, 10, 1);
    b.poly({{32, 10}, {44, 10}, {44, 18}, {32, 18}}, 2);
    b.rect(6, 4, 16, 6, 3);
    b.ellipse(12, 20, 5, 5, 4);
    b.ellipse(36, 20, 5, 5, 4);
    return b;
}

gs::Bitmap hopperArt() {
    gs::Bitmap b(28, 36);
    b.poly({{2, 4}, {26, 4}, {20, 22}, {8, 22}}, 1);
    b.rect(10, 22, 8, 12, 2);
    b.rect(4, 32, 20, 3, 3);
    return b;
}

gs::Bitmap kegArt() {
    gs::Bitmap b(16, 18);
    b.ellipse(8, 9, 6, 7, 1);
    b.rect(3, 6, 10, 2, 2);
    b.rect(3, 11, 10, 2, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(8, 30);
    b.rect(3, 0, 2, 30, 1);
    b.rect(1, 2, 6, 3, 2);
    return b;
}

gs::Bitmap crusherArt() {
    gs::Bitmap b(32, 40);
    b.rect(4, 10, 24, 26, 1);
    b.rect(2, 6, 28, 6, 2);
    b.rect(8, 18, 6, 8, 3);
    b.rect(18, 18, 6, 8, 4);
    b.rect(12, 34, 8, 4, 5);
    return b;
}

gs::Bitmap hawkArt() {
    gs::Bitmap b(22, 10);
    b.poly({{2, 6}, {10, 3}, {11, 5}, {2, 8}}, 1);
    b.poly({{20, 6}, {12, 3}, {11, 5}, {20, 8}}, 1);
    b.rect(10, 4, 2, 3, 2);
    return b;
}

gs::Bitmap plungerArt() {
    gs::Bitmap b(22, 40);
    b.rect(8, 16, 6, 20, 1);
    b.rect(4, 32, 14, 6, 2);
    b.rect(9, 4, 4, 14, 3);
    b.ellipse(11, 4, 4, 4, 4);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(8, 8, 2, 2, 2);
    b.rect(7, 0, 2, 4, 3);
    b.rect(7, 12, 2, 4, 3);
    b.rect(0, 7, 4, 2, 3);
    b.rect(12, 7, 4, 2, 3);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4, 4, 1);
    b.ellipse(5, 5, 2, 2, 2);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 10, 8, 8, 1);
    b.ellipse(10, 10, 4, 4, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(18, 12);
    b.ellipse(6, 7, 5, 3, 1);
    b.ellipse(12, 5, 4, 3, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(24, 8);
    b.ellipse(12, 4, 10, 3, 1);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(32, 6);
    b.rect(0, 1, 32, 4, 1);
    b.rect(0, 2, 8, 2, 2);
    b.rect(16, 2, 8, 2, 2);
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
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 11), gs::rgb4(9, 8, 6), gs::rgb4(4, 3, 2)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 11, 2), gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 1), gs::rgb4(9, 6, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), gs::rgb4(15, 9, 6), gs::rgb4(5, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(7, 15, 8), gs::rgb4(13, 15, 10), gs::rgb4(1, 4, 2)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(11, 10, 8), gs::rgb4(7, 6, 5), gs::rgb4(4, 3, 3), gs::rgb4(2, 2, 2),
                            gs::rgb4(13, 11, 7), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_FIGURE, {0, gs::rgb4(8, 5, 2), gs::rgb4(4, 3, 2), gs::rgb4(1, 1, 1), gs::rgb4(13, 9, 6),
                             gs::rgb4(10, 9, 7), gs::rgb4(3, 3, 3), gs::rgb4(12, 7, 3), gs::rgb4(15, 13, 9),
                             gs::rgb4(2, 1, 1), gs::rgb4(14, 12, 6), gs::rgb4(6, 4, 2), gs::rgb4(3, 2, 2)});
    setPal(vdp, PAL_HOLD, {0, gs::rgb4(15, 10, 2), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(6, 15, 7), gs::rgb4(2, 9, 4), gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(11, 7, 3), gs::rgb4(7, 4, 2), gs::rgb4(4, 2, 1), gs::rgb4(13, 10, 5)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 12), gs::rgb4(14, 12, 8), gs::rgb4(10, 8, 6), gs::rgb4(15, 9, 3)});
    setPal(vdp, PAL_ORE, {0, gs::rgb4(9, 6, 3), gs::rgb4(12, 5, 2), gs::rgb4(14, 11, 5), gs::rgb4(3, 2, 1),
                          gs::rgb4(8, 8, 6)});
    setPal(vdp, PAL_METAL, {0, gs::rgb4(12, 12, 11), gs::rgb4(7, 7, 6), gs::rgb4(3, 3, 3), gs::rgb4(15, 9, 2),
                            gs::rgb4(5, 4, 2)});

    const uint16_t floor[16] = {
        0,
        gs::rgb4(8, 7, 5), gs::rgb4(5, 4, 3), gs::rgb4(10, 8, 5),
        gs::rgb4(6, 5, 3), gs::rgb4(4, 3, 2),
        gs::rgb4(12, 10, 6), gs::rgb4(3, 2, 2),
        gs::rgb4(9, 8, 6), gs::rgb4(7, 5, 3), gs::rgb4(11, 9, 7),
        gs::rgb4(2, 2, 1), gs::rgb4(5, 4, 4), gs::rgb4(8, 6, 4),
        gs::rgb4(14, 12, 7), gs::rgb4(6, 6, 5),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, floor[i]);

    loadFont(vdp, art);
    art.walk[0] = gs::uploadMipped(vdp, blaster(0));
    art.walk[1] = gs::uploadMipped(vdp, blaster(1));
    art.fallen = gs::uploadMipped(vdp, fallenMan());
    art.face = gs::uploadMipped(vdp, rockFace());
    art.derrick = gs::uploadMipped(vdp, derrickArt());
    art.sign = gs::uploadMipped(vdp, quarrySign());
    art.lamp = gs::uploadMipped(vdp, workLamp());
    for (int i = 0; i < 3; i++) art.stake[i] = gs::uploadMipped(vdp, stakeArt(i));
    art.truck = gs::uploadMipped(vdp, truckArt());
    art.hopper = gs::uploadMipped(vdp, hopperArt());
    art.keg = gs::uploadMipped(vdp, kegArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.crusher = gs::uploadMipped(vdp, crusherArt());
    art.hawk = gs::uploadMipped(vdp, hawkArt());
    art.plunger = gs::uploadMipped(vdp, plungerArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
}

}  // namespace quarrypace
