#include "game/art.h"

#include <initializer_list>

namespace viaductpace {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) vdp.setColor(pal * 16 + i++, c);
}

gs::Bitmap walker(int step) {
    gs::Bitmap b(26, 46);
    b.rect(9, 1, 8, 7, 9);
    b.rect(10, 3, 6, 4, 8);
    b.rect(7, 8, 12, 16, 1);
    b.rect(9, 10, 8, 6, 5);
    b.rect(5, 11, 3, 11, 2);
    b.rect(18, 11, 3, 11, 2);
    int lx = step ? 8 : 11;
    int rx = step ? 15 : 12;
    b.rect(lx, 24, 4, 16, 6);
    b.rect(rx, 24, 4, 16, 11);
    b.rect(lx - 1, 38, 6, 3, 3);
    b.rect(rx - 1, 38, 6, 3, 3);
    b.rect(18, 15, 6, 2, 4);
    return b;
}

gs::Bitmap fallenMan() {
    gs::Bitmap b(46, 16);
    b.ellipse(24, 9, 16, 5, 1);
    b.rect(4, 5, 9, 7, 9);
    b.rect(28, 7, 12, 4, 5);
    return b;
}

gs::Bitmap archArt() {
    gs::Bitmap b(48, 36);
    b.rect(0, 0, 6, 36, 1);
    b.rect(42, 0, 6, 36, 1);
    b.rect(0, 0, 48, 6, 2);
    b.ellipse(24, 22, 16, 14, 3);
    b.ellipse(24, 24, 12, 10, 0);
    b.rect(18, 4, 12, 4, 4);
    return b;
}

gs::Bitmap pierArt() {
    gs::Bitmap b(18, 56);
    b.rect(2, 0, 14, 56, 1);
    b.rect(0, 0, 18, 6, 2);
    for (int y = 10; y < 50; y += 8) b.rect(4, y, 10, 2, 3);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 28);
    b.rect(3, 0, 4, 24, 1);
    b.rect(1, 22, 8, 6, 2);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(12, 20);
    b.rect(5, 0, 2, 6, 2);
    b.rect(2, 6, 8, 9, 1);
    b.rect(4, 8, 4, 5, 3);
    return b;
}

gs::Bitmap boardArt(int which) {
    gs::Bitmap b(18, 24);
    int body = which == 2 ? 3 : 1;
    b.rect(2, 0, 14, 12, body);
    b.rect(8, 12, 2, 12, 2);
    if (which >= 1) b.rect(5, 3, 3, 6, 4);
    if (which >= 2) b.rect(10, 3, 3, 6, 4);
    return b;
}

gs::Bitmap trainArt() {
    gs::Bitmap b(64, 22);
    b.rect(4, 6, 48, 12, 1);
    b.rect(48, 8, 12, 10, 2);
    b.rect(8, 2, 14, 6, 3);
    for (int x = 10; x < 46; x += 10) b.rect(x, 8, 6, 4, 4);
    b.ellipse(16, 18, 4, 4, 5);
    b.ellipse(40, 18, 4, 4, 5);
    return b;
}

gs::Bitmap railArt() {
    gs::Bitmap b(20, 16);
    b.rect(2, 2, 3, 12, 1);
    b.rect(15, 2, 3, 12, 1);
    b.rect(0, 6, 20, 3, 2);
    return b;
}

gs::Bitmap cloudArt() {
    gs::Bitmap b(28, 12);
    b.ellipse(10, 7, 8, 4, 1);
    b.ellipse(18, 6, 7, 4, 2);
    return b;
}

gs::Bitmap rifleArt() {
    gs::Bitmap b(14, 40);
    b.rect(6, 0, 3, 28, 1);
    b.rect(4, 26, 7, 6, 2);
    b.rect(5, 32, 4, 8, 3);
    return b;
}

gs::Bitmap beadArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(8, 8, 2, 2, 3);
    return b;
}

gs::Bitmap pipArt() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 6, 5, 5, 1);
    b.ellipse(6, 6, 2, 2, 2);
    return b;
}

