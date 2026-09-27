#include "game/art.h"

#include "console/gfx.h"

#include <cstdint>
#include <initializer_list>
#include <string>

namespace mazebell {
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
    } else {
        b.rect(2, 11, 2, 5, 2);
        b.rect(12, 11 + s, 2, 5, 2);
    }
    b.rect(4, 17, 3, 4, 7);
    b.rect(9, 17, 3, 4, 7);
    b.rect(4, 21, 3, 2, 1);
    b.rect(9, 21 - s, 3, 2, 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap bellPic(bool real) {
    gs::Bitmap b(14, 16);
    b.rect(6, 0, 2, 2, 2);
    b.ellipse(7, 7, 6, 5, real ? 3 : 5);
    b.ellipse(7, 6, 3, 2, real ? 4 : 6);
    b.rect(2, 11, 10, 2, real ? 3 : 5);
    b.ellipse(7, 14, 2, 2, real ? 4 : 1);
    b.outline(1, false);
    return b;
}

gs::Bitmap titlePoster() {
    gs::Bitmap t(160, 80);
    t.rect(0, 0, 160, 80, 1);
    t.rect(0, 0, 160, 28, 2);
    t.ellipse(132, 14, 10, 10, 3);
    t.ellipse(136, 12, 8, 8, 2);
    const int stars[][2] = {{14, 8}, {32, 16}, {54, 6}, {80, 12}, {104, 8}};
    for (auto s : stars) t.set(s[0], s[1], 4);
    t.rect(0, 40, 160, 40, 5);
    for (int i = 0; i < 6; i++) t.ellipse(8 + i * 14, 40, 10, 8, 6);
    t.rect(48, 48, 64, 20, 7);
    t.rect(76, 36, 2, 8, 8);
    t.ellipse(77, 50, 10, 8, 8);
    t.ellipse(77, 48, 4, 3, 9);
    t.ellipse(77, 58, 2, 2, 9);
    t.rect(0, 0, 160, 2, 8);
    t.rect(0, 78, 160, 2, 8);
    return t;
}

gs::Bitmap paintMaze(const uint8_t* h, int bx, int by, int fx, int fy, int px, int py) {
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
                if (!border && (hv % 7u) == 0) c = 8;
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
            bool lip = false;
            if (lx < 2 && wall(h, cx - 1, cy)) lip = true;
            if (lx >= CELL - 2 && wall(h, cx + 1, cy)) lip = true;
            if (ly < 2 && wall(h, cx, cy - 1)) lip = true;
            if (ly >= CELL - 2 && wall(h, cx, cy + 1)) lip = true;
            if (lip) c = 1;
            b.set(x, y, c);
        }
    }
    auto mark = [&](int cx, int cy, int fill) {
        b.ellipse(float(cx * CELL + 8), float(cy * CELL + 8), 5, 5, fill);
    };
    mark(bx, by, 10);
    mark(fx, fy, 11);
    mark(px, py, 12);
    return b;
}

}  // namespace

void Art::bake(gs::VDP& vdp, const uint8_t* hedge, int bx, int by, int fx, int fy, int px, int py) {
    setPal(vdp, PAL_MAZE,
           {0, gs::rgb4(3, 3, 2), gs::rgb4(6, 5, 3), gs::rgb4(8, 7, 4), gs::rgb4(4, 3, 2), gs::rgb4(9, 8, 5),
            gs::rgb4(1, 3, 1), gs::rgb4(2, 6, 2), gs::rgb4(3, 8, 3), gs::rgb4(6, 12, 4), gs::rgb4(12, 9, 2),
            gs::rgb4(8, 6, 3), gs::rgb4(4, 2, 2), 0, 0, 0});
    setPal(vdp, PAL_YOU,
           {0, gs::rgb4(1, 1, 2), gs::rgb4(2, 4, 11), gs::rgb4(5, 8, 15), gs::rgb4(14, 10, 8), gs::rgb4(3, 2, 1),
            gs::rgb4(15, 12, 3), gs::rgb4(2, 2, 4), gs::rgb4(15, 15, 15), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_BELL,
           {0, gs::rgb4(3, 2, 1), gs::rgb4(8, 6, 2), gs::rgb4(13, 9, 2), gs::rgb4(15, 13, 5), gs::rgb4(6, 5, 4),
            gs::rgb4(4, 4, 4), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_FAKE,
           {0, gs::rgb4(2, 1, 1), gs::rgb4(6, 4, 3), gs::rgb4(9, 6, 4), gs::rgb4(12, 8, 6), gs::rgb4(5, 4, 3),
            gs::rgb4(3, 3, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SHADE, {0, gs::rgb4(0, 0, 0), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_SKY, {0, gs::rgb4(15, 15, 12), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_TITLE,
           {0, gs::rgb4(1, 1, 4), gs::rgb4(2, 3, 8), gs::rgb4(15, 15, 12), gs::rgb4(15, 15, 15), gs::rgb4(1, 3, 2),
            gs::rgb4(2, 6, 2), gs::rgb4(5, 4, 2), gs::rgb4(13, 10, 3), gs::rgb4(15, 14, 6), 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_INK, {0, gs::rgb4(14, 13, 10), gs::rgb4(1, 1, 3), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_GOLD, {0, gs::rgb4(15, 12, 3), gs::rgb4(3, 2, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_RED, {0, gs::rgb4(14, 3, 3), gs::rgb4(2, 0, 1), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    setPal(vdp, PAL_DIM, {0, gs::rgb4(6, 6, 8), gs::rgb4(1, 1, 2), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    loadFont(vdp, fontBase);
    gs::TileAlloc tiles(vdp, 80);
    gs::bitmapToPlane(tiles, vdp.A, OX / 8, OY / 8, paintMaze(hedge, bx, by, fx, fy, px, py), PAL_MAZE);

    for (int face = 0; face < 3; face++)
        for (int stride = 0; stride < 2; stride++) body[face][stride] = gs::uploadImage(vdp, coat(face, stride));
    shadow = gs::uploadImage(vdp, [] {
        gs::Bitmap s(12, 5);
        s.ellipse(6, 2, 5, 2, 1);
        return s;
    }());
    bell = gs::uploadImage(vdp, bellPic(true));
    fake = gs::uploadImage(vdp, bellPic(false));
    poster = gs::uploadImage(vdp, titlePoster());
    wordTitle = say(vdp, "S3 MAZE BELL", 2);
    wordRule = say(vdp, "RING IT BEFORE THE THIRD TRY DIES", 1);
    wordMove = say(vdp, "ARROWS WALK   PITS AND THE DULL BELL DIE", 1);
    wordGo = say(vdp, "PRESS ANY KEY", 1);
}

}  // namespace mazebell
