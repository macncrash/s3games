#include "art.h"

#include <cstdint>
#include <initializer_list>
#include <string>

namespace quarryladd {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void inkPal(gs::VDP& vdp, int pal, int r, int g, int b) {
    setPal(vdp, pal, {0, gs::rgb4(r, g, b), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(1, 1, 2)});
}

void tileSolid(uint8_t* px, int c, int speck) {
    for (int i = 0; i < 64; i++) px[i] = uint8_t(c);
    px[0] = uint8_t(speck);
    px[27] = uint8_t(speck);
    px[45] = uint8_t(speck);
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

gs::Bitmap manArt(int step) {
    gs::Bitmap b(28, 42);
    b.rect(8, 1, 12, 4, 1);             // hard hat
    b.rect(7, 4, 14, 2, 2);
    b.ellipse(14, 11, 5, 5, 3);         // face
    b.rect(12, 10, 2, 2, 4);
    b.rect(16, 10, 2, 2, 4);
    b.rect(7, 16, 14, 12, 5);           // vest
    b.rect(7, 16, 3, 12, 6);
    b.rect(11, 18, 6, 4, 7);
    int lx = step == 0 ? 8 : step == 1 ? 6 : 10;
    int rx = step == 0 ? 15 : step == 1 ? 17 : 14;
    b.rect(lx, 28, 5, 9, 8);
    b.rect(rx, 28, 5, 9, 8);
    b.rect(lx - 1, 36, 7, 3, 9);
    b.rect(rx - 1, 36, 7, 3, 9);
    if (step == 2) {
        b.rect(6, 18, 4, 3, 8);
        b.rect(18, 16, 4, 3, 8);
    }
    b.outline(10, false);
    return b;
}

gs::Bitmap climbArt(int step) {
    gs::Bitmap b(24, 42);
    b.rect(6, 1, 12, 4, 1);
    b.ellipse(12, 11, 5, 5, 3);
    b.rect(7, 16, 10, 12, 5);
    int dy = step ? 2 : 0;
    b.rect(3, 18 + dy, 4, 8, 8);
    b.rect(17, 16 + (step ? 0 : 2), 4, 8, 8);
    b.rect(8, 28, 4, 10, 8);
    b.rect(13, 28 + (step ? 2 : 0), 4, 10, 8);
    b.outline(10, false);
    return b;
}

gs::Bitmap ladderArt() {
    gs::Bitmap b(16, 28);
    b.rect(2, 0, 3, 28, 1);
    b.rect(11, 0, 3, 28, 1);
    for (int y = 3; y < 26; y += 6) b.rect(2, y, 12, 2, 2);
    return b;
}

gs::Bitmap skipArt() {
    gs::Bitmap b(40, 26);
    b.poly({{4, 8}, {36, 8}, {32, 18}, {8, 18}}, 1);
    b.rect(2, 6, 36, 4, 2);
    b.rect(10, 10, 8, 5, 3);
    b.ellipse(10, 20, 5, 5, 4);
    b.ellipse(30, 20, 5, 5, 4);
    b.ellipse(10, 20, 2, 2, 5);
    b.ellipse(30, 20, 2, 2, 5);
    b.outline(6, false);
    return b;
}

gs::Bitmap dustArt() {
    gs::Bitmap b(20, 20);
    b.ellipse(10, 12, 8, 6, 1);
    b.ellipse(8, 8, 4, 4, 2);
    b.ellipse(13, 7, 3, 3, 3);
    return b;
}

gs::Bitmap hookArt() {
    gs::Bitmap b(16, 22);
    b.rect(7, 0, 2, 10, 1);
    b.rect(6, 10, 6, 3, 2);
    b.rect(10, 12, 2, 6, 1);
    b.rect(6, 16, 6, 2, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap cableArt() {
    gs::Bitmap b(6, 36);
    b.rect(2, 0, 2, 36, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(10, 22);
    b.rect(4, 0, 2, 10, 1);
    b.poly({{2, 12}, {8, 12}, {7, 18}, {3, 18}}, 2);
    b.rect(3, 18, 4, 2, 3);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 1);
    b.ellipse(9, 9, 6, 6, 2);
    b.line(9, 9, 9, 4, 3, 1.f);
    b.line(9, 9, 13, 10, 3, 1.f);
    return b;
}

gs::Bitmap boulderArt() {
    gs::Bitmap b(22, 16);
    b.ellipse(11, 10, 10, 6, 1);
    b.ellipse(8, 8, 4, 3, 2);
    b.outline(3, false);
    return b;
}

gs::Bitmap rippleArt() {
    gs::Bitmap b(28, 8);
    b.line(2, 4, 10, 2, 1, 1.f);
    b.line(10, 2, 18, 6, 2, 1.f);
    b.line(18, 6, 26, 3, 1, 1.f);
    return b;
}

void loadTiles(gs::VDP& vdp, Art& a) {
    gs::TileAlloc tiles(vdp, 200);
    uint8_t px[64];
    auto put = [&](int c, int speck) {
        tileSolid(px, c, speck);
        return tiles.shared(px);
    };
    a.rock = put(1, 2);
    a.rockB = put(2, 3);
    a.strata = put(3, 1);
    a.lip = put(4, 5);
    a.grate = put(5, 6);
    a.water = put(7, 8);
    a.pit = put(6, 1);
    a.farWall = put(2, 4);
    a.farLip = put(3, 5);
    for (int i = 0; i < 64; i++) px[i] = 0;
    px[18] = 1;
    px[27] = 1;
    px[36] = 2;
    a.star = tiles.shared(px);
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    loadFont(vdp, art);
    loadTiles(vdp, art);
    art.stand = gs::uploadMipped(vdp, manArt(0));
    art.walkA = gs::uploadMipped(vdp, manArt(1));
    art.walkB = gs::uploadMipped(vdp, manArt(2));
    art.jump = gs::uploadMipped(vdp, manArt(2));
    art.climbA = gs::uploadMipped(vdp, climbArt(0));
    art.climbB = gs::uploadMipped(vdp, climbArt(1));
    art.ladder = gs::uploadMipped(vdp, ladderArt());
    art.skip = gs::uploadMipped(vdp, skipArt());
    art.dust = gs::uploadMipped(vdp, dustArt());
    art.hook = gs::uploadMipped(vdp, hookArt());
    art.cable = gs::uploadMipped(vdp, cableArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.boulder = gs::uploadMipped(vdp, boulderArt());
    art.ripple = gs::uploadMipped(vdp, rippleArt());

    setPal(vdp, PAL_HUD, {0, gs::rgb4(14, 13, 10), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, gs::rgb4(2, 2, 3)});
    setPal(vdp, PAL_ROCK, {0, gs::rgb4(6, 5, 4), gs::rgb4(8, 7, 5), gs::rgb4(4, 3, 3), gs::rgb4(12, 10, 3),
                           gs::rgb4(5, 5, 6), gs::rgb4(3, 3, 4), gs::rgb4(2, 5, 8), gs::rgb4(4, 8, 11)});
    setPal(vdp, PAL_MAN, {0, gs::rgb4(14, 12, 2), gs::rgb4(10, 8, 1), gs::rgb4(12, 8, 5), gs::rgb4(2, 2, 2),
                          gs::rgb4(13, 6, 1), gs::rgb4(9, 4, 1), gs::rgb4(15, 14, 8), gs::rgb4(3, 3, 5),
                          gs::rgb4(2, 2, 2), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_IRON, {0, gs::rgb4(7, 8, 9), gs::rgb4(11, 12, 12), gs::rgb4(3, 3, 4)});
    setPal(vdp, PAL_DUST, {0, gs::rgb4(10, 8, 5), gs::rgb4(13, 11, 7), gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_WATER, {0, gs::rgb4(3, 7, 10), gs::rgb4(6, 11, 13)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(8, 5, 2), gs::rgb4(5, 3, 1), gs::rgb4(3, 2, 1)});
    setPal(vdp, PAL_SKIP, {0, gs::rgb4(10, 4, 2), gs::rgb4(13, 7, 2), gs::rgb4(6, 6, 6), gs::rgb4(2, 2, 2),
                           gs::rgb4(8, 8, 8), gs::rgb4(1, 1, 1)});
    setPal(vdp, PAL_ALERT, {0, gs::rgb4(15, 4, 2), gs::rgb4(15, 10, 3)});
    setPal(vdp, PAL_OK, {0, gs::rgb4(4, 13, 6), gs::rgb4(10, 15, 8)});
    inkPal(vdp, PAL_GOLD, 14, 11, 3);
    inkPal(vdp, PAL_DIM, 7, 6, 5);
    setPal(vdp, 10, {0, gs::rgb4(14, 13, 9), gs::rgb4(8, 8, 10)});
}

}  // namespace quarryladd
