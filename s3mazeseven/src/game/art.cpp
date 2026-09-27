#include "game/art.h"

#include "console/gfx.h"

#include <cstdint>
#include <initializer_list>
#include <string>

namespace mazeseven {
namespace {

uint32_t mix(int x, int y) {
    uint32_t h = uint32_t(x) * 0x9E3779B1u ^ uint32_t(y) * 0x85EBCA77u;
    h ^= h >> 16;
    h *= 0x7FEB352Du;
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

gs::Image say(gs::VDP& vdp, const std::string& s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.shadow = 2;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

bool wall(const uint8_t* h, int x, int y) {
    if (x < 0 || y < 0 || x >= MW || y >= MH) return true;
    return h[y * MW + x] != 0;
}

void stampDigit(gs::Bitmap& b, int val, int x, int y) {
    auto p = [&](int dx, int dy) { b.set(x + dx, y + dy, 6); };
    if (val == 1) {
        p(1, 0);
        p(2, 0);
        p(2, 1);
        p(2, 2);
        p(2, 3);
        p(2, 4);
        p(1, 4);
        p(3, 4);
    } else if (val == 2) {
        for (int i = 0; i < 4; i++) p(i, 0);
        p(3, 1);
        for (int i = 0; i < 4; i++) p(i, 2);
        p(0, 3);
        for (int i = 0; i < 4; i++) p(i, 4);
    } else {
        for (int i = 0; i < 4; i++) p(i, 0);
        p(3, 1);
        for (int i = 0; i < 4; i++) p(i, 2);
        p(3, 3);
        for (int i = 0; i < 4; i++) p(i, 4);
    }
}

gs::Bitmap lampPic(int val) {
    gs::Bitmap b(16, 16);
    b.rect(7, 0, 2, 2, 2);
    b.ellipse(8, 6, 5, 4, 3);
    b.ellipse(8, 5, 2, 2, 4);
    b.set(8, 5, 6);
    b.rect(3, 10, 10, 6, 1);
    stampDigit(b, val, 6, 11);
    return b;
}

// One coat, two palettes: blue for you, red for them.
gs::Bitmap coat(int facing, int stride) {
    gs::Bitmap b(16, 24);
    int s = stride ? 1 : 0;
    b.ellipse(8, 3, 4, 3, 5);
    if (facing == 1) {
        b.ellipse(8, 6, 3, 3, 5);
    } else if (facing == 2) {
        b.ellipse(9, 6, 3, 3, 4);
        b.set(11, 5, 8);
        b.set(11, 6, 9);
    } else {
        b.ellipse(8, 6, 3, 3, 4);
        b.set(6, 5, 8);
        b.set(9, 5, 8);
        b.set(6, 6, 9);
        b.set(9, 6, 9);
        b.set(7, 8, 6);
        b.set(8, 8, 6);
    }
    b.rect(4, 9, 8, 2, 6);
    b.rect(4, 11, 8, 6, 2);
    b.rect(6, 12, 4, 4, 3);
    if (facing == 2) {
        b.rect(11, 11, 3, 2, 2);
        b.rect(13, 12, 2, 3, 4);
    } else {
        b.rect(2, 11, 2, 5, 2);
        b.rect(12, 11 + s, 2, 5, 2);
        b.set(2, 15, 4);
        b.set(13, 15 + s, 4);
    }
    b.rect(4, 17, 3, 4, 7);
    b.rect(9, 17, 3, 4, 7);
    b.rect(4, 21, 3, 2, 1);
    b.rect(9, 21 - s, 3, 2, 1);
    if (stride) b.rect(9, 20, 3, 2, 7);
    b.outline(1, false);
    return b;
}

gs::Bitmap titlePoster() {
    gs::Bitmap t(176, 88);
    t.rect(0, 0, 176, 88, 1);
    t.rect(0, 0, 176, 30, 2);
    t.ellipse(150, 14, 9, 9, 3);
    t.ellipse(154, 12, 8, 8, 2);
    const int stars[][2] = {{16, 8}, {36, 16}, {58, 7}, {88, 14}, {112, 8}, {20, 20}};
    for (auto s : stars) t.set(s[0], s[1], 4);
    t.rect(0, 52, 176, 36, 5);
    t.rect(0, 34, 46, 54, 6);
    t.rect(130, 34, 46, 54, 6);
    for (int i = 0; i < 4; i++) {
        t.ellipse(10 + i * 12, 36, 9, 7, 7);
        t.ellipse(138 + i * 12, 36, 9, 7, 7);
    }
    t.rect(46, 56, 84, 22, 8);
    for (int i = 0; i < 7; i++) {
        int x = 54 + i * 11;
        bool on = i < 6;
        t.rect(x + 2, 64, 3, 8, 9);
        t.ellipse(x + 3, 60, 4, 4, on ? 10 : 11);
        if (on) t.ellipse(x + 3, 60, 2, 2, 12);
    }
    t.rect(60, 70, 5, 8, 13);
    t.rect(61, 66, 3, 4, 14);
    t.rect(108, 70, 5, 8, 15);
    t.rect(109, 66, 3, 4, 14);
    t.rect(0, 0, 176, 2, 9);
    t.rect(0, 86, 176, 2, 9);
    t.rect(0, 0, 2, 88, 9);
    t.rect(174, 0, 2, 88, 9);
    return t;
}

gs::Bitmap paintMaze(const uint8_t* h, const uint8_t* lamps, int hx, int hy, int rx, int ry) {
    gs::Bitmap b(MW * CELL, MH * CELL);
    for (int y = 0; y < b.h; y++) {
        for (int x = 0; x < b.w; x++) {
            int cx = x / CELL;
            int cy = y / CELL;
            int lx = x - cx * CELL;
            int ly = y - cy * CELL;
            uint32_t hv = mix(x, y);
            if (wall(h, cx, cy)) {
                bool border = cx == 0 || cy == 0 || cx == MW - 1 || cy == MH - 1;
                int c = border ? 6 : 7;
                if (!border && (hv % 8u) == 0) c = 8;
                else if (!border && (hv % 5u) == 0) c = 6;
                bool rim = false;
                if (lx < 2 && !wall(h, cx - 1, cy)) rim = true;
                if (lx >= CELL - 2 && !wall(h, cx + 1, cy)) rim = true;
                if (ly < 2 && !wall(h, cx, cy - 1)) rim = true;
                if (ly >= CELL - 2 && !wall(h, cx, cy + 1)) rim = true;
                if (rim) c = 9;
                b.set(x, y, c);
                continue;
            }
            int c = 2;
            if ((hv % 11u) == 0) c = 3;
            else if ((hv % 19u) == 0) c = 5;
            bool lip = false;
            if (lx < 2 && wall(h, cx - 1, cy)) lip = true;
            if (lx >= CELL - 2 && wall(h, cx + 1, cy)) lip = true;
            if (ly < 2 && wall(h, cx, cy - 1)) lip = true;
            if (ly >= CELL - 2 && wall(h, cx, cy + 1)) lip = true;
            if (lip) c = 1;
            b.set(x, y, c);
        }
    }
    for (int cy = 1; cy < MH - 1; cy++) {
        for (int cx = 1; cx < MW - 1; cx++) {
            if (!wall(h, cx, cy)) continue;
            if ((mix(cx, cy) % 11u) != 0) continue;
            float fx = float(cx * CELL + 8);
            float fy = float(cy * CELL + 8);
            b.ellipse(fx, fy, 2, 2, 10);
            b.set(cx * CELL + 8, cy * CELL + 8, 11);
        }
    }
    for (int i = 0; i < MW * MH; i++) {
        if (!lamps[i]) continue;
        int cx = i % MW;
        int cy = i / MW;
        b.rect(float(cx * CELL + 3), float(cy * CELL + 3), 10, 10, 15);
    }
    auto mat = [&](int cx, int cy, int fill, int mark) {
        b.rect(float(cx * CELL + 2), float(cy * CELL + 2), 12, 12, fill);
        b.rect(float(cx * CELL + 5), float(cy * CELL + 5), 6, 6, mark);
    };
    mat(hx, hy, 12, 3);
    mat(rx, ry, 13, 3);
    return b;
}

}  // namespace

void Art::bake(gs::VDP& vdp, const uint8_t* hedge, const uint8_t* lamps, int yx, int yy, int tx, int ty) {
    setPal(vdp, PAL_MAZE,
           {0, gs::rgb4(2, 2, 2), gs::rgb4(6, 5, 3), gs::rgb4(8, 7, 5), gs::rgb4(4, 3, 2), gs::rgb4(9, 8, 6),
            gs::rgb4(1, 3, 2), gs::rgb4(2, 5, 2), gs::rgb4(3, 8, 3), gs::rgb4(5, 11, 4), gs::rgb4(12, 4, 6),
            gs::rgb4(15, 13, 5), gs::rgb4(2, 4, 9), gs::rgb4(9, 2, 3), gs::rgb4(4, 4, 6), gs::rgb4(11, 8, 3)});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 4, 11), gs::rgb4(5, 8, 15), gs::rgb4(14, 10, 8), gs::rgb4(3, 2, 1),
            gs::rgb4(15, 12, 3), gs::rgb4(2, 2, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_THEM,
           {0, gs::rgb4(2, 0, 1), gs::rgb4(11, 2, 3), gs::rgb4(15, 6, 6), gs::rgb4(13, 9, 7), gs::rgb4(1, 1, 1),
            gs::rgb4(5, 4, 5), gs::rgb4(3, 1, 1), gs::rgb4(15, 15, 15), gs::rgb4(2, 0, 0), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_LAMP,
           {0, gs::rgb4(2, 2, 3), gs::rgb4(7, 7, 8), gs::rgb4(12, 8, 2), gs::rgb4(15, 10, 1), gs::rgb4(15, 14, 5),
            gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(0, 0, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_MOON, {0, gs::rgb4(15, 15, 12), gs::rgb4(2, 3, 8), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_STAR, {0, gs::rgb4(15, 15, 14), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_TITLE,
           {0, gs::rgb4(1, 1, 4), gs::rgb4(2, 3, 8), gs::rgb4(15, 15, 12), gs::rgb4(15, 15, 15), gs::rgb4(1, 3, 2),
            gs::rgb4(1, 4, 2), gs::rgb4(3, 8, 3), gs::rgb4(7, 6, 3), gs::rgb4(3, 3, 4), gs::rgb4(15, 12, 3),
            gs::rgb4(4, 4, 5), gs::rgb4(15, 15, 10), gs::rgb4(3, 6, 13), gs::rgb4(14, 10, 7), gs::rgb4(12, 3, 3)});
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 13, 10), gs::rgb4(1, 1, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 3, 3), gs::rgb4(2, 0, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_GREEN, {0, gs::rgb4(6, 13, 5), gs::rgb4(1, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(3, 3, 5), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    loadFont(vdp, fontBase);
    gs::TileAlloc tiles(vdp, 80);
    gs::bitmapToPlane(tiles, vdp.A, OX / 8, OY / 8, paintMaze(hedge, lamps, yx, yy, tx, ty), PAL_MAZE);

    for (int face = 0; face < 3; face++)
        for (int stride = 0; stride < 2; stride++) body[face][stride] = gs::uploadImage(vdp, coat(face, stride));
    shadow = gs::uploadImage(vdp, [] {
        gs::Bitmap s(12, 5);
        s.ellipse(6, 2, 5, 2, 1);
        return s;
    }());
    for (int v = 1; v <= 3; v++) lamp[v - 1] = gs::uploadImage(vdp, lampPic(v));
    pip = gs::uploadImage(vdp, [] {
        gs::Bitmap s(8, 8);
        s.ellipse(4, 4, 3, 3, 1);
        return s;
    }());
    moon = gs::uploadImage(vdp, [] {
        gs::Bitmap s(14, 14);
        s.ellipse(6, 7, 6, 6, 1);
        s.ellipse(9, 6, 5, 5, 2);
        return s;
    }());
    star = gs::uploadImage(vdp, [] {
        gs::Bitmap s(3, 3);
        s.set(1, 0, 1);
        s.set(0, 1, 1);
        s.set(1, 1, 1);
        s.set(2, 1, 1);
        s.set(1, 2, 1);
        return s;
    }());
    poster = gs::uploadImage(vdp, titlePoster());
    wordTitle = say(vdp, "S3 MAZE SEVEN", 2);
    wordFirst = say(vdp, "FIRST TO SEVEN", 2);
    wordOne = say(vdp, "ONE LAMP APIECE", 1);
    wordSix = say(vdp, "A SIX IS STILL SHORT", 1);
    wordMove = say(vdp, "ARROWS WALK THE HEDGE", 1);
    wordTake = say(vdp, "Z TAKES THE LAMP", 1);
    wordEnter = say(vdp, "PRESS ENTER", 1);
    wordYou = say(vdp, "YOU", 1);
    wordThem = say(vdp, "THEM", 1);
    sayPiece = say(vdp, "ONE LAMP APIECE", 1);
    sayShort = say(vdp, "STILL SHORT", 1);
    sayWin = say(vdp, "FIRST TO SEVEN", 1);
    sayLose = say(vdp, "THEY REACHED SEVEN", 1);
}

}  // namespace mazeseven
