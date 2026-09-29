#include "game/art.h"

namespace orchardladd {
namespace {

uint16_t C(int r, int g, int b) { return gs::rgb4(r, g, b); }

void setPal(gs::VDP& vdp, int pal, const uint16_t* c) {
    for (int i = 0; i < 16; i++) vdp.setColor(pal * 16 + i, c[i]);
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    gs::TextStyle big{2, 1, 0, 15, 1};
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        if (g) {
            for (int y = 0; y < 7; y++)
                for (int x = 0; x < 5; x++)
                    if (g[y * 5 + x]) {
                        px[y * 8 + x + 1] = 1;
                        if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                    }
        }
        int t = tiles.shared(px);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
        a.glyph[c - 32] = gs::uploadMipped(vdp, gs::textBitmap(std::string(1, char(c)), big));
    }
}

void tileSolid(uint8_t* px, int c, int d) {
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) px[y * 8 + x] = ((x + y) & 1) ? uint8_t(d) : uint8_t(c);
}

gs::Bitmap picker(int step, bool climb, bool air) {
    gs::Bitmap b(24, 40);
    b.ellipse(12, 5, 4, 4, 3);
    b.rect(8, 1, 8, 3, 5);
    b.rect(9, 10, 7, 11, 4);
    b.rect(10, 12, 5, 4, 6);
    b.rect(4, 12, 5, 8, 4);
    b.rect(16, 12, 5, 8, 4);
    if (air) {
        b.rect(7, 22, 3, 10, 2);
        b.rect(14, 22, 3, 8, 2);
    } else if (climb) {
        b.rect(8, 22, 3, 14, 2);
        b.rect(13, 22, 3, 14, 2);
        b.rect(5, step ? 14 : 18, 3, 3, 3);
        b.rect(17, step ? 18 : 14, 3, 3, 3);
    } else {
        int ly = step ? 22 : 24;
        int ry = step ? 24 : 22;
        b.rect(8, ly, 3, 13, 2);
        b.rect(13, ry, 3, 13, 2);
        b.rect(7, 35, 5, 3, 7);
        b.rect(13, 35, 5, 3, 7);
    }
    return b;
}

gs::Bitmap ladderBmp() {
    gs::Bitmap b(16, 28);
    b.rect(1, 0, 2, 28, 2);
    b.rect(12, 0, 2, 28, 2);
    for (int i = 0; i < 5; i++) b.rect(1, 2 + i * 5, 13, 2, 3);
    return b;
}

gs::Bitmap barrelBmp() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 2);
    b.ellipse(11, 11, 7, 7, 3);
    b.rect(4, 9, 14, 2, 4);
    b.rect(4, 13, 14, 2, 4);
    b.ellipse(8, 8, 2, 1, 5);
    return b;
}

gs::Bitmap waspBmp() {
    gs::Bitmap b(20, 14);
    b.ellipse(10, 8, 6, 4, 2);
    b.ellipse(10, 8, 3, 2, 3);
    b.ellipse(4, 4, 4, 2, 4);
    b.ellipse(16, 4, 4, 2, 4);
    b.ellipse(6, 8, 1, 1, 5);
    return b;
}

gs::Bitmap appleBmp() {
    gs::Bitmap b(12, 12);
    b.ellipse(6, 7, 5, 4, 2);
    b.ellipse(5, 6, 2, 2, 3);
    b.line(6, 1, 7, 4, 4, 1);
    b.set(8, 2, 1);
    return b;
}

gs::Bitmap treeBmp() {
    gs::Bitmap b(40, 72);
    b.rect(17, 40, 6, 32, 4);
    b.rect(18, 40, 2, 30, 5);
    b.ellipse(20, 28, 16, 18, 2);
    b.ellipse(12, 22, 8, 7, 3);
    b.ellipse(28, 20, 7, 6, 1);
    b.set(14, 18, 6);
    b.set(26, 26, 6);
    b.set(20, 14, 6);
    return b;
}

gs::Bitmap hiveBmp() {
    gs::Bitmap b(18, 16);
    b.ellipse(9, 9, 8, 6, 2);
    b.ellipse(9, 9, 5, 3, 3);
    b.rect(7, 2, 4, 4, 4);
    return b;
}

