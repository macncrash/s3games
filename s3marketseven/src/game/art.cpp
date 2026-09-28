#include "game/art.h"

namespace marketseven {
namespace {

void paint(gs::VDP& v, int p, int cr, int cg, int cb, int tr, int tg, int tb) {
    const uint16_t c[16] = {
        gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(14, 11, 8), gs::rgb4(cr, cg, cb),
        gs::rgb4(tr, tg, tb), gs::rgb4(8, 5, 2), gs::rgb4(4, 3, 1), gs::rgb4(15, 13, 4),
        gs::rgb4(11, 5, 2), gs::rgb4(12, 13, 10), gs::rgb4(13, 2, 2), gs::rgb4(3, 10, 3),
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
    gs::Bitmap b(168, 36);
    b.rect(2, 2, 164, 4, 6);
    for (int i = 0; i < 7; i++) {
        int c = (i & 1) ? 3 : 11;
        int x = 4 + i * 23;
        b.rect(float(x), 6, 23, 14, c);
        b.ellipse(float(x + 11), 24, 12, 7, c);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 72);
    b.rect(3, 2, 6, 66, 5);
    b.rect(4, 2, 2, 66, 13);
    b.rect(2, 2, 8, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap counterArt() {
    gs::Bitmap b(188, 34);
    b.rect(3, 3, 182, 8, 13);
    b.rect(3, 11, 182, 3, 12);
    b.rect(3, 14, 182, 14, 5);
    for (int i = 0; i < 6; i++) b.rect(float(14 + i * 28), 18, 3, 7, 6);
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
    b.rect(17, 24, 6, 1, 1);
    b.rect(11, 29, 18, 16, 4);
    b.rect(14, 31, 12, 4, 3);
    b.rect(7, 29, 5, 12, 3);
    b.rect(28, 29, 5, 12, 3);
    b.rect(13, 45, 6, 12, 6);
    b.rect(21, 45, 6, 12, 6);
    b.rect(12, 55, 8, 4, 1);
    b.rect(20, 55, 8, 4, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap buyerArt() {
    gs::Bitmap b(36, 58);
    b.rect(11, 4, 14, 5, 6);
    b.ellipse(18, 16, 7, 8, 2);
    b.rect(13, 14, 3, 3, 1);
    b.rect(21, 14, 3, 3, 1);
    b.rect(15, 20, 5, 1, 1);
    b.rect(10, 25, 16, 14, 3);
    b.rect(8, 26, 4, 10, 2);
    b.rect(24, 26, 4, 10, 2);
    b.rect(12, 39, 5, 12, 6);
    b.rect(19, 39, 5, 12, 6);
    b.rect(11, 49, 7, 3, 1);
    b.rect(18, 49, 7, 3, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap creamArt() {
    gs::Bitmap b(28, 26);
    b.ellipse(14, 16, 9, 6, 13);
    b.ellipse(14, 14, 6, 3, 14);
    b.rect(12, 6, 4, 6, 7);
    b.ellipse(18, 8, 4, 2, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap loafArt() {
    gs::Bitmap b(30, 22);
    b.ellipse(15, 12, 12, 7, 12);
    b.ellipse(15, 11, 8, 4, 13);
    for (int i = 0; i < 3; i++) b.rect(float(8 + i * 5), 10, 2, 3, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap goldArt() {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 14, 9, 8, 7);
    b.ellipse(10, 11, 3, 2, 14);
    b.rect(12, 4, 2, 5, 11);
    b.ellipse(17, 6, 4, 2, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap coinArt(int r, int mark) {
    gs::Bitmap b(r * 2 + 4, r * 2 + 4);
    float c = float(r + 2);
    b.ellipse(c, c, float(r), float(r), 7);
    b.ellipse(c, c, float(r - 3), float(r - 3), 13);
    if (mark > 1) b.rect(c - 1, c - 4, 2, 8, 1);
    else b.rect(c - 1, c - 3, 2, 6, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap dishArt() {
    gs::Bitmap b(40, 14);
    b.ellipse(20, 8, 16, 5, 6);
    b.ellipse(20, 7, 12, 3, 13);
    b.outline(1, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(18, 20);
    b.ellipse(9, 11, 7, 6, 7);
    b.rect(8, 3, 2, 4, 6);
    b.rect(3, 16, 12, 2, 5);
    b.outline(1, false);
    return b;
}

gs::Bitmap bracketArt() {
    gs::Bitmap b(22, 10);
    b.rect(1, 6, 20, 2, 1);
    b.rect(1, 2, 2, 6, 1);
    b.rect(19, 2, 2, 6, 1);
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

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    ink(vdp, PAL_HUD, 15, 15, 15);
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(2, 1, 2));
    paint(vdp, PAL_STALL, 12, 3, 2, 15, 13, 8);
    paint(vdp, PAL_CLERK, 2, 5, 11, 15, 15, 15);
    paint(vdp, PAL_RIVAL, 11, 3, 3, 15, 12, 8);
    paint(vdp, PAL_CREAM, 14, 12, 8, 8, 6, 3);
    paint(vdp, PAL_LOAF, 12, 7, 3, 15, 12, 6);
    paint(vdp, PAL_GOLD, 15, 12, 2, 10, 6, 1);
    paint(vdp, PAL_COIN, 15, 13, 4, 12, 7, 2);
    ink(vdp, PAL_OK, 3, 13, 4);
    ink(vdp, PAL_WARN, 15, 12, 2);
    ink(vdp, PAL_BAD, 14, 3, 2);
    ink(vdp, PAL_DIM, 3, 3, 5);
    ink(vdp, PAL_PIP, 15, 14, 6);

    art.awning = gs::uploadMipped(vdp, awningArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.counter = gs::uploadMipped(vdp, counterArt());
    art.clerk = gs::uploadMipped(vdp, clerkArt());
    art.buyer = gs::uploadMipped(vdp, buyerArt());
    art.good[0] = gs::uploadMipped(vdp, creamArt());
    art.good[1] = gs::uploadMipped(vdp, loafArt());
    art.good[2] = gs::uploadMipped(vdp, goldArt());
    art.coin[0] = gs::uploadMipped(vdp, coinArt(8, 1));
    art.coin[1] = gs::uploadMipped(vdp, coinArt(11, 2));
    art.dish = gs::uploadMipped(vdp, dishArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.shade = gs::uploadImage(vdp, shadeArt());
    art.bracket = gs::uploadImage(vdp, bracketArt());
    art.solid = gs::uploadImage(vdp, solidArt());
    for (int d = 0; d < 10; d++) {
        char s[2] = {char('0' + d), 0};
        art.digit[d] = say(vdp, s, 2);
    }
    art.title = say(vdp, "S3 MARKET", 2);
    art.seven = say(vdp, "FIRST TO SEVEN", 2);
    art.shortW = say(vdp, "STILL SHORT", 2);
    art.exact = say(vdp, "EXACT", 2);
    art.winW = say(vdp, "FIRST TO SEVEN", 2);
    art.loseW = say(vdp, "RIVAL TOOK IT", 1);

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.setFogColor(gs::rgb4(6, 5, 4));
}

}  // namespace marketseven
