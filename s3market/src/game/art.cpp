#include "game/art.h"

namespace market {
namespace {

void paint(gs::VDP& v, int p, int cr, int cg, int cb, int tr, int tg, int tb) {
    const uint16_t c[16] = {
        gs::rgb4(0, 0, 0), gs::rgb4(1, 1, 2), gs::rgb4(14, 10, 7), gs::rgb4(cr, cg, cb),
        gs::rgb4(tr, tg, tb), gs::rgb4(9, 6, 3), gs::rgb4(4, 3, 1), gs::rgb4(15, 12, 3),
        gs::rgb4(12, 6, 2), gs::rgb4(12, 13, 9), gs::rgb4(13, 2, 2), gs::rgb4(3, 10, 3),
        gs::rgb4(13, 9, 5), gs::rgb4(15, 14, 11), gs::rgb4(15, 15, 15), gs::rgb4(2, 1, 2),
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
    gs::Bitmap b(180, 42);
    b.rect(4, 3, 172, 5, 6);
    for (int i = 0; i < 8; i++) {
        int c = (i & 1) ? 3 : 4;
        int x = 6 + i * 21;
        b.rect(float(x), 8, 21, 16, c);
        b.ellipse(float(x + 10), 28, 11, 8, c);
    }
    b.poly({{148, 10}, {172, 16}, {148, 22}}, 10);
    b.outline(1, false);
    return b;
}

gs::Bitmap postArt() {
    gs::Bitmap b(14, 84);
    b.rect(3, 2, 8, 78, 5);
    b.rect(4, 2, 3, 78, 13);
    b.rect(2, 2, 10, 5, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap counterArt() {
    gs::Bitmap b(200, 40);
    b.rect(4, 4, 192, 10, 13);
    b.rect(4, 14, 192, 4, 12);
    b.rect(4, 18, 192, 16, 5);
    for (int i = 0; i < 7; i++) b.rect(float(16 + i * 26), 22, 3, 8, 6);
    b.rect(70, 24, 28, 6, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap crateArt() {
    gs::Bitmap b(36, 28);
    b.rect(2, 6, 32, 18, 5);
    b.rect(2, 8, 32, 2, 6);
    b.rect(2, 14, 32, 2, 6);
    b.rect(2, 20, 32, 2, 6);
    b.rect(10, 6, 2, 18, 6);
    b.rect(24, 6, 2, 18, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap clerkArt() {
    gs::Bitmap b(44, 70);
    b.ellipse(22, 12, 11, 6, 3);
    b.rect(11, 12, 22, 4, 3);
    b.ellipse(22, 22, 8, 9, 2);
    b.rect(16, 20, 3, 3, 1);
    b.rect(25, 20, 3, 3, 1);
    b.set(31, 22, 1);
    b.rect(18, 26, 6, 1, 1);
    b.rect(12, 32, 20, 18, 4);
    b.rect(16, 34, 12, 4, 3);
    b.rect(8, 32, 5, 14, 3);
    b.rect(31, 32, 5, 14, 3);
    b.rect(32, 42, 8, 5, 2);
    b.rect(15, 50, 6, 14, 6);
    b.rect(23, 50, 6, 14, 6);
    b.rect(13, 62, 9, 4, 1);
    b.rect(22, 62, 9, 4, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap personArt() {
    gs::Bitmap b(46, 70);
    b.rect(14, 6, 16, 6, 6);
    b.ellipse(22, 18, 8, 9, 2);
    b.rect(16, 16, 3, 3, 1);
    b.rect(25, 16, 3, 3, 1);
    b.set(32, 18, 1);
    b.rect(18, 22, 5, 1, 1);
    b.rect(12, 28, 22, 18, 3);
    b.rect(16, 32, 12, 8, 4);
    b.rect(8, 30, 5, 12, 3);
    b.rect(33, 30, 5, 12, 3);
    b.rect(36, 36, 7, 5, 2);
    b.rect(15, 46, 6, 16, 6);
    b.rect(24, 46, 6, 16, 6);
    b.rect(13, 60, 9, 4, 1);
    b.rect(23, 60, 9, 4, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap hatArt() {
    gs::Bitmap b(28, 14);
    b.ellipse(14, 6, 8, 4, 3);
    b.rect(2, 8, 24, 3, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap bagArt() {
    gs::Bitmap b(16, 18);
    b.rect(3, 5, 10, 11, 12);
    b.rect(5, 2, 6, 4, 6);
    b.rect(6, 9, 4, 2, 13);
    b.outline(1, false);
    return b;
}

gs::Bitmap appleArt() {
    gs::Bitmap b(30, 28);
    b.ellipse(15, 16, 9, 8, 10);
    b.ellipse(12, 13, 3, 2, 14);
    b.rect(14, 5, 2, 5, 6);
    b.ellipse(20, 8, 5, 3, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap loafArt() {
    gs::Bitmap b(30, 28);
    b.ellipse(15, 16, 12, 7, 12);
    b.ellipse(15, 15, 8, 4, 13);
    b.rect(8, 14, 14, 2, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap fishArt() {
    gs::Bitmap b(30, 28);
    b.poly({{4, 14}, {12, 9}, {12, 19}}, 9);
    b.ellipse(18, 14, 8, 5, 9);
    b.ellipse(22, 13, 2, 2, 14);
    b.ellipse(22, 12, 1, 1, 1);
    b.poly({{16, 18}, {24, 24}, {14, 20}}, 11);
    b.outline(1, false);
    return b;
}

gs::Bitmap jarArt() {
    gs::Bitmap b(30, 28);
    b.rect(9, 8, 12, 14, 14);
    b.rect(10, 13, 10, 8, 10);
    b.rect(8, 5, 14, 4, 8);
    b.ellipse(15, 5, 2, 2, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap pearArt() {
    gs::Bitmap b(30, 28);
    b.ellipse(15, 17, 7, 8, 11);
    b.ellipse(15, 10, 4, 4, 11);
    b.rect(14, 3, 2, 4, 6);
    b.ellipse(20, 7, 4, 2, 4);
    b.outline(1, false);
    return b;
}

gs::Bitmap eggsArt() {
    gs::Bitmap b(30, 28);
    b.rect(4, 12, 22, 10, 12);
    b.ellipse(10, 12, 4, 5, 14);
    b.ellipse(19, 12, 4, 5, 14);
    b.rect(4, 16, 22, 2, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap soapArt() {
    gs::Bitmap b(30, 28);
    b.ellipse(15, 15, 10, 6, 13);
    b.rect(8, 14, 14, 2, 11);
    b.ellipse(12, 12, 2, 1, 14);
    b.outline(1, false);
    return b;
}

gs::Bitmap honeyArt() {
    gs::Bitmap b(30, 28);
    b.ellipse(15, 18, 8, 7, 6);
    b.ellipse(15, 16, 5, 4, 7);
    b.rect(9, 8, 12, 4, 5);
    b.ellipse(15, 7, 2, 2, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap coinArt(int metal, int mark) {
    gs::Bitmap b(26, 26);
    b.ellipse(13, 13, 10, 10, metal);
    b.ellipse(13, 13, 7, 7, 13);
    b.ellipse(13, 13, 5, 5, metal);
    if (mark == 1) b.ellipse(13, 13, 1.5f, 1.5f, 1);
    else if (mark == 2) {
        b.ellipse(10, 13, 1.3f, 1.3f, 1);
        b.ellipse(16, 13, 1.3f, 1.3f, 1);
    } else if (mark == 5) {
        b.set(13, 9, 1);
        b.set(10, 12, 1);
        b.set(16, 12, 1);
        b.set(10, 16, 1);
        b.set(16, 16, 1);
    } else {
        b.ellipse(13, 13, 2.4f, 2.4f, 1);
        b.ellipse(13, 13, 1.1f, 1.1f, metal);
    }
    b.outline(1, false);
    return b;
}

gs::Bitmap billArt() {
    gs::Bitmap b(42, 24);
    b.rect(2, 3, 38, 18, 9);
    b.ellipse(12, 12, 5, 5, 11);
    b.ellipse(12, 12, 2, 2, 7);
    b.rect(22, 8, 14, 2, 7);
    b.rect(22, 13, 10, 2, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap dishArt() {
    gs::Bitmap b(56, 18);
    b.ellipse(28, 10, 24, 6, 6);
    b.ellipse(28, 9, 18, 4, 13);
    b.outline(1, false);
    return b;
}

gs::Bitmap bellArt() {
    gs::Bitmap b(18, 20);
    b.rect(8, 1, 2, 4, 6);
    b.ellipse(9, 11, 6, 5, 7);
    b.ellipse(9, 10, 3, 2, 14);
    b.rect(8, 15, 2, 3, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap boardArt() {
    gs::Bitmap b(128, 52);
    b.rect(1, 1, 126, 50, 6);
    b.rect(5, 5, 118, 42, 13);
    b.rect(63, 10, 2, 32, 6);
    b.outline(1, false);
    return b;
}

gs::Bitmap bracketArt() {
    gs::Bitmap b(28, 12);
    b.rect(1, 7, 26, 3, 1);
    b.rect(1, 2, 3, 8, 1);
    b.rect(24, 2, 3, 8, 1);
    return b;
}

gs::Bitmap solidArt() {
    gs::Bitmap b(4, 4);
    b.rect(0, 0, 4, 4, 1);
    return b;
}

gs::Bitmap shadeArt() {
    gs::Bitmap b(36, 14);
    b.ellipse(18, 7, 16, 5, 1);
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
    int plaster[4], cobble[4];
    uint8_t base[64];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) base[y * 8 + x] = (y < 2) ? 6 : 5;
    int baseTile = tiles.shared(base);
    for (int i = 0; i < 4; i++) {
        uint8_t p[64], c[64];
        for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) {
                bool speck = ((x + i * 3) % 5 == 0 && (y + i) % 4 == 0);
                p[y * 8 + x] = uint8_t(speck ? 12 : 13);
                bool mortar = (x == 0 || y == 0);
                bool grit = ((x * 2 + y + i) % 7 == 0);
                c[y * 8 + x] = uint8_t(mortar ? 6 : grit ? 12 : 5);
            }
        plaster[i] = tiles.shared(p);
        cobble[i] = tiles.shared(c);
    }
    for (int cy = 8; cy <= 14; cy++)
        for (int cx = 0; cx < 40; cx++) vdp.A.set(cx, cy, gs::entry(plaster[(cx + cy) & 3], PAL_STALL));
    for (int cx = 0; cx < 40; cx++) vdp.A.set(cx, 15, gs::entry(baseTile, PAL_STALL));
    for (int cy = 16; cy <= 24; cy++)
        for (int cx = 0; cx < 40; cx++) vdp.A.set(cx, cy, gs::entry(cobble[(cx + cy) & 3], PAL_STALL));
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    ink(vdp, PAL_HUD, 15, 15, 15);
    vdp.setColor(PAL_HUD * 16 + 15, gs::rgb4(2, 1, 2));
    paint(vdp, PAL_STALL, 13, 2, 2, 15, 14, 11);
    paint(vdp, PAL_CLERK, 2, 5, 11, 15, 15, 15);
    paint(vdp, PAL_C0, 2, 8, 4, 15, 13, 8);
    paint(vdp, PAL_C1, 2, 4, 10, 15, 12, 3);
    paint(vdp, PAL_C2, 11, 4, 2, 15, 14, 12);
    paint(vdp, PAL_C3, 8, 3, 9, 14, 14, 15);
    paint(vdp, PAL_GOODS, 2, 8, 3, 1, 6, 2);
    vdp.setColor(PAL_GOODS * 16 + 9, gs::rgb4(5, 8, 12));
    vdp.setColor(PAL_GOODS * 16 + 10, gs::rgb4(13, 2, 2));
    vdp.setColor(PAL_GOODS * 16 + 11, gs::rgb4(8, 12, 3));
    paint(vdp, PAL_COIN, 15, 12, 3, 12, 6, 2);
    paint(vdp, PAL_BILL, 2, 7, 3, 14, 12, 4);
    vdp.setColor(PAL_BILL * 16 + 9, gs::rgb4(11, 14, 9));
    vdp.setColor(PAL_BILL * 16 + 11, gs::rgb4(2, 9, 4));
    ink(vdp, PAL_OK, 3, 13, 4);
    ink(vdp, PAL_WARN, 15, 12, 2);
    ink(vdp, PAL_BAD, 14, 3, 2);
    ink(vdp, PAL_DIM, 2, 2, 4);

    art.awning = gs::uploadMipped(vdp, awningArt());
    art.post = gs::uploadMipped(vdp, postArt());
    art.counter = gs::uploadMipped(vdp, counterArt());
    art.crate = gs::uploadMipped(vdp, crateArt());
    art.clerk = gs::uploadMipped(vdp, clerkArt());
    art.person = gs::uploadMipped(vdp, personArt());
    art.hat = gs::uploadMipped(vdp, hatArt());
    art.bag = gs::uploadMipped(vdp, bagArt());
    art.good[0] = gs::uploadMipped(vdp, appleArt());
    art.good[1] = gs::uploadMipped(vdp, loafArt());
    art.good[2] = gs::uploadMipped(vdp, fishArt());
    art.good[3] = gs::uploadMipped(vdp, jarArt());
    art.good[4] = gs::uploadMipped(vdp, pearArt());
    art.good[5] = gs::uploadMipped(vdp, eggsArt());
    art.good[6] = gs::uploadMipped(vdp, soapArt());
    art.good[7] = gs::uploadMipped(vdp, honeyArt());
    art.coin[0] = gs::uploadMipped(vdp, coinArt(8, 1));
    art.coin[1] = gs::uploadMipped(vdp, coinArt(8, 2));
    art.coin[2] = gs::uploadMipped(vdp, coinArt(14, 5));
    art.coin[3] = gs::uploadMipped(vdp, coinArt(7, 10));
    art.bill = gs::uploadMipped(vdp, billArt());
    art.dish = gs::uploadMipped(vdp, dishArt());
    art.bell = gs::uploadMipped(vdp, bellArt());
    art.board = gs::uploadMipped(vdp, boardArt());
    art.shade = gs::uploadImage(vdp, shadeArt());
    art.bracket = gs::uploadImage(vdp, bracketArt());
    art.solid = gs::uploadImage(vdp, solidArt());
    for (int d = 0; d < 10; d++) {
        char s[2] = {char('0' + d), 0};
        art.digit[d] = say(vdp, s, 3);
    }
    art.title = say(vdp, "S3 MARKET", 3);
    art.stall = say(vdp, "ONE STALL", 2);
    art.moved = say(vdp, "LINE MOVED", 2);
    art.stalled = say(vdp, "STALLED", 2);
    art.exact = say(vdp, "EXACT", 2);
    art.brief = say(vdp, "SHORT", 2);
    art.over = say(vdp, "OVER", 2);
    art.line = say(vdp, "LINE", 1);

    gs::TileAlloc tiles(vdp, 1);
    loadFont(vdp, tiles, art);
    stampRoom(vdp, tiles);
    vdp.A.enabled = true;
    vdp.B.enabled = false;
    vdp.hudEnabled = true;
    vdp.A.scroll(0, 0);
    vdp.setFogColor(gs::rgb4(6, 5, 4));
}

}  // namespace market