gs::Bitmap crateBmp() {
    gs::Bitmap b(28, 18);
    b.rect(1, 2, 26, 14, 2);
    b.rect(2, 3, 24, 12, 3);
    b.line(2, 3, 26, 15, 4, 1);
    b.line(26, 3, 2, 15, 4, 1);
    return b;
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t hud[] = {C(0, 0, 0), C(15, 14, 10), C(8, 6, 3), C(15, 15, 15), C(4, 3, 2), C(12, 10, 6),
                            C(15, 12, 6), C(6, 8, 4), C(2, 2, 1), C(10, 8, 4), C(14, 11, 7), C(7, 5, 2),
                            C(3, 2, 1), C(13, 13, 10), C(9, 7, 4), C(5, 4, 2)};
    const uint16_t orch[] = {C(0, 0, 0), C(6, 12, 3), C(3, 8, 2), C(8, 6, 3), C(5, 4, 2), C(10, 14, 5),
                             C(2, 6, 2), C(12, 9, 4), C(4, 10, 3), C(7, 5, 2), C(9, 13, 4), C(1, 4, 1),
                             C(11, 8, 3), C(6, 9, 3), C(13, 11, 6), C(2, 3, 1)};
    const uint16_t pick[] = {C(0, 0, 0), C(14, 10, 7), C(4, 5, 8), C(12, 8, 5), C(12, 3, 2), C(14, 12, 4),
                             C(15, 13, 10), C(3, 2, 1), C(8, 3, 2), C(6, 7, 9), C(15, 14, 8), C(9, 6, 4),
                             C(2, 2, 3), C(13, 5, 3), C(7, 4, 3), C(15, 15, 12)};
    const uint16_t wood[] = {C(0, 0, 0), C(10, 7, 3), C(7, 4, 2), C(12, 8, 4), C(5, 3, 1), C(14, 10, 5),
                             C(8, 5, 2), C(4, 2, 1), C(11, 7, 3), C(6, 4, 2), C(13, 9, 4), C(9, 6, 3),
                             C(3, 2, 1), C(15, 11, 6), C(7, 5, 3), C(2, 1, 0)};
    const uint16_t apple[] = {C(0, 0, 0), C(4, 10, 3), C(13, 2, 2), C(15, 6, 4), C(6, 4, 1), C(15, 12, 8),
                              C(8, 1, 1), C(14, 8, 3), C(3, 6, 1), C(12, 3, 2), C(15, 10, 6), C(5, 1, 1),
                              C(9, 12, 4), C(7, 2, 2), C(11, 5, 2), C(15, 14, 10)};
    const uint16_t wasp[] = {C(0, 0, 0), C(2, 2, 2), C(15, 13, 2), C(2, 2, 1), C(12, 12, 12), C(8, 8, 8),
                             C(14, 8, 1), C(6, 5, 1), C(15, 15, 8), C(4, 4, 3), C(10, 9, 2), C(7, 6, 2),
                             C(13, 11, 3), C(3, 3, 2), C(15, 14, 6), C(1, 1, 1)};
    const uint16_t leaf[] = {C(0, 0, 0), C(8, 14, 4), C(3, 9, 2), C(5, 12, 3), C(10, 15, 6), C(2, 6, 1),
                             C(12, 14, 7), C(6, 10, 3), C(1, 4, 1), C(9, 13, 4), C(4, 8, 2), C(14, 15, 8),
                             C(7, 11, 3), C(2, 5, 2), C(11, 14, 5), C(15, 15, 10)};
    const uint16_t fx[] = {C(0, 0, 0), C(15, 15, 12), C(12, 14, 8), C(15, 12, 4), C(8, 10, 4), C(14, 14, 10),
                           C(6, 8, 3), C(15, 15, 15), C(10, 12, 6), C(4, 5, 2), C(13, 13, 8), C(9, 11, 5),
                           C(15, 10, 3), C(7, 8, 4), C(11, 12, 7), C(2, 2, 1)};
    const uint16_t alert[] = {C(0, 0, 0), C(15, 4, 3), C(10, 2, 2), C(15, 8, 4), C(8, 1, 1), C(14, 6, 3),
                              C(6, 1, 1), C(15, 12, 8), C(12, 3, 2), C(9, 2, 1), C(15, 10, 6), C(4, 1, 1),
                              C(13, 5, 3), C(7, 2, 1), C(15, 14, 10), C(3, 0, 0)};
    const uint16_t ok[] = {C(0, 0, 0), C(8, 15, 6), C(4, 10, 3), C(12, 15, 8), C(2, 6, 2), C(10, 14, 5),
                           C(6, 12, 4), C(14, 15, 10), C(3, 8, 2), C(9, 13, 4), C(15, 15, 12), C(5, 9, 3),
                           C(1, 4, 1), C(11, 14, 6), C(7, 11, 4), C(13, 15, 9)};
    const uint16_t sky[] = {C(0, 0, 0), C(8, 12, 15), C(12, 14, 15), C(6, 10, 14), C(15, 14, 10), C(4, 8, 12),
                            C(14, 12, 8), C(9, 11, 14), C(15, 15, 13), C(5, 7, 11), C(11, 13, 15), C(7, 9, 13),
                            C(13, 10, 6), C(3, 5, 9), C(10, 12, 14), C(15, 13, 9)};
    const uint16_t gold[] = {C(0, 0, 0), C(15, 13, 4), C(12, 9, 2), C(15, 15, 8), C(8, 6, 1), C(14, 11, 3),
                             C(10, 8, 2), C(15, 14, 10), C(6, 4, 1), C(13, 10, 3), C(15, 12, 6), C(9, 7, 2),
                             C(4, 3, 1), C(14, 12, 5), C(11, 8, 2), C(15, 15, 12)};
    const uint16_t dim[] = {C(0, 0, 0), C(7, 8, 6), C(4, 5, 3), C(9, 10, 7), C(3, 3, 2), C(6, 7, 5),
                            C(5, 6, 4), C(10, 11, 8), C(2, 3, 2), C(8, 8, 6), C(11, 11, 8), C(4, 4, 3),
                            C(6, 6, 4), C(3, 4, 2), C(9, 9, 6), C(5, 5, 3)};
    const uint16_t barn[] = {C(0, 0, 0), C(12, 3, 2), C(8, 2, 1), C(14, 6, 3), C(6, 3, 2), C(10, 4, 2),
                             C(15, 8, 4), C(4, 2, 1), C(13, 5, 3), C(7, 2, 1), C(11, 3, 2), C(9, 4, 2),
                             C(5, 1, 1), C(14, 7, 4), C(3, 1, 1), C(15, 10, 6)};
    const uint16_t ditch[] = {C(0, 0, 0), C(2, 6, 8), C(1, 4, 6), C(4, 8, 10), C(3, 5, 4), C(6, 10, 12),
                              C(1, 3, 4), C(8, 12, 13), C(2, 4, 5), C(5, 7, 6), C(3, 7, 8), C(1, 2, 3),
                              C(7, 9, 8), C(4, 6, 7), C(9, 12, 11), C(2, 3, 2)};
    setPal(vdp, PAL_HUD, hud);
    setPal(vdp, PAL_ORCH, orch);
    setPal(vdp, PAL_PICK, pick);
    setPal(vdp, PAL_WOOD, wood);
    setPal(vdp, PAL_APPLE, apple);
    setPal(vdp, PAL_WASP, wasp);
    setPal(vdp, PAL_LEAF, leaf);
    setPal(vdp, PAL_FX, fx);
    setPal(vdp, PAL_ALERT, alert);
    setPal(vdp, PAL_OK, ok);
    setPal(vdp, PAL_SKY, sky);
    setPal(vdp, PAL_GOLD, gold);
    setPal(vdp, PAL_DIM, dim);
    setPal(vdp, PAL_BARN, barn);
    setPal(vdp, PAL_DITCH, ditch);

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);

    auto put = [&](int& slot, int a, int b) {
        uint8_t px[64];
        tileSolid(px, a, b);
        slot = tiles.shared(px);
        vdp.loadTile(slot, px);
    };
    put(art.grass, 1, 5);
    put(art.soil, 3, 4);
    put(art.soilB, 4, 9);
    put(art.leaf, 1, 2);
    put(art.leafB, 8, 1);
    put(art.board, 2, 3);
    put(art.boardB, 3, 5);
    put(art.crateT, 7, 3);
    put(art.pit, 11, 6);
    put(art.sky, 1, 3);
    put(art.blossom, 4, 8);
    put(art.barn, 1, 2);
    put(art.barnB, 2, 5);

    art.stand = gs::uploadMipped(vdp, picker(0, false, false));
    art.walkA = gs::uploadMipped(vdp, picker(0, false, false));
    art.walkB = gs::uploadMipped(vdp, picker(1, false, false));
    art.jump = gs::uploadMipped(vdp, picker(0, false, true));
    art.climbA = gs::uploadMipped(vdp, picker(0, true, false));
    art.climbB = gs::uploadMipped(vdp, picker(1, true, false));
    art.ladder = gs::uploadMipped(vdp, ladderBmp());
    art.barrel = gs::uploadMipped(vdp, barrelBmp());
    art.wasp = gs::uploadMipped(vdp, waspBmp());
    art.apple = gs::uploadMipped(vdp, appleBmp());
    art.tree = gs::uploadMipped(vdp, treeBmp());
    art.hive = gs::uploadMipped(vdp, hiveBmp());
    art.crate = gs::uploadMipped(vdp, crateBmp());
}

}  // namespace orchardladd
