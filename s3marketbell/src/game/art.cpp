#include "game/art.h"

namespace marketbell {
namespace {

void paint(gs::VDP& v, int p, int a, int b, int c, int d, int e, int f) {
    const uint16_t col[16] = {
        gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(14, 11, 7), gs::rgb4(a, b, c),
        gs::rgb4(d, e, f), gs::rgb4(8, 5, 2), gs::rgb4(4, 3, 1), gs::rgb4(15, 13, 4),
        gs::rgb4(11, 7, 2), gs::rgb4(12, 13, 10), gs::rgb4(12, 3, 2), gs::rgb4(3, 10, 4),
        gs::rgb4(13, 9, 5), gs::rgb4(15, 14, 12), gs::rgb4(15, 15, 15), gs::rgb4(2, 1, 2),
    };
    for (int i = 0; i < 16; i++) v.setColor(p * 16 + i, col[i]);
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
    b.rect(2, 2, 164, 6, 6);
    for (int i = 0; i < 7; i++) {
        int c = (i & 1) ? 3 : 4;
        int x = 6 + i * 22;
        b.rect(float(x), 8, 22, 14, c);
        b.ellipse(float(x + 11), 24, 12, 7, c);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(12, 90);
    b.rect(3, 2, 6, 84, 5);
    b.rect(4, 2, 2, 84, 13);
    b.rect(2, 2, 8, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap counterArt() {
    gs::Bitmap b(150, 36);
    b.rect(2, 2, 146, 8, 13);
    b.rect(2, 10, 146, 4, 12);
    b.rect(2, 14, 146, 16, 5);
    for (int i = 0; i < 5; i++) b.rect(float(18 + i * 24), 18, 3, 8, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(32, 26);
    b.rect(2, 4, 28, 18, 5);
    b.rect(2, 8, 28, 2, 6);
    b.rect(2, 14, 28, 2, 6);
    b.rect(8, 4, 2, 18, 6);
    b.rect(20, 4, 2, 18, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap basketArt() {
    gs::Bitmap b(34, 28);
    b.ellipse(17, 18, 14, 8, 5);
    b.rect(4, 12, 26, 8, 6);
    b.ellipse(10, 10, 5, 5, 10);
    b.ellipse(18, 8, 5, 5, 11);
    b.ellipse(24, 11, 4, 4, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap clerkArt() {
    gs::Bitmap b(40, 64);
    b.ellipse(20, 12, 10, 6, 3);
    b.rect(10, 12, 20, 3, 3);
    b.ellipse(20, 22, 7, 8, 2);
    b.rect(15, 20, 2, 2, 1);
    b.rect(23, 20, 2, 2, 1);
    b.rect(17, 25, 5, 1, 1);
    b.rect(12, 30, 16, 16, 4);
    b.rect(8, 30, 5, 12, 3);
    b.rect(27, 30, 5, 12, 3);
    b.rect(28, 38, 8, 4, 2);
    b.rect(14, 46, 5, 12, 6);
    b.rect(21, 46, 5, 12, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap buyerArt() {
    gs::Bitmap b(40, 64);
    b.rect(12, 4, 14, 5, 6);
    b.ellipse(20, 16, 7, 8, 2);
    b.rect(15, 14, 2, 2, 1);
    b.rect(23, 14, 2, 2, 1);
    b.rect(17, 20, 5, 1, 1);
    b.rect(12, 24, 16, 16, 3);
    b.rect(8, 26, 5, 10, 2);
    b.rect(27, 26, 5, 10, 2);
    b.rect(14, 40, 5, 16, 6);
    b.rect(21, 40, 5, 16, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(30, 36);
    b.rect(13, 1, 4, 5, 6);
    b.ellipse(15, 16, 12, 10, 7);
    b.rect(4, 16, 22, 8, 7);
    b.ellipse(15, 24, 13, 4, 8);
    b.rect(14, 14, 2, 8, 1);
    b.ellipse(15, 23, 2, 2, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 18);
    b.rect(6, 1, 2, 4, 6);
    b.ellipse(7, 11, 5, 6, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap coinArt(int r, int mark) {
    gs::Bitmap b(r * 2 + 4, r * 2 + 4);
    float c = float(r + 2);
    b.ellipse(c, c, float(r), float(r), 7);
    b.ellipse(c, c, float(r - 3), float(r - 3), mark == 10 ? 8 : 3);
    b.outline(1, false);
    return b;
}

gs::Bitmap dishArt() {
    gs::Bitmap b(48, 14);
    b.ellipse(24, 7, 22, 5, 13);
    b.ellipse(24, 6, 16, 3, 9);
    b.outline(1, false);
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(28, 10);
    b.ellipse(14, 5, 12, 4, 1);
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

void stampRoom(gs::VDP& vdp, gs::TileAlloc& tiles) {
    int wall[4], floor[4];
    for (int i = 0; i < 4; i++) {
        uint8_t w[64], f[64];
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) {
                bool seam = (y == 0);
                bool nick = ((x + i * 2) % 6 == 0 && (y + i) % 3 == 1);
                w[y * 8 + x] = uint8_t(seam ? 6 : nick ? 12 : 13);
                bool grout = (x == 0 || y == 7);
                bool grit = ((x * 3 + y + i) % 5 == 0);
                f[y * 8 + x] = uint8_t(grout ? 6 : grit ? 12 : 5);
            }
        wall[i] = tiles.shared(w);
        floor[i] = tiles.shared(f);
    }
    for (int cy = 2; cy <= 12; cy++)
        for (int cx = 0; cx < 40; cx++) vdp.B.set(cx, cy, gs::entry(wall[(cx + cy) & 3], PAL_STALL));
    for (int cy = 13; cy <= 27; cy++)
        for (int cx = 0; cx < 40; cx++) vdp.B.set(cx, cy, gs::entry(floor[(cx * 2 + cy) & 3], PAL_STALL));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    ink(vdp, PAL_HUD, 15, 15, 15);
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(2, 1, 2));
    paint(vdp, PAL_STALL, 12, 3, 2, 15, 14, 10);
    paint(vdp, PAL_CLERK, 2, 6, 11, 15, 15, 15);
    paint(vdp, PAL_CUST, 9, 4, 2, 15, 12, 6);
    paint(vdp, PAL_GOODS, 3, 9, 3, 2, 5, 1);
    paint(vdp, PAL_COIN, 15, 12, 3, 12, 7, 2);
    paint(vdp, PAL_BELL, 14, 11, 3, 10, 6, 1);
    paint(vdp, PAL_LAMP, 15, 13, 4, 6, 5, 2);
    ink(vdp, PAL_OK, 3, 13, 4);
    ink(vdp, PAL_WARN, 15, 12, 2);
    ink(vdp, PAL_BAD, 14, 3, 2);
    ink(vdp, PAL_DIM, 2, 2, 4);
    paint(vdp, PAL_BOARD, 9, 7, 3, 14, 12, 8);

    art.awning = gs::uploadMipped(vdp, awningArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.counter = gs::uploadMipped(vdp, counterArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.basket = gs::uploadMipped(vdp, basketArt());
    art.clerk = gs::uploadMipped(vdp, clerkArt());
    art.buyer = gs::uploadMipped(vdp, buyerArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.coin[0] = gs::uploadMipped(vdp, coinArt(7, 1));
    art.coin[1] = gs::uploadMipped(vdp, coinArt(8, 2));
    art.coin[2] = gs::uploadMipped(vdp, coinArt(11, 5));
    art.coin[3] = gs::uploadMipped(vdp, coinArt(6, 10));
    art.dish = gs::uploadMipped(vdp, dishArt());
    art.shade = gs::uploadImage(vdp, shadeArt());
    art.bracket = gs::uploadImage(vdp, bracketArt());
    art.solid = gs::uploadImage(vdp, solidArt());
    for (int d = 0; d < 10; d++) {
        char s[2] = {char('0' + d), 0};
        art.digit[d] = say(vdp, s, 2);
    }
    art.title = say(vdp, "MARKET BELL", 2);
    art.sub = say(vdp, "THREE TRIES", 1);
    art.rung = say(vdp, "BELL", 2);
    art.dead = say(vdp, "CLOSED", 2);
    art.exact = say(vdp, "EXACT", 1);
    art.shortw = say(vdp, "SHORT", 1);
    art.overw = say(vdp, "HEAVY", 1);

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    stampRoom(vdp, tiles);
    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    vdp.B.scroll(0, 0);
    vdp.setFogColor(gs::rgb4(6, 5, 3));
}

}  // namespace marketbell
