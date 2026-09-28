#include "game/art.h"

#include <cmath>
#include <initializer_list>

namespace safeseven {
namespace {

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void stamp(gs::Bitmap& b, int x, int y, const char* s, int c) {
    for (int i = 0; s[i]; i++) {
        const uint8_t* g = gs::glyph(s[i]);
        for (int gy = 0; gy < 7; gy++)
            for (int gx = 0; gx < 5; gx++)
                if (g[gy * 5 + gx]) b.set(x + i * 6 + gx, y + gy, c);
    }
}

constexpr int NAVY = 1, PANEL = 2, MOLD = 3, COPPER = 4, DARK = 5, HOLE = 6;
constexpr int BAR = 7, PAPER = 8, INK = 9, LAMP = 10, FLOOR = 11, SEAM = 12, RIM = 13;

constexpr int kDoorX = 112, kDoorY = 28, kDoorW = 96, kDoorH = 96;

void paintRoom(gs::Bitmap& b, Art& art) {
    b.rect(0, 0, 320, 224, NAVY);
    b.rect(0, 0, 320, 14, DARK);
    b.rect(0, 14, 320, 4, MOLD);
    for (int x = 8; x < 312; x += 22) b.rect(x, 18, 2, 150, PANEL);
    b.rect(18, 22, 284, 140, PANEL);
    b.rect(24, 28, 272, 128, NAVY);

    b.ellipse(160, 76, 58, 58, RIM);
    b.ellipse(160, 76, 46, 46, HOLE);
    b.rect(142, 58, 36, 8, BAR);
    b.rect(142, 72, 36, 8, BAR);
    b.rect(142, 86, 36, 8, BAR);

    // Ledger slip, left. The first digit is the amount's last figure.
    b.rect(32, 40, 52, 64, DARK);
    b.rect(36, 44, 44, 56, PAPER);
    stamp(b, 40, 48, "SLIP", INK);
    art.clueX[0] = 58;
    art.clueY[0] = 82;

    // Key tag hanging under the lamp, right.
    b.rect(236, 36, 6, 28, COPPER);
    b.ellipse(250, 78, 22, 26, COPPER);
    b.ellipse(250, 78, 16, 20, PAPER);
    b.ellipse(250, 48, 10, 6, LAMP);
    art.clueX[1] = 250;
    art.clueY[1] = 80;

    // Floor stencil between the tumblers' shadow and the cage.
    b.rect(128, 168, 64, 22, PAPER);
    stamp(b, 140, 171, "BIN", INK);
    art.clueX[2] = 160;
    art.clueY[2] = 184;

    b.rect(0, 188, 320, 8, MOLD);
    b.rect(0, 196, 320, 28, FLOOR);
    for (int x = 0; x < 320; x += 16) b.rect(x, 196, 1, 28, SEAM);
}

gs::Bitmap paintDoor() {
    gs::Bitmap b(kDoorW, kDoorH);
    b.ellipse(48, 48, 46, 46, 1);
    b.ellipse(48, 48, 40, 40, 2);
    b.ellipse(48, 48, 18, 18, 3);
    b.ellipse(48, 48, 6, 6, 4);
    b.rect(46, 18, 4, 16, 4);
    b.rect(46, 62, 4, 14, 4);
    b.rect(22, 46, 14, 4, 4);
    for (int i = 0; i < 8; i++) {
        float a = float(i) * 0.785f;
        float x = 48 + 32 * std::cos(a);
        float y = 48 + 32 * std::sin(a);
        b.ellipse(x, y, 3.2f, 3.2f, 5);
    }
    return b;
}

gs::Bitmap paintCaret() {
    gs::Bitmap b(14, 8);
    b.poly({{7, 7}, {1, 1}, {13, 1}}, 1);
    return b;
}

gs::Bitmap glyphRows(const char* row[7], int scale, int cols) {
    gs::Bitmap b(cols * scale, 7 * scale);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < cols; x++)
            if (row[y][x] == '#') b.rect(float(x * scale), float(y * scale), float(scale), float(scale), 1);
    return b;
}

gs::Bitmap digitBitmap(int d) {
    static const char* row[10][7] = {
        {"#...#", "#...#", "#...#", "#...#", "#...#", "#...#", "#...#"},
        {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."},
        {"#####", "....#", "....#", "#####", "#....", "#....", "#####"},
        {"#####", "....#", "....#", "#####", "....#", "....#", "#####"},
        {"#...#", "#...#", "#...#", "#####", "....#", "....#", "....#"},
        {"#####", "#....", "#....", "#####", "....#", "....#", "#####"},
        {"#####", "#....", "#....", "#####", "#...#", "#...#", "#####"},
        {"#####", "....#", "....#", "...#.", "..#..", ".#...", ".#..."},
        {"#####", "#...#", "#...#", "#####", "#...#", "#...#", "#####"},
        {"#####", "#...#", "#...#", "#####", "....#", "....#", "#####"},
    };
    // Zero is an open ring so it does not read as a filled block.
    static const char* zero[7] = {"#####", "#...#", "#...#", "#...#", "#...#", "#...#", "#####"};
    const char** use = (d == 0) ? zero : row[d];
    return glyphRows(use, 3, 5);
}

gs::Bitmap wheelBitmap(int d) {
    static const char* row[10][7] = {
        {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."},
        {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."},
        {".###.", "#...#", "....#", "..##.", ".#...", "#....", "#####"},
        {".###.", "#...#", "....#", "..##.", "....#", "#...#", ".###."},
        {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."},
        {"#####", "#....", "####.", "....#", "....#", "#...#", ".###."},
        {".###.", "#....", "####.", "#...#", "#...#", "#...#", ".###."},
        {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."},
        {".###.", "#...#", ".###.", "#...#", "#...#", "#...#", ".###."},
        {".###.", "#...#", ".####", "....#", "....#", "#...#", ".###."},
    };
    return glyphRows(row[d], 2, 5);
}

void keepFull(gs::Mipped& m, gs::VDP& vdp, const gs::Bitmap& g) {
    gs::Image img = gs::uploadImage(vdp, g);
    m.w = g.w;
    m.h = g.h;
    m.lv[0] = m.lv[1] = m.lv[2] = img;
}

void loadFont(gs::VDP& vdp, Art& art, gs::TileAlloc& tiles) {
    for (int c = 32; c < 128; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[y * 8 + x + 1] = 1;
        int t = tiles.alloc(1);
        vdp.loadTile(t, px);
        art.font[c - 32] = t;
    }
    for (int d = 0; d < 10; d++) {
        keepFull(art.digit[d], vdp, digitBitmap(d));
        keepFull(art.wheel[d], vdp, wheelBitmap(d));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11)});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3)});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3)});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 15, 8)});
    setPal(vdp, PAL_ROOM,
           {0, gs::rgb4(1, 2, 5), gs::rgb4(2, 3, 7), gs::rgb4(8, 6, 3), gs::rgb4(12, 8, 3), gs::rgb4(1, 1, 2),
            gs::rgb4(0, 0, 1), gs::rgb4(14, 11, 3), gs::rgb4(14, 13, 10), gs::rgb4(3, 2, 2), gs::rgb4(15, 14, 8),
            gs::rgb4(3, 3, 4), gs::rgb4(2, 2, 3), gs::rgb4(10, 9, 7), 0, 0});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(15, 14, 12)});
    setPal(vdp, PAL_DOOR,
           {0, gs::rgb4(6, 7, 8), gs::rgb4(10, 11, 12), gs::rgb4(3, 3, 4), gs::rgb4(14, 12, 6), gs::rgb4(8, 7, 4)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3)});
    setPal(vdp, PAL_PIP, {0, gs::rgb4(4, 8, 12)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(1, 1, 2)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap room(gs::SCREEN_W, gs::SCREEN_H);
    paintRoom(room, art);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, room, PAL_ROOM);
    vdp.A.enabled = false;
    vdp.A.clear();
    vdp.B.enabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(1, 2, 5);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.setFogColor(gs::rgb4(1, 1, 2));

    art.door = gs::uploadMipped(vdp, paintDoor());
    art.caret = gs::uploadMipped(vdp, paintCaret());
    gs::Bitmap solid(8, 8);
    solid.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadMipped(vdp, solid);

    art.doorX = kDoorX;
    art.doorY = kDoorY;
    art.doorW = kDoorW;
    art.doorH = kDoorH;
    art.dialX[0] = 96;
    art.dialX[1] = 160;
    art.dialX[2] = 224;
    art.dialY = 150;
}

}  // namespace safeseven
