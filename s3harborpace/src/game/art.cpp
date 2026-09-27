#include "game/art.h"

#include <initializer_list>

namespace harborpace {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cols) {
    int i = 0;
    for (uint16_t c : cols) vdp.setColor(pal * 16 + i++, c);
}

gs::Bitmap watchman(int step) {
    gs::Bitmap b(28, 48);
    b.rect(10, 2, 8, 8, 4);
    b.rect(11, 3, 6, 5, 10);
    b.rect(8, 10, 12, 16, 1);
    b.rect(10, 12, 8, 8, 7);
    b.rect(6, 12, 3, 10, 2);
    b.rect(19, 12, 3, 10, 2);
    int lx = step ? 9 : 11;
    int rx = step ? 16 : 14;
    b.rect(lx, 26, 4, 16, 6);
    b.rect(rx, 26, 4, 16, 11);
    b.rect(lx - 1, 40, 6, 4, 3);
    b.rect(rx - 1, 40, 6, 4, 3);
    b.rect(20, 16, 6, 2, 5);
    return b;
}

gs::Bitmap fallenMan() {
    gs::Bitmap b(48, 18);
    b.ellipse(24, 10, 18, 6, 1);
    b.rect(6, 6, 10, 8, 4);
    b.rect(30, 8, 12, 4, 7);
    return b;
}

gs::Bitmap warehouse() {
    gs::Bitmap b(40, 72);
    b.rect(2, 8, 36, 64, 1);
    b.rect(0, 4, 40, 8, 2);
    for (int y = 18; y < 64; y += 12) b.rect(8, y, 10, 7, 3);
    b.rect(22, 40, 12, 28, 4);
    return b;
}

gs::Bitmap craneArm() {
    gs::Bitmap b(48, 28);
    b.line(4, 24, 40, 6, 1, 3);
    b.rect(36, 4, 8, 6, 2);
    b.line(40, 10, 40, 26, 3, 1);
    return b;
}

gs::Bitmap harborSign() {
    gs::Bitmap b(56, 16);
    b.rect(0, 2, 56, 12, 1);
    b.rect(4, 5, 48, 6, 2);
    return b;
}

gs::Bitmap lantern() {
    gs::Bitmap b(12, 18);
    b.rect(5, 0, 2, 4, 2);
    b.rect(2, 4, 8, 10, 1);
    b.rect(4, 6, 4, 6, 3);
    return b;
}

gs::Bitmap buoyArt(int which) {
    gs::Bitmap b(16, 22);
    int body = which == 2 ? 3 : 1;
    b.ellipse(8, 8, 6, 6, body);
    b.rect(7, 14, 2, 8, 2);
    b.ellipse(8, 8, 2, 2, 4);
    return b;
}

gs::Bitmap hullArt() {
    gs::Bitmap b(56, 24);
    b.poly({{4, 6}, {50, 6}, {54, 18}, {2, 18}}, 1);
    b.rect(10, 2, 22, 6, 2);
    b.rect(14, 8, 8, 4, 3);
    return b;
}

gs::Bitmap mastArt() {
    gs::Bitmap b(20, 40);
    b.rect(9, 2, 2, 36, 1);
    b.poly({{11, 4}, {18, 16}, {11, 16}}, 2);
    return b;
}

gs::Bitmap bollardArt() {
    gs::Bitmap b(14, 20);
    b.ellipse(7, 5, 5, 4, 1);
    b.rect(5, 8, 4, 10, 2);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(18, 16);
    b.rect(1, 1, 16, 14, 1);
    b.line(1, 1, 16, 14, 2, 1);
    b.line(16, 1, 1, 14, 3, 1);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(8, 28);
    b.rect(3, 0, 2, 28, 1);
    return b;
}

gs::Bitmap stackArt() {
    gs::Bitmap b(16, 48);
    b.rect(3, 8, 10, 40, 1);
    b.rect(1, 4, 14, 6, 2);
    return b;
}

gs::Bitmap gullArt() {
    gs::Bitmap b(20, 8);
    b.line(0, 4, 10, 2, 1, 1);
    b.line(10, 2, 20, 5, 1, 1);
    return b;
}

gs::Bitmap rifleArt() {
    gs::Bitmap b(14, 36);
    b.rect(6, 0, 3, 28, 1);
    b.rect(3, 22, 8, 5, 2);
    b.rect(5, 28, 4, 6, 3);
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
    b.ellipse(9, 9, 8, 5, 1);
    b.ellipse(9, 9, 3, 2, 2);
    return b;
}

