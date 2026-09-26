#include "game/art.h"

#include <initializer_list>

namespace drawer {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void shadow(gs::VDP& vdp, int pal, uint16_t c) { vdp.setColor(pal * 16 + 15, c); }

void stamp(gs::Bitmap& b, int x, int y, const char* s, int c) {
    for (int i = 0; s[i]; i++) {
        const uint8_t* g = gs::glyph(s[i]);
        for (int gy = 0; gy < 7; gy++)
            for (int gx = 0; gx < 5; gx++)
                if (g[gy * 5 + gx]) b.set(x + i * 6 + gx, y + gy, c);
    }
}

// Shop indices, palette PAL_SHOP.
constexpr int WALL = 1, WALLD = 2, NIGHT = 3, MOON = 4, WOOD = 5, WOODD = 6, WOODL = 7;
constexpr int FELT = 8, FELTD = 9, BRASS = 10, BRASSL = 11, PAPER = 12, RULE = 13, GLOW = 14, INK = 15;

void paintShop(gs::Bitmap& b) {
    b.rect(0, 0, 320, 224, WALL);

    // Night window above the register. The tape covers the left of it.
    b.rect(118, 0, 202, 44, NIGHT);
    b.rect(118, 0, 202, 3, WOODD);
    b.rect(118, 41, 202, 3, WOOD);
    b.rect(118, 0, 3, 44, WOODD);
    b.rect(317, 0, 3, 44, WOODD);
    b.ellipse(226, 16, 7, 7, MOON);
    b.ellipse(223, 15, 5, 5, NIGHT);
    const int stars[][2] = {{132, 8}, {156, 18}, {188, 6}, {208, 22}, {246, 28}, {304, 8}, {286, 20}};
    for (auto s : stars) {
        b.set(s[0], s[1], MOON);
        b.set(s[0] + 1, s[1], GLOW);
    }
    stamp(b, 140, 12, "TIN & LAMP", PAPER);

    // Open sign. The closed sprite covers this on the last night.
    b.rect(kSignX + 24, 0, 2, 6, WOODD);
    b.rect(kSignX, kSignY, kSignW, kSignH, RULE);
    b.rect(kSignX + 2, kSignY + 2, kSignW - 4, kSignH - 4, PAPER);
    stamp(b, kSignX + 16, kSignY + 5, "OPEN", INK);

    // Receipt. Red band, cream body, a torn tail.
    b.rect(8, 10, 104, 92, WALLD);
    b.rect(4, 4, 104, 94, PAPER);
    b.rect(4, 4, 104, 3, RULE);
    for (int x = 0; x < 104; x += 8) b.rect(4 + x, 94, 4, 5, PAPER);

    // Register body and a dark figure window.
    b.rect(124, 48, 190, 58, WOODD);
    b.rect(128, 52, 182, 50, WOOD);
    b.rect(132, 56, 174, 48, INK);
    b.rect(132, 56, 174, 2, BRASSL);
    b.ellipse(300, 70, 5, 5, BRASS);
    b.ellipse(300, 70, 2, 2, INK);

    // Counter, then the open till.
    b.rect(0, 108, 320, 6, WOODL);
    b.rect(0, 114, 320, 26, WOOD);
    b.rect(0, 136, 320, 4, WOODD);
    stamp(b, 8, 122, "ASIDE", INK);
    b.rect(292, 118, 16, 12, WOODD);
    b.ellipse(300, 114, 11, 7, FELT);
    b.ellipse(294, 112, 4, 3, WOODL);
    b.ellipse(306, 112, 4, 3, WOODL);

    b.rect(4, 142, 312, 66, FELTD);
    b.rect(8, 146, 304, 58, FELT);
    for (int i = 0; i < 8; i++) {
        float cx = kSlotX0 + i * kSlotPitch;
        if (i < 4) {
            b.rect(cx - 16, 152, 32, 40, FELTD);
            b.rect(cx - 14, 154, 28, 36, FELT);
        } else {
            b.ellipse(cx, 172, 14, 15, FELTD);
            b.ellipse(cx, 172, 11, 12, FELT);
        }
        if (i > 0) b.rect(cx - kSlotPitch * 0.5f, 146, 2, 54, FELTD);
    }
    const char* slotName[8] = {"20", "10", "5", "1", "Q", "D", "N", "P"};
    for (int i = 0; i < 8; i++) {
        float cx = kSlotX0 + i * kSlotPitch;
        int tw = slotName[i][1] ? 11 : 5;
        stamp(b, int(cx) - tw / 2, 200, slotName[i], PAPER);
    }

    // Lip under the till. Play text sits on this band.
    b.rect(0, 208, 320, 16, INK);
    b.rect(0, 208, 320, 2, BRASS);
}

gs::Bitmap billBitmap(const char* label) {
    gs::Bitmap b(34, 18);
    b.rect(0, 0, 34, 18, 2);
    b.rect(1, 1, 32, 16, 1);
    b.rect(1, 1, 32, 2, 3);
    b.rect(1, 15, 32, 2, 2);
    b.ellipse(8, 9, 5, 5, 5);
    b.ellipse(8, 9, 2, 2, 4);
    int n = label[1] ? 2 : 1;
    stamp(b, n == 2 ? 16 : 20, 5, label, 4);
    return b;
}

gs::Bitmap coinBitmap(int d, const char* label, int fill, int rim, int lite, int ink) {
    gs::Bitmap b(d, d);
    float c = (d - 1) * 0.5f;
    b.ellipse(c, c, c, c, rim);
    b.ellipse(c, c, c - 1.7f, c - 1.7f, fill);
    b.ellipse(c - c * 0.28f, c - c * 0.32f, c * 0.22f, c * 0.14f, lite);
    int tw = label[1] ? 11 : 5;
    stamp(b, int(c) - tw / 2, int(c) - 3, label, ink);
    return b;
}

gs::Bitmap buttonBitmap() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 3);
    b.ellipse(9, 9, 2.2f, 2.2f, 4);
    b.ellipse(5, 6, 1.3f, 1.3f, 4);
    b.ellipse(13, 6, 1.3f, 1.3f, 4);
    b.ellipse(5, 12, 1.3f, 1.3f, 4);
    b.ellipse(13, 12, 1.3f, 1.3f, 4);
    return b;
}

