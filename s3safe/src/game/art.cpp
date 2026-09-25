#include "game/art.h"

#include <initializer_list>
#include <string>

namespace vault {
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

// Room indices, palette PAL_ROOM.
constexpr int WALL = 1, STRIPE = 2, CEIL = 3, MOLD = 4, WOOD = 5, WOODD = 6, FLOOR = 7, SEAM = 8;
constexpr int PAPER = 9, BRASS = 10, RED = 11, HOLE = 12, GOLD = 13, GLOW = 14, SHADOW = 15;

constexpr int kDoorX = 114, kDoorY = 104, kDoorW = 92, kDoorH = 84;
constexpr int kHoleX = 118, kHoleY = 108, kHoleW = 84, kHoleH = 76;
constexpr float kDialLx[3] = {16, 46, 76};
constexpr float kDialLy = 30;

void paintRoom(gs::Bitmap& b, Art& art) {
    b.rect(0, 0, 320, 224, WALL);
    b.rect(0, 0, 320, 26, CEIL);
    b.ellipse(160, 8, 70, 10, GLOW);
    b.rect(0, 26, 320, 6, MOLD);
    for (int x = 0; x < 320; x += 18) b.rect(x, 32, 3, 124, STRIPE);

    b.rect(0, 156, 320, 36, WOOD);
    b.rect(0, 156, 320, 4, MOLD);
    b.rect(0, 192, 320, 4, WOODD);
    b.rect(0, 196, 320, 28, FLOOR);
    for (int y = 200; y < 224; y += 8) b.rect(0, y, 320, 1, SEAM);
    for (int x = 10; x < 320; x += 28) b.rect(x, 196, 1, 28, SEAM);

    // Round clock. The digit sits in the paper face.
    const float ccx = 52, ccy = 64, cr = 28;
    b.ellipse(ccx + 2, ccy + 3, cr + 2, cr + 2, SHADOW);
    b.ellipse(ccx, ccy, cr + 3, cr + 3, WOODD);
    b.ellipse(ccx, ccy, cr, cr, BRASS);
    b.ellipse(ccx, ccy, cr - 6, cr - 6, PAPER);
    b.rect(ccx - 4, ccy - cr - 9, 8, 10, BRASS);
    b.ellipse(ccx, ccy - cr - 9, 5, 3, BRASS);
    art.clueX[0] = ccx;
    art.clueY[0] = ccy;

    // Framed picture. The digit is the picture.
    const int px = 126, py = 34, pw = 68, ph = 54;
    b.rect(px + 3, py + 3, pw, ph, SHADOW);
    b.rect(px, py, pw, ph, WOOD);
    b.rect(px + 4, py + 4, pw - 8, ph - 8, BRASS);
    b.rect(px + 8, py + 8, pw - 16, ph - 16, PAPER);
    art.clueX[1] = px + pw * 0.5f;
    art.clueY[1] = py + ph * 0.5f;

    // Tear-off calendar. The digit is the date.
    const int kx = 236, ky = 32, kw = 70, kh = 64;
    b.rect(kx + 3, ky + 3, kw, kh, SHADOW);
    b.rect(kx, ky, kw, kh, WOODD);
    b.rect(kx + 4, ky + 4, kw - 8, 14, RED);
    b.rect(kx + 4, ky + 18, kw - 8, kh - 22, PAPER);
    b.ellipse(kx + 20, ky + 2, 4, 5, BRASS);
    b.ellipse(kx + kw - 20, ky + 2, 4, 5, BRASS);
    stamp(b, kx + 23, ky + 8, "DATE", PAPER);
    art.clueX[2] = kx + 4 + (kw - 8) * 0.5f;
    art.clueY[2] = ky + 18 + (kh - 22) * 0.5f;

    // Side lamps, no digits.
    b.rect(86, 118, 8, 22, BRASS);
    b.ellipse(90, 112, 12, 8, GLOW);
    b.ellipse(90, 112, 5, 4, PAPER);
    b.rect(248, 118, 8, 22, BRASS);
    b.ellipse(252, 112, 12, 8, GLOW);
    b.ellipse(252, 112, 5, 4, PAPER);

    // Wall safe: brass bezel, dark cavity, the prize behind the door.
    b.rect(110, 100, 100, 92, BRASS);
    b.rect(112, 102, 96, 88, WOODD);
    b.rect(kHoleX, kHoleY, kHoleW, kHoleH, HOLE);
    b.rect(132, 118, 56, 8, GOLD);
    b.rect(132, 132, 56, 8, GOLD);
    b.rect(132, 146, 56, 8, GOLD);
    b.rect(140, 160, 40, 16, PAPER);
    b.ellipse(160, 168, 5, 5, RED);

    b.rect(78, 204, 164, 12, RED);
    b.rect(84, 207, 152, 6, WOODD);
}

gs::Bitmap paintDoor() {
    gs::Bitmap b(kDoorW, kDoorH);
    b.rect(0, 0, kDoorW, kDoorH, 1);
    b.rect(0, 0, kDoorW, 3, 2);
    b.rect(0, 0, 3, kDoorH, 2);
    b.rect(0, kDoorH - 3, kDoorW, 3, 3);
    b.rect(kDoorW - 3, 0, 3, kDoorH, 3);
    for (float x : kDialLx) {
        b.ellipse(x, kDialLy, 12, 12, 5);
        b.ellipse(x, kDialLy, 10, 10, 4);
        b.rect(x - 1, kDialLy - 15, 2, 4, 7);
    }
    b.rect(40, 50, 12, 22, 6);
    b.rect(42, 48, 8, 24, 5);
    b.ellipse(46, 48, 5, 4, 5);
    b.ellipse(46, 72, 5, 4, 6);
    b.ellipse(8, 8, 2.4f, 2.4f, 7);
    b.ellipse(kDoorW - 8, 8, 2.4f, 2.4f, 7);
    b.ellipse(8, kDoorH - 8, 2.4f, 2.4f, 7);
    b.ellipse(kDoorW - 8, kDoorH - 8, 2.4f, 2.4f, 7);
    return b;
}

gs::Bitmap paintCaret() {
    gs::Bitmap b(16, 10);
    b.poly({{8, 9}, {1, 1}, {15, 1}}, 1);
    return b;
}

gs::Bitmap wheelBitmap(int d) {
    // Native dial size. A scaled 0 collapses into two bars.
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

void keepFull(gs::Mipped& m, gs::VDP& vdp, const gs::Bitmap& g) {
    gs::Image img = gs::uploadImage(vdp, g);
    m.w = g.w;
    m.h = g.h;
    m.lv[0] = m.lv[1] = m.lv[2] = img;
}

gs::Bitmap digitBitmap(int d) {
    // Open shapes. The system 0 is slashed and reads as a block on a dial.
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
    const int scale = 4;
    gs::Bitmap b(5 * scale, 7 * scale);
    for (int y = 0; y < 7; y++)
        for (int x = 0; x < 5; x++)
            if (row[d][y][x] == '#') b.rect(float(x * scale), float(y * scale), float(scale), float(scale), 1);
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
    for (int d = 0; d < 10; d++) {
        keepFull(art.digit[d], vdp, digitBitmap(d));
        keepFull(art.wheel[d], vdp, wheelBitmap(d));
    }
}

}  // namespace

void buildArt(gs::VDP& vdp, Art& art) {
    const uint16_t shadow = gs::rgb4(1, 1, 2);
    setPal(vdp, PAL_TEXT, {0, gs::rgb4(15, 14, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_AMBER, {0, gs::rgb4(15, 12, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_RED, {0, gs::rgb4(15, 4, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(8, 15, 6), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, shadow});
    setPal(vdp, PAL_ROOM,
           {0, gs::rgb4(2, 5, 4), gs::rgb4(3, 7, 5), gs::rgb4(13, 12, 10), gs::rgb4(9, 6, 3), gs::rgb4(7, 4, 2),
            gs::rgb4(4, 2, 1), gs::rgb4(8, 5, 3), gs::rgb4(5, 3, 2), gs::rgb4(15, 14, 12), gs::rgb4(13, 10, 3),
            gs::rgb4(12, 2, 2), gs::rgb4(1, 1, 2), gs::rgb4(15, 12, 4), gs::rgb4(15, 14, 8), gs::rgb4(1, 2, 2)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(2, 1, 1)});
    setPal(vdp, PAL_WHEEL, {0, gs::rgb4(15, 15, 15)});
    setPal(vdp, PAL_DOOR,
           {0, gs::rgb4(8, 9, 10), gs::rgb4(14, 15, 15), gs::rgb4(3, 4, 5), gs::rgb4(1, 1, 2), gs::rgb4(13, 10, 3),
            gs::rgb4(7, 5, 2), gs::rgb4(15, 14, 11)});
    setPal(vdp, PAL_RING, {0, gs::rgb4(15, 12, 2)});
    setPal(vdp, PAL_PICK, {0, gs::rgb4(15, 14, 8)});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(2, 3, 4)});

    gs::TileAlloc tiles(vdp);
    loadFont(vdp, art, tiles);

    gs::Bitmap room(gs::SCREEN_W, gs::SCREEN_H);
    paintRoom(room, art);
    vdp.B.clear();
    gs::bitmapToPlane(tiles, vdp.B, 0, 0, room, PAL_ROOM);
    vdp.A.enabled = false;
    vdp.A.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        vdp.lineBackdrop[y] = gs::rgb4(2, 5, 4);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }

    art.door = gs::uploadMipped(vdp, paintDoor());
    art.caret = gs::uploadMipped(vdp, paintCaret());
    gs::Bitmap solid(16, 16);
    solid.rect(0, 0, 16, 16, 1);
    art.solid = gs::uploadMipped(vdp, solid);

    art.doorX = kDoorX;
    art.doorY = kDoorY;
    art.doorW = kDoorW;
    art.doorH = kDoorH;
    art.dialX[0] = kDoorX + kDialLx[0];
    art.dialX[1] = kDoorX + kDialLx[1];
    art.dialX[2] = kDoorX + kDialLx[2];
    art.dialY = kDoorY + kDialLy;
    // Closed door covers the cavity. This slide parks the door on the cavity's right edge.
    art.slideOpen = float(kHoleX + kHoleW - kDoorX);
}

}  // namespace vault
