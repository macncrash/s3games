#include "game/art.h"

#include <initializer_list>

namespace safebell {
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

constexpr int WALL = 1, PANEL = 2, CEIL = 3, MOLD = 4, WOOD = 5, WOODD = 6, FLOOR = 7, SEAM = 8;
constexpr int PAPER = 9, BRASS = 10, RED = 11, HOLE = 12, GOLD = 13, GLOW = 14, SHADOW = 15;

constexpr int kDoorX = 118, kDoorY = 108, kDoorW = 84, kDoorH = 78;
constexpr int kHoleX = 122, kHoleY = 112, kHoleW = 76, kHoleH = 70;
constexpr float kDialLx[3] = {14, 42, 70};
constexpr float kDialLy = 28;

void paintRoom(gs::Bitmap& b, Art& art) {
    b.rect(0, 0, 320, 224, WALL);
    b.rect(0, 0, 320, 22, CEIL);
    b.ellipse(160, 6, 90, 8, GLOW);
    b.rect(0, 22, 320, 5, MOLD);
    for (int x = 8; x < 320; x += 32) b.rect(x, 27, 2, 118, PANEL);

    b.rect(0, 150, 320, 40, WOOD);
    b.rect(0, 150, 320, 4, MOLD);
    b.rect(0, 190, 320, 34, FLOOR);
    for (int y = 196; y < 224; y += 7) b.rect(0, y, 320, 1, SEAM);

    // Ledger book. The digit is the page number.
    b.rect(28, 40, 58, 72, SHADOW);
    b.rect(24, 36, 58, 72, WOODD);
    b.rect(30, 42, 46, 60, PAPER);
    b.rect(30, 42, 8, 60, RED);
    stamp(b, 42, 46, "PG", WOODD);
    art.clueX[0] = 56;
    art.clueY[0] = 78;

    // Station clock under the bell hook. Digit is the hour.
    b.ellipse(162, 62, 26, 26, SHADOW);
    b.ellipse(160, 60, 26, 26, WOOD);
    b.ellipse(160, 60, 22, 22, BRASS);
    b.ellipse(160, 60, 16, 16, PAPER);
    b.line(160, 60, 160, 48, WOODD, 1.4f);
    b.line(160, 60, 170, 64, WOODD, 1.4f);
    art.clueX[1] = 160;
    art.clueY[1] = 60;

    // Coat check ticket. Digit is the claim number.
    b.rect(232, 38, 62, 70, SHADOW);
    b.rect(228, 34, 62, 70, PAPER);
    b.rect(228, 34, 62, 12, RED);
    b.ellipse(248, 32, 4, 5, BRASS);
    b.ellipse(270, 32, 4, 5, BRASS);
    stamp(b, 244, 38, "CHK", PAPER);
    art.clueX[2] = 259;
    art.clueY[2] = 74;

    // Bell hook and rope. The bell itself is a sprite so it can swing.
    b.rect(156, 22, 8, 10, BRASS);
    b.rect(158, 30, 4, 8, WOODD);
    art.bellX = 160;
    art.bellY = 48;

    // Wall safe cavity and bars of gold behind the door.
    b.rect(114, 104, 92, 86, BRASS);
    b.rect(116, 106, 88, 82, WOODD);
    b.rect(kHoleX, kHoleY, kHoleW, kHoleH, HOLE);
    b.rect(136, 124, 48, 7, GOLD);
    b.rect(136, 136, 48, 7, GOLD);
    b.rect(136, 148, 48, 7, GOLD);
    b.ellipse(160, 168, 5, 5, RED);

    b.rect(70, 204, 180, 10, RED);
    b.rect(76, 206, 168, 6, WOODD);
}

gs::Bitmap paintDoor() {
    gs::Bitmap b(kDoorW, kDoorH);
    b.rect(0, 0, kDoorW, kDoorH, 1);
    b.rect(0, 0, kDoorW, 3, 2);
    b.rect(0, 0, 3, kDoorH, 2);
    b.rect(0, kDoorH - 3, kDoorW, 3, 3);
    b.rect(kDoorW - 3, 0, 3, kDoorH, 3);
    for (float x : kDialLx) {
        b.ellipse(x, kDialLy, 11, 11, 5);
        b.ellipse(x, kDialLy, 9, 9, 4);
        b.rect(x - 1, kDialLy - 14, 2, 4, 7);
    }
    b.rect(36, 46, 12, 20, 6);
    b.rect(38, 44, 8, 22, 5);
    b.ellipse(42, 44, 5, 4, 5);
    b.ellipse(42, 66, 5, 4, 6);
    b.ellipse(7, 7, 2.2f, 2.2f, 7);
    b.ellipse(kDoorW - 7, 7, 2.2f, 2.2f, 7);
    b.ellipse(7, kDoorH - 7, 2.2f, 2.2f, 7);
    b.ellipse(kDoorW - 7, kDoorH - 7, 2.2f, 2.2f, 7);
    return b;
}

gs::Bitmap paintBell() {
    gs::Bitmap b(28, 24);
    b.rect(13, 0, 2, 4, 3);
    b.ellipse(14, 12, 12, 9, 1);
    b.ellipse(14, 11, 8, 6, 2);
    b.ellipse(14, 17, 3, 3, 4);
    b.rect(6, 16, 16, 2, 3);
    return b;
}

gs::Bitmap paintCaret() {
    gs::Bitmap b(14, 8);
    b.poly({{7, 7}, {1, 1}, {13, 1}}, 1);
    return b;
}

gs::Bitmap paintLamp() {
    gs::Bitmap b(10, 10);
    b.ellipse(5, 5, 4.2f, 4.2f, 1);
    b.ellipse(5, 5, 2.2f, 2.2f, 2);
    return b;
}

gs::Bitmap wheelBitmap(int d) {
    static const char* row[10][9] = {
        {".#####.", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", ".#####."},
        {"...#...", "..##...", "...#...", "...#...", "...#...", "...#...", "...#...", "...#...", "..###.."},
        {".#####.", "#.....#", "......#", ".....#.", "....#..", "...#...", "..#....", ".#.....", "#######"},
        {".#####.", "#.....#", "......#", "..####.", "......#", "......#", "#.....#", "#.....#", ".#####."},
        {"....#..", "...##..", "..#.#..", ".#..#..", "#...#..", "#######", "....#..", "....#..", "....#.."},
        {"#######", "#......", "#......", "######.", "......#", "......#", "#.....#", "#.....#", ".#####."},
        {"..####.", ".#.....", "#......", "#......", "######.", "#.....#", "#.....#", "#.....#", ".#####."},
        {"#######", "......#", ".....#.", "....#..", "...#...", "..#....", "..#....", "..#....", "..#...."},
        {".#####.", "#.....#", "#.....#", "#.....#", ".#####.", "#.....#", "#.....#", "#.....#", ".#####."},
        {".#####.", "#.....#", "#.....#", "#.....#", ".######", "......#", "......#", ".#....#", "..####."},
    };
    const int scale = 2;
    gs::Bitmap b(7 * scale, 9 * scale);
    for (int y = 0; y < 9; y++)
        for (int x = 0; x < 7; x++)
            if (row[d][y][x] == '#') b.rect(float(x * scale), float(y * scale), float(scale), float(scale), 1);
    return b;
}

gs::Bitmap digitBitmap(int d) {
    static const char* row[10][7] = {
        {".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###."},
        {"..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###."},
        {".###.", "#...#", "....#", "..##.", ".#...", "#....", "#####"},
        {".###.", "#...#", "....#", "..##.", "....#", "#...#", ".###."},
        {"...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#."},
        {"#####", "#....", "#....", "####.", "....#", "#...#", ".###."},
        {".###.", "#....", "#....", "####.", "#...#", "#...#", ".###."},
        {"#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..."},
        {".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###."},
        {".###.", "#...#", "#...#", ".####", "....#", "....#", ".###."},
    };
    const int scale = 3;
    gs::Bitmap b(5 * scale, 7 * scale);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (row[d][y][x] == '#') b.rect(float(x * scale), float(y * scale), float(scale), float(scale), 1);
    return b;
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
                if (g[y * 5 + x]) {
                    px[y * 8 + x + 1] = 1;
                    if (y + 1 < 8 && x + 2 < 8) px[(y + 1) * 8 + x + 2] = 15;
                }
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
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(14, 13, 11), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 11, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 15, 7), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROOM,
           {0, gs::rgb4(1, 3, 6), gs::rgb4(2, 5, 8), gs::rgb4(12, 11, 9), gs::rgb4(8, 5, 2), gs::rgb4(6, 3, 1),
            gs::rgb4(3, 2, 1), gs::rgb4(7, 4, 2), gs::rgb4(4, 2, 1), gs::rgb4(15, 14, 11), gs::rgb4(12, 9, 3),
            gs::rgb4(11, 2, 2), gs::rgb4(1, 1, 2), gs::rgb4(15, 11, 3), gs::rgb4(14, 13, 7), gs::rgb4(0, 1, 2)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_DOOR,
           {0, gs::rgb4(7, 8, 9), gs::rgb4(13, 14, 15), gs::rgb4(2, 3, 4), gs::rgb4(1, 1, 2), gs::rgb4(12, 9, 2),
            gs::rgb4(6, 4, 2), gs::rgb4(14, 13, 10)});
    setPal(vdp, PAL_BELL, {0, gs::rgb4(14, 11, 3), gs::rgb4(15, 14, 8), gs::rgb4(8, 5, 1), gs::rgb4(4, 2, 1)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(1, 2, 4)});
    setPal(vdp, PAL_LAMP, {0, gs::rgb4(4, 2, 2), gs::rgb4(15, 12, 3)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap room(gs::SCREEN_W, gs::SCREEN_H);
    paintRoom(room, art);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, room, PAL_ROOM);
    vdp.A.enabled = false;
    vdp.A.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(1, 3, 6);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    art.door = gs::uploadMipped(vdp, paintDoor());
    art.bell = gs::uploadMipped(vdp, paintBell());
    art.caret = gs::uploadMipped(vdp, paintCaret());
    art.lamp = gs::uploadMipped(vdp, paintLamp());
    gs::Bitmap solid(8, 8);
    solid.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadMipped(vdp, solid);

    art.doorX = kDoorX;
    art.doorY = kDoorY;
    art.doorW = kDoorW;
    art.doorH = kDoorH;
    art.dialX[0] = kDoorX + kDialLx[0];
    art.dialX[1] = kDoorX + kDialLx[1];
    art.dialX[2] = kDoorX + kDialLx[2];
    art.dialY = kDoorY + kDialLy;
    art.slideOpen = float(kHoleX + kHoleW - kDoorX);
}

}  // namespace safebell