gs::Bitmap sprayArt() {
    gs::Bitmap b(24, 14);
    b.ellipse(12, 8, 10, 4, 2);
    b.ellipse(8, 7, 4, 3, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
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
    setPal(vdp, PAL_STONE, {0, gs::rgb4(9, 10, 11), gs::rgb4(6, 7, 8), gs::rgb4(3, 4, 6), gs::rgb4(2, 2, 3),
                            gs::rgb4(12, 11, 8), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_FIGURE, {0, gs::rgb4(4, 6, 9), gs::rgb4(2, 3, 5), gs::rgb4(1, 1, 2), gs::rgb4(12, 9, 6),
                             gs::rgb4(8, 8, 9), gs::rgb4(3, 4, 6), gs::rgb4(10, 7, 4), gs::rgb4(14, 12, 8),
                             gs::rgb4(1, 1, 1), gs::rgb4(13, 11, 8), gs::rgb4(6, 4, 3), gs::rgb4(2, 2, 4)});
    setPal(vdp, PAL_HOLD, {0, gs::rgb4(15, 11, 3), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_LIVE, {0, gs::rgb4(8, 15, 10), gs::rgb4(3, 10, 8), gs::rgb4(1, 3, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(10, 7, 4), gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(12, 10, 6)});
    setPal(vdp, PAL_FX, {0, gs::rgb4(15, 15, 13), gs::rgb4(12, 14, 15), gs::rgb4(8, 10, 12), gs::rgb4(15, 12, 6)});
    setPal(vdp, PAL_HULL, {0, gs::rgb4(5, 7, 8), gs::rgb4(10, 4, 3), gs::rgb4(14, 12, 8), gs::rgb4(2, 3, 4),
                           gs::rgb4(8, 9, 10)});
    setPal(vdp, PAL_METAL, {0, gs::rgb4(11, 12, 13), gs::rgb4(7, 8, 9), gs::rgb4(3, 4, 5), gs::rgb4(14, 10, 3),
                            gs::rgb4(6, 5, 3)});

    const uint16_t water[16] = {
        0,
        gs::rgb4(2, 5, 8), gs::rgb4(1, 3, 6), gs::rgb4(3, 7, 10),
        gs::rgb4(4, 8, 11), gs::rgb4(2, 4, 7),
        gs::rgb4(6, 9, 11), gs::rgb4(1, 2, 4),
        gs::rgb4(8, 11, 12), gs::rgb4(3, 6, 8), gs::rgb4(5, 8, 9),
        gs::rgb4(1, 2, 3), gs::rgb4(2, 3, 5), gs::rgb4(4, 6, 8),
        gs::rgb4(12, 13, 10), gs::rgb4(7, 10, 11),
    };
    for (int i = 0; i < 16; i++) vdp.setColor(PAL_ROAD * 16 + i, water[i]);

    loadFont(vdp, art);
    art.walk[0] = gs::uploadMipped(vdp, watchman(0));
    art.walk[1] = gs::uploadMipped(vdp, watchman(1));
    art.fallen = gs::uploadMipped(vdp, fallenMan());
    art.shed = gs::uploadMipped(vdp, warehouse());
    art.crane = gs::uploadMipped(vdp, craneArm());
    art.sign = gs::uploadMipped(vdp, harborSign());
    art.lamp = gs::uploadMipped(vdp, lantern());
    for (int i = 0; i < 3; i++) art.buoy[i] = gs::uploadMipped(vdp, buoyArt(i));
    art.hull = gs::uploadMipped(vdp, hullArt());
    art.mast = gs::uploadMipped(vdp, mastArt());
    art.bollard = gs::uploadMipped(vdp, bollardArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.stack = gs::uploadMipped(vdp, stackArt());
    art.gull = gs::uploadMipped(vdp, gullArt());
    art.rifle = gs::uploadMipped(vdp, rifleArt());
    art.bead = gs::uploadMipped(vdp, beadArt());
    art.pip = gs::uploadMipped(vdp, pipArt());
    art.flash = gs::uploadMipped(vdp, flashArt());
    art.spray = gs::uploadMipped(vdp, sprayArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.stripe = gs::uploadMipped(vdp, stripeArt());
}

}  // namespace harborpace