gs::Bitmap iouBitmap() {
    gs::Bitmap b(34, 18);
    b.rect(0, 0, 34, 18, 2);
    b.rect(1, 1, 32, 16, 1);
    stamp(b, 8, 2, "IOU", 2);
    b.rect(6, 12, 22, 1, 2);
    b.rect(6, 14, 14, 1, 2);
    return b;
}

gs::Bitmap tokenBitmap() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8, 8, 6);
    b.ellipse(9, 9, 6, 6, 5);
    b.rect(7, 7, 4, 4, 4);
    return b;
}

gs::Bitmap clipBitmap() {
    gs::Bitmap b(26, 12);
    b.rect(1, 0, 22, 12, 7);
    b.rect(4, 2, 16, 8, 0);
    b.rect(7, 4, 12, 4, 7);
    b.rect(10, 5, 6, 2, 0);
    b.rect(20, 1, 4, 3, 7);
    return b;
}

gs::Bitmap caretBitmap() {
    gs::Bitmap b(14, 8);
    b.poly({{7, 7}, {1, 1}, {13, 1}}, 1);
    return b;
}

gs::Bitmap coverBitmap() {
    gs::Bitmap b(312, 84);
    b.rect(0, 0, 312, 84, 2);
    b.rect(3, 3, 306, 78, 1);
    b.rect(3, 3, 306, 5, 3);
    b.rect(3, 74, 306, 6, 2);
    for (int x = 18; x < 300; x += 28) b.rect(x, 14, 2, 56, 2);
    b.rect(132, 32, 48, 18, 4);
    b.rect(136, 36, 40, 10, 5);
    b.ellipse(148, 41, 3, 3, 6);
    b.ellipse(164, 41, 3, 3, 6);
    b.ellipse(16, 16, 3, 3, 6);
    b.ellipse(296, 16, 3, 3, 6);
    b.ellipse(16, 68, 3, 3, 6);
    b.ellipse(296, 68, 3, 3, 6);
    return b;
}

gs::Bitmap signBitmap() {
    gs::Bitmap b(kSignW, kSignH);
    b.rect(0, 0, kSignW, kSignH, 3);
    b.rect(2, 2, kSignW - 4, kSignH - 4, 1);
    stamp(b, 10, 5, "CLOSED", 2);
    return b;
}

