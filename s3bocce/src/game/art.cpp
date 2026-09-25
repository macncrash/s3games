#include "game/art.h"

#include <initializer_list>

namespace bocce {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) {
        if (i < 16) vdp.setColor(pal * 16 + i, c);
        i++;
    }
    for (; i < 16; i++) vdp.setColor(pal * 16 + i, 0);
    vdp.setColor(pal * 16 + 15, gs::rgb4(1, 1, 2));
}

int uploadTile(gs::VDP& vdp, gs::TileAlloc& tiles, const gs::Bitmap& b) {
    uint8_t px[64] = {};
    for (int y = 0; y < 8 && y < b.h; y++)
        for (int x = 0; x < 8 && x < b.w; x++) px[y * 8 + x] = uint8_t(b.get(x, y) & 15);
    int t = tiles.alloc(1);
    vdp.loadTile(t, px);
    return t;
}

gs::Bitmap ballArt() {
    gs::Bitmap b(18, 18);
    b.ellipse(9, 9, 8.0f, 8.0f, 3);
    b.ellipse(9, 9, 7.1f, 7.1f, 2);
    b.ellipse(8.4f, 8.2f, 6.0f, 6.0f, 2);
    for (int y = 7; y <= 10; y++)
        for (int x = 2; x <= 15; x++)
            if (b.get(x, y)) b.set(x, y, 4);
    b.ellipse(6.4f, 6.0f, 2.1f, 1.5f, 1);
    return b;
}

gs::Bitmap jackArt() {
    gs::Bitmap b(14, 14);
    b.ellipse(7, 7, 6.0f, 6.0f, 4);
    b.ellipse(7, 7, 4.4f, 4.4f, 1);
    b.ellipse(5.6f, 5.3f, 1.7f, 1.3f, 2);
    b.ellipse(7.2f, 7.4f, 1.2f, 1.2f, 4);
    return b;
}

gs::Bitmap shadowArt() {
    gs::Bitmap b(16, 8);
    b.ellipse(8, 4, 6.2f, 2.4f, 1);
    return b;
}

gs::Bitmap treeArt() {
    gs::Bitmap b(16, 40);
    b.rect(7, 30, 2, 9, 3);
    b.rect(6, 37, 4, 2, 3);
    b.ellipse(8, 24, 5.2f, 11.0f, 2);
    b.ellipse(8, 16, 3.6f, 9.0f, 1);
    b.ellipse(8, 8, 2.2f, 6.0f, 1);
    return b;
}

gs::Bitmap sunArt() {
    gs::Bitmap b(20, 20);
    b.rect(9, 0, 2, 3, 1);
    b.rect(9, 17, 2, 3, 1);
    b.rect(0, 9, 3, 2, 1);
    b.rect(17, 9, 3, 2, 1);
    b.ellipse(10, 10, 7.2f, 7.2f, 1);
    b.ellipse(8.2f, 8.2f, 2.8f, 2.8f, 2);
    return b;
}

gs::Bitmap awningArt() {
    gs::Bitmap b(56, 16);
    b.rect(0, 1, 56, 2, 3);
    b.rect(0, 3, 56, 10, 2);
    for (int i = 0; i < 7; i++) b.rect(i * 8, 3, 4, 10, 1);
    b.rect(4, 13, 2, 3, 3);
    b.rect(50, 13, 2, 3, 3);
    return b;
}

gs::Bitmap potArt() {
    gs::Bitmap b(16, 20);
    b.ellipse(5, 7, 3.2f, 3.0f, 3);
    b.ellipse(11, 7, 3.2f, 3.0f, 3);
    b.ellipse(8, 6, 3.4f, 2.6f, 4);
    b.ellipse(8, 5, 1.6f, 1.6f, 5);
    b.rect(5, 11, 6, 2, 2);
    b.poly({{5, 12}, {11, 12}, {10, 18}, {6, 18}}, 1);
    b.rect(7, 14, 2, 2, 2);
    return b;
}

gs::Bitmap tuftArt() {
    gs::Bitmap b(8, 12);
    b.line(4, 11, 1, 2, 1, 1);
    b.line(4, 11, 7, 1, 2, 1);
    b.line(4, 11, 4, 1, 1, 1);
    return b;
}

