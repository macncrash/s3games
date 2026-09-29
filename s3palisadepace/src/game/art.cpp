#include "game/art.h"

#include <initializer_list>

namespace palisadepace {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) vdp.setColor(pal * 16 + i++, c);
}

gs::Bitmap raider(int step) {
    gs::Bitmap b(26, 46);
    b.rect(9, 1, 8, 7, 4);
    b.rect(8, 0, 10, 3, 5);
    b.rect(10, 3, 6, 4, 8);
    b.rect(7, 9, 12, 15, 1);
    b.rect(9, 11, 8, 7, 6);
    b.rect(4, 11, 4, 9, 2);
    b.rect(18, 11, 4, 9, 2);
    int lx = step ? 8 : 11;
    int rx = step ? 15 : 12;
    b.rect(lx, 24, 4, 15, 3);
    b.rect(rx, 24, 4, 15, 7);
    b.rect(lx - 1, 38, 6, 4, 9);
    b.rect(rx - 1, 38, 6, 4, 9);
    b.rect(18, 16, 7, 2, 10);
    return b;
}

gs::Bitmap fallenRaider() {
    gs::Bitmap b(46, 16);
    b.ellipse(22, 9, 16, 5, 1);
    b.rect(4, 5, 10, 7, 4);
    b.rect(28, 7, 12, 4, 6);
    b.rect(16, 4, 8, 3, 5);
    return b;
}

gs::Bitmap stakeArt() {
    gs::Bitmap b(12, 48);
    b.poly({{6, 0}, {10, 10}, {9, 46}, {3, 46}, {2, 10}}, 1);
    b.line(6, 2, 6, 44, 2, 1);
    b.rect(2, 14, 8, 2, 3);
    return b;
}

gs::Bitmap gateArt() {
    gs::Bitmap b(48, 56);
    b.rect(2, 8, 6, 48, 1);
    b.rect(40, 8, 6, 48, 1);
    b.rect(4, 6, 40, 6, 2);
    for (int x = 10; x < 40; x += 6) b.rect(x, 14, 3, 40, 3);
    b.rect(8, 28, 32, 3, 4);
    return b;
}

gs::Bitmap bannerArt() {
    gs::Bitmap b(40, 22);
    b.rect(2, 2, 36, 14, 1);
    b.rect(6, 6, 28, 6, 2);
    b.poly({{4, 16}, {12, 16}, {8, 21}}, 3);
    b.poly({{28, 16}, {36, 16}, {32, 21}}, 3);
    return b;
}

gs::Bitmap torchArt() {
    gs::Bitmap b(10, 22);
    b.rect(4, 10, 2, 12, 1);
    b.ellipse(5, 6, 4, 5, 2);
    b.ellipse(5, 7, 2, 2, 3);
    return b;
}

gs::Bitmap pineArt() {
    gs::Bitmap b(28, 48);
    b.poly({{14, 2}, {26, 28}, {2, 28}}, 1);
    b.poly({{14, 12}, {24, 36}, {4, 36}}, 2);
    b.rect(12, 34, 4, 14, 3);
    return b;
}

gs::Bitmap drumArt() {
    gs::Bitmap b(18, 16);
    b.ellipse(9, 8, 8, 6, 1);
    b.ellipse(9, 6, 6, 3, 2);
    b.rect(2, 8, 2, 4, 3);
    return b;
}

gs::Bitmap musketArt() {
    gs::Bitmap b(12, 40);
    b.rect(5, 0, 2, 30, 1);
    b.rect(2, 22, 8, 5, 2);
    b.rect(4, 28, 4, 8, 3);
    b.rect(4, 2, 4, 3, 4);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6, 6, 1);
    b.ellipse(7, 7, 2, 2, 2);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 4, 1);
    b.ellipse(9, 9, 3, 2, 2);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(22, 12);
    b.ellipse(11, 7, 9, 4, 1);
    b.ellipse(6, 6, 3, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(30, 10);
    b.ellipse(15, 5, 13, 3, 1);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(32, 8);
    for (int x = 0; x < 32; x += 8) {
        b.rect(float(x), 1, 4, 6, 1);
        b.rect(float(x + 4), 1, 4, 6, 2);
    }
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
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 11, 3), gs::rgb4(15, 14, 8), gs::rgb4(4, 3, 1), gs::rgb4(8, 5, 2)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 9, 6), gs::rgb4(5, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(7, 14, 6), gs::rgb4(13, 15, 10), gs::rgb4(1, 4, 2)});
    setPal(vdp, PAL_TIMBER, {0, gs::rgb4(9, 6, 3), gs::rgb4(6, 4, 2), gs::rgb4(12, 9, 5), gs::rgb4(4, 3, 2),
                             gs::rgb4(14, 11, 6)});
    setPal(vdp, PAL_FIGURE, {0, gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(10, 7, 4), gs::rgb4(3, 3, 4),
                             gs::rgb4(12, 9, 6), gs::rgb4(5, 4, 3), gs::rgb4(8, 3, 2), gs::rgb4(13, 10, 7),
                             gs::rgb4(14, 12, 9), gs::rgb4(2, 2, 2), gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_HOLD, {0, gs::rgb4(15, 10, 2), gs::rgb4(8, 5, 1), gs::rgb4(15, 15, 10)});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(8, 15, 6), gs::rgb4(3, 9, 4), gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_PINE, {0, gs::rgb4(2, 6, 3), gs::rgb4(3, 8, 4), gs::rgb4(6, 4, 2), gs::rgb4(1, 3, 2)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 14, 8), gs::rgb4(10, 8, 5), gs::rgb4(15, 8, 3), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_CLOTH, {0, gs::rgb4(10, 2, 2), gs::rgb4(14, 12, 6), gs::rgb4(6, 1, 1), gs::rgb4(12, 8, 3)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(10, 10, 11), gs::rgb4(6, 6, 7), gs::rgb4(3, 3, 4), gs::rgb4(14, 9, 3),
                           gs::rgb4(8, 7, 5)});

    const uint16_t earth[16] = {
        0,
        gs::rgb4(6, 4, 2), gs::rgb4(4, 3, 2), gs::rgb4(8, 6, 3),
        gs::rgb4(9, 7, 4), gs::rgb4(5, 4, 2),
        gs::rgb4(7, 5, 3), gs::rgb4(3, 2, 1),
        gs::rgb4(10, 8, 4), gs::rgb4(5, 3, 2), gs::rgb4(8, 5, 3),
        gs::rgb4(2, 2, 1), gs::rgb4(3, 2, 2), gs::rgb4(7, 6, 4),
        gs::rgb4(12, 10, 6), gs::rgb4(6, 5, 3),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, earth[i]);

    loadFont(vdp, art);
    art.walk[0] = gs::uploadMipped(vdp, raider(0));
    art.walk[1] = gs::uploadMipped(vdp, raider(1));
    art.fallen = gs::uploadMipped(vdp, fallenRaider());
    art.stake = gs::uploadMipped(vdp, stakeArt());
    art.gate = gs::uploadMipped(vdp, gateArt());
    art.banner = gs::uploadMipped(vdp, bannerArt());
    art.torch = gs::uploadMipped(vdp, torchArt());
    art.pine = gs::uploadMipped(vdp, pineArt());
    art.drum = gs::uploadMipped(vdp, drumArt());
    art.musket = gs::uploadMipped(vdp, musketArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
}

}  // namespace palisadepace
