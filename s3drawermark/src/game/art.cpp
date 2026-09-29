#include "game/art.h"

#include <initializer_list>

namespace drawermark {
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

constexpr int WALL = 1, WALLD = 2, NIGHT = 3, MOON = 4, WOOD = 5, WOODD = 6, WOODL = 7;
constexpr int FELT = 8, FELTD = 9, BRASS = 10, PAPER = 11, RULE = 12, GOLD = 13, INK = 14, GLOW = 15;

void paintShop(gs::Bitmap& b) {
    b.rect(0, 0, 320, 224, WALL);
    b.rect(0, 0, 320, 36, NIGHT);
    b.rect(0, 33, 320, 3, WOODD);
    b.ellipse(46, 16, 8, 8, MOON);
    b.ellipse(43, 14, 6, 6, NIGHT);
    const int stars[][2] = {{80, 8}, {110, 18}, {150, 6}, {190, 14}, {230, 8}, {270, 18}, {300, 7}};
    for (auto s : stars) b.set(s[0], s[1], GLOW);

    b.rect(8, 42, 120, 62, WALLD);
    b.rect(6, 40, 118, 60, PAPER);
    b.rect(6, 40, 118, 3, RULE);
    stamp(b, 28, 48, "THE MARK", INK);
    stamp(b, 16, 62, "GOLD COIN", GOLD);
    stamp(b, 22, 76, "5.00 ONLY", INK);
    stamp(b, 14, 88, "SILVER STAYS", INK);

    b.rect(148, 44, 160, 48, WOODD);
    b.rect(152, 48, 152, 40, WOOD);
    b.rect(160, 54, 96, 22, INK);
    b.rect(160, 54, 96, 2, BRASS);
    stamp(b, 176, 60, "DRAWER", PAPER);

    b.rect(0, 100, 320, 6, WOODL);
    b.rect(0, 106, 320, 22, WOOD);
    b.rect(0, 126, 320, 3, WOODD);
    stamp(b, 8, 112, "COUNTER", INK);

    b.rect(8, 134, 304, 74, FELTD);
    b.rect(12, 138, 296, 66, FELT);
    b.ellipse(kWellX, kWellY, 28, 22, FELTD);
    b.ellipse(kWellX, kWellY, 22, 16, GOLD);
    b.ellipse(kWellX, kWellY, 16, 11, FELT);
    stamp(b, int(kWellX) - 12, 198, "MARK", PAPER);

    b.rect(0, 208, 320, 16, INK);
    b.rect(0, 208, 320, 2, BRASS);
}

gs::Bitmap goldBitmap() {
    gs::Bitmap b(22, 22);
    b.ellipse(11, 11, 10, 10, 2);
    b.ellipse(11, 11, 8, 8, 1);
    b.ellipse(8, 8, 2, 1.4f, 3);
    stamp(b, 5, 7, "G", 4);
    return b;
}

gs::Bitmap oneBitmap() {
    gs::Bitmap b(30, 16);
    b.rect(0, 0, 30, 16, 2);
    b.rect(1, 1, 28, 14, 1);
    b.rect(1, 1, 28, 2, 3);
    b.ellipse(8, 8, 4, 4, 4);
    stamp(b, 16, 4, "1", 5);
    return b;
}

gs::Bitmap buttonBitmap() {
    gs::Bitmap b(16, 16);
    b.ellipse(8, 8, 7, 7, 2);
    b.ellipse(8, 8, 4, 4, 1);
    b.ellipse(8, 8, 1.4f, 1.4f, 3);
    return b;
}

gs::Bitmap caretBitmap() {
    gs::Bitmap b(14, 8);
    b.poly({{7, 7}, {1, 1}, {13, 1}}, 1);
    return b;
}

gs::Bitmap coverBitmap() {
    gs::Bitmap b(300, 78);
    b.rect(0, 0, 300, 78, 2);
    b.rect(3, 3, 294, 72, 1);
    b.rect(3, 3, 294, 5, 3);
    for (int x = 16; x < 290; x += 26) b.rect(x, 14, 2, 50, 2);
    b.rect(126, 28, 48, 16, 4);
    b.ellipse(142, 36, 3, 3, 5);
    b.ellipse(158, 36, 3, 3, 5);
    return b;
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
    const uint16_t inkS = gs::rgb4(3, 1, 1);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 12)});
    shadow(vdp, PAL_TEXT, inkS);
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3)});
    shadow(vdp, PAL_AMBER, inkS);
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    shadow(vdp, PAL_RED, inkS);
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 6)});
    shadow(vdp, PAL_GREEN, inkS);
    setPal(vdp, PAL_SHOP,
           {0, gs::rgb4(6, 4, 3), gs::rgb4(3, 2, 2), gs::rgb4(1, 2, 6), gs::rgb4(13, 13, 10), gs::rgb4(8, 4, 2),
            gs::rgb4(4, 2, 1), gs::rgb4(12, 8, 3), gs::rgb4(1, 7, 4), gs::rgb4(0, 4, 2), gs::rgb4(12, 9, 3),
            gs::rgb4(15, 14, 11), gs::rgb4(12, 3, 3), gs::rgb4(14, 10, 2), gs::rgb4(2, 1, 1), gs::rgb4(15, 13, 6)});
    setPal(vdp, PAL_BILL, {0, gs::rgb4(6, 11, 5), gs::rgb4(1, 4, 2), gs::rgb4(11, 14, 8), gs::rgb4(3, 8, 4),
                           gs::rgb4(1, 3, 1)});
    setPal(vdp, PAL_GOLD,
           {0, gs::rgb4(14, 10, 2), gs::rgb4(8, 5, 1), gs::rgb4(15, 14, 6), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_JUNK, {0, gs::rgb4(12, 4, 5), gs::rgb4(6, 1, 2), gs::rgb4(15, 10, 8)});
    setPal(vdp, PAL_WOOD,
           {0, gs::rgb4(9, 5, 2), gs::rgb4(4, 2, 1), gs::rgb4(12, 8, 4), gs::rgb4(13, 10, 3), gs::rgb4(15, 14, 7),
            gs::rgb4(5, 5, 6)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(3, 1, 1)});
    shadow(vdp, PAL_INK, gs::rgb4(8, 5, 4));
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
        vdp.lineBackdrop[y] = gs::rgb4(2, 1, 2);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    art.piece[int(Kind::Gold)] = gs::uploadMipped(vdp, goldBitmap());
    art.piece[int(Kind::One)] = gs::uploadMipped(vdp, oneBitmap());
    art.piece[int(Kind::Button)] = gs::uploadMipped(vdp, buttonBitmap());
    art.caret = gs::uploadMipped(vdp, caretBitmap());
    art.cover = gs::uploadMipped(vdp, coverBitmap());
    gs::Bitmap solid(8, 8);
    solid.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadMipped(vdp, solid);
}

}  // namespace drawermark