gs::Bitmap pieceBitmap(Kind k) {
    switch (k) {
    case Kind::Bill20: return billBitmap("20");
    case Kind::Bill10: return billBitmap("10");
    case Kind::Bill5: return billBitmap("5");
    case Kind::Bill1: return billBitmap("1");
    case Kind::Quarter: return coinBitmap(18, "25", 1, 2, 3, 7);
    case Kind::Dime: return coinBitmap(15, "10", 1, 2, 3, 7);
    case Kind::Nickel: return coinBitmap(16, "5", 1, 2, 3, 7);
    case Kind::Penny: return coinBitmap(16, "1", 4, 5, 6, 7);
    case Kind::Button: return buttonBitmap();
    case Kind::Iou: return iouBitmap();
    case Kind::Token: return tokenBitmap();
    case Kind::Clip: return clipBitmap();
    case Kind::Count: break;
    }
    return billBitmap("?");
}

void loadFont(gs::VDP& vdp, Art& art, gs::TileAlloc& tiles) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8 && x + 2 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t inkS = gs::rgb4(4, 2, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 12)});
    shadow(vdp, PAL_TEXT, gs::rgb4(2, 1, 2));
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3)});
    shadow(vdp, PAL_AMBER, inkS);
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    shadow(vdp, PAL_RED, inkS);
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 6)});
    shadow(vdp, PAL_GREEN, inkS);
    setPal(vdp, PAL_SHOP,
           {0, gs::rgb4(7, 5, 4), gs::rgb4(3, 2, 2), gs::rgb4(1, 2, 6), gs::rgb4(13, 13, 11), gs::rgb4(9, 5, 2),
            gs::rgb4(4, 2, 1), gs::rgb4(12, 8, 3), gs::rgb4(1, 6, 4), gs::rgb4(0, 3, 2), gs::rgb4(12, 9, 3),
            gs::rgb4(15, 13, 6), gs::rgb4(15, 14, 11), gs::rgb4(12, 3, 3), gs::rgb4(15, 12, 6), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_BILL,
           {0, gs::rgb4(6, 11, 5), gs::rgb4(1, 4, 2), gs::rgb4(11, 14, 8), gs::rgb4(1, 3, 1), gs::rgb4(8, 12, 6)});
    setPal(vdp, PAL_COIN,
           {0, gs::rgb4(12, 12, 13), gs::rgb4(5, 5, 6), gs::rgb4(15, 15, 15), gs::rgb4(12, 6, 3), gs::rgb4(6, 3, 1),
            gs::rgb4(15, 10, 5), gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_JUNK,
           {0, gs::rgb4(14, 13, 10), gs::rgb4(3, 1, 1), gs::rgb4(13, 2, 3), gs::rgb4(2, 1, 1), gs::rgb4(13, 10, 3),
            gs::rgb4(7, 5, 1), gs::rgb4(10, 11, 12)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(9, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(12, 8, 4), gs::rgb4(13, 10, 3), gs::rgb4(15, 14, 7),
            gs::rgb4(6, 6, 7)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(3, 1, 1)});
    shadow(vdp, PAL_INK, gs::rgb4(8, 5, 4));
    setPal(vdp, PAL_SIGN, {0, gs::rgb4(12, 2, 2), gs::rgb4(15, 14, 11), gs::rgb4(5, 1, 1)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(1, 1, 2)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap shop(gs::SCREEN_W, gs::SCREEN_H);
    paintShop(shop);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, shop, PAL_SHOP);
    vdp.A.enabled = false;
    vdp.A.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(3, 2, 2);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    for (int i = 0; i < int(Kind::Count); i++) art.piece[i] = gs::uploadMipped(vdp, pieceBitmap(Kind(i)));
    art.caret = gs::uploadMipped(vdp, caretBitmap());
    art.cover = gs::uploadMipped(vdp, coverBitmap());
    art.sign = gs::uploadMipped(vdp, signBitmap());
    gs::Bitmap solid(16, 16);
    solid.rect(0, 0, 16, 16, 1);
    art.solid = gs::uploadMipped(vdp, solid);
}

}  // namespace drawer