gs::Bitmap flashArt() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 6, 6, 1);
    b.ellipse(8, 8, 3, 3, 2);
    return b;
}

gs::Bitmap sprayArt() {
    gs::Bitmap b(18, 14);
    b.ellipse(9, 8, 7, 4, 1);
    b.ellipse(6, 6, 3, 2, 2);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(30, 10);
    b.ellipse(15, 5, 12, 3, 1);
    return b;
}

gs::Bitmap stripeArt() {
    gs::Bitmap b(32, 8);
    for (int x = 0; x < 32; x += 8) {
        b.rect(float(x), 1, 4, 6, 1);
        b.rect(float(x + 4), 1, 4, 6, 3);
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
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 15, 15), gs::rgb4(8, 10, 12), gs::rgb4(3, 4, 6)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(15, 14, 9), gs::rgb4(3, 3, 2), gs::rgb4(8, 6, 2)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 3, 2), gs::rgb4(15, 10, 8), gs::rgb4(4, 1, 1)});
    setPal(vdp, PAL_GOOD, {0, gs::rgb4(6, 15, 10), gs::rgb4(12, 15, 13), gs::rgb4(1, 4, 3)});
    setPal(vdp, PAL_STONE, {0, gs::rgb4(10, 10, 11), gs::rgb4(7, 7, 8), gs::rgb4(4, 4, 5), gs::rgb4(2, 2, 3),
                            gs::rgb4(13, 11, 8), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_FIGURE, {0, gs::rgb4(5, 6, 8), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2), gs::rgb4(12, 9, 6),
                             gs::rgb4(8, 7, 6), gs::rgb4(4, 5, 6), gs::rgb4(9, 6, 4), gs::rgb4(14, 12, 9),
                             gs::rgb4(2, 2, 2), gs::rgb4(13, 11, 8), gs::rgb4(6, 4, 3), gs::rgb4(3, 3, 5)});
    setPal(vdp, PAL_HOLD, {0, gs::rgb4(15, 11, 3), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(8, 15, 10), gs::rgb4(3, 10, 8), gs::rgb4(1, 3, 2)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(6, 7, 8), gs::rgb4(9, 8, 6), gs::rgb4(3, 3, 4), gs::rgb4(12, 11, 9)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 14, 15), gs::rgb4(8, 10, 12), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_TRAIN, {0, gs::rgb4(8, 3, 3), gs::rgb4(4, 4, 5), gs::rgb4(12, 10, 6), gs::rgb4(14, 13, 10),
                            gs::rgb4(2, 2, 2), gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_METAL, {0, gs::rgb4(11, 12, 13), gs::rgb4(7, 8, 9), gs::rgb4(3, 4, 5), gs::rgb4(14, 10, 3),
                            gs::rgb4(6, 5, 3)});

    const uint16_t deck[16] = {
        0,
        gs::rgb4(6, 6, 7), gs::rgb4(4, 4, 5), gs::rgb4(8, 8, 8),
        gs::rgb4(10, 9, 7), gs::rgb4(3, 3, 4),
        gs::rgb4(5, 5, 6), gs::rgb4(2, 2, 3),
        gs::rgb4(9, 8, 6), gs::rgb4(7, 6, 5), gs::rgb4(12, 11, 8),
        gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 3), gs::rgb4(5, 4, 4),
        gs::rgb4(14, 13, 10), gs::rgb4(8, 7, 6),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, deck[i]);

    loadFont(vdp, art);
    art.walk[0] = gs::uploadMipped(vdp, walker(0));
    art.walk[1] = gs::uploadMipped(vdp, walker(1));
    art.fallen = gs::uploadMipped(vdp, fallenMan());
    art.arch = gs::uploadMipped(vdp, archArt());
    art.pier = gs::uploadMipped(vdp, pierArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    for (int i = 0; i < 3; i++) art.board[i] = gs::uploadMipped(vdp, boardArt(i));
    art.train = gs::uploadMipped(vdp, trainArt());
    art.rail = gs::uploadMipped(vdp, railArt());
    art.cloud = gs::uploadMipped(vdp, cloudArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
}

}  // namespace viaductpace
