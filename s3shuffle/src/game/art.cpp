#include "game/art.h"

#include <initializer_list>

namespace shuffle {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 1));
}

int uploadTile(gs::VDP& vdp, gs::TileAlloc& tiles, const gs::Bitmap& b) {
    uint8_t px[64] = {};
    for (int y = 0; y < 8 && y < b.h; y++)
        for (int x = 0; x < 8 && x < b.w; x++) px[y * 8 + x] = uint8_t(b.get(x, y) & 15);
    int t = tiles.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

gs::Bitmap waxArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(0, 2, 8, 1, 2);
    b.rect(0, 5, 8, 1, 2);
    b.set(3, 6, 3);
    b.set(6, 1, 3);
    return b;
}

gs::Bitmap lineArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 2);
    b.rect(0, 1, 8, 6, 1);
    return b;
}

gs::Bitmap hRailArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(0, 0, 8, 1, 2);
    b.rect(0, 5, 8, 2, 3);
    return b;
}

gs::Bitmap vRailArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(0, 0, 1, 8, 2);
    b.rect(6, 0, 2, 8, 3);
    return b;
}

gs::Bitmap woodArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.set(2, 2, 2);
    b.set(5, 5, 2);
    b.set(6, 1, 2);
    return b;
}

gs::Bitmap gutterArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.set(1, 2, 2);
    b.set(4, 5, 2);
    b.set(6, 1, 2);
    b.set(2, 6, 2);
    return b;
}

gs::Bitmap puckArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8.0f, 8.0f, 3);
    b.ellipse(9, 9, 6.3f, 6.3f, 2);
    b.ellipse(9, 9, 2.5f, 2.5f, 4);
    b.ellipse(6.8f, 6.4f, 2.0f, 1.3f, 1);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(8, 4, 6.4f, 2.4f, 1);
    return b;
}

gs::Bitmap lampArt() {
    gs::Bitmap b(14, 30);
    b.line(7, 0, 7, 7, 4, 1);
    b.poly({{1, 8}, {13, 8}, {11, 20}, {3, 20}}, 3);
    b.poly({{3, 9}, {11, 9}, {10, 18}, {4, 18}}, 2);
    b.ellipse(7, 17, 2.2f, 1.6f, 1);
    b.rect(6, 20, 2, 3, 4);
    return b;
}

gs::Bitmap bottleArt() {
    gs::Bitmap b(14, 28);
    b.rect(6, 1, 2, 7, 5);
    b.rect(5, 6, 4, 2, 6);
    b.ellipse(7, 17, 5.2f, 8.0f, 6);
    b.ellipse(7, 17, 3.8f, 6.2f, 5);
    b.ellipse(7, 19, 2.6f, 3.6f, 7);
    b.ellipse(5.4f, 14, 1.1f, 2.2f, 8);
    return b;
}

gs::Bitmap glassArt() {
    gs::Bitmap b(12, 18);
    b.poly({{2, 1}, {10, 1}, {8, 8}, {4, 8}}, 8);
    b.rect(5, 8, 2, 5, 8);
    b.ellipse(6, 15, 3.2f, 1.4f, 2);
    b.line(3, 2, 4, 6, 1, 1);
    return b;
}

gs::Bitmap chevArt() {
    gs::Bitmap b(11, 8);
    b.line(1, 7, 5, 1, 1, 1);
    b.line(5, 1, 9, 7, 1, 1);
    b.line(2, 7, 5, 2, 2, 1);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(4, 4);
    b.ellipse(2, 2, 1.4f, 1.4f, 1);
    return b;
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
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        a.font[c - 32] = t;
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t ink = gs::rgb4(15, 15, 15);
    setPal(vdp, PAL_INK, {0, ink, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(15, 15, 12), gs::rgb4(14, 11, 5), gs::rgb4(8, 6, 2), gs::rgb4(3, 6, 12)});
    setPal(vdp, PAL_HOUSE, {0, gs::rgb4(15, 6, 5), gs::rgb4(12, 2, 2), gs::rgb4(6, 1, 1), gs::rgb4(15, 12, 5)});
    setPal(vdp, PAL_WAX, {0, gs::rgb4(14, 12, 8), gs::rgb4(12, 10, 6), gs::rgb4(9, 7, 4)});
    setPal(vdp, PAL_Z3, {0, gs::rgb4(13, 7, 3), gs::rgb4(10, 5, 2), gs::rgb4(8, 3, 2)});
    setPal(vdp, PAL_Z2, {0, gs::rgb4(13, 10, 5), gs::rgb4(11, 8, 4), gs::rgb4(8, 6, 3)});
    setPal(vdp, PAL_Z1, {0, gs::rgb4(14, 12, 7), gs::rgb4(12, 10, 5), gs::rgb4(9, 7, 4)});
    setPal(vdp, PAL_LINE, {0, gs::rgb4(15, 15, 13), gs::rgb4(6, 5, 3)});
    setPal(vdp, PAL_RAIL, {0, gs::rgb4(6, 3, 1), gs::rgb4(11, 7, 3), gs::rgb4(13, 10, 4)});
    setPal(vdp, PAL_GUTTER, {0, gs::rgb4(1, 4, 3), gs::rgb4(3, 8, 5)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 5), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(15, 14, 8), gs::rgb4(14, 12, 6), gs::rgb4(8, 6, 3), gs::rgb4(10, 9, 7),
                           gs::rgb4(5, 12, 7), gs::rgb4(2, 6, 4), gs::rgb4(12, 9, 3), gs::rgb4(13, 15, 15)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 15, 13), gs::rgb4(15, 13, 4), gs::rgb4(11, 8, 2), gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_BAD, {0, gs::rgb4(15, 5, 4), gs::rgb4(4, 1, 1)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.wax = uploadTile(vdp, tiles, waxArt());
    art.line = uploadTile(vdp, tiles, lineArt());
    art.hRail = uploadTile(vdp, tiles, hRailArt());
    art.vRail = uploadTile(vdp, tiles, vRailArt());
    art.wood = uploadTile(vdp, tiles, woodArt());
    art.gutter = uploadTile(vdp, tiles, gutterArt());
    art.puck = gs::uploadMipped(vdp, puckArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.lamp = gs::uploadMipped(vdp, lampArt());
    art.bottle = gs::uploadMipped(vdp, bottleArt());
    art.glass = gs::uploadMipped(vdp, glassArt());
    art.chev = gs::uploadMipped(vdp, chevArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.digit[0] = gs::uploadMipped(vdp, gs::textBitmap("1", {2, 1, 2, 0, 0}));
    art.digit[1] = gs::uploadMipped(vdp, gs::textBitmap("2", {2, 1, 2, 0, 0}));
    art.digit[2] = gs::uploadMipped(vdp, gs::textBitmap("3", {2, 1, 2, 0, 0}));
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 SHUFFLE", {2, 1, 2, 0, 1}));
    art.win = gs::uploadMipped(vdp, gs::textBitmap("FIRST TO FIFTEEN", {2, 1, 2, 0, 1}));
}

}  // namespace shuffle
