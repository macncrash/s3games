#include "game/art.h"

#include "console/gfx.h"

#include <cstdint>
#include <initializer_list>
#include <string>

namespace maze {
namespace {

uint32_t mix(int x, int y) {
    uint32_t h = uint32_t(x) * 0x8DA6B343u ^ uint32_t(y) * 0xD8163841u;
    h ^= h >> 13;
    h *= 0x5BD1E995u;
    return h ^ (h >> 15);
}

void setPal(gs::VDP& vdp, int pal, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(pal * 16 + i++, c);
    while (i < 16) vdp.setColor(pal * 16 + i++, 0);
}

void loadFont(gs::VDP& vdp, int base) {
    for (int c = 32; c < 96; c++) {
        uint8_t px[64] = {};
        const uint8_t* g = gs::glyph(char(c));
        for (int y = 0; y < 7; y++)
            for (int x = 0; x < 5; x++)
                if (g[y * 5 + x]) px[(y + 1) * 8 + (x + 1)] = 1;
        vdp.loadTile(base + (c - 32), px);
    }
}

gs::Image say(gs::VDP& vdp, const std::string& s, int scale, bool outline) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.shadow = 2;
    st.outline = outline ? 2 : 0;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

// Chunky walker: 0 faces the camera, 1 shows the back, 2 faces right.
gs::Bitmap person(int facing, int stride) {
    gs::Bitmap b(16, 24);
    int swing = stride ? 1 : 0;
    b.rect(4, 0, 8, 2, 1);
    b.rect(5, 0, 6, 2, 6);
    b.rect(3, 2, 10, 2, 1);
    b.rect(4, 2, 8, 2, 6);
    if (facing == 1) {
        b.rect(5, 4, 6, 3, 5);
    } else if (facing == 2) {
        b.rect(5, 4, 2, 3, 5);
        b.rect(6, 4, 6, 4, 1);
        b.rect(7, 4, 4, 3, 4);
        b.set(10, 5, 1);
        b.set(9, 6, 5);
    } else {
        b.rect(5, 4, 6, 4, 1);
        b.rect(6, 4, 4, 3, 4);
        b.set(6, 5, 1);
        b.set(9, 5, 1);
        b.set(7, 6, 5);
        b.set(8, 6, 5);
    }
    b.rect(3, 8, 10, 9, 1);
    b.rect(4, 9, 8, 7, 2);
    b.rect(5, 14, 6, 2, 3);
    if (facing == 0) b.rect(6, 8, 4, 2, 7);
    if (facing == 2) {
        b.rect(11, 10, 2, 5, 2);
        b.rect(12, 14, 3, 4, 1);
        b.rect(13, 15, 2, 2, 10);
        b.set(13, 15, 11);
    } else {
        b.rect(2, 9, 2, 6, 1);
        b.rect(2, 10, 2, 4, 2);
        b.rect(12, 9 + swing, 2, 6, 1);
        b.rect(12, 10 + swing, 2, 4, 2);
        b.rect(13, 13 + swing, 3, 4, 1);
        b.rect(14, 14 + swing, 2, 2, 10);
        b.set(14, 14 + swing, 11);
    }
    b.rect(4, 17, 3, 5, 1);
    b.rect(9, 17, 3, 5, 1);
    b.rect(5, 17, 2, 4, 8);
    b.rect(9, 17 + swing, 2, 4, 8);
    b.rect(4, 21, 4, 2, 9);
    b.rect(8, 21, 4, 2, 9);
    if (stride) b.rect(8, 20, 4, 2, 9);
    return b;
}

gs::Bitmap birdPic(int wing) {
    gs::Bitmap b(16, 10);
    b.ellipse(8, 6, 5, 3, 1);
    b.ellipse(8, 6, 4, 2, 2);
    b.ellipse(8, 7, 3, 1, 5);
    b.ellipse(12, 5, 2, 2, 1);
    b.ellipse(12, 5, 1, 1, 2);
    b.set(13, 4, 6);
    b.rect(14, 5, 2, 1, 4);
    if (wing == 0) b.ellipse(6, 5, 4, 2, 3);
    else {
        b.ellipse(6, 3, 3, 2, 3);
        b.rect(4, 1, 4, 2, 3);
    }
    return b;
}

// The title card: hedge walls, a path, a walker, and the lit gate.
gs::Bitmap diorama() {
    gs::Bitmap t(176, 104);
    t.rect(0, 0, 176, 104, 1);
    t.rect(0, 0, 176, 34, 2);
    t.ellipse(40, 16, 18, 7, 3);
    t.ellipse(52, 14, 12, 5, 3);
    t.ellipse(128, 18, 20, 7, 3);
    t.rect(8, 36, 160, 18, 4);
    for (int i = 0; i < 8; i++) t.ellipse(18 + i * 20, 38, 14, 10, 5);
    for (int i = 0; i < 7; i++) t.ellipse(28 + i * 20, 34, 8, 6, 6);
    t.poly({{64, 100}, {112, 100}, {100, 46}, {76, 46}}, 7);
    t.poly({{74, 100}, {102, 100}, {94, 52}, {82, 52}}, 8);
    t.rect(6, 56, 58, 42, 4);
    t.rect(112, 56, 58, 42, 4);
    for (int i = 0; i < 4; i++) {
        t.ellipse(16 + i * 14, 58, 12, 10, 5);
        t.ellipse(124 + i * 14, 58, 12, 10, 5);
        t.ellipse(22 + i * 14, 72, 8, 6, 6);
        t.ellipse(130 + i * 14, 74, 8, 6, 6);
    }
    t.ellipse(24, 80, 2, 2, 12);
    t.ellipse(40, 86, 2, 2, 12);
    t.ellipse(136, 82, 2, 2, 12);
    t.ellipse(152, 70, 2, 2, 12);
    t.rect(74, 34, 5, 22, 11);
    t.rect(97, 34, 5, 22, 11);
    t.rect(74, 32, 28, 5, 11);
    t.ellipse(88, 30, 5, 5, 9);
    t.ellipse(88, 30, 2, 2, 15);
    t.rect(84, 76, 8, 10, 10);
    t.rect(85, 72, 6, 5, 14);
    t.rect(86, 78, 4, 2, 13);
    t.rect(84, 86, 3, 4, 11);
    t.rect(89, 86, 3, 4, 11);
    t.rect(92, 80, 3, 4, 9);
    t.set(93, 81, 15);
    t.rect(0, 0, 176, 3, 11);
    t.rect(0, 101, 176, 3, 11);
    t.rect(0, 0, 3, 104, 11);
    t.rect(173, 0, 3, 104, 11);
    return t;
}

bool isWall(const uint8_t* h, int x, int y) {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return h[y * MW + x] != 0;
}

// Light only in the hedge past the exit, so the corner reads as the way out.
bool exitGlow(const uint8_t* h, int cx, int cy, int ex, int ey) {
    if (cx < ex || cy < ey || cx > ex + 1 || cy > ey + 1) return false;
    bool border = cx == 0 || cy == 0 || cx == MW - 1 || cy == MH - 1;
    return border && isWall(h, cx, cy);
}

gs::Bitmap paintMaze(const uint8_t* h, int ex, int ey, int sx, int sy, int dx, int dy) {
    gs::Bitmap b(MW * CELL, MH * CELL);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            int cx = x / CELL, cy = y / CELL;
            int lx = x - cx * CELL, ly = y - cy * CELL;
            uint32_t hv = mix(x, y);
            if (exitGlow(h, cx, cy, ex, ey)) {
                b.set(x, y, (hv & 3u) == 0 ? 15 : 14);
                continue;
            }
            if (isWall(h, cx, cy)) {
                bool border = cx == 0 || cy == 0 || cx == MW - 1 || cy == MH - 1;
                bool rim = false;
                if (lx < 3 && !isWall(h, cx - 1, cy)) rim = true;
                if (lx >= CELL - 3 && !isWall(h, cx + 1, cy)) rim = true;
                if (ly < 3 && !isWall(h, cx, cy - 1)) rim = true;
                if (ly >= CELL - 3 && !isWall(h, cx, cy + 1)) rim = true;
                int c = border ? ((hv % 5u) == 0 ? 7 : 6) : 8;
                if (!border) {
                    uint32_t n = hv % 10u;
                    if (n < 2) c = 7;
                    else if (n < 5) c = 9;
                    else if (n == 5) c = 6;
                }
                if (rim) c = (hv % 3u) == 0 ? 10 : 9;
                if (!border && !rim && lx > 3 && ly > 3 && lx < 12 && ly < 12 && (hv % 67u) == 0) c = 11;
                b.set(x, y, c);
                continue;
            }
            bool lip = false;
            if (isWall(h, (x - 1) / CELL, cy) || isWall(h, (x + 1) / CELL, cy) || isWall(h, cx, (y - 1) / CELL) ||
                isWall(h, cx, (y + 1) / CELL)) {
                // A two-pixel soil lip where the hedge meets the path.
                if (lx < 2 || lx >= CELL - 2 || ly < 2 || ly >= CELL - 2) lip = true;
            }
            int c = 3;
            if (lip) c = 1;
            else if ((hv % 17u) == 0) c = 4;
            else if ((hv % 11u) == 0) c = 2;
            else if ((hv % 29u) == 0) c = 5;
            b.set(x, y, c);
        }
    }
    auto court = [&](int cx, int cy, int fill) {
        b.rect(float(cx * CELL), float(cy * CELL), float(CELL), float(CELL), fill);
    };
    court(ex, ey, 4);
    b.rect(float(ex * CELL + 1), float(ey * CELL + 1), 3, 3, 12);
    b.rect(float(ex * CELL + 12), float(ey * CELL + 1), 3, 3, 13);
    b.ellipse(float(ex * CELL + 8), float(ey * CELL + 8), 5, 5, 14);
    b.ellipse(float(ex * CELL + 8), float(ey * CELL + 8), 2, 2, 15);
    if (dx >= 0) {
        court(dx, dy, 2);
        b.rect(float(dx * CELL + 1), float(dy * CELL + 1), float(CELL - 2), 3, 12);
        for (int i = 0; i < CELL; i += 4) b.rect(float(dx * CELL + i), float(dy * CELL + 4), 2, float(CELL - 5), 13);
    }
    b.rect(float(sx * CELL + 4), float(sy * CELL + 11), 8, 3, 12);
    return b;
}

}  // namespace

