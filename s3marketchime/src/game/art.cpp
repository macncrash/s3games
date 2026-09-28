#include "game/art.h"

namespace marketchime {
namespace {

void paint(gs::VDP& v, int p, int a, int b, int c, int d, int e, int f) {
    const uint16_t col[16] = {
        gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(14, 12, 8), gs::rgb4(a, b, c),
        gs::rgb4(d, e, f), gs::rgb4(7, 4, 2), gs::rgb4(3, 2, 1), gs::rgb4(15, 13, 5),
        gs::rgb4(10, 6, 2), gs::rgb4(12, 13, 11), gs::rgb4(13, 3, 2), gs::rgb4(3, 10, 5),
        gs::rgb4(14, 10, 6), gs::rgb4(15, 14, 12), gs::rgb4(15, 15, 15), gs::rgb4(2, 1, 2),
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
    gs::Bitmap b(180, 32);
    b.rect(2, 2, 176, 5, 6);
    for (int i = 0; i < 8; i++) {
        int c = (i & 1) ? 10 : 3;
        int x = 4 + i * 22;
        b.rect(float(x), 7, 20, 12, c);
        b.ellipse(float(x + 10), 20, 11, 6, c);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(10, 96);
    b.rect(3, 0, 4, 96, 5);
    b.rect(4, 0, 2, 96, 12);
    b.rect(2, 0, 6, 4, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap counterArt() {
    gs::Bitmap b(168, 32);
    b.rect(2, 2, 164, 7, 13);
    b.rect(2, 9, 164, 3, 12);
    b.rect(2, 12, 164, 16, 5);
    for (int i = 0; i < 6; i++) b.rect(float(16 + i * 24), 16, 2, 8, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(36, 28);
    b.rect(2, 4, 32, 20, 8);
    b.rect(2, 9, 32, 2, 6);
    b.rect(2, 16, 32, 2, 6);
    b.rect(10, 4, 2, 20, 6);
    b.rect(22, 4, 2, 20, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap clerkArt() {
    gs::Bitmap b(36, 58);
    b.ellipse(18, 10, 9, 5, 7);
    b.rect(10, 10, 16, 3, 7);
    b.ellipse(18, 20, 6, 7, 2);
    b.rect(14, 18, 2, 2, 1);
    b.rect(20, 18, 2, 2, 1);
    b.rect(16, 23, 4, 1, 1);
    b.rect(11, 28, 14, 14, 4);
    b.rect(7, 28, 5, 11, 3);
    b.rect(24, 28, 5, 11, 3);
    b.rect(13, 42, 4, 12, 6);
    b.rect(19, 42, 4, 12, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap buyerArt() {
    gs::Bitmap b(36, 58);
    b.rect(11, 3, 14, 4, 8);
    b.ellipse(18, 16, 6, 7, 2);
    b.rect(14, 14, 2, 2, 1);
    b.rect(20, 14, 2, 2, 1);
    b.rect(16, 19, 4, 1, 1);
    b.rect(11, 24, 14, 14, 3);
    b.rect(7, 26, 5, 10, 2);
    b.rect(24, 26, 5, 10, 2);
    b.rect(13, 38, 4, 14, 6);
    b.rect(19, 38, 4, 14, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(26, 30);
    b.rect(11, 1, 4, 4, 6);
    b.ellipse(13, 14, 10, 8, 7);
    b.rect(4, 14, 18, 6, 7);
    b.ellipse(13, 20, 11, 3, 8);
    b.ellipse(13, 19, 2, 2, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap clockArt() {
    gs::Bitmap b(48, 48);
    b.ellipse(24, 24, 22, 22, 13);
    b.ellipse(24, 24, 18, 18, 9);
    b.rect(22, 8, 4, 3, 1);
    b.rect(22, 37, 4, 3, 1);
    b.rect(8, 22, 3, 4, 1);
    b.rect(37, 22, 3, 4, 1);
    b.ellipse(24, 24, 2, 2, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap coinArt(int r) {
    gs::Bitmap b(r * 2 + 4, r * 2 + 4);
    float c = float(r + 2);
    b.ellipse(c, c, float(r), float(r), 7);
    b.ellipse(c, c, float(r - 2), float(r - 2), 8);
    b.outline(1, false);
    return b;
}

gs::Bitmap appleArt() {
    gs::Bitmap b(22, 24);
    b.ellipse(11, 14, 8, 8, 10);
    b.rect(10, 4, 2, 5, 11);
    b.ellipse(14, 6, 4, 2, 11);
    b.outline(1, false);
    return b;
}

gs::Bitmap loafArt() {
    gs::Bitmap b(28, 18);
    b.ellipse(14, 10, 12, 6, 12);
    b.rect(6, 8, 16, 6, 8);
    for (int i = 0; i < 3; i++) b.rect(float(8 + i * 5), 6, 1, 6, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap milkArt() {
    gs::Bitmap b(16, 28);
    b.rect(4, 8, 8, 16, 13);
    b.rect(5, 4, 6, 5, 9);
    b.rect(6, 2, 4, 3, 7);
    b.rect(5, 14, 6, 2, 3);
    b.outline(1, false);
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
                bool seam = y == 0;
                bool nick = ((x + i) % 5 == 0 && y % 3 == 1);
                w[y * 8 + x] = uint8_t(seam ? 6 : nick ? 8 : 4);
                bool grout = x == 0 || y == 7;
                bool grit = ((x + y * 2 + i) % 7 == 0);
                f[y * 8 + x] = uint8_t(grout ? 6 : grit ? 8 : 5);
            }
        wall[i] = tiles.shared(w);
        floor[i] = tiles.shared(f);
    }
    for (int cy = 0; cy <= 14; cy++)
        for (int cx = 0; cx < 40; cx++) vdp.B.set(cx, cy, gs::entry(wall[(cx + cy) & 3], PAL_STALL));
    for (int cy = 15; cy <= 27; cy++)
        for (int cx = 0; cx < 40; cx++) vdp.B.set(cx, cy, gs::entry(floor[(cx + cy) & 3], PAL_STALL));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    ink(vdp, PAL_HUD, 15, 15, 15);
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(2, 1, 2));
    paint(vdp, PAL_STALL, 11, 3, 3, 14, 9, 5);
    paint(vdp, PAL_CLERK, 3, 7, 12, 15, 15, 15);
    paint(vdp, PAL_CUST, 8, 4, 2, 15, 11, 5);
    paint(vdp, PAL_GOODS, 4, 10, 3, 12, 6, 2);
    paint(vdp, PAL_COIN, 15, 12, 3, 11, 7, 2);
    paint(vdp, PAL_CLOCK, 14, 12, 6, 4, 3, 2);
    ink(vdp, PAL_OK, 3, 13, 5);
    ink(vdp, PAL_WARN, 15, 12, 3);
    ink(vdp, PAL_BAD, 14, 3, 2);
    ink(vdp, PAL_DIM, 3, 3, 5);

    art.awning = gs::uploadMipped(vdp, awningArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.counter = gs::uploadMipped(vdp, counterArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.clerk = gs::uploadMipped(vdp, clerkArt());
    art.buyer = gs::uploadMipped(vdp, buyerArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.clock = gs::uploadMipped(vdp, clockArt());
    art.coin[0] = gs::uploadMipped(vdp, coinArt(6));
    art.coin[1] = gs::uploadMipped(vdp, coinArt(8));
    art.coin[2] = gs::uploadMipped(vdp, coinArt(10));
    art.coin[3] = gs::uploadMipped(vdp, coinArt(7));
    art.good[0] = gs::uploadMipped(vdp, appleArt());
    art.good[1] = gs::uploadMipped(vdp, loafArt());
    art.good[2] = gs::uploadMipped(vdp, milkArt());
    art.solid = gs::uploadImage(vdp, solidArt());
    art.title = say(vdp, "MARKET CHIME", 2);
    art.sub = say(vdp, "LEAVE ON THE HOUR", 1);
    art.chime = say(vdp, "THE HOUR CHIMES", 1);
    art.leave = say(vdp, "LEAVE", 2);
    art.gone = say(vdp, "THE HOUR IS GONE", 1);
    art.exact = say(vdp, "EXACT", 1);

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    stampRoom(vdp, tiles);
    vdp.A.enabled = false;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
    vdp.B.scroll(0, 0);
    vdp.setFogColor(gs::rgb4(4, 3, 6));
}

}  // namespace marketchime
