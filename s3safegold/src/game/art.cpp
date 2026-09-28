#include "game/art.h"

#include <initializer_list>
#include <string>

namespace safegold {
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

constexpr int WALL = 1, CEIL = 3, MOLD = 4, WOOD = 5, WOODD = 6, FLOOR = 7, SEAM = 8;
constexpr int PAPER = 9, BRASS = 10, RED = 11, HOLE = 12, GOLD = 13, CREAM = 14, SHADOW = 15;

constexpr int kDoorX = 188, kDoorY = 40, kDoorW = 108, kDoorH = 128;
constexpr int kHoleX = 192, kHoleY = 44, kHoleW = 100, kHoleH = 120;
constexpr float kDialLx[3] = {22, 54, 86};
constexpr float kDialLy = 30;

void paintRoom(gs::Bitmap& b, Art& art) {
    b.rect(0, 0, 320, 224, WALL);
    b.rect(0, 0, 320, 16, CEIL);
    b.rect(0, 16, 320, 4, MOLD);
    b.rect(0, 186, 320, 8, WOOD);
    b.rect(0, 194, 320, 30, FLOOR);
    for (int y = 198; y < 224; y += 7) b.rect(0, y, 320, 1, SEAM);

    // The safe is on the right. The door lifts off it. Bars wait in the cavity.
    b.rect(176, 28, 132, 152, BRASS);
    b.rect(180, 32, 124, 144, WOODD);
    b.rect(kHoleX, kHoleY, kHoleW, kHoleH, HOLE);
    b.rect(204, 70, 76, 10, GOLD);
    b.rect(204, 88, 76, 10, GOLD);
    b.rect(204, 106, 76, 10, CREAM);
    b.rect(210, 128, 64, 16, PAPER);
    b.ellipse(242, 136, 5, 5, RED);

    // Round clock. Its digit is the first tumbler.
    const float ccx = 58, ccy = 64, cr = 28;
    b.ellipse(ccx + 2, ccy + 3, cr + 2, cr + 2, SHADOW);
    b.ellipse(ccx, ccy, cr + 3, cr + 3, WOODD);
    b.ellipse(ccx, ccy, cr, cr, BRASS);
    b.ellipse(ccx, ccy, cr - 6, cr - 6, PAPER);
    b.rect(ccx - 3, ccy - cr - 8, 6, 8, BRASS);
    art.clueX[0] = ccx;
    art.clueY[0] = ccy;

    // Ledger card. Its digit is the second tumbler.
    const int px = 108, py = 28, pw = 52, ph = 70;
    b.rect(px + 2, py + 3, pw, ph, SHADOW);
    b.rect(px, py, pw, ph, WOOD);
    b.rect(px + 4, py + 4, pw - 8, 12, RED);
    b.rect(px + 4, py + 18, pw - 8, ph - 24, PAPER);
    stamp(b, px + 10, py + 7, "NOTE", PAPER);
    art.clueX[1] = px + pw * 0.5f;
    art.clueY[1] = py + 40;

    // Plate under the clock. Its digit finishes the lock, and that tumbler is gold.
    const int kx = 16, ky = 112, kw = 148, kh = 58;
    b.rect(kx + 3, ky + 3, kw, kh, SHADOW);
    b.rect(kx, ky, kw, kh, BRASS);
    b.rect(kx + 6, ky + 6, kw - 12, 14, GOLD);
    b.rect(kx + 6, ky + 22, kw - 12, kh - 30, PAPER);
    stamp(b, kx + 40, ky + 9, "GOLD X2", WOODD);
    art.clueX[2] = kx + kw * 0.5f;
    art.clueY[2] = ky + 38;

    art.cavityX = kHoleX;
    art.cavityY = kHoleY;
    art.cavityW = kHoleW;
    art.cavityH = kHoleH;
}

gs::Bitmap paintDoor() {
    gs::Bitmap b(kDoorW, kDoorH);
    b.rect(0, 0, kDoorW, kDoorH, 1);
    b.rect(0, 0, kDoorW, 4, 2);
    b.rect(0, 0, 4, kDoorH, 2);
    b.rect(0, kDoorH - 4, kDoorW, 4, 3);
    b.rect(kDoorW - 4, 0, 4, kDoorH, 3);
    for (float x : kDialLx) {
        b.ellipse(x, kDialLy, 13, 13, 5);
        b.ellipse(x, kDialLy, 11, 11, 4);
        b.rect(x - 1, kDialLy - 16, 2, 4, 7);
    }
    b.rect(46, 62, 16, 28, 6);
    b.rect(48, 58, 12, 34, 5);
    b.ellipse(54, 58, 7, 5, 5);
    b.ellipse(54, 92, 7, 5, 6);
    b.ellipse(10, 10, 2.5f, 2.5f, 7);
    b.ellipse(kDoorW - 10, 10, 2.5f, 2.5f, 7);
    b.ellipse(10, kDoorH - 10, 2.5f, 2.5f, 7);
    b.ellipse(kDoorW - 10, kDoorH - 10, 2.5f, 2.5f, 7);
    return b;
}

gs::Bitmap paintCaret() {
    gs::Bitmap b(16, 10);
    b.poly({{8, 9}, {1, 1}, {15, 1}}, 1);
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
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROOM,
           {0, gs::rgb4(2, 4, 5), gs::rgb4(3, 6, 7), gs::rgb4(8, 10, 12), gs::rgb4(10, 8, 4), gs::rgb4(6, 4, 2),
            gs::rgb4(3, 2, 1), gs::rgb4(7, 5, 3), gs::rgb4(4, 3, 2), gs::rgb4(15, 14, 11), gs::rgb4(12, 9, 3),
            gs::rgb4(11, 3, 2), gs::rgb4(1, 1, 2), gs::rgb4(15, 11, 2), gs::rgb4(14, 12, 8), gs::rgb4(1, 1, 2)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(15, 15, 14)});
    setPal(vdp, PAL_DOOR,
           {0, gs::rgb4(7, 8, 9), gs::rgb4(13, 14, 15), gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2), gs::rgb4(13, 10, 3),
            gs::rgb4(6, 4, 2), gs::rgb4(15, 13, 8)});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 2)});
    setPal(vdp, PAL_CREAM, {0, gs::rgb4(14, 12, 9)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(1, 2, 3)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap room(gs::SCREEN_W, gs::SCREEN_H);
    paintRoom(room, art);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, room, PAL_ROOM);
    vdp.A.enabled = false;
    vdp.A.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(1, 3, 4);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    art.door = gs::uploadMipped(vdp, paintDoor());
    art.caret = gs::uploadMipped(vdp, paintCaret());
    gs::Bitmap solid(8, 8);
    solid.rect(0, 0, 8, 8, 1);
    art.solid = gs::uploadMipped(vdp, solid);
    gs::Bitmap bar(8, 4);
    bar.rect(0, 0, 8, 4, 1);
    art.bar = gs::uploadMipped(vdp, bar);

    art.doorX = kDoorX;
    art.doorY = kDoorY;
    art.doorW = kDoorW;
    art.doorH = kDoorH;
    for (int i = 0; i < 3; i++) art.dialX[i] = kDoorX + kDialLx[i];
    art.dialY = kDoorY + kDialLy;
    // The door lifts clear of the cavity. Clues stay on the left, out of its path.
    art.slideOpen = float(kHoleY - kDoorY - kDoorH);
}

}  // namespace safegold
