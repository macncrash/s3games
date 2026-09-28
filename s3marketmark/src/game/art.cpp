#include "game/art.h"

namespace marketmark {
namespace {

void paint(gs::VDP& v, int p, int cr, int cg, int cb, int tr, int tg, int tb) {
    const uint16_t c[16] = {
        gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(14, 11, 8), gs::rgb4(cr, cg, cb),
        gs::rgb4(tr, tg, tb), gs::rgb4(8, 5, 2), gs::rgb4(3, 2, 1), gs::rgb4(15, 13, 4),
        gs::rgb4(11, 6, 2), gs::rgb4(12, 13, 10), gs::rgb4(13, 3, 2), gs::rgb4(3, 11, 4),
        gs::rgb4(13, 9, 5), gs::rgb4(15, 14, 12), gs::rgb4(15, 15, 15), gs::rgb4(2, 1, 2),
    };
    for (int i = 0; i < 16; i++) v.setColor(p * 16 + i, c[i]);
}

void ink(gs::VDP& v, int p, int r, int g, int b) {
    uint16_t c[16] = {};
    c[1] = gs::rgb4(r, g, b);
    c[14] = gs::rgb4(15, 15, 15);
    c[15] = gs::rgb4(1, 1, 2);
    for (int i = 0; i < 16; i++) v.setColor(p * 16 + i, c[i]);
}

gs::Bitmap awningArt() {
    gs::Bitmap b(160, 36);
    b.rect(2, 2, 156, 6, 6);
    for (int i = 0; i < 7; i++) {
        int c = (i & 1) ? 7 : 3;
        int x = 4 + i * 22;
        b.rect(float(x), 8, 22, 14, c);
        b.ellipse(float(x + 11), 26, 11, 7, c);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 72);
    b.rect(3, 2, 6, 66, 5);
    b.rect(4, 2, 2, 66, 12);
    b.rect(2, 2, 8, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap counterArt() {
    gs::Bitmap b(168, 34);
    b.rect(2, 2, 164, 8, 13);
    b.rect(2, 10, 164, 4, 12);
    b.rect(2, 14, 164, 16, 5);
    for (int i = 0; i < 6; i++) b.rect(float(14 + i * 24), 18, 3, 8, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap clerkArt() {
    gs::Bitmap b(40, 64);
    b.ellipse(20, 10, 10, 5, 3);
    b.rect(10, 10, 20, 4, 3);
    b.ellipse(20, 20, 7, 8, 2);
    b.rect(15, 18, 3, 3, 1);
    b.rect(23, 18, 3, 3, 1);
    b.rect(17, 24, 5, 1, 1);
    b.rect(11, 30, 18, 16, 4);
    b.rect(14, 32, 10, 4, 3);
    b.rect(7, 30, 5, 12, 3);
    b.rect(28, 30, 5, 12, 3);
    b.rect(14, 46, 5, 12, 6);
    b.rect(21, 46, 5, 12, 6);
    b.rect(12, 56, 8, 4, 1);
    b.rect(20, 56, 8, 4, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap buyerArt() {
    gs::Bitmap b(40, 62);
    b.ellipse(20, 16, 7, 8, 2);
    b.rect(15, 14, 3, 3, 1);
    b.rect(23, 14, 3, 3, 1);
    b.rect(17, 20, 5, 1, 1);
    b.rect(11, 26, 18, 16, 3);
    b.rect(14, 30, 10, 6, 4);
    b.rect(7, 28, 5, 11, 3);
    b.rect(28, 28, 5, 11, 3);
    b.rect(14, 42, 5, 14, 6);
    b.rect(21, 42, 5, 14, 6);
    b.rect(12, 54, 8, 4, 1);
    b.rect(20, 54, 8, 4, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap hatArt() {
    gs::Bitmap b(26, 12);
    b.ellipse(13, 5, 7, 4, 7);
    b.rect(2, 7, 22, 3, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap starArt() {
    gs::Bitmap b(16, 16);
    b.poly({{8, 1}, {10, 6}, {15, 6}, {11, 9}, {13, 15}, {8, 11}, {3, 15}, {5, 9}, {1, 6}, {6, 6}}, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap pearArt() {
    gs::Bitmap b(18, 22);
    b.ellipse(9, 13, 7, 8, 11);
    b.rect(8, 2, 2, 5, 3);
    b.ellipse(12, 5, 3, 2, 11);
    b.outline(1, false);
    return b;
}

gs::Bitmap loafArt() {
    gs::Bitmap b(28, 16);
    b.ellipse(14, 9, 12, 6, 12);
    b.rect(6, 6, 16, 2, 8);
    b.outline(1, false);
    return b;
}

gs::Bitmap jarArt() {
    gs::Bitmap b(16, 22);
    b.rect(4, 2, 8, 3, 6);
    b.rect(3, 5, 10, 14, 9);
    b.rect(5, 8, 6, 8, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap coinArt(int r, int pip) {
    gs::Bitmap b(r * 2 + 4, r * 2 + 4);
    float c = float(r + 2);
    b.ellipse(c, c, float(r), float(r), 7);
    b.ellipse(c, c, float(r) * 0.55f, float(r) * 0.55f, pip == 10 ? 8 : 13);
    b.outline(1, false);
    return b;
}

gs::Bitmap noteArt() {
    gs::Bitmap b(36, 20);
    b.rect(1, 2, 34, 16, 7);
    b.rect(4, 5, 10, 10, 13);
    b.poly({{9, 6}, {11, 10}, {15, 10}, {12, 12}, {13, 16}, {9, 13}, {5, 16}, {6, 12}, {3, 10}, {7, 10}}, 8);
    b.rect(18, 6, 12, 2, 1);
    b.rect(18, 10, 10, 2, 6);
    b.rect(18, 14, 8, 2, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap dishArt() {
    gs::Bitmap b(40, 14);
    b.ellipse(20, 8, 18, 5, 6);
    b.ellipse(20, 7, 12, 3, 13);
    b.outline(1, false);
    return b;
}

gs::Bitmap bracketArt() {
    gs::Bitmap b(24, 10);
    b.rect(1, 6, 22, 2, 1);
    b.rect(1, 2, 3, 6, 1);
    b.rect(20, 2, 3, 6, 1);
    return b;
}

gs::Bitmap solidArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(32, 12);
    b.ellipse(16, 6, 14, 4, 1);
    return b;
}

gs::Image say(gs::VDP& v, const char* s, int scale) {
    gs::TextStyle st{scale, 1, 0, 15, 1};
    return gs::uploadImage(v, gs::textBitmap(s, st));
}

void loadFont(gs::VDP& vdp, gs::TileAlloc& tiles, Art& a) {
    (void)vdp;
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        a.font[c - 32] = tiles.shared(px);
    }
}

void stampRoom(gs::VDP& vdp, gs::TileAlloc& tiles) {
    uint8_t wall[64], floor[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            bool speck = ((x * 3 + y) % 7 == 0);
            wall[y * 8 + x] = uint8_t(speck ? 12 : 13);
            bool grout = (x == 0 || y == 0);
            floor[y * 8 + x] = uint8_t(grout ? 6 : 5);
        }
    int w = tiles.shared(wall);
    int f = tiles.shared(floor);
    for (int cy = 2; cy <= 12; cy++)
        for (int cx = 0; cx < 40; cx++) vdp.A.set(cx, cy, gs::entry(w, PAL_STALL));
    for (int cy = 13; cy <= 27; cy++)
        for (int cx = 0; cx < 40; cx++) vdp.A.set(cx, cy, gs::entry(f, PAL_STALL));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    ink(vdp, PAL_HUD, 15, 15, 15);
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(2, 1, 2));
    paint(vdp, PAL_STALL, 12, 2, 2, 15, 14, 11);
    paint(vdp, PAL_CLERK, 2, 5, 11, 15, 15, 15);
    paint(vdp, PAL_BUYER, 2, 7, 4, 14, 12, 6);
    paint(vdp, PAL_MARK, 10, 7, 1, 15, 13, 3);
    paint(vdp, PAL_GOODS, 2, 8, 3, 1, 6, 2);
    paint(vdp, PAL_COIN, 15, 12, 3, 12, 6, 2);
    paint(vdp, PAL_NOTE, 12, 9, 1, 15, 14, 6);
    ink(vdp, PAL_OK, 3, 13, 4);
    ink(vdp, PAL_WARN, 15, 12, 2);
    ink(vdp, PAL_BAD, 14, 3, 2);
    ink(vdp, PAL_DIM, 2, 2, 4);

    art.awning = gs::uploadMipped(vdp, awningArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.counter = gs::uploadMipped(vdp, counterArt());
    art.clerk = gs::uploadMipped(vdp, clerkArt());
    art.buyer = gs::uploadMipped(vdp, buyerArt());
    art.hat = gs::uploadMipped(vdp, hatArt());
    art.star = gs::uploadMipped(vdp, starArt());
    art.pear = gs::uploadMipped(vdp, pearArt());
    art.loaf = gs::uploadMipped(vdp, loafArt());
    art.jar = gs::uploadMipped(vdp, jarArt());
    art.coin[0] = gs::uploadMipped(vdp, coinArt(7, 1));
    art.coin[1] = gs::uploadMipped(vdp, coinArt(8, 2));
    art.coin[2] = gs::uploadMipped(vdp, coinArt(10, 5));
    art.coin[3] = gs::uploadMipped(vdp, coinArt(6, 10));
    art.note = gs::uploadMipped(vdp, noteArt());
    art.dish = gs::uploadMipped(vdp, dishArt());
    art.shade = gs::uploadImage(vdp, shadeArt());
    art.bracket = gs::uploadImage(vdp, bracketArt());
    art.solid = gs::uploadImage(vdp, solidArt());
    for (int d = 0; d < 10; d++) {
        char s[2] = {char('0' + d), 0};
        art.digit[d] = say(vdp, s, 3);
    }
    art.title = say(vdp, "S3 MARKETMARK", 2);
    art.sub = say(vdp, "THE GOLD NOTE", 2);
    art.done = say(vdp, "MARK DONE", 2);
    art.walked = say(vdp, "MARK WALKED", 2);
    art.exact = say(vdp, "EXACT", 2);
    art.brief = say(vdp, "SHORT", 2);
    art.heavy = say(vdp, "OVER", 2);
    art.markWord = say(vdp, "MARK", 1);

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    stampRoom(vdp, tiles);
    vdp.A.enabled = true;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.A.scroll(0, 0);
    vdp.setFogColor(gs::rgb4(6, 5, 4));
}

}  // namespace marketmark