void Art::bake(gs::VDP& vdp, const uint8_t* hedge, int ex, int ey, int sx, int sy, int dx, int dy) {
    setPal(vdp, 0,
           {0, gs::rgb4(5, 3, 2), gs::rgb4(8, 6, 4), gs::rgb4(11, 9, 6), gs::rgb4(13, 12, 9), gs::rgb4(4, 8, 3),
            gs::rgb4(1, 3, 1), gs::rgb4(1, 5, 2), gs::rgb4(2, 7, 2), gs::rgb4(3, 9, 3), gs::rgb4(6, 12, 4),
            gs::rgb4(13, 4, 6), gs::rgb4(8, 8, 9), gs::rgb4(4, 4, 5), gs::rgb4(15, 12, 4), gs::rgb4(15, 15, 11)});
    setPal(vdp, 1,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(13, 2, 2), gs::rgb4(8, 1, 2), gs::rgb4(14, 10, 7), gs::rgb4(4, 2, 1),
            gs::rgb4(6, 3, 2), gs::rgb4(14, 13, 11), gs::rgb4(2, 2, 5), gs::rgb4(1, 1, 1), gs::rgb4(15, 12, 4),
            gs::rgb4(15, 15, 12), 0, 0, 0, 0});
    setPal(vdp, 2, {0, gs::rgb4(15, 14, 10), gs::rgb4(1, 1, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, 3,
           {0, gs::rgb4(6, 10, 14), gs::rgb4(10, 14, 15), gs::rgb4(15, 15, 15), gs::rgb4(1, 4, 2), gs::rgb4(2, 8, 3),
            gs::rgb4(7, 13, 4), gs::rgb4(6, 5, 3), gs::rgb4(12, 10, 7), gs::rgb4(15, 13, 4), gs::rgb4(13, 2, 2),
            gs::rgb4(6, 6, 7), gs::rgb4(13, 4, 6), gs::rgb4(14, 10, 7), gs::rgb4(5, 3, 2), gs::rgb4(15, 15, 12)});
    setPal(vdp, 4,
           {0, gs::rgb4(1, 1, 1), gs::rgb4(7, 5, 3), gs::rgb4(10, 7, 4), gs::rgb4(14, 8, 2), gs::rgb4(13, 11, 9),
            gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0});

    loadFont(vdp, fontBase);
    gs::TileAlloc tiles(vdp, 80);
    gs::bitmapToPlane(tiles, vdp.A, OX / 8, OY / 8, paintMaze(hedge, ex, ey, sx, sy, dx, dy), 0);

    for (int face = 0; face < 3; face++)
        for (int stride = 0; stride < 2; stride++) body[face][stride] = gs::uploadImage(vdp, person(face, stride));
    shadow = gs::uploadImage(vdp, [] {
        gs::Bitmap s(14, 6);
        s.ellipse(7, 3, 6, 2, 1);
        return s;
    }());
    bird[0] = gs::uploadImage(vdp, birdPic(0));
    bird[1] = gs::uploadImage(vdp, birdPic(1));
    titlePic = gs::uploadImage(vdp, diorama());
    titleName = say(vdp, "S3 MAZE", 3, true);
    titleSub = say(vdp, "A HEDGE FROM THE PATH", 1, false);
    titleRule = say(vdp, "THE EXIT IS THE ONLY WIN", 1, false);
    titleMove = say(vdp, "ARROWS TO WALK", 1, false);
    titleGo = say(vdp, "PRESS ENTER", 1, false);
    sayOnly = say(vdp, "ONLY THE EXIT WINS", 1, false);
    sayFind = say(vdp, "FIND THE LIT GATE", 1, false);
    sayShut = say(vdp, "A SHUT GATE. NOT THE EXIT", 1, false);
    sayOut = say(vdp, "OUT. THE EXIT WAS THE ONLY WIN", 1, false);
}

}  // namespace maze