gs::Bitmap dotArt() {
    gs::Bitmap b(6, 6);
    b.ellipse(3, 3, 2.2f, 2.2f, 1);
    return b;
}

gs::Bitmap gravelArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.set(1, 2, 2);
    b.set(2, 2, 2);
    b.set(5, 1, 3);
    b.set(6, 4, 2);
    b.set(3, 5, 3);
    b.set(7, 6, 2);
    b.set(1, 6, 3);
    b.set(4, 3, 2);
    return b;
}

gs::Bitmap chalkArt() {
    gs::Bitmap b = gravelArt();
    for (int y = 0; y < 8; y++) {
        b.set(0, y, 4);
        b.set(1, y, 4);
    }
    return b;
}

gs::Bitmap woodArt() {
    gs::Bitmap b(8, 8);
    b.rect(0, 0, 8, 8, 1);
    b.rect(0, 0, 8, 1, 2);
    b.rect(0, 3, 8, 1, 3);
    b.rect(0, 7, 8, 1, 3);
    b.set(2, 5, 2);
    b.set(6, 2, 3);
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
    setPal(vdp, PAL_INK, {0, ink});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 13, 4)});
    setPal(vdp, PAL_YOU, {0, gs::rgb4(8, 15, 8), gs::rgb4(2, 12, 4), gs::rgb4(1, 6, 2), ink});
    setPal(vdp, PAL_THEM, {0, gs::rgb4(15, 8, 7), gs::rgb4(12, 2, 3), gs::rgb4(6, 1, 2), ink});
    setPal(vdp, PAL_JACK, {0, gs::rgb4(15, 14, 6), gs::rgb4(15, 15, 13), gs::rgb4(10, 7, 2), gs::rgb4(13, 2, 2)});
    setPal(vdp, PAL_WOOD, {0, gs::rgb4(10, 6, 3), gs::rgb4(14, 10, 5), gs::rgb4(5, 3, 1)});
    setPal(vdp, PAL_GRAVEL, {0, gs::rgb4(12, 10, 7), gs::rgb4(8, 6, 4), gs::rgb4(14, 12, 9), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_TREE, {0, gs::rgb4(3, 10, 4), gs::rgb4(1, 6, 2), gs::rgb4(6, 4, 2)});
    setPal(vdp, PAL_SUN, {0, gs::rgb4(15, 13, 5), gs::rgb4(15, 15, 12)});
    setPal(vdp, PAL_AIM, {0, gs::rgb4(15, 14, 8), gs::rgb4(15, 8, 2)});
    setPal(vdp, PAL_LOGO, {0, gs::rgb4(15, 13, 5), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_AWN, {0, gs::rgb4(13, 2, 3), gs::rgb4(15, 14, 10), gs::rgb4(6, 1, 2)});
    setPal(vdp, PAL_POT, {0, gs::rgb4(12, 5, 3), gs::rgb4(7, 2, 2), gs::rgb4(2, 9, 3), gs::rgb4(6, 14, 5), gs::rgb4(15, 12, 4)});
    setPal(vdp, PAL_TUFT, {0, gs::rgb4(5, 13, 4), gs::rgb4(2, 8, 3)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, tiles, art);
    art.gravel = uploadTile(vdp, tiles, gravelArt());
    art.wood = uploadTile(vdp, tiles, woodArt());
    art.chalk = uploadTile(vdp, tiles, chalkArt());
    art.ball = gs::uploadMipped(vdp, ballArt());
    art.jack = gs::uploadMipped(vdp, jackArt());
    art.shadow = gs::uploadMipped(vdp, shadowArt());
    art.tree = gs::uploadMipped(vdp, treeArt());
    art.sun = gs::uploadMipped(vdp, sunArt());
    art.awning = gs::uploadMipped(vdp, awningArt());
    art.pot = gs::uploadMipped(vdp, potArt());
    art.tuft = gs::uploadMipped(vdp, tuftArt());
    art.dot = gs::uploadMipped(vdp, dotArt());
    art.logo = gs::uploadMipped(vdp, gs::textBitmap("S3 BOCCE", {2, 1, 2, 0, 1}));
    art.win = gs::uploadMipped(vdp, gs::textBitmap("FIRST TO SEVEN", {2, 1, 2, 0, 1}));
}

}  // namespace bocce
